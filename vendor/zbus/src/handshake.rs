use async_trait::async_trait;
use futures_util::{future::poll_fn, StreamExt};
#[cfg(unix)]
use nix::unistd::Uid;
use std::{
    collections::VecDeque,
    convert::{TryFrom, TryInto},
    fmt::{self, Debug},
    path::PathBuf,
    str::FromStr,
};
use tracing::{instrument, trace};
use zvariant::Str;

use sha1::{Digest, Sha1};

use xdg_home::home_dir;

#[cfg(windows)]
use crate::win32;
use crate::{
    file::FileLines,
    guid::Guid,
    raw::{Connection, Socket},
    Error, Result,
};

/// Authentication mechanisms
///
/// See <https://dbus.freedesktop.org/doc/dbus-specification.html#auth-mechanisms>
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum AuthMechanism {
    /// This is the recommended authentication mechanism on platforms where credentials can be
    /// transferred out-of-band, in particular Unix platforms that can perform credentials-passing
    /// over the `unix:` transport.
    External,

    /// This mechanism is designed to establish that a client has the ability to read a private
    /// file owned by the user being authenticated.
    Cookie,

    /// Does not perform any authentication at all, and should not be accepted by message buses.
    /// However, it might sometimes be useful for non-message-bus uses of D-Bus.
    Anonymous,
}

/// The result of a finalized handshake
///
/// The result of a finalized [`ClientHandshake`] or [`ServerHandshake`]. It can be passed to
/// [`Connection::new_authenticated`] to initialize a connection.
///
/// [`ClientHandshake`]: struct.ClientHandshake.html
/// [`ServerHandshake`]: struct.ServerHandshake.html
/// [`Connection::new_authenticated`]: ../struct.Connection.html#method.new_authenticated
#[derive(Debug)]
pub struct Authenticated<S> {
    pub(crate) conn: Connection<S>,
    /// The server Guid
    pub(crate) server_guid: Guid,
    /// Whether file descriptor passing has been accepted by both sides
    #[cfg(unix)]
    pub(crate) cap_unix_fd: bool,
}

impl<S> Authenticated<S>
where
    S: Socket + Unpin,
{
    /// Create a client-side `Authenticated` for the given `socket`.
    pub async fn client(socket: S, mechanisms: Option<VecDeque<AuthMechanism>>) -> Result<Self> {
        ClientHandshake::new(socket, mechanisms).perform().await
    }

    /// Create a server-side `Authenticated` for the given `socket`.
    ///
    /// The function takes `client_uid` on Unix only. On Windows, it takes `client_sid` instead.
    pub async fn server(
        socket: S,
        guid: Guid,
        #[cfg(unix)] client_uid: Option<u32>,
        #[cfg(windows)] client_sid: Option<String>,
        auth_mechanisms: Option<VecDeque<AuthMechanism>>,
        cookie_id: Option<usize>,
        cookie_context: CookieContext<'_>,
    ) -> Result<Self> {
        ServerHandshake::new(
            socket,
            guid,
            #[cfg(unix)]
            client_uid,
            #[cfg(windows)]
            client_sid,
            auth_mechanisms,
            cookie_id,
            cookie_context,
        )?
        .perform()
        .await
    }
}

/*
 * Client-side handshake logic
 */

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[allow(clippy::upper_case_acronyms)]
enum ClientHandshakeStep {
    Init,
    MechanismInit,
    WaitingForData,
    WaitingForOK,
    WaitingForAgreeUnixFD,
    Done,
}

// The plain-text SASL profile authentication protocol described here:
// <https://dbus.freedesktop.org/doc/dbus-specification.html#auth-protocol>
//
// These are all the known commands, which can be parsed from or serialized to text.
#[derive(Debug)]
#[allow(clippy::upper_case_acronyms)]
enum Command {
    Auth(Option<AuthMechanism>, Option<Vec<u8>>),
    Cancel,
    Begin,
    Data(Option<Vec<u8>>),
    Error(String),
    NegotiateUnixFD,
    Rejected(Vec<AuthMechanism>),
    Ok(Guid),
    AgreeUnixFD,
}

/// A representation of an in-progress handshake, client-side
///
/// This struct is an async-compatible representation of the initial handshake that must be
/// performed before a D-Bus connection can be used. To use it, you should call the
/// [`advance_handshake`] method whenever the underlying socket becomes ready (tracking the
/// readiness itself is not managed by this abstraction) until it returns `Ok(())`, at which point
/// you can invoke the [`try_finish`] method to get an [`Authenticated`], which can be given to
/// [`Connection::new_authenticated`].
///
/// [`advance_handshake`]: struct.ClientHandshake.html#method.advance_handshake
/// [`try_finish`]: struct.ClientHandshake.html#method.try_finish
/// [`Authenticated`]: struct.AUthenticated.html
/// [`Connection::new_authenticated`]: ../struct.Connection.html#method.new_authenticated
#[derive(Debug)]
pub struct ClientHandshake<S> {
    common: HandshakeCommon<S>,
    step: ClientHandshakeStep,
}

