use futures::{SinkExt, StreamExt};
use std::{
    net::SocketAddr,
    sync::atomic::{AtomicBool, Ordering},
    time::Instant,
};
use tokio::net::TcpStream;
//use tokio_tungstenite::Connector::NativeTls;
use tokio_tungstenite::{tungstenite::Message as WsMessage, MaybeTlsStream, WebSocketStream};

use hbb_common::{
    allow_err,
    anyhow::bail,
    config::{Config, CONNECT_TIMEOUT, RENDEZVOUS_PORT},
    futures::future::join_all,
    log,
    rendezvous_proto::*,
    sleep, socket_client,
    tokio::{
        self, select,
        time::{interval, Duration},
    },
    ResultType,
};

use crate::{
    rendezvous_messages::{self, ToJson},
    server::{check_zombie, new as new_server, ServerPtr},
    turn_client, ui_interface,
};

use std::fs;
use std::io::Read;

static SHOULD_EXIT: AtomicBool = AtomicBool::new(false);

#[derive(Clone)]
pub struct RendezvousMediator {
    //addr: SocketAddr,
}

impl RendezvousMediator {
    pub fn restart() {
        SHOULD_EXIT.store(true, Ordering::SeqCst);
        log::info!("server restart");
    }

    pub async fn start_all() {
		let mut nat_tested = false;
        check_zombie();
        let server = new_server();
        if Config::get_nat_type() == NatType::UNKNOWN_NAT as i32 {
            crate::test_nat_type();
            nat_tested = true;
        }
        if !Config::get_option("stop-service").is_empty() {
            crate::test_rendezvous_server();
        }
        let server_cloned = server.clone();
        tokio::spawn(async move {
            direct_server(server_cloned).await;
        });
        #[cfg(not(any(target_os = "android", target_os = "ios")))]
        if crate::platform::is_installed() {
            std::thread::spawn(move || {
                allow_err!(super::lan::start_listening());
            });
        }

        loop {
            Config::reset_online();
            if Config::get_option("stop-service").is_empty() {
                if !nat_tested {
                    crate::test_nat_type();
                    nat_tested = true;
                }
                let mut futs = Vec::new();
                if let Some(servers) = Config::get_rendezvous_servers().await {
                    SHOULD_EXIT.store(false, Ordering::SeqCst);
                    for host in servers.clone() {
                        let server = server.clone();
                        futs.push(tokio::spawn(async move {
	                        if let Err(err) = Self::start(server, host).await {
	                            log::error!("Signal error: {err}");
	                        }
							//allow_err!(Self::start(server, host).await);
                            // SHOULD_EXIT here is to ensure once one exits, the others also exit.
                            SHOULD_EXIT.store(true, Ordering::SeqCst);
                        }));
                    }
                    join_all(futs).await;
                }
            }

            sleep(1.).await;
        }
    }

