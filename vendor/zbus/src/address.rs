#[cfg(target_os = "macos")]
use crate::process::run;
#[cfg(windows)]
use crate::win32::windows_autolaunch_bus_address;
use crate::{Error, Result};
#[cfg(not(feature = "tokio"))]
use async_io::Async;
#[cfg(all(unix, not(target_os = "macos")))]
use nix::unistd::Uid;
#[cfg(not(feature = "tokio"))]
use std::net::{SocketAddr, TcpStream, ToSocketAddrs};
#[cfg(all(unix, not(feature = "tokio")))]
use std::os::unix::net::UnixStream;
use std::{collections::HashMap, convert::TryFrom, env, str::FromStr};
#[cfg(feature = "tokio")]
use tokio::net::TcpStream;
#[cfg(all(unix, feature = "tokio"))]
use tokio::net::UnixStream;
#[cfg(feature = "tokio-vsock")]
use tokio_vsock::VsockStream;
#[cfg(all(windows, not(feature = "tokio")))]
use uds_windows::UnixStream;
#[cfg(all(feature = "vsock", not(feature = "tokio")))]
use vsock::VsockStream;

use std::{
    ffi::OsString,
    fmt::{Display, Formatter},
    str::from_utf8_unchecked,
};

/// A `tcp:` address family.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum TcpAddressFamily {
    Ipv4,
    Ipv6,
}

/// A `tcp:` D-Bus address.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TcpAddress {
    pub(crate) host: String,
    pub(crate) bind: Option<String>,
    pub(crate) port: u16,
    pub(crate) family: Option<TcpAddressFamily>,
}

impl TcpAddress {
    /// Returns the `tcp:` address `host` value.
    pub fn host(&self) -> &str {
        &self.host
    }

    /// Returns the `tcp:` address `bind` value.
    pub fn bind(&self) -> Option<&str> {
        self.bind.as_deref()
    }

    /// Returns the `tcp:` address `port` value.
    pub fn port(&self) -> u16 {
        self.port
    }

    /// Returns the `tcp:` address `family` value.
    pub fn family(&self) -> Option<TcpAddressFamily> {
        self.family
    }

    // Helper for FromStr
    fn from_tcp(opts: HashMap<&str, &str>) -> Result<Self> {
        let bind = None;
        if opts.contains_key("bind") {
            return Err(Error::Address("`bind` isn't yet supported".into()));
        }

        let host = opts
            .get("host")
            .ok_or_else(|| Error::Address("tcp address is missing `host`".into()))?
            .to_string();
        let port = opts
            .get("port")
            .ok_or_else(|| Error::Address("tcp address is missing `port`".into()))?;
        let port = port
            .parse::<u16>()
            .map_err(|_| Error::Address("invalid tcp `port`".into()))?;
        let family = opts
            .get("family")
            .map(|f| TcpAddressFamily::from_str(f))
            .transpose()?;

        Ok(Self {
            host,
            bind,
            port,
            family,
        })
    }

    fn write_options(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_str("host=")?;

        encode_percents(f, self.host.as_ref())?;

        write!(f, ",port={}", self.port)?;

        if let Some(bind) = &self.bind {
            f.write_str(",bind=")?;
            encode_percents(f, bind.as_ref())?;
        }

        if let Some(family) = &self.family {
            write!(f, ",family={family}")?;
        }

        Ok(())
    }
}

#[cfg(any(
    all(feature = "vsock", not(feature = "tokio")),
    feature = "tokio-vsock"
))]
/// A `tcp:` D-Bus address.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct VsockAddress {
    pub(crate) cid: u32,
    pub(crate) port: u32,
}

#[cfg(any(
    all(feature = "vsock", not(feature = "tokio")),
    feature = "tokio-vsock"
))]
impl VsockAddress {
    /// Create a new VSOCK address.
    pub fn new(cid: u32, port: u32) -> Self {
        Self { cid, port }
    }
}