#[async_trait]
pub trait Handshake<S> {
    /// Perform the handshake.
    ///
    /// On a successful handshake, you get an `Authenticated`. If you need to send a Bus Hello,
    /// this remains to be done.
    async fn perform(mut self) -> Result<Authenticated<S>>;
}

impl<S: Socket> ClientHandshake<S> {
    /// Start a handshake on this client socket
    pub fn new(socket: S, mechanisms: Option<VecDeque<AuthMechanism>>) -> ClientHandshake<S> {
        let mechanisms = mechanisms.unwrap_or_else(|| {
            let mut mechanisms = VecDeque::new();
            mechanisms.push_back(AuthMechanism::External);
            mechanisms.push_back(AuthMechanism::Cookie);
            mechanisms.push_back(AuthMechanism::Anonymous);
            mechanisms
        });

        ClientHandshake {
            common: HandshakeCommon::new(socket, mechanisms, None),
            step: ClientHandshakeStep::Init,
        }
    }

    fn mechanism_init(&mut self) -> Result<(ClientHandshakeStep, Command)> {
        use ClientHandshakeStep::*;
        let mech = self.common.mechanism()?;
        match mech {
            AuthMechanism::Anonymous => Ok((
                WaitingForOK,
                Command::Auth(Some(*mech), Some("zbus".into())),
            )),
            AuthMechanism::External => Ok((
                WaitingForOK,
                Command::Auth(Some(*mech), Some(sasl_auth_id()?.into_bytes())),
            )),
            AuthMechanism::Cookie => Ok((
                WaitingForData,
                Command::Auth(Some(*mech), Some(sasl_auth_id()?.into_bytes())),
            )),
        }
    }

    async fn mechanism_data(&mut self, data: Vec<u8>) -> Result<(ClientHandshakeStep, Command)> {
        let mech = self.common.mechanism()?;
        match mech {
            AuthMechanism::Cookie => {
                let context = std::str::from_utf8(&data)
                    .map_err(|_| Error::Handshake("Cookie context was not valid UTF-8".into()))?;
                let mut split = context.split_ascii_whitespace();
                let context = split
                    .next()
                    .ok_or_else(|| Error::Handshake("Missing cookie context name".into()))?;
                let context = Str::from(context).try_into()?;
                let id = split
                    .next()
                    .ok_or_else(|| Error::Handshake("Missing cookie ID".into()))?;
                let id = id
                    .parse()
                    .map_err(|e| Error::Handshake(format!("Invalid cookie ID `{id}`: {e}")))?;
                let server_challenge = split
                    .next()
                    .ok_or_else(|| Error::Handshake("Missing cookie challenge".into()))?;

                let cookie = Cookie::lookup(&context, id).await?.cookie;
                let client_challenge = random_ascii(16);
                let sec = format!("{server_challenge}:{client_challenge}:{cookie}");
                let sha1 = hex::encode(Sha1::digest(sec));
                let data = format!("{client_challenge} {sha1}");
                Ok((
                    ClientHandshakeStep::WaitingForOK,
                    Command::Data(Some(data.into())),
                ))
            }
            _ => Err(Error::Handshake("Unexpected mechanism DATA".into())),
        }
    }
}

fn random_ascii(len: usize) -> String {
    use rand::{distributions::Alphanumeric, thread_rng, Rng};
    use std::iter;

    let mut rng = thread_rng();
    iter::repeat(())
        .map(|()| rng.sample(Alphanumeric))
        .map(char::from)
        .take(len)
        .collect()
}

fn sasl_auth_id() -> Result<String> {
    let id = {
        #[cfg(unix)]
        {
            Uid::effective().to_string()
        }

        #[cfg(windows)]
        {
            win32::ProcessToken::open(None)?.sid()?
        }
    };

    Ok(id)
}

#[derive(Debug)]
struct Cookie {
    id: usize,
    cookie: String,
}

impl Cookie {
    fn keyring_path() -> Result<PathBuf> {
        let mut path = home_dir()
            .ok_or_else(|| Error::Handshake("Failed to determine home directory".into()))?;
        path.push(".dbus-keyrings");
        Ok(path)
    }