    pub async fn start(server: ServerPtr, host_list: String) -> ResultType<()> {
        let public_addr = match turn_client::get_public_ip().await {
            Some(addr) => addr,
            None => bail!("Failed to retreive public IP address"),
        };

        log::info!("Signal server list: {} ", host_list);
        let (local_ip, host, websocket_client) = create_websocket(&host_list).await?;

        let (mut sender, receiver) = websocket_client.split();
        Config::update_latency(&host, 200);
        Config::set_key_confirmed(true);
        Config::set_host_key_confirmed(&host, true);

        const HEALTHCHECK: &str = r#"{"protocol":"one-to-self","data":"healthcheck"}"#;
        const HEALTHCHECK_ERROR: &str = r#"You are removed"#;

        const TIMER_OUT: Duration = Duration::from_secs(1);
        let mut last_timer = Instant::now();
        let mut last_log = Instant::now();
        let mut timer = interval(TIMER_OUT);

        let mut last_healthcheck_sent = None;
        let mut last_data_received = chrono::Utc::now();

        let socket_packets = futures::stream::unfold(receiver, move |mut receiver| async {
            match receiver.next().await {
                Some(Ok(msg)) => Some((Ok(msg), receiver)),
                Some(Err(err)) => Some((Err(err), receiver)),
                None => None,
            }
        });

        #[cfg(not(any(target_os = "android", target_os = "ios")))]
        if let Ok(mut file) = fs::File::open(&Config::path("TeamID.toml")) {
            let mut teamid = String::new();
            let myid = Config::get_id();
            file.read_to_string(&mut teamid)?;
            let _res = reqwest::get(&format!(
                "https://api.hoptodesk.com/?teamid={}&id={}",
                teamid, myid
            ))
            .await?
            .text()
            .await?;
        }
        tokio::pin!(socket_packets);
        loop {
            select! {
                _ = timer.tick() => {
                    if SHOULD_EXIT.load(Ordering::SeqCst) {
                        break;
                    }
                    let now = Instant::now();
                    let now_utc = chrono::Utc::now();
                    if now.duration_since(last_timer) < TIMER_OUT {
                        // a workaround of tokio timer bug
                                    continue;
                    }
                    last_timer = now;
                    if let Some(last_healthcheck_sent) = last_healthcheck_sent {
                        if now_utc - last_healthcheck_sent > chrono::Duration::seconds(10) {
                            log::info!("Server is unresponding, disconnect.");
                            break;
                        }
                    }
                    if now_utc - last_data_received > chrono::Duration::seconds(90) {
                        log::info!("Sending healthcheck.");
                        if let Err(error) = sender.send(WsMessage::Text(HEALTHCHECK.to_owned())).await {
                            log::info!("Send error: {error}, disconnect.");
                            break;
                        };

                        last_healthcheck_sent = Some(chrono::Utc::now());
                    }

                    if (now - last_log).as_secs() >= 30 {
						#[cfg(not(any(target_os = "android", target_os = "ios")))]
                        if let Ok(mut file) = fs::File::open(&Config::path("TeamID.toml")) {
                            let mut teamid = String::new();
                            let myid = Config::get_id();
                            file.read_to_string(&mut teamid)?;
                            //log::info!("Sent TeamID: teamid={:?}, id={:?}", teamid, myid);
							let _res = reqwest::get(&format!("https://api.hoptodesk.com/?teamid={}&id={}", teamid, myid)).await?.text().await?;

                            match crate::ipc::connect(1000, "_cm").await {
                                Ok(mut conn) => if let Err(e) = conn.send(&crate::ipc::Data::ListSessions{ id: teamid }).await {
                                    log::error!("Failed to list sessions: {}", e);
                                }
                                Err(_e) => {}
                                //Err(e) => log::error!("Can't connect to IPC: {}", e)
                            }
                        }
                        last_log = now;
                    }

                }
                Some(data) = socket_packets.next() => {
                    match data {
                    Ok(tokio_tungstenite::tungstenite::Message::Text(msg)) => {
                        log::info!("signal msg: {msg}");

                        if let Ok(close_sessions) = serde_json::from_str::<rendezvous_messages::CloseSessions>(&msg) {
                            let data: Vec<&str> = close_sessions.data.split(':').collect();
                            let mut invalid = true;
                            if data.len() == 2 {
                                if data[0] == "closeoutgoing" {
                                    invalid = false;
                                    ui_interface::close_remote(data[1]);
                                } else if data[0] == "closeincoming" {
                                    invalid = false;
                                    match crate::ipc::connect(1000, "_cm").await {
                                        Ok(mut conn) => if let Err(e) = conn.send(&crate::ipc::Data::CloseIncoming { id: data[1].to_owned() }).await {
                                            log::error!("Failed to send to ipc: {}", e);
                                        }
                                        Err(e) => log::error!("Can't connect to IPC: {}", e)
                                    }
                                }
                            }
                            if invalid {
                                log::error!("Invalid data received: {}", close_sessions.data);
                            }
                        } else if let Ok(connect_request) =
                            serde_json::from_str::<rendezvous_messages::ConnectRequest>(&msg){
                            last_data_received = chrono::Utc::now();
                            let listener =
                                hbb_common::tcp::new_listener(SocketAddr::new(local_ip, 0), true)
                                .await?;
                                let nat_type = Config::get_nat_type();

                                let listening = rendezvous_messages::Listening::new(
                                    connect_request.sender_id,
                                    listener.local_addr().unwrap(),
                                    public_addr,
                                    Config::get_key_pair().1,
                                    nat_type,
                                );
                                let result = sender
                                    .send(
                                        tokio_tungstenite::tungstenite::Message::Text(listening.to_json()),
                                    )
                                    .await;
                                match result {
                                Ok(_) => {
                                    let server_clone = server.clone();
                                    tokio::spawn(async move {
                                        if let Err(error) = crate::accept(listener, server_clone, true, false).await {
                                            log::error!("accept() failed: {:?}", error);
                                        }
                                        });
                                        if listening.require_listen_ipv4() {
                                            let ipv4_listener = hbb_common::tcp::new_listener(listening.lan_ipv4.unwrap(), true).await?;
                                            let server_clone = server.clone();
                                            tokio::spawn(async move {
                                                if let Err(error) = crate::accept(ipv4_listener, server_clone, true, false).await {
                                                    log::error!("accept() failed: {:?}", error);
                                                }
                                            });
                                        }
                                    }
                                    Err(error) => {
                                        log::error!("WS send failed: {:?}", error);
                                    }
                                }
                            } else if let Ok(relay_connection) =
                            serde_json::from_str::<rendezvous_messages::RelayConnection>(&msg)
                        {
                            last_data_received = chrono::Utc::now();
                            if let Ok(stream) = socket_client::connect_tcp(
                                relay_connection.addr,
                                //Config::get_any_listen_addr(true),
                                CONNECT_TIMEOUT,
                            ).await
                            {
                                match tokio::time::timeout_at(tokio::time::Instant::now() + Duration::from_secs(10), socket_packets.next()).await {
                                    Ok(data) => {
                                        if let Some(Ok(tokio_tungstenite::tungstenite::Message::Text(msg))) = data {
                                            if let Ok(_) = serde_json::from_str::<rendezvous_messages::RelayReady,>(&msg){
                                                let server_clone = server.clone();
                                                let addr = relay_connection.addr;
                                                tokio::spawn(async move {
                                                    let _ = crate::create_tcp_connection(
                                                        server_clone,
                                                        stream,
                                                        addr,
                                                        true,
                                                        false
                                                    )
                                                        .await;
                                                });
                                            }
                                        }
                                    }
                                    Err(err) => log::error!("receive relay from channel timeout: {}", err),
                                }
                            }
                        } else if msg == HEALTHCHECK {
                            last_healthcheck_sent = None;
                            last_data_received = chrono::Utc::now();
                        } else if msg == HEALTHCHECK_ERROR {
                            log::info!("Connection removed on server, disconnect.");
                            break;
                        }
                    }
                    Err(e) => bail!("Failed to receive next {}", e),
                    _ => bail!("Received binary message from signal server"),
                    }
                }
            }
        }
        Ok(())
    }
}

