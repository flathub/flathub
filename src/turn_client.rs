use futures::SinkExt;
use hbb_common::{
    bail, lazy_static, log, socket_client,
    tcp::FramedStream,
    tokio::{self, net::TcpStream, sync::mpsc, time::timeout},
    ResultType,
};
use std::sync::Mutex;
use std::{
    net::{IpAddr, SocketAddr},
    sync::Arc,
    time::{Duration, Instant},
};
use tokio_rustls::rustls::{self, ClientConfig as TlsClientConfig, OwnedTrustAnchor};
use tokio_tungstenite::tungstenite::Message as WsMessage;
use turn::client::{tcp::TcpTurn, ClientConfig, TlsConfig};
use webrtc_util::conn::Conn;

use crate::rendezvous_messages::{self, ToJson};

lazy_static::lazy_static! {
    static ref PUBLIC_IP: Arc<Mutex<Option<(IpAddr, SocketAddr, Instant)>>> = Default::default();
}

#[derive(Debug)]
pub struct TurnConfig {
    addr: String,
    username: String,
    password: String,
    tls_config: Option<TlsConfig>,
}

async fn get_turn_servers() -> Option<Vec<TurnConfig>> {
    let mut root_cert_store = rustls::RootCertStore::empty();
    root_cert_store.add_server_trust_anchors(webpki_roots::TLS_SERVER_ROOTS.0.iter().map(|ta| {
	//root_cert_store.add_trust_anchors(webpki_roots::TLS_SERVER_ROOTS.0.iter().map(|ta| {
        OwnedTrustAnchor::from_subject_spki_name_constraints(
            ta.subject,
            ta.spki,
            ta.name_constraints,
        )
    }));
    let tls_config = Arc::new(
        TlsClientConfig::builder()
            .with_safe_defaults()
            .with_root_certificates(root_cert_store)
            .with_no_client_auth(),
    );

    let map = hbb_common::api::call_api().await.ok()?;
    let mut servers = Vec::new();
    for server in map["turnservers"].as_array()? {
        if server["protocol"].as_str()? == "turn" {
            servers.push(TurnConfig {
                addr: format!("{}:{}", server["host"].as_str()?, server["port"].as_str()?),
                username: server["username"].as_str()?.to_string(),
                password: server["password"].as_str()?.to_string(),
                tls_config: None,
            });
        } else if server["protocol"].as_str()? == "turn-tls" {
            servers.push(TurnConfig {
                addr: format!("{}:{}", server["host"].as_str()?, server["port"].as_str()?),
                username: server["username"].as_str()?.to_string(),
                password: server["password"].as_str()?.to_string(),
                tls_config: Some(TlsConfig {
                    client_config: tls_config.clone(),
                    domain: server["host"].as_str()?.try_into().unwrap(),
                }),
            });
        }
    }
    Some(servers)
}

pub async fn connect_over_turn_servers(
    peer_id: &str,
    peer_addr: SocketAddr,
    sender: Arc<tokio::sync::Mutex<crate::client::WsSender>>,
) -> ResultType<(Arc<impl Conn>, FramedStream)> {
    let turn_servers = match get_turn_servers().await {
        Some(servers) => servers,
        None => bail!("empty turn servers!"),
    };
    let srv_len = turn_servers.len();
    let (tx, mut rx) = mpsc::channel(srv_len);
    let mut handles = Vec::new();
    for config in turn_servers {
        let sender = sender.clone();
        let peer_id = peer_id.to_owned();
        let tx = tx.clone();
        let handle = tokio::spawn(async move {
            let turn_server = config.addr.clone();
            log::info!(
                "[turn] start establishing over TURN server: {}",
                turn_server
            );
            let conn = match timeout(
                tokio::time::Duration::from_secs(7),
                create_relay_connection(config, &peer_id, peer_addr, sender.clone()),
            )
            .await
            {
                Ok(conn) => conn,
                Err(err) => {
                    log::warn!(
                        "[turn] didn't establish over TURN server: {} - {}",
                        turn_server,
                        err
                    );
                    None
                }
            };
            if conn.is_none() {
                log::warn!("[turn] didn't establish over TURN server: {}", turn_server);
            } else {
                log::info!("[turn] established over TURN server: {}", turn_server);
            }
            if tx.send(conn).await.is_err() {
                log::warn!("failed to send result to channel: {}", turn_server);
            }
        });
        handles.push(handle);
    }
    for handle in handles {
        handle.await?;
    }
    drop(tx); // drop tx to end the channel
    while let Some(ret) = rx.recv().await {
        if let Some(ret) = ret {
            return Ok(ret);
        }
    }
    bail!("Failed to connect via relay server: all candidates are failed!")
}

async fn create_relay_connection(
    config: TurnConfig,
    peer_id: &str,
    peer_addr: SocketAddr,
    sender: Arc<tokio::sync::Mutex<crate::client::WsSender>>,
) -> Option<(Arc<impl Conn>, FramedStream)> {
    if let Ok(turn_client) = TurnClient::new(config).await {
        match turn_client.create_relay_connection(peer_addr).await {
            Ok(relay) => {
                let conn = relay.0;
                let relay_addr = relay.1;
                if let Ok(stream) =
                    establish_over_relay(&peer_id, turn_client, relay_addr, sender).await
                {
                    return Some((conn, stream));
                }
            }
            Err(err) => log::warn!("create relay conn failed: {}", err),
        }
    }
    return None;
}

