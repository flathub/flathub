#[derive(Clone, Copy, Debug)]
pub enum Station {
    Jpop,
    Kpop,
}

impl Station {
    pub fn stream_url(self) -> &'static str {
        match self {
            Station::Jpop => "https://listen.moe/stream",
            Station::Kpop => "https://listen.moe/kpop/stream",
        }
    }

    pub fn ws_url(self) -> &'static str {
        match self {
            Station::Jpop => "wss://listen.moe/gateway_v2",
            Station::Kpop => "wss://listen.moe/kpop/gateway_v2",
        }
    }

    pub const fn name(self) -> &'static str {
        match self {
            Station::Jpop => "jpop",
            Station::Kpop => "kpop",
        }
    }

    pub const fn display_name(self) -> &'static str {
        match self {
            Station::Jpop => "J-POP",
            Station::Kpop => "K-POP",
        }
    }
}