    async fn read_keyring(context: &CookieContext<'_>) -> Result<Vec<Cookie>> {
        let mut path = Cookie::keyring_path()?;
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;

            let perms = crate::file::metadata(&path).await?.permissions().mode();
            if perms & 0o066 != 0 {
                return Err(Error::Handshake(
                    "DBus keyring has invalid permissions".into(),
                ));
            }
        }
        #[cfg(not(unix))]
        {
            // FIXME: add code to check directory permissions
        }
        path.push(&*context.0);
        trace!("Reading keyring {:?}", path);
        let mut lines = FileLines::open(&path).await?.enumerate();
        let mut cookies = vec![];
        while let Some((n, line)) = lines.next().await {
            let line = line?;
            let mut split = line.split_whitespace();
            let id = split
                .next()
                .ok_or_else(|| {
                    Error::Handshake(format!(
                        "DBus cookie `{}` missing ID at line {n}",
                        path.display(),
                    ))
                })?
                .parse()
                .map_err(|e| {
                    Error::Handshake(format!(
                        "Failed to parse cookie ID in file `{}` at line {n}: {e}",
                        path.display(),
                    ))
                })?;
            let _ = split.next().ok_or_else(|| {
                Error::Handshake(format!(
                    "DBus cookie `{}` missing creation time at line {n}",
                    path.display(),
                ))
            })?;
            let cookie = split
                .next()
                .ok_or_else(|| {
                    Error::Handshake(format!(
                        "DBus cookie `{}` missing cookie data at line {}",
                        path.to_str().unwrap(),
                        n
                    ))
                })?
                .to_string();
            cookies.push(Cookie { id, cookie })
        }
        trace!("Loaded keyring {:?}", cookies);
        Ok(cookies)
    }

    async fn lookup(context: &CookieContext<'_>, id: usize) -> Result<Cookie> {
        let keyring = Self::read_keyring(context).await?;
        keyring
            .into_iter()
            .find(|c| c.id == id)
            .ok_or_else(|| Error::Handshake(format!("DBus cookie ID {id} not found")))
    }

    async fn first(context: &CookieContext<'_>) -> Result<Cookie> {
        let keyring = Self::read_keyring(context).await?;
        keyring
            .into_iter()
            .next()
            .ok_or_else(|| Error::Handshake("No cookies available".into()))
    }
}

#[derive(Debug)]
pub struct CookieContext<'c>(Str<'c>);

impl<'c> TryFrom<Str<'c>> for CookieContext<'c> {
    type Error = Error;

    fn try_from(value: Str<'c>) -> Result<Self> {
        if value.is_empty() {
            return Err(Error::Handshake("Empty cookie context".into()));
        } else if !value.is_ascii() || value.contains(['/', '\\', ' ', '\n', '\r', '\t', '.']) {
            return Err(Error::Handshake(
                "Invalid characters in cookie context".into(),
            ));
        }

        Ok(Self(value))
    }
}

impl Default for CookieContext<'_> {
    fn default() -> Self {
        Self(Str::from_static("org_freedesktop_general"))
    }
}

#[async_trait]
impl<S: Socket> Handshake<S> for ClientHandshake<S> {
    #[instrument(skip(self))]
    async fn perform(mut self) -> Result<Authenticated<S>> {
        use ClientHandshakeStep::*;
        loop {
            let (next_step, cmd) = match self.step {
                Init => {
                    trace!("Initializing");
                    #[allow(clippy::let_and_return)]
                    let ret = self.mechanism_init()?;
                    // The dbus daemon on some platforms requires sending the zero byte as a
                    // separate message with SCM_CREDS.
                    #[cfg(any(target_os = "freebsd", target_os = "dragonfly"))]
                    let written = self
                        .common
                        .socket
                        .send_zero_byte()
                        .map_err(|e| {
                            Error::Handshake(format!(
                                "Could not send zero byte with credentials: {}",
                                e
                            ))
                        })
                        .and_then(|n| match n {
                            None => Err(Error::Handshake(
                                "Could not send zero byte with credentials".to_string(),
                            )),
                            Some(n) => Ok(n),
                        })?;

                    // leading 0 is sent separately already for `freebsd` and `dragonfly` above.
                    #[cfg(not(any(target_os = "freebsd", target_os = "dragonfly")))]
                    let written = poll_fn(|cx| {
                        self.common.socket.poll_sendmsg(
                            cx,
                            &[b'\0'],
                            #[cfg(unix)]
                            &[],
                        )
                    })
                    .await?;

                    if written != 1 {
                        return Err(Error::Handshake(
                            "Could not send zero byte with credentials".to_string(),
                        ));
                    }

                    ret
                }
                MechanismInit => {
                    trace!("Initializing auth mechanisms");
                    self.mechanism_init()?
                }
                WaitingForData | WaitingForOK => {
                    trace!("Waiting for DATA or OK from server");
                    let reply = self.common.read_command().await?;
                    match (self.step, reply) {
                        (_, Command::Data(data)) => {
                            trace!("Received DATA from server");
                            let data = data.ok_or_else(|| {
                                Error::Handshake("Received DATA with no data from server".into())
                            })?;
                            self.mechanism_data(data).await?
                        }
                        (_, Command::Rejected(_)) => {
                            trace!("Received REJECT from server. Will try next auth mechanism..");
                            self.common.mechanisms.pop_front();
                            self.step = MechanismInit;
                            continue;
                        }
                        (WaitingForOK, Command::Ok(guid)) => {
                            trace!("Received OK from server");
                            self.common.server_guid = Some(guid);
                            if self.common.socket.can_pass_unix_fd() {
                                (WaitingForAgreeUnixFD, Command::NegotiateUnixFD)
                            } else {
                                (Done, Command::Begin)
                            }
                        }
                        (_, reply) => {
                            return Err(Error::Handshake(format!(
                                "Unexpected server AUTH OK reply: {reply}"
                            )));
                        }
                    }
                }
                WaitingForAgreeUnixFD => {
                    trace!("Waiting for Unix FD passing agreement from server");
                    let reply = self.common.read_command().await?;
                    match reply {
                        Command::AgreeUnixFD => {
                            trace!("Unix FD passing agreed by server");
                            self.common.cap_unix_fd = true
                        }
                        Command::Error(_) => {
                            trace!("Unix FD passing rejected by server");
                            self.common.cap_unix_fd = false
                        }
                        _ => {
                            return Err(Error::Handshake(format!(
                                "Unexpected server UNIX_FD reply: {reply}"
                            )));
                        }
                    }
                    (Done, Command::Begin)
                }
                Done => {
                    trace!("Handshake done");
                    return Ok(Authenticated {
                        conn: Connection::new(self.common.socket, self.common.recv_buffer),
                        server_guid: self.common.server_guid.unwrap(),
                        #[cfg(unix)]
                        cap_unix_fd: self.common.cap_unix_fd,
                    });
                }
            };
            self.common.write_command(cmd).await?;
            self.step = next_step;
        }
    }
}

