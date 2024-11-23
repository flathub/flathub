use std::{net::SocketAddr, convert::TryInto, fmt::{Display, Debug}, sync::Arc, io, pin::Pin, task::{Context, Poll}};
use async_trait::async_trait;
use stun::{message::{Getter, Setter}, attributes::ATTR_CONNECTION_ID};
use tokio::{net::TcpStream, io::{split, ReadBuf, AsyncReadExt, AsyncWriteExt, AsyncRead, AsyncWrite, ReadHalf, WriteHalf}, sync::RwLock};
use tokio_rustls::{client::TlsStream, TlsConnector, rustls::{ClientConfig, ServerName}};
use util::Conn;


#[derive(Debug)]
pub enum MaybeTlsStream {
    Plain(TcpStream),
    Rustls(TlsStream<TcpStream>),
}

impl AsyncRead for MaybeTlsStream {
    fn poll_read(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<std::io::Result<()>> {
        match self.get_mut() {
            MaybeTlsStream::Plain(ref mut s) => Pin::new(s).poll_read(cx, buf),
            MaybeTlsStream::Rustls(s) => Pin::new(s).poll_read(cx, buf),
        }
    }
}

impl AsyncWrite for MaybeTlsStream {
    fn poll_write(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &[u8],
    ) -> Poll<Result<usize, std::io::Error>> {
        match self.get_mut() {
            MaybeTlsStream::Plain(ref mut s) => Pin::new(s).poll_write(cx, buf),
            MaybeTlsStream::Rustls(s) => Pin::new(s).poll_write(cx, buf),
        }
    }

    fn poll_flush(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Result<(), std::io::Error>> {
        match self.get_mut() {
            MaybeTlsStream::Plain(ref mut s) => Pin::new(s).poll_flush(cx),
            MaybeTlsStream::Rustls(s) => Pin::new(s).poll_flush(cx),
        }
    }

    fn poll_shutdown(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
    ) -> Poll<Result<(), std::io::Error>> {
        match self.get_mut() {
            MaybeTlsStream::Plain(ref mut s) => Pin::new(s).poll_shutdown(cx),
            MaybeTlsStream::Rustls(s) => Pin::new(s).poll_shutdown(cx),
        }
    }
}

#[derive(Debug)]
pub struct TcpTurn {
    reader: RwLock<Option<ReadHalf<MaybeTlsStream>>>,
    writer: RwLock<Option<WriteHalf<MaybeTlsStream>>>,
    local_addr: SocketAddr,
    peer_addr: Option<SocketAddr>,
}

impl From<TcpStream> for TcpTurn {
    fn from(stream: TcpStream) -> Self {
        let local_addr = stream.local_addr().unwrap();
        let peer_addr = stream.peer_addr().ok();
        let (reader, writer) = split(MaybeTlsStream::Plain(stream));
        Self {
            reader: RwLock::new(Some(reader)),
            writer: RwLock::new(Some(writer)),
            local_addr,
            peer_addr,
        }
    }
}

impl TcpTurn {
    pub async fn new_tls(config: Arc<ClientConfig>, stream: TcpStream, domain: ServerName) -> io::Result<Self> {
        let local_addr = stream.local_addr()?;
        let peer_addr = stream.peer_addr().ok();
        let tls_stream = TlsConnector::from(config).connect(domain, stream).await?;
        let (reader, writer) = split(MaybeTlsStream::Rustls(tls_stream));
        Ok(Self {
            reader: RwLock::new(Some(reader)),
            writer: RwLock::new(Some(writer)),
            local_addr,
            peer_addr,
        })
    }

    pub async fn into_stream(&self) -> MaybeTlsStream {
        self.reader.write().await.take().unwrap().unsplit(self.writer.write().await.take().unwrap())
    }
}

#[async_trait]
impl Conn for TcpTurn {
    async fn connect(&self, _: SocketAddr) -> util::Result<()> {
        unimplemented!();
    }

    async fn recv(&self, buf: &mut [u8]) -> util::Result<usize> {
        Ok(self.reader.write().await.as_mut().unwrap().read(buf).await?)
    }

    async fn recv_from(&self, buf: &mut [u8]) -> util::Result<(usize, SocketAddr)> {
        Ok((self.recv(buf).await?, self.peer_addr.ok_or(util::Error::ErrNoRemAddr)?))
    }

    async fn send(&self, buf: &[u8]) -> util::Result<usize> {
        Ok(self.writer.write().await.as_mut().unwrap().write(buf).await?)
    }

    async fn send_to(&self, buf: &[u8], _: SocketAddr) -> util::Result<usize> {
        self.send(buf).await
    }

    fn local_addr(&self) -> util::Result<SocketAddr> {
        Ok(self.local_addr)
    }

    fn remote_addr(&self) -> Option<SocketAddr> {
        self.peer_addr
    }

    async fn close(&self) -> util::Result<()> {
        Ok(())
    }
}


#[derive(Default)]
pub struct ConnectionID(u32);

impl Display for ConnectionID {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl Getter for ConnectionID {
    fn get_from(&mut self, m: &stun::message::Message) -> Result<(), stun::Error> {
        let value = m.get(ATTR_CONNECTION_ID)?;
        self.0 = u32::from_be_bytes(value.try_into().map_err(|_| stun::Error::ErrAttributeSizeInvalid)?);
        Ok(())
    }
}

impl Setter for ConnectionID {
    fn add_to(&self, m: &mut stun::message::Message) -> Result<(), stun::Error> {
        m.add(ATTR_CONNECTION_ID, &self.0.to_be_bytes());
        Ok(())
    }
}