async fn establish_over_relay(
    peer_id: &str,
    turn_client: TurnClient,
    relay_addr: SocketAddr,
    sender: Arc<tokio::sync::Mutex<crate::client::WsSender>>,
) -> ResultType<FramedStream> {
    let mut sender = sender.lock().await;
    sender
        .send(WsMessage::Text(
            rendezvous_messages::RelayConnection::new(peer_id, relay_addr).to_json(),
        ))
        .await?;
    match turn_client.wait_new_connection().await {
        Ok(stream) => {
            sender
                .send(WsMessage::Text(
                    rendezvous_messages::RelayReady::new(peer_id).to_json(),
                ))
                .await?;
            let _ = sender.close().await; // close after established
            return Ok(stream);
        }
        Err(e) => bail!("Failed to connect via relay server: {}", e),
    }
}

pub async fn get_public_ip() -> Option<SocketAddr> {
    {
        let mut cached = PUBLIC_IP.lock().unwrap();
        if let Some((cached_local_ip, public_ip, cached_at)) = *cached {


            //  Time since cached is in 10 minutes.
            if cached_at.elapsed() < Duration::from_secs(600) {
				//use hbb_common::{log};
				//log::info!("get_public_ip {:?}, {:?}", public_ip, cached_at.elapsed());

                let local_ip = socket_client::get_lan_ipv4().ok()?;
                // The network environment shouldn't be changed,
                // as the local ip haven't changed.
                if cached_local_ip == local_ip {
                    log::info!("Got public ip from cache");
                    return Some(public_ip);
                }
            }
        }
        *cached = None;
    }
    let servers = get_turn_servers().await?;
    let len = servers.len();
    let (tx, mut rx) = tokio::sync::mpsc::channel(len);
    for config in servers {
        let tx = tx.clone();
        tokio::spawn(async move {
            //log::info!("start retrieve public ip via: {}", config.addr);
            //let turn_addr = config.addr.clone();
            if let Ok(turn_client) = TurnClient::new(config).await {
                if let Ok(addr) = turn_client.get_public_ip().await {
                    //tx.send(Some(addr)).await;
                    match tx.send(Some(addr)).await {
                        Ok(()) => {}
                        Err(_) => {}
                    }

                    //log::info!("Got public ip via {}", turn_addr);
                    return;
                }
            }
            match tx.send(None).await {
                Ok(()) => {}
                Err(_) => {}
            }
            //tx.send(None).await;
        });
    }
    for _ in 0..len {
        if let Some(addr) = rx.recv().await {
            if addr.is_some() {
                if let Ok(local_ip) = socket_client::get_lan_ipv4() {
                    let mut cached = PUBLIC_IP.lock().unwrap();
                    *cached = Some((local_ip, addr.unwrap(), Instant::now()));
                }
                return addr;
            }
        }
    }
    return None;
}

pub struct TurnClient {
    client: turn::client::Client,
    local_addr: SocketAddr,
}

impl TurnClient {
    pub async fn new(config: TurnConfig) -> ResultType<Self> {
        let stream = TcpStream::connect(&config.addr).await?;
        let local_addr = stream.local_addr()?;
        let tcp_turn = if let Some(tls) = config.tls_config.as_ref() {
            TcpTurn::new_tls(tls.client_config.clone(), stream, tls.domain.clone()).await?
        } else {
            TcpTurn::from(stream)
        };
        let mut client = turn::client::Client::new(ClientConfig {
            stun_serv_addr: config.addr.clone(),
            turn_serv_addr: config.addr,
            username: config.username,
            password: config.password,
            realm: String::new(),
            tls_config: config.tls_config,
            software: String::new(),
            rto_in_ms: 0,
            conn: Arc::new(tcp_turn),
            vnet: None,
        })
        .await?;
        client.listen().await?;
        Ok(Self { client, local_addr })
    }

    pub async fn get_public_ip(&self) -> ResultType<SocketAddr> {
        Ok(self.client.send_binding_request().await?)
    }

    pub async fn create_relay_connection(
        &self,
        peer_addr: SocketAddr,
    ) -> ResultType<(Arc<impl Conn>, SocketAddr)> {
        let relay_connection = self.client.allocate().await?;
        relay_connection.send_to(b"init", peer_addr).await?;
        let local_addr = relay_connection.local_addr()?;

        Ok((
            // Avoid the conn to be dropped, otherwise the timer in it will be
            // stopped. That will stop to send refresh transaction periodically.
            // More detail to check:
            //
            //   https://datatracker.ietf.org/doc/html/rfc5766#page-31
            Arc::new(relay_connection),
            local_addr,
        ))
    }

    pub async fn wait_new_connection(&self) -> ResultType<FramedStream> {
        let tcp_stream = self.client.wait_new_connection().await.unwrap();
        Ok(FramedStream::from(tcp_stream, self.local_addr))
    }
}