/*
 * Server-side handshake logic
 */

#[derive(Debug)]
#[allow(clippy::upper_case_acronyms)]
enum ServerHandshakeStep {
    WaitingForNull,
    WaitingForAuth,
    WaitingForData(AuthMechanism),
    WaitingForBegin,
    Done,
}

/// A representation of an in-progress handshake, server-side
///
/// This would typically be used to implement a D-Bus broker, or in the context of a P2P connection.
///
/// This struct is an async-compatible representation of the initial handshake that must be
/// performed before a D-Bus connection can be used. To use it, you should call the
/// [`advance_handshake`] method whenever the underlying socket becomes ready (tracking the
/// readiness itself is not managed by this abstraction) until it returns `Ok(())`, at which point
/// you can invoke the [`try_finish`] method to get an [`Authenticated`], which can be given to
/// [`Connection::new_authenticated`].
///
/// [`advance_handshake`]: struct.ServerHandshake.html#method.advance_handshake
/// [`try_finish`]: struct.ServerHandshake.html#method.try_finish
/// [`Authenticated`]: struct.Authenticated.html
/// [`Connection::new_authenticated`]: ../struct.Connection.html#method.new_authenticated
#[derive(Debug)]
pub struct ServerHandshake<'s, S> {
    common: HandshakeCommon<S>,
    step: ServerHandshakeStep,
    #[cfg(unix)]
    client_uid: Option<u32>,
    #[cfg(windows)]
    client_sid: Option<String>,
    cookie_id: Option<usize>,
    cookie_context: CookieContext<'s>,
}

impl<'s, S: Socket> ServerHandshake<'s, S> {
    pub fn new(
        socket: S,
        guid: Guid,
        #[cfg(unix)] client_uid: Option<u32>,
        #[cfg(windows)] client_sid: Option<String>,
        mechanisms: Option<VecDeque<AuthMechanism>>,
        cookie_id: Option<usize>,
        cookie_context: CookieContext<'s>,
    ) -> Result<ServerHandshake<'s, S>> {
        let mechanisms = match mechanisms {
            Some(mechanisms) => mechanisms,
            None => {
                let mut mechanisms = VecDeque::new();
                mechanisms.push_back(AuthMechanism::External);

                mechanisms
            }
        };

