use hbb_common::socket_client;
use serde::{Deserialize, Serialize};
use std::net::{SocketAddr};

const PROTOCOL: &str = "one-to-one";

#[derive(Serialize, Deserialize)]
pub struct ConnectRequest<'a> {
    protocol: &'a str,
    endpoint: &'a str,
    pub sender_id: &'a str,
}

impl<'a> ConnectRequest<'a> {
    pub fn new(endpoint: &'a str, sender_id: &'a str) -> Self {
        Self {
            protocol: PROTOCOL,
            endpoint,
            sender_id,
        }
    }
}

// Sent by the receiver, indicates which address he is listening to
#[derive(Serialize, Deserialize)]
pub struct Listening<'a> {
    protocol: &'a str,
    endpoint: &'a str,
    pub addr: SocketAddr,
    pub public_addr: SocketAddr,
    pub pk: String,

    #[serde(default)]
    pub nat_type: i32,

    #[serde(default)]
    pub lan_ipv4: Option<SocketAddr>,
}

impl<'a> Listening<'a> {
    pub fn new(
        endpoint: &'a str,
        addr: SocketAddr,
        public_addr: SocketAddr,
        pk: Vec<u8>,
        nat_type: i32,
    ) -> Self {
		let lan_ipv4 = match socket_client::get_lan_ipv4() {
            Ok(ipv4) => Some(SocketAddr::new(ipv4, addr.port())),
            Err(_) => None,
        };
        Self {
            protocol: PROTOCOL,
            endpoint,
            addr,
            public_addr,
            pk: crate::encode64(pk),
            nat_type,
            lan_ipv4,
        }
    }

    pub fn require_listen_ipv4(&self) -> bool {
        self.addr.is_ipv6() && self.lan_ipv4.is_some()
    }
}

// Sent by the initiator, indicates the ralay address
#[derive(Serialize, Deserialize)]
pub struct RelayConnection<'a> {
    protocol: &'a str,
    endpoint: &'a str,
    pub addr: SocketAddr,
}

impl<'a> RelayConnection<'a> {
    pub fn new(endpoint: &'a str, addr: SocketAddr) -> Self {
        Self {
            protocol: PROTOCOL,
            endpoint,
            addr,
        }
    }
}

// Sent by the initiator when he receives a new connection on the relay address
#[derive(Serialize, Deserialize)]
pub struct RelayReady<'a> {
    protocol: &'a str,
    endpoint: &'a str,
}

impl<'a> RelayReady<'a> {
    pub fn new(endpoint: &'a str) -> Self {
        Self {
            protocol: PROTOCOL,
            endpoint,
        }
    }
}

#[derive(Serialize, Deserialize)]
pub struct CloseSessions<'a> {
    protocol: &'a str,
    endpoint: &'a str,
    pub data: &'a str,
    sender_id: &'a str,
}

pub trait ToJson {
    fn to_json(&self) -> String;
}

impl<T: Serialize> ToJson for T {
    fn to_json(&self) -> String {
        serde_json::to_string(self).unwrap()
    }
}