/// A bus address
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum Address {
    /// A path on the filesystem
    Unix(OsString),
    /// TCP address details
    Tcp(TcpAddress),
    /// TCP address details with nonce file path
    NonceTcp {
        addr: TcpAddress,
        nonce_file: Vec<u8>,
    },
    /// Autolaunch address with optional scope
    Autolaunch(Option<String>),
    /// Launchd address with a required env key
    Launchd(String),
    #[cfg(any(
        all(feature = "vsock", not(feature = "tokio")),
        feature = "tokio-vsock"
    ))]
    /// VSOCK address
    ///
    /// This variant is only available when either `vsock` or `tokio-vsock` feature is enabled. The
    /// type of `stream` is `vsock::VsockStream` with `vsock` feature and
    /// `tokio_vsock::VsockStream` with `tokio-vsock` feature.
    Vsock(VsockAddress),
}

#[cfg(not(feature = "tokio"))]
#[derive(Debug)]
pub(crate) enum Stream {
    Unix(Async<UnixStream>),
    Tcp(Async<TcpStream>),
    #[cfg(feature = "vsock")]
    Vsock(Async<VsockStream>),
}

#[cfg(feature = "tokio")]
#[derive(Debug)]
pub(crate) enum Stream {
    #[cfg(unix)]
    Unix(UnixStream),
    Tcp(TcpStream),
    #[cfg(feature = "tokio-vsock")]
    Vsock(VsockStream),
}

#[cfg(not(feature = "tokio"))]
async fn connect_tcp(addr: TcpAddress) -> Result<Async<TcpStream>> {
    let addrs = crate::Task::spawn_blocking(
        move || -> Result<Vec<SocketAddr>> {
            let addrs = (addr.host(), addr.port()).to_socket_addrs()?.filter(|a| {
                if let Some(family) = addr.family() {
                    if family == TcpAddressFamily::Ipv4 {
                        a.is_ipv4()
                    } else {
                        a.is_ipv6()
                    }
                } else {
                    true
                }
            });
            Ok(addrs.collect())
        },
        "connect tcp",
    )
    .await
    .map_err(|e| Error::Address(format!("Failed to receive TCP addresses: {e}")))?;

    // we could attempt connections in parallel?
    let mut last_err = Error::Address("Failed to connect".into());
    for addr in addrs {
        match Async::<TcpStream>::connect(addr).await {
            Ok(stream) => return Ok(stream),
            Err(e) => last_err = e.into(),
        }
    }

    Err(last_err)
}

#[cfg(feature = "tokio")]
async fn connect_tcp(addr: TcpAddress) -> Result<TcpStream> {
    TcpStream::connect((addr.host(), addr.port()))
        .await
        .map_err(|e| Error::InputOutput(e.into()))
}

#[cfg(target_os = "macos")]
pub(crate) async fn macos_launchd_bus_address(env_key: &str) -> Result<Address> {
    let output = run("launchctl", ["getenv", env_key])
        .await
        .expect("failed to wait on launchctl output");

    if !output.status.success() {
        return Err(crate::Error::Address(format!(
            "launchctl terminated with code: {}",
            output.status
        )));
    }

    let addr = String::from_utf8(output.stdout).map_err(|e| {
        crate::Error::Address(format!("Unable to parse launchctl output as UTF-8: {}", e))
    })?;

    format!("unix:path={}", addr.trim()).parse()
}