        Ok(ServerHandshake {
            common: HandshakeCommon::new(socket, mechanisms, Some(guid)),
            step: ServerHandshakeStep::WaitingForNull,
            #[cfg(unix)]
            client_uid,
            #[cfg(windows)]
            client_sid,
            cookie_id,
            cookie_context,
        })
    }

    async fn auth_ok(&mut self) -> Result<()> {
        let cmd = Command::Ok(self.guid().clone());
        trace!("Sending authentication OK");
        self.common.write_command(cmd).await?;
        self.step = ServerHandshakeStep::WaitingForBegin;

        Ok(())
    }

    async fn check_external_auth(&mut self, sasl_id: &[u8]) -> Result<()> {
        let auth_ok = {
            let id = std::str::from_utf8(sasl_id)
                .map_err(|e| Error::Handshake(format!("Invalid ID: {e}")))?;
            #[cfg(unix)]
            {
                let uid = id
                    .parse::<u32>()
                    .map_err(|e| Error::Handshake(format!("Invalid UID: {e}")))?;
                self.client_uid.map(|u| u == uid).unwrap_or(false)
            }
            #[cfg(windows)]
            {
                self.client_sid.as_ref().map(|u| u == id).unwrap_or(false)
            }
        };

        if auth_ok {
            self.auth_ok().await
        } else {
            self.rejected_error().await
        }
    }

    async fn check_cookie_auth(&mut self, sasl_id: &[u8]) -> Result<()> {
        let cookie = match self.cookie_id {
            Some(cookie_id) => Cookie::lookup(&self.cookie_context, cookie_id).await?,
            None => Cookie::first(&self.cookie_context).await?,
        };
        let id = std::str::from_utf8(sasl_id)
            .map_err(|e| Error::Handshake(format!("Invalid ID: {e}")))?;
        if sasl_auth_id()? != id {
            // While the spec will make you believe that DBUS_COOKIE_SHA1 can be used to
            // authenticate any user, it is not even possible (or correct) for the server to manage
            // contents in random users' home directories.
            //
            // The dbus reference implementation also has the same limitation/behavior.
            self.rejected_error().await?;
            return Ok(());
        }
        let server_challenge = random_ascii(16);
        let data = format!("{} {} {server_challenge}", self.cookie_context.0, cookie.id);
        let cmd = Command::Data(Some(data.into_bytes()));
        trace!("Sending DBUS_COOKIE_SHA1 authentication challenge");
        self.common.write_command(cmd).await?;

        let auth_data = match self.common.read_command().await? {
            Command::Data(data) => data,
            _ => None,
        };
        let auth_data = auth_data.ok_or_else(|| {
            Error::Handshake("Expected DBUS_COOKIE_SHA1 authentication challenge response".into())
        })?;
        let client_auth = std::str::from_utf8(&auth_data)
            .map_err(|e| Error::Handshake(format!("Invalid COOKIE authentication data: {e}")))?;
        let mut split = client_auth.split_ascii_whitespace();
        let client_challenge = split
            .next()
            .ok_or_else(|| Error::Handshake("Missing cookie challenge".into()))?;
        let client_sha1 = split
            .next()
            .ok_or_else(|| Error::Handshake("Missing client cookie data".into()))?;
        let sec = format!("{server_challenge}:{client_challenge}:{}", cookie.cookie);
        let sha1 = hex::encode(Sha1::digest(sec));

        if sha1 == client_sha1 {
            self.auth_ok().await
        } else {
            self.rejected_error().await
        }
    }

    async fn unsupported_command_error(&mut self) -> Result<()> {
        let cmd = Command::Error("Unsupported command".to_string());
        trace!("Sending authentication error");
        self.common.write_command(cmd).await?;
        self.step = ServerHandshakeStep::WaitingForAuth;

        Ok(())
    }

    async fn rejected_error(&mut self) -> Result<()> {
        let mechanisms = self.common.mechanisms.iter().cloned().collect();
        let cmd = Command::Rejected(mechanisms);
        trace!("Sending authentication error");
        self.common.write_command(cmd).await?;
        self.step = ServerHandshakeStep::WaitingForAuth;

        Ok(())
    }

    fn guid(&self) -> &Guid {
        // SAFETY: We know that the server GUID is set because we set it in the constructor.
        self.common
            .server_guid
            .as_ref()
            .expect("Server GUID not set")
    }
}

