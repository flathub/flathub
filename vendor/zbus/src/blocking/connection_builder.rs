use static_assertions::assert_impl_all;
use std::convert::TryInto;
#[cfg(not(feature = "tokio"))]
use std::net::TcpStream;
#[cfg(all(unix, not(feature = "tokio")))]
use std::os::unix::net::UnixStream;
#[cfg(feature = "tokio")]
use tokio::net::TcpStream;
#[cfg(all(unix, feature = "tokio"))]
use tokio::net::UnixStream;
#[cfg(windows)]
use uds_windows::UnixStream;

use zvariant::{ObjectPath, Str};

use crate::{
    address::Address,
    blocking::Connection,
    names::{UniqueName, WellKnownName},
    utils::block_on,
    AuthMechanism, Error, Guid, Interface, Result,
};

/// A builder for [`zbus::blocking::Connection`].
#[derive(Debug)]
#[must_use]
pub struct ConnectionBuilder<'a>(crate::ConnectionBuilder<'a>);

assert_impl_all!(ConnectionBuilder<'_>: Send, Sync, Unpin);

impl<'a> ConnectionBuilder<'a> {
    /// Create a builder for the session/user message bus connection.
    pub fn session() -> Result<Self> {
        crate::ConnectionBuilder::session().map(Self)
    }

    /// Create a builder for the system-wide message bus connection.
    pub fn system() -> Result<Self> {
        crate::ConnectionBuilder::system().map(Self)
    }

    /// Create a builder for connection that will use the given [D-Bus bus address].
    ///
    /// [D-Bus bus address]: https://dbus.freedesktop.org/doc/dbus-specification.html#addresses
    pub fn address<A>(address: A) -> Result<Self>
    where
        A: TryInto<Address>,
        A::Error: Into<Error>,
    {
        crate::ConnectionBuilder::address(address).map(Self)
    }

    /// Create a builder for connection that will use the given unix stream.
    ///
    /// If the default `async-io` feature is disabled, this method will expect
    /// [`tokio::net::UnixStream`](https://docs.rs/tokio/latest/tokio/net/struct.UnixStream.html)
    /// argument.
    pub fn unix_stream(stream: UnixStream) -> Self {
        Self(crate::ConnectionBuilder::unix_stream(stream))
    }

    /// Create a builder for connection that will use the given TCP stream.
    ///
    /// If the default `async-io` feature is disabled, this method will expect
    /// [`tokio::net::TcpStream`](https://docs.rs/tokio/latest/tokio/net/struct.TcpStream.html)
    /// argument.
    pub fn tcp_stream(stream: TcpStream) -> Self {
        Self(crate::ConnectionBuilder::tcp_stream(stream))
    }

    /// Specify the mechanisms to use during authentication.
    pub fn auth_mechanisms(self, auth_mechanisms: &[AuthMechanism]) -> Self {
        Self(self.0.auth_mechanisms(auth_mechanisms))
    }

    /// The cookie context to use during authentication.
    ///
    /// This is only used when the `cookie` authentication mechanism is enabled and only valid for
    /// server connection.
    ///
    /// If not specified, the default cookie context of `org_freedesktop_general` will be used.
    ///
    /// # Errors
    ///
    /// If the given string is not a valid cookie context.
    pub fn cookie_context<C>(self, context: C) -> Result<Self>
    where
        C: Into<Str<'a>>,
    {
        self.0.cookie_context(context).map(Self)
    }

    /// The ID of the cookie to use during authentication.
    ///
    /// This is only used when the `cookie` authentication mechanism is enabled and only valid for
    /// server connection.
    ///
    /// If not specified, the first cookie found in the cookie context file will be used.
    pub fn cookie_id(self, id: usize) -> Self {
        Self(self.0.cookie_id(id))
    }

    /// The to-be-created connection will be a peer-to-peer connection.
    pub fn p2p(self) -> Self {
        Self(self.0.p2p())
    }

    /// The to-be-created connection will be a server using the given GUID.
    ///
    /// The to-be-created connection will wait for incoming client authentication handshake and
    /// negotiation messages, for peer-to-peer communications after successful creation.
    pub fn server(self, guid: &'a Guid) -> Self {
        Self(self.0.server(guid))
    }

    /// Set the capacity of the main (unfiltered) queue.
    ///
    /// Since typically you'd want to set this at instantiation time, you can set it through the
    /// builder.
    ///
    /// # Example
    ///
    /// ```
    /// # use std::error::Error;
    /// # use zbus::blocking::ConnectionBuilder;
    /// #
    /// let conn = ConnectionBuilder::session()?
    ///     .max_queued(30)
    ///     .build()?;
    /// assert_eq!(conn.max_queued(), 30);
    ///
    /// // Do something useful with `conn`..
    /// # Ok::<_, Box<dyn Error + Send + Sync>>(())
    /// ```
    pub fn max_queued(self, max: usize) -> Self {
        Self(self.0.max_queued(max))
    }

    /// Register a D-Bus [`Interface`] to be served at a given path.
    ///
    /// This is similar to [`zbus::blocking::ObjectServer::at`], except that it allows you to have
    /// your interfaces available immediately after the connection is established. Typically, this
    /// is exactly what you'd want. Also in contrast to [`zbus::blocking::ObjectServer::at`], this
    /// method will replace any previously added interface with the same name at the same path.
    pub fn serve_at<P, I>(self, path: P, iface: I) -> Result<Self>
    where
        I: Interface,
        P: TryInto<ObjectPath<'a>>,
        P::Error: Into<Error>,
    {
        self.0.serve_at(path, iface).map(Self)
    }

    /// Register a well-known name for this connection on the bus.
    ///
    /// This is similar to [`zbus::blocking::Connection::request_name`], except the name is
    /// requested as part of the connection setup ([`ConnectionBuilder::build`]), immediately after
    /// interfaces registered (through [`ConnectionBuilder::serve_at`]) are advertised. Typically
    /// this is exactly what you want.
    pub fn name<W>(self, well_known_name: W) -> Result<Self>
    where
        W: TryInto<WellKnownName<'a>>,
        W::Error: Into<Error>,
    {
        self.0.name(well_known_name).map(Self)
    }

    /// Sets the unique name of the connection.
    ///
    /// # Panics
    ///
    /// This method panics if the to-be-created connection is not a peer-to-peer connection.
    /// It will always panic if the connection is to a message bus as it's the bus that assigns
    /// peers their unique names. This is mainly provided for bus implementations. All other users
    /// should not need to use this method.
    pub fn unique_name<U>(self, unique_name: U) -> Result<Self>
    where
        U: TryInto<UniqueName<'a>>,
        U::Error: Into<Error>,
    {
        self.0.unique_name(unique_name).map(Self)
    }

    /// Build the connection, consuming the builder.
    ///
    /// # Errors
    ///
    /// Until server-side bus connection is supported, attempting to build such a connection will
    /// result in [`Error::Unsupported`] error.
    pub fn build(self) -> Result<Connection> {
        block_on(self.0.build()).map(Into::into)
    }
}