impl Address {
    #[async_recursion::async_recursion]
    pub(crate) async fn connect(self) -> Result<Stream> {
        match self {
            Address::Unix(p) => {
                #[cfg(not(feature = "tokio"))]
                {
                    #[cfg(windows)]
                    {
                        let stream = crate::Task::spawn_blocking(
                            move || UnixStream::connect(p),
                            "unix stream connection",
                        )
                        .await?;
                        Async::new(stream)
                            .map(Stream::Unix)
                            .map_err(|e| Error::InputOutput(e.into()))
                    }

                    #[cfg(not(windows))]
                    {
                        Async::<UnixStream>::connect(p)
                            .await
                            .map(Stream::Unix)
                            .map_err(|e| Error::InputOutput(e.into()))
                    }
                }

                #[cfg(feature = "tokio")]
                {
                    #[cfg(unix)]
                    {
                        UnixStream::connect(p)
                            .await
                            .map(Stream::Unix)
                            .map_err(|e| Error::InputOutput(e.into()))
                    }

                    #[cfg(not(unix))]
                    {
                        let _ = p;
                        Err(Error::Unsupported)
                    }
                }
            }

            #[cfg(all(feature = "vsock", not(feature = "tokio")))]
            Address::Vsock(addr) => {
                let stream = VsockStream::connect_with_cid_port(addr.cid, addr.port)?;
                Async::new(stream).map(Stream::Vsock).map_err(Into::into)
            }

            #[cfg(feature = "tokio-vsock")]
            Address::Vsock(addr) => VsockStream::connect(addr.cid, addr.port)
                .await
                .map(Stream::Vsock)
                .map_err(Into::into),

            Address::Tcp(addr) => connect_tcp(addr).await.map(Stream::Tcp),

            Address::NonceTcp { addr, nonce_file } => {
                let mut stream = connect_tcp(addr).await?;

                #[cfg(unix)]
                let nonce_file = {
                    use std::os::unix::ffi::OsStrExt;
                    std::ffi::OsStr::from_bytes(&nonce_file)
                };

                #[cfg(windows)]
                let nonce_file = std::str::from_utf8(&nonce_file)
                    .map_err(|_| Error::Address("nonce file path is invalid UTF-8".to_owned()))?;

                #[cfg(not(feature = "tokio"))]
                {
                    let nonce = std::fs::read(nonce_file)?;
                    let mut nonce = &nonce[..];

                    while !nonce.is_empty() {
                        let len = stream
                            .write_with_mut(|s| std::io::Write::write(s, nonce))
                            .await?;
                        nonce = &nonce[len..];
                    }
                }

                #[cfg(feature = "tokio")]
                {
                    let nonce = tokio::fs::read(nonce_file).await?;
                    tokio::io::AsyncWriteExt::write_all(&mut stream, &nonce).await?;
                }

                Ok(Stream::Tcp(stream))
            }

            #[cfg(not(windows))]
            Address::Autolaunch(_) => Err(Error::Address(
                "Autolaunch addresses are only supported on Windows".to_owned(),
            )),

            #[cfg(windows)]
            Address::Autolaunch(Some(_)) => Err(Error::Address(
                "Autolaunch scopes are currently unsupported".to_owned(),
            )),

            #[cfg(windows)]
            Address::Autolaunch(None) => {
                let addr = windows_autolaunch_bus_address()?;
                addr.connect().await
            }

            #[cfg(not(target_os = "macos"))]
            Address::Launchd(_) => Err(Error::Address(
                "Launchd addresses are only supported on macOS".to_owned(),
            )),

            #[cfg(target_os = "macos")]
            Address::Launchd(env) => {
                let addr = macos_launchd_bus_address(&env).await?;
                addr.connect().await
            }
        }
    }

    /// Get the address for session socket respecting the DBUS_SESSION_BUS_ADDRESS environment
    /// variable. If we don't recognize the value (or it's not set) we fall back to
    /// $XDG_RUNTIME_DIR/bus
    pub fn session() -> Result<Self> {
        match env::var("DBUS_SESSION_BUS_ADDRESS") {
            Ok(val) => Self::from_str(&val),
            _ => {
                #[cfg(windows)]
                {
                    #[cfg(feature = "windows-gdbus")]
                    return Self::from_str("autolaunch:");

                    #[cfg(not(feature = "windows-gdbus"))]
                    return Self::from_str("autolaunch:scope=*user");
                }

                #[cfg(all(unix, not(target_os = "macos")))]
                {
                    let runtime_dir = env::var("XDG_RUNTIME_DIR")
                        .unwrap_or_else(|_| format!("/run/user/{}", Uid::effective()));
                    let path = format!("unix:path={runtime_dir}/bus");

                    Self::from_str(&path)
                }

                #[cfg(target_os = "macos")]
                return Self::from_str("launchd:env=DBUS_LAUNCHD_SESSION_BUS_SOCKET");
            }
        }
    }