#[async_trait]
impl<S: Socket> Handshake<S> for ServerHandshake<'_, S> {
    #[instrument(skip(self))]
    async fn perform(mut self) -> Result<Authenticated<S>> {
        loop {
            match self.step {
                ServerHandshakeStep::WaitingForNull => {
                    trace!("Waiting for NULL");
                    let mut buffer = [0; 1];
                    let read =
                        poll_fn(|cx| self.common.socket.poll_recvmsg(cx, &mut buffer)).await?;
                    #[cfg(unix)]
                    let read = read.0;
                    // recvmsg cannot return anything else than Ok(1) or Err
                    debug_assert!(read == 1);
                    if buffer[0] != 0 {
                        return Err(Error::Handshake(
                            "First client byte is not NUL!".to_string(),
                        ));
                    }
                    trace!("Received NULL from client");
                    self.step = ServerHandshakeStep::WaitingForAuth;
                }
                ServerHandshakeStep::WaitingForAuth => {
                    trace!("Waiting for authentication");
                    let reply = self.common.read_command().await?;
                    match reply {
                        Command::Auth(mech, resp) => {
                            let mech = mech.filter(|m| self.common.mechanisms.contains(m));

                            match (mech, &resp) {
                                (Some(mech), None) => {
                                    trace!("Sending data request");
                                    self.common.write_command(Command::Data(None)).await?;
                                    self.step = ServerHandshakeStep::WaitingForData(mech);
                                }
                                (Some(AuthMechanism::Anonymous), Some(_)) => {
                                    self.auth_ok().await?;
                                }
                                (Some(AuthMechanism::External), Some(sasl_id)) => {
                                    self.check_external_auth(sasl_id).await?;
                                }
                                (Some(AuthMechanism::Cookie), Some(sasl_id)) => {
                                    self.check_cookie_auth(sasl_id).await?;
                                }
                                _ => self.rejected_error().await?,
                            }
                        }
                        Command::Error(_) => self.rejected_error().await?,
                        Command::Begin => {
                            return Err(Error::Handshake(
                                "Received BEGIN while not authenticated".to_string(),
                            ));
                        }
                        _ => self.unsupported_command_error().await?,
                    }
                }
                ServerHandshakeStep::WaitingForData(mech) => {
                    trace!("Waiting for authentication");
                    let reply = self.common.read_command().await?;
                    match (mech, reply) {
                        (AuthMechanism::External, Command::Data(None)) => self.auth_ok().await?,
                        (AuthMechanism::External, Command::Data(Some(data))) => {
                            self.check_external_auth(&data).await?;
                        }
                        (AuthMechanism::Anonymous, Command::Data(_)) => self.auth_ok().await?,
                        (_, Command::Data(_)) => self.rejected_error().await?,
                        (_, _) => self.unsupported_command_error().await?,
                    }
                }
                ServerHandshakeStep::WaitingForBegin => {
                    trace!("Waiting for Begin command from the client");
                    let reply = self.common.read_command().await?;
                    match reply {
                        Command::Begin => {
                            trace!("Received Begin command from the client");
                            self.step = ServerHandshakeStep::Done;
                        }
                        Command::Cancel | Command::Error(_) => {
                            trace!("Received CANCEL or ERROR command from the client");
                            self.rejected_error().await?;
                        }
                        #[cfg(unix)]
                        Command::NegotiateUnixFD => {
                            trace!("Received NEGOTIATE_UNIX_FD command from the client");
                            self.common.cap_unix_fd = true;
                            trace!("Sending AGREE_UNIX_FD to the client");
                            self.common.write_command(Command::AgreeUnixFD).await?;
                            self.step = ServerHandshakeStep::WaitingForBegin;
                        }
                        _ => self.unsupported_command_error().await?,
                    }
                }
                ServerHandshakeStep::Done => {
                    trace!("Handshake done");
                    return Ok(Authenticated {
                        conn: Connection::new(self.common.socket, self.common.recv_buffer),
                        // SAFETY: We know that the server GUID is set because we set it in the
                        // constructor.
                        server_guid: self.common.server_guid.expect("Server GUID not set"),
                        #[cfg(unix)]
                        cap_unix_fd: self.common.cap_unix_fd,
                    });
                }
            }
        }
    }
}

impl fmt::Display for AuthMechanism {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mech = match self {
            AuthMechanism::External => "EXTERNAL",
            AuthMechanism::Cookie => "DBUS_COOKIE_SHA1",
            AuthMechanism::Anonymous => "ANONYMOUS",
        };
        write!(f, "{mech}")
    }
}

impl FromStr for AuthMechanism {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        match s {
            "EXTERNAL" => Ok(AuthMechanism::External),
            "DBUS_COOKIE_SHA1" => Ok(AuthMechanism::Cookie),
            "ANONYMOUS" => Ok(AuthMechanism::Anonymous),
            _ => Err(Error::Handshake(format!("Unknown mechanism: {s}"))),
        }
    }
}

impl From<Command> for Vec<u8> {
    fn from(c: Command) -> Self {
        c.to_string().into()
    }
}

impl fmt::Display for Command {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Command::Auth(mech, resp) => match (mech, resp) {
                (Some(mech), Some(resp)) => write!(f, "AUTH {mech} {}", hex::encode(resp)),
                (Some(mech), None) => write!(f, "AUTH {mech}"),
                _ => write!(f, "AUTH"),
            },
            Command::Cancel => write!(f, "CANCEL"),
            Command::Begin => write!(f, "BEGIN"),
            Command::Data(data) => match data {
                None => write!(f, "DATA"),
                Some(data) => write!(f, "DATA {}", hex::encode(data)),
            },
            Command::Error(expl) => write!(f, "ERROR {expl}"),
            Command::NegotiateUnixFD => write!(f, "NEGOTIATE_UNIX_FD"),
            Command::Rejected(mechs) => {
                write!(
                    f,
                    "REJECTED {}",
                    mechs
                        .iter()
                        .map(|m| m.to_string())
                        .collect::<Vec<_>>()
                        .join(" ")
                )
            }
            Command::Ok(guid) => write!(f, "OK {guid}"),
            Command::AgreeUnixFD => write!(f, "AGREE_UNIX_FD"),
        }?;
        write!(f, "\r\n")
    }
}