async fn create_websocket(
    host_list: &str,
) -> ResultType<(
    std::net::IpAddr,
    String,
    WebSocketStream<MaybeTlsStream<TcpStream>>,
)> {
    //let hosts = host_list.split(';');
	
    let mut hosts: Vec<&str> = host_list.split(';').collect();
    
    // Sort the hosts by protocol, "ws" first, then "wss"
    hosts.sort_by(|a, b| {
        let a_protocol = if a.starts_with("ws:") { 0 } else { 1 };
        let b_protocol = if b.starts_with("ws:") { 0 } else { 1 };
        a_protocol.cmp(&b_protocol)
    });

	
    for host in hosts {
		let ret = crate::rendezvous_ws::create_websocket_(host, None).await;
        allow_err!(&ret);

        if ret.is_ok() {
            return ret;
        }
    }

    bail!("Failed to connect any of the hosts in list");
}

fn get_direct_port() -> i32 {
    let mut port = Config::get_option("direct-access-port")
        .parse::<i32>()
        .unwrap_or(0);
    if port <= 0 {
        port = RENDEZVOUS_PORT + 2;
    }
    port
}

async fn direct_server(server: ServerPtr) {
    let mut listener = None;
    let mut port = 0;
    loop {
        let disabled = Config::get_option("direct-server").is_empty()
            || !Config::get_option("stop-service").is_empty();
        if !disabled && listener.is_none() {
            port = get_direct_port();
            let addr = format!("0.0.0.0:{}", port);
            match hbb_common::tcp::new_listener(&addr, false).await {
                Ok(l) => {
                    listener = Some(l);
                    log::info!(
                        "Direct server listening on: {:?}",
                        listener.as_ref().unwrap().local_addr()
                    );
                }
                Err(err) => {
                    // to-do: pass to ui
                    log::error!(
                        "Failed to start direct server on : {}, error: {}",
                        addr,
                        err
                    );
                    loop {
                        if port != get_direct_port() {
                            break;
                        }
                        sleep(1.).await;
                    }
                }
            }
        }
        if let Some(l) = listener.as_mut() {
            if disabled || port != get_direct_port() {
                log::info!("Exit direct access listen");
                listener = None;
                continue;
            }
            if let Ok(Ok((stream, addr))) = hbb_common::timeout(1000, l.accept()).await {
                stream.set_nodelay(true).ok();
                log::info!("direct access from {}", addr);
                let local_addr = stream
                    .local_addr()
                    .unwrap_or(Config::get_any_listen_addr(true));
                let server = server.clone();
                tokio::spawn(async move {
                    allow_err!(
                        crate::server::create_tcp_connection(
                            server,
                            hbb_common::Stream::from(stream, local_addr),
                            addr,
                            true,
                            true
                        )
                        .await
                    );
                });
            } else {
                sleep(0.1).await;
            }
        } else {
            sleep(1.).await;
        }
    }
}
/*
pub fn get_mac() -> String {
    #[cfg(not(any(target_os = "android", target_os = "ios")))]
    if let Ok(Some(mac)) = mac_address::get_mac_address() {
        mac.to_string()
    } else {
        "".to_owned()
    }
    #[cfg(any(target_os = "android", target_os = "ios"))]
    "".to_owned()
}
*/