    /// Get the address for system bus respecting the DBUS_SYSTEM_BUS_ADDRESS environment
    /// variable. If we don't recognize the value (or it's not set) we fall back to
    /// /var/run/dbus/system_bus_socket
    pub fn system() -> Result<Self> {
        match env::var("DBUS_SYSTEM_BUS_ADDRESS") {
            Ok(val) => Self::from_str(&val),
            _ => {
                #[cfg(all(unix, not(target_os = "macos")))]
                return Self::from_str("unix:path=/var/run/dbus/system_bus_socket");

                #[cfg(windows)]
                return Self::from_str("autolaunch:");

                #[cfg(target_os = "macos")]
                return Self::from_str("launchd:env=DBUS_LAUNCHD_SESSION_BUS_SOCKET");
            }
        }
    }

    // Helper for FromStr
    #[cfg(any(unix, not(feature = "tokio")))]
    fn from_unix(opts: HashMap<&str, &str>) -> Result<Self> {
        let path = if let Some(abs) = opts.get("abstract") {
            if opts.get("path").is_some() {
                return Err(Error::Address(
                    "`path` and `abstract` cannot be specified together".into(),
                ));
            }
            let mut s = OsString::from("\0");
            s.push(abs);
            s
        } else if let Some(path) = opts.get("path") {
            OsString::from(path)
        } else {
            return Err(Error::Address(
                "unix address is missing path or abstract".to_owned(),
            ));
        };

        Ok(Address::Unix(path))
    }

    #[cfg(all(feature = "vsock", not(feature = "tokio")))]
    fn from_vsock(opts: HashMap<&str, &str>) -> Result<Self> {
        let cid = opts
            .get("cid")
            .ok_or_else(|| Error::Address("VSOCK address is missing cid=".into()))?;
        let cid = cid
            .parse::<u32>()
            .map_err(|e| Error::Address(format!("Failed to parse VSOCK cid `{}`: {}", cid, e)))?;
        let port = opts
            .get("port")
            .ok_or_else(|| Error::Address("VSOCK address is missing port=".into()))?;
        let port = port
            .parse::<u32>()
            .map_err(|e| Error::Address(format!("Failed to parse VSOCK port `{}`: {}", port, e)))?;

        Ok(Address::Vsock(VsockAddress { cid, port }))
    }
}

impl FromStr for TcpAddressFamily {
    type Err = Error;

    fn from_str(family: &str) -> Result<Self> {
        match family {
            "ipv4" => Ok(Self::Ipv4),
            "ipv6" => Ok(Self::Ipv6),
            _ => Err(Error::Address(format!(
                "invalid tcp address `family`: {family}"
            ))),
        }
    }
}

impl Display for TcpAddressFamily {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Ipv4 => write!(f, "ipv4"),
            Self::Ipv6 => write!(f, "ipv6"),
        }
    }
}

fn decode_hex(c: char) -> Result<u8> {
    match c {
        '0'..='9' => Ok(c as u8 - b'0'),
        'a'..='f' => Ok(c as u8 - b'a' + 10),
        'A'..='F' => Ok(c as u8 - b'A' + 10),

        _ => Err(Error::Address(
            "invalid hexadecimal character in percent-encoded sequence".to_owned(),
        )),
    }
}

fn decode_percents(value: &str) -> Result<Vec<u8>> {
    let mut iter = value.chars();
    let mut decoded = Vec::new();

    while let Some(c) = iter.next() {
        if matches!(c, '-' | '0'..='9' | 'A'..='Z' | 'a'..='z' | '_' | '/' | '.' | '\\' | '*') {
            decoded.push(c as u8)
        } else if c == '%' {
            decoded.push(
                decode_hex(iter.next().ok_or_else(|| {
                    Error::Address("incomplete percent-encoded sequence".to_owned())
                })?)?
                    << 4
                    | decode_hex(iter.next().ok_or_else(|| {
                        Error::Address("incomplete percent-encoded sequence".to_owned())
                    })?)?,
            );
        } else {
            return Err(Error::Address("Invalid character in address".to_owned()));
        }
    }

    Ok(decoded)
}