impl From<hex::FromHexError> for Error {
    fn from(e: hex::FromHexError) -> Self {
        Error::Handshake(format!("Invalid hexcode: {e}"))
    }
}

impl FromStr for Command {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        let mut words = s.split_ascii_whitespace();
        let cmd = match words.next() {
            Some("AUTH") => {
                let mech = if let Some(m) = words.next() {
                    Some(m.parse()?)
                } else {
                    None
                };
                let resp = match words.next() {
                    Some(resp) => Some(hex::decode(resp)?),
                    None => None,
                };
                Command::Auth(mech, resp)
            }
            Some("CANCEL") => Command::Cancel,
            Some("BEGIN") => Command::Begin,
            Some("DATA") => {
                let data = match words.next() {
                    Some(data) => Some(hex::decode(data)?),
                    None => None,
                };

                Command::Data(data)
            }
            Some("ERROR") => Command::Error(s.into()),
            Some("NEGOTIATE_UNIX_FD") => Command::NegotiateUnixFD,
            Some("REJECTED") => {
                let mechs = words.map(|m| m.parse()).collect::<Result<_>>()?;
                Command::Rejected(mechs)
            }
            Some("OK") => {
                let guid = words
                    .next()
                    .ok_or_else(|| Error::Handshake("Missing OK server GUID!".into()))?;
                Command::Ok(guid.parse()?)
            }
            Some("AGREE_UNIX_FD") => Command::AgreeUnixFD,
            _ => return Err(Error::Handshake(format!("Unknown command: {s}"))),
        };
        Ok(cmd)
    }
}

// Common code for the client and server side of the handshake.
#[derive(Debug)]
pub struct HandshakeCommon<S> {
    socket: S,
    recv_buffer: Vec<u8>,
    server_guid: Option<Guid>,
    cap_unix_fd: bool,
    // the current AUTH mechanism is front, ordered by priority
    mechanisms: VecDeque<AuthMechanism>,
}

impl<S: Socket> HandshakeCommon<S> {
    /// Start a handshake on this client socket
    pub fn new(socket: S, mechanisms: VecDeque<AuthMechanism>, server_guid: Option<Guid>) -> Self {
        Self {
            socket,
            recv_buffer: Vec::new(),
            server_guid,
            cap_unix_fd: false,
            mechanisms,
        }
    }

    #[instrument(skip(self))]
    async fn write_command(&mut self, command: Command) -> Result<()> {
        let mut send_buffer = Vec::<u8>::from(command);
        while !send_buffer.is_empty() {
            let written = poll_fn(|cx| {
                self.socket.poll_sendmsg(
                    cx,
                    &send_buffer,
                    #[cfg(unix)]
                    &[],
                )
            })
            .await?;
            send_buffer.drain(..written);
        }
        Ok(())
    }

    #[instrument(skip(self))]
    async fn read_command(&mut self) -> Result<Command> {
        let mut cmd_end = 0;
        loop {
            if let Some(i) = self.recv_buffer[cmd_end..].iter().position(|b| *b == b'\n') {
                if cmd_end + i == 0 || self.recv_buffer.get(cmd_end + i - 1) != Some(&b'\r') {
                    return Err(Error::Handshake("Invalid line ending in handshake".into()));
                }
                cmd_end += i + 1;

                break;
            } else {
                cmd_end = self.recv_buffer.len();
            }

            let mut buf = [0; 64];
            let res = poll_fn(|cx| self.socket.poll_recvmsg(cx, &mut buf)).await?;
            let read = {
                #[cfg(unix)]
                {
                    let (read, fds) = res;
                    if !fds.is_empty() {
                        return Err(Error::Handshake("Unexpected FDs during handshake".into()));
                    }
                    read
                }
                #[cfg(not(unix))]
                {
                    res
                }
            };
            if read == 0 {
                return Err(Error::Handshake("Unexpected EOF during handshake".into()));
            }
            self.recv_buffer.extend(&buf[..read]);
        }

        let line_bytes = self.recv_buffer.drain(..cmd_end);
        let line = std::str::from_utf8(line_bytes.as_slice())
            .map_err(|e| Error::Handshake(e.to_string()))?;

        line.parse()
    }

    fn mechanism(&self) -> Result<&AuthMechanism> {
        self.mechanisms
            .front()
            .ok_or_else(|| Error::Handshake("Exhausted available AUTH mechanisms".into()))
    }
}