fn encode_percents(f: &mut Formatter<'_>, mut value: &[u8]) -> std::fmt::Result {
    const LOOKUP: &str = "\
%00%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f\
%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%1f\
%20%21%22%23%24%25%26%27%28%29%2a%2b%2c%2d%2e%2f\
%30%31%32%33%34%35%36%37%38%39%3a%3b%3c%3d%3e%3f\
%40%41%42%43%44%45%46%47%48%49%4a%4b%4c%4d%4e%4f\
%50%51%52%53%54%55%56%57%58%59%5a%5b%5c%5d%5e%5f\
%60%61%62%63%64%65%66%67%68%69%6a%6b%6c%6d%6e%6f\
%70%71%72%73%74%75%76%77%78%79%7a%7b%7c%7d%7e%7f\
%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f\
%90%91%92%93%94%95%96%97%98%99%9a%9b%9c%9d%9e%9f\
%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af\
%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%ba%bb%bc%bd%be%bf\
%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf\
%d0%d1%d2%d3%d4%d5%d6%d7%d8%d9%da%db%dc%dd%de%df\
%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef\
%f0%f1%f2%f3%f4%f5%f6%f7%f8%f9%fa%fb%fc%fd%fe%ff";

    loop {
        let pos = value.iter().position(
            |c| !matches!(c, b'-' | b'0'..=b'9' | b'A'..=b'Z' | b'a'..=b'z' | b'_' | b'/' | b'.' | b'\\' | b'*'),
        );

        if let Some(pos) = pos {
            // SAFETY: The above `position()` call made sure that only ASCII chars are in the string
            // up to `pos`
            f.write_str(unsafe { from_utf8_unchecked(&value[..pos]) })?;

            let c = value[pos];
            value = &value[pos + 1..];

            let pos = c as usize * 3;
            f.write_str(&LOOKUP[pos..pos + 3])?;
        } else {
            // SAFETY: The above `position()` call made sure that only ASCII chars are in the rest
            // of the string
            f.write_str(unsafe { from_utf8_unchecked(value) })?;
            return Ok(());
        }
    }
}

impl Display for Address {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Tcp(addr) => {
                f.write_str("tcp:")?;
                addr.write_options(f)?;
            }

            Self::NonceTcp { addr, nonce_file } => {
                f.write_str("nonce-tcp:noncefile=")?;
                encode_percents(f, nonce_file)?;
                f.write_str(",")?;
                addr.write_options(f)?;
            }

            Self::Unix(path) => {
                #[cfg(unix)]
                {
                    use std::os::unix::ffi::OsStrExt;
                    f.write_str("unix:path=")?;
                    encode_percents(f, path.as_bytes())?;
                }

                #[cfg(windows)]
                write!(f, "unix:path={}", path.to_str().ok_or(std::fmt::Error)?)?;
            }

            #[cfg(any(
                all(feature = "vsock", not(feature = "tokio")),
                feature = "tokio-vsock"
            ))]
            Self::Vsock(addr) => {
                write!(f, "vsock:cid={},port={}", addr.cid, addr.port)?;
            }

            Self::Autolaunch(scope) => {
                write!(f, "autolaunch:")?;
                if let Some(scope) = scope {
                    write!(f, "scope={scope}")?;
                }
            }

            Self::Launchd(env) => {
                write!(f, "launchd:env={}", env)?;
            }
        }

        Ok(())
    }
}

impl FromStr for Address {
    type Err = Error;

    /// Parse a D-BUS address and return its path if we recognize it
    fn from_str(address: &str) -> Result<Self> {
        let col = address
            .find(':')
            .ok_or_else(|| Error::Address("address has no colon".to_owned()))?;
        let transport = &address[..col];
        let mut options = HashMap::new();

        if address.len() > col + 1 {
            for kv in address[col + 1..].split(',') {
                let (k, v) = match kv.find('=') {
                    Some(eq) => (&kv[..eq], &kv[eq + 1..]),
                    None => {
                        return Err(Error::Address(
                            "missing = when parsing key/value".to_owned(),
                        ))
                    }
                };
                if options.insert(k, v).is_some() {
                    return Err(Error::Address(format!(
                        "Key `{k}` specified multiple times"
                    )));
                }
            }
        }

        match transport {
            #[cfg(any(unix, not(feature = "tokio")))]
            "unix" => Self::from_unix(options),
            "tcp" => TcpAddress::from_tcp(options).map(Self::Tcp),

            "nonce-tcp" => Ok(Self::NonceTcp {
                nonce_file: decode_percents(
                    options
                        .get("noncefile")
                        .ok_or_else(|| Error::Address("missing nonce file parameter".into()))?,
                )?,
                addr: TcpAddress::from_tcp(options)?,
            }),
            #[cfg(all(feature = "vsock", not(feature = "tokio")))]
            "vsock" => Self::from_vsock(options),
            "autolaunch" => Ok(Self::Autolaunch(
                options
                    .get("scope")
                    .map(|scope| -> Result<_> {
                        String::from_utf8(decode_percents(scope)?).map_err(|_| {
                            Error::Address("autolaunch scope is not valid UTF-8".to_owned())
                        })
                    })
                    .transpose()?,
            )),
            "launchd" => Ok(Self::Launchd(
                options
                    .get("env")
                    .ok_or_else(|| Error::Address("missing env key".into()))?
                    .to_string(),
            )),

            _ => Err(Error::Address(format!(
                "unsupported transport '{transport}'"
            ))),
        }
    }
}