#[cfg(unix)]
#[cfg(test)]
mod tests {
    #[cfg(not(feature = "tokio"))]
    use async_std::io::{Write as AsyncWrite, WriteExt};
    use futures_util::future::join;
    use ntest::timeout;
    #[cfg(not(feature = "tokio"))]
    use std::os::unix::net::UnixStream;
    use test_log::test;
    #[cfg(feature = "tokio")]
    use tokio::{
        io::{AsyncWrite, AsyncWriteExt},
        net::UnixStream,
    };

    use super::*;

    use crate::Guid;

    fn create_async_socket_pair() -> (impl AsyncWrite + Socket, impl AsyncWrite + Socket) {
        // Tokio needs us to call the sync function from async context. :shrug:
        let (p0, p1) = crate::utils::block_on(async { UnixStream::pair().unwrap() });

        // initialize both handshakes
        #[cfg(not(feature = "tokio"))]
        let (p0, p1) = {
            p0.set_nonblocking(true).unwrap();
            p1.set_nonblocking(true).unwrap();

            (
                async_io::Async::new(p0).unwrap(),
                async_io::Async::new(p1).unwrap(),
            )
        };

        (p0, p1)
    }

    #[test]
    fn handshake() {
        let (p0, p1) = create_async_socket_pair();

        let client = ClientHandshake::new(p0, None);
        let server = ServerHandshake::new(
            p1,
            Guid::generate(),
            Some(Uid::effective().into()),
            None,
            None,
            CookieContext::default(),
        )
        .unwrap();

        // proceed to the handshakes
        let (client, server) = crate::utils::block_on(join(
            async move { client.perform().await.unwrap() },
            async move { server.perform().await.unwrap() },
        ));

        assert_eq!(client.server_guid, server.server_guid);
        assert_eq!(client.cap_unix_fd, server.cap_unix_fd);
    }

    #[test]
    #[timeout(15000)]
    fn pipelined_handshake() {
        let (mut p0, p1) = create_async_socket_pair();
        let server = ServerHandshake::new(
            p1,
            Guid::generate(),
            Some(Uid::effective().into()),
            None,
            None,
            CookieContext::default(),
        )
        .unwrap();

        crate::utils::block_on(
            p0.write_all(
                format!(
                    "\0AUTH EXTERNAL {}\r\nNEGOTIATE_UNIX_FD\r\nBEGIN\r\n",
                    hex::encode(sasl_auth_id().unwrap())
                )
                .as_bytes(),
            ),
        )
        .unwrap();
        let server = crate::utils::block_on(server.perform()).unwrap();

        assert!(server.cap_unix_fd);
    }

    #[test]
    #[timeout(15000)]
    fn separate_external_data() {
        let (mut p0, p1) = create_async_socket_pair();
        let server = ServerHandshake::new(
            p1,
            Guid::generate(),
            Some(Uid::effective().into()),
            None,
            None,
            CookieContext::default(),
        )
        .unwrap();

        crate::utils::block_on(
            p0.write_all(
                format!(
                    "\0AUTH EXTERNAL\r\nDATA {}\r\nBEGIN\r\n",
                    hex::encode(sasl_auth_id().unwrap())
                )
                .as_bytes(),
            ),
        )
        .unwrap();
        crate::utils::block_on(server.perform()).unwrap();
    }

    #[test]
    #[timeout(15000)]
    fn missing_external_data() {
        let (mut p0, p1) = create_async_socket_pair();
        let server = ServerHandshake::new(
            p1,
            Guid::generate(),
            Some(Uid::effective().into()),
            None,
            None,
            CookieContext::default(),
        )
        .unwrap();

        crate::utils::block_on(p0.write_all(b"\0AUTH EXTERNAL\r\nDATA\r\nBEGIN\r\n")).unwrap();
        crate::utils::block_on(server.perform()).unwrap();
    }

    #[test]
    #[timeout(15000)]
    fn anonymous_handshake() {
        let (mut p0, p1) = create_async_socket_pair();
        let server = ServerHandshake::new(
            p1,
            Guid::generate(),
            Some(Uid::effective().into()),
            Some(vec![AuthMechanism::Anonymous].into()),
            None,
            CookieContext::default(),
        )
        .unwrap();

        crate::utils::block_on(p0.write_all(b"\0AUTH ANONYMOUS abcd\r\nBEGIN\r\n")).unwrap();
        crate::utils::block_on(server.perform()).unwrap();
    }

    #[test]
    #[timeout(15000)]
    fn separate_anonymous_data() {
        let (mut p0, p1) = create_async_socket_pair();
        let server = ServerHandshake::new(
            p1,
            Guid::generate(),
            Some(Uid::effective().into()),
            Some(vec![AuthMechanism::Anonymous].into()),
            None,
            CookieContext::default(),
        )
        .unwrap();

        crate::utils::block_on(p0.write_all(b"\0AUTH ANONYMOUS\r\nDATA abcd\r\nBEGIN\r\n"))
            .unwrap();
        crate::utils::block_on(server.perform()).unwrap();
    }
}