impl TryFrom<&str> for Address {
    type Error = Error;

    fn try_from(value: &str) -> Result<Self> {
        Self::from_str(value)
    }
}

#[cfg(test)]
mod tests {
    use super::Address;
    use crate::{Error, TcpAddress, TcpAddressFamily};
    use std::str::FromStr;
    use test_log::test;

    #[test]
    fn parse_dbus_addresses() {
        match Address::from_str("").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "address has no colon"),
            _ => panic!(),
        }
        match Address::from_str("foo").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "address has no colon"),
            _ => panic!(),
        }
        match Address::from_str("foo:opt").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "missing = when parsing key/value"),
            _ => panic!(),
        }
        match Address::from_str("foo:opt=1,opt=2").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "Key `opt` specified multiple times"),
            _ => panic!(),
        }
        match Address::from_str("tcp:host=localhost").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "tcp address is missing `port`"),
            _ => panic!(),
        }
        match Address::from_str("tcp:host=localhost,port=32f").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "invalid tcp `port`"),
            _ => panic!(),
        }
        match Address::from_str("tcp:host=localhost,port=123,family=ipv7").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "invalid tcp address `family`: ipv7"),
            _ => panic!(),
        }
        match Address::from_str("unix:foo=blah").unwrap_err() {
            Error::Address(e) => assert_eq!(e, "unix address is missing path or abstract"),
            _ => panic!(),
        }
        match Address::from_str("unix:path=/tmp,abstract=foo").unwrap_err() {
            Error::Address(e) => {
                assert_eq!(e, "`path` and `abstract` cannot be specified together")
            }
            _ => panic!(),
        }
        assert_eq!(
            Address::Unix("/tmp/dbus-foo".into()),
            Address::from_str("unix:path=/tmp/dbus-foo").unwrap()
        );
        assert_eq!(
            Address::Unix("/tmp/dbus-foo".into()),
            Address::from_str("unix:path=/tmp/dbus-foo,guid=123").unwrap()
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: None
            }),
            Address::from_str("tcp:host=localhost,port=4142").unwrap()
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: Some(TcpAddressFamily::Ipv4)
            }),
            Address::from_str("tcp:host=localhost,port=4142,family=ipv4").unwrap()
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: Some(TcpAddressFamily::Ipv6)
            }),
            Address::from_str("tcp:host=localhost,port=4142,family=ipv6").unwrap()
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: Some(TcpAddressFamily::Ipv6)
            }),
            Address::from_str("tcp:host=localhost,port=4142,family=ipv6,noncefile=/a/file/path")
                .unwrap()
        );
        assert_eq!(
            Address::NonceTcp {
                addr: TcpAddress {
                    host: "localhost".into(),
                    port: 4142,
                    bind: None,
                    family: Some(TcpAddressFamily::Ipv6),
                },
                nonce_file: b"/a/file/path to file 1234".to_vec()
            },
            Address::from_str(
                "nonce-tcp:host=localhost,port=4142,family=ipv6,noncefile=/a/file/path%20to%20file%201234"
            )
            .unwrap()
        );
        assert_eq!(
            Address::Autolaunch(None),
            Address::from_str("autolaunch:").unwrap()
        );
        assert_eq!(
            Address::Autolaunch(Some("*my_cool_scope*".to_owned())),
            Address::from_str("autolaunch:scope=*my_cool_scope*").unwrap()
        );
        assert_eq!(
            Address::Launchd("my_cool_env_key".to_owned()),
            Address::from_str("launchd:env=my_cool_env_key").unwrap()
        );

        #[cfg(all(feature = "vsock", not(feature = "tokio")))]
        assert_eq!(
            Address::Vsock(crate::VsockAddress {
                cid: 98,
                port: 2934
            }),
            Address::from_str("vsock:cid=98,port=2934,guid=123").unwrap()
        );
    }

    #[test]
    fn stringify_dbus_addresses() {
        assert_eq!(
            Address::Unix("/tmp/dbus-foo".into()).to_string(),
            "unix:path=/tmp/dbus-foo"
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: None
            })
            .to_string(),
            "tcp:host=localhost,port=4142"
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: Some(TcpAddressFamily::Ipv4)
            })
            .to_string(),
            "tcp:host=localhost,port=4142,family=ipv4"
        );
        assert_eq!(
            Address::Tcp(TcpAddress {
                host: "localhost".into(),
                port: 4142,
                bind: None,
                family: Some(TcpAddressFamily::Ipv6)
            })
            .to_string(),
            "tcp:host=localhost,port=4142,family=ipv6"
        );
        assert_eq!(
            Address::NonceTcp {
                addr: TcpAddress {
                    host: "localhost".into(),
                    port: 4142,
                    bind: None,
                    family: Some(TcpAddressFamily::Ipv6),
                },
                nonce_file: b"/a/file/path to file 1234".to_vec()
            }
            .to_string(),
            "nonce-tcp:noncefile=/a/file/path%20to%20file%201234,host=localhost,port=4142,family=ipv6"
        );
        assert_eq!(Address::Autolaunch(None).to_string(), "autolaunch:");
        assert_eq!(
            Address::Autolaunch(Some("*my_cool_scope*".to_owned())).to_string(),
            "autolaunch:scope=*my_cool_scope*"
        );
        assert_eq!(
            Address::Launchd("my_cool_key".to_owned()).to_string(),
            "launchd:env=my_cool_key"
        );

        #[cfg(all(feature = "vsock", not(feature = "tokio")))]
        assert_eq!(
            Address::Vsock(crate::VsockAddress {
                cid: 98,
                port: 2934
            })
            .to_string(),
            "vsock:cid=98,port=2934,guid=123",
        );
    }

    #[test]
    fn connect_tcp() {
        let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
        let port = listener.local_addr().unwrap().port();
        let addr = Address::from_str(&format!("tcp:host=localhost,port={port}")).unwrap();
        crate::utils::block_on(async { addr.connect().await }).unwrap();
    }

    #[test]
    fn connect_nonce_tcp() {
        struct PercentEncoded<'a>(&'a [u8]);

        impl std::fmt::Display for PercentEncoded<'_> {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                super::encode_percents(f, self.0)
            }
        }

        use std::io::Write;

        const TEST_COOKIE: &[u8] = b"VERILY SECRETIVE";

        let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
        let port = listener.local_addr().unwrap().port();

        let mut cookie = tempfile::NamedTempFile::new().unwrap();
        cookie.as_file_mut().write_all(TEST_COOKIE).unwrap();

        let encoded_path = format!(
            "{}",
            PercentEncoded(cookie.path().to_str().unwrap().as_ref())
        );

        let addr = Address::from_str(&format!(
            "nonce-tcp:host=localhost,port={port},noncefile={encoded_path}"
        ))
        .unwrap();

        let (sender, receiver) = std::sync::mpsc::sync_channel(1);

        std::thread::spawn(move || {
            use std::io::Read;

            let mut client = listener.incoming().next().unwrap().unwrap();

            let mut buf = [0u8; 16];
            client.read_exact(&mut buf).unwrap();

            sender.send(buf == TEST_COOKIE).unwrap();
        });

        crate::utils::block_on(addr.connect()).unwrap();

        let saw_cookie = receiver
            .recv_timeout(std::time::Duration::from_millis(100))
            .expect("nonce file content hasn't been received by server thread in time");

        assert!(
            saw_cookie,
            "nonce file content has been received, but was invalid"
        );
    }
}
