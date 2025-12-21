use serde::Deserialize;
use serde_json::Value;
use std::cell::RefCell;
use std::collections::VecDeque;
use std::io::{Read, Write};
use std::rc::Rc;
use std::sync::mpsc;
use std::sync::{
    atomic::{AtomicU64, Ordering},
    Arc,
};
use std::thread;
use std::time::{Duration, Instant, SystemTime};
use time::format_description::well_known::Rfc3339;
use time::OffsetDateTime;
use tungstenite::client::connect;
use tungstenite::protocol::WebSocket;
use tungstenite::stream::MaybeTlsStream;
use tungstenite::Message;

#[cfg(debug_assertions)]
use crate::log::now_string;
use crate::station::Station;

const ALBUM_COVER_BASE: &str = "https://cdn.listen.moe/covers/";
const ARTIST_IMAGE_BASE: &str = "https://cdn.listen.moe/artists/";

type MetaError = Box<dyn std::error::Error + Send + Sync + 'static>;
type MetaResult<T> = Result<T, MetaError>;

/// Track info sent to the UI thread.
#[derive(Debug, Clone)]
pub struct TrackInfo {
    pub artist: String,
    pub title: String,
    pub album_cover: Option<String>,
    pub artist_image: Option<String>,
    pub start_time_utc: SystemTime,
    pub duration_secs: u32,
}

#[derive(Debug)]
enum Control {
    Stop,
    Pause,
    Resume,
}

#[derive(Debug)]
enum State {
    Stopped,
    Running { tx: mpsc::Sender<Control> },
}

#[derive(Debug)]
struct Inner {
    station: Station,
    state: State,
    sender: mpsc::Sender<TrackInfo>,
    lag_ms: Arc<AtomicU64>,
    ui_sched_id: Arc<AtomicU64>,
}

#[derive(Debug)]
pub struct Meta {
    inner: RefCell<Inner>,
}

impl Meta {
    pub fn new(
        station: Station,
        sender: mpsc::Sender<TrackInfo>,
        lag_ms: Arc<AtomicU64>,
    ) -> Rc<Self> {
        Rc::new(Self {
            inner: RefCell::new(Inner {
                station,
                state: State::Stopped,
                sender,
                lag_ms,
                ui_sched_id: Arc::new(AtomicU64::new(0)),
            }),
        })
    }

    pub fn set_station(&self, station: Station) {
        let mut inner = self.inner.borrow_mut();
        let was_running = matches!(inner.state, State::Running { .. });
        if was_running {
            Self::stop_inner(&mut inner);
        }
        inner.station = station;
        if was_running {
            Self::start_inner(&mut inner);
        }
    }

    pub fn start(&self) {
        let tx_opt = {
            let inner = self.inner.borrow();
            match &inner.state {
                State::Running { tx } => Some(tx.clone()),
                State::Stopped => None,
            }
        };
        if let Some(tx) = tx_opt {
            let _ = tx.send(Control::Resume);
            return;
        }
        // stopped: actually start thread
        let mut inner = self.inner.borrow_mut();
        Self::start_inner(&mut inner);
    }

    pub fn pause(&self) {
        let inner = self.inner.borrow();
        if let State::Running { tx } = &inner.state {
            let _ = tx.send(Control::Pause);
        }
    }

    pub fn stop(&self) {
        let mut inner = self.inner.borrow_mut();
        Self::stop_inner(&mut inner);
    }

    fn start_inner(inner: &mut Inner) {
        match inner.state {
            State::Running { .. } => return,
            State::Stopped => {
                let (tx, rx) = mpsc::channel::<Control>();
                let station = inner.station;
                let sender = inner.sender.clone();
                let lag_ms = inner.lag_ms.clone();
                let ui_sched_id = inner.ui_sched_id.clone();

                inner.state = State::Running { tx: tx.clone() };

                thread::spawn(move || {
                    if let Err(err) = run_meta_loop(station, sender, rx, lag_ms, ui_sched_id) {
                        eprintln!("Gateway error in metadata loop: {err}");
                    }
                });
            }
        }
    }

    fn stop_inner(inner: &mut Inner) {
        if let State::Running { tx } = &inner.state {
            let _ = tx.send(Control::Stop);
        }
        inner.state = State::Stopped;
    }
}

impl Drop for Meta {
    fn drop(&mut self) {
        let mut inner = self.inner.borrow_mut();
        Self::stop_inner(&mut inner);
    }
}

/// Protocol-level types for the LISTEN.moe gateway

#[derive(Debug, Deserialize)]
struct GatewayHello {
    heartbeat: u64,
}

#[derive(Debug, Deserialize)]
struct GatewaySongPayload {
    song: Song,
    #[serde(rename = "startTime")]
    start_time: String,
}

#[derive(Debug, Deserialize)]
struct Song {
    title: Option<String>,
    #[serde(default)]
    artists: Vec<Artist>,
    #[serde(default)]
    albums: Vec<Album>,
    duration: Option<u32>,
}

#[derive(Debug, Deserialize)]
struct Artist {
    name: Option<String>,
    image: Option<String>,
}

#[derive(Debug, Deserialize)]
struct Album {
    image: Option<String>,
}

#[derive(Debug, Deserialize)]
struct GatewayEnvelope {
    op: u8,
    #[serde(default)]
    t: Option<String>,
    #[serde(default)]
    d: Value,
}

const OP_HELLO: u8 = 0;
const OP_DISPATCH: u8 = 1;
const OP_HEARTBEAT_ACK: u8 = 10;
const EVENT_TRACK_UPDATE: &str = "TRACK_UPDATE";

/// Outer reconnect loop using blocking tungstenite.
fn run_meta_loop(
    station: Station,
    sender: mpsc::Sender<TrackInfo>,
    rx: mpsc::Receiver<Control>,
    lag_ms: Arc<AtomicU64>,
    ui_sched_id: Arc<AtomicU64>,
) -> MetaResult<()> {
    loop {
        if let Ok(Control::Stop) | Err(mpsc::TryRecvError::Disconnected) = rx.try_recv() {
            return Ok(());
        }
        match run_once(
            station,
            sender.clone(),
            &rx,
            lag_ms.clone(),
            ui_sched_id.clone(),
        ) {
            Ok(()) => {
                // Normal end (server closed the connection). Respect stop; otherwise retry.
                match rx.try_recv() {
                    Ok(Control::Stop) | Err(mpsc::TryRecvError::Disconnected) => return Ok(()),
                    Err(mpsc::TryRecvError::Empty) => thread::sleep(Duration::from_secs(5)),
                    Ok(_) => thread::sleep(Duration::from_secs(1)),
                }
            }
            Err(err) => {
                eprintln!("Gateway connection error: {err}, retrying in 5s…");
                match rx.try_recv() {
                    Ok(Control::Stop) | Err(mpsc::TryRecvError::Disconnected) => return Ok(()),
                    Err(mpsc::TryRecvError::Empty) => thread::sleep(Duration::from_secs(5)),
                    Ok(_) => thread::sleep(Duration::from_secs(1)),
                }
            }
        }
    }
}

/// Single websocket session, with a simple heartbeat loop.
/// Keeps history and does "snap-to-buffered-track" on Resume.
fn run_once(
    station: Station,
    sender: mpsc::Sender<TrackInfo>,
    rx: &mpsc::Receiver<Control>,
    lag_ms: Arc<AtomicU64>,
    ui_sched_id: Arc<AtomicU64>,
) -> MetaResult<()> {
    if let Ok(Control::Stop) | Err(mpsc::TryRecvError::Disconnected) = rx.try_recv() {
        return Ok(());
    }

    let url = station.ws_url();
    let (mut ws, _response) = connect(url)?;
    set_maybe_tls_read_timeout(ws.get_mut(), Duration::from_millis(200))?;
    #[cfg(debug_assertions)]
    println!("[{}] Gateway connected to LISTEN.moe", now_string());

    // Read hello and get heartbeat interval (if any).
    let heartbeat_ms = read_hello_heartbeat(&mut ws)?;
    // Send an immediate heartbeat once after HELLO, then continue on the interval.
    let _ = ws.send(Message::Text(r#"{"op":9}"#.into()));

    let heartbeat_dur = heartbeat_ms.map(Duration::from_millis);
    let mut last_heartbeat: Option<Instant> = heartbeat_dur.map(|_| Instant::now());

    let mut paused = false;
    let mut history: VecDeque<TrackInfo> = VecDeque::with_capacity(32);

    loop {
        // Check for control messages first.
        match rx.try_recv() {
            Ok(Control::Stop) | Err(mpsc::TryRecvError::Disconnected) => {
                ui_sched_id.fetch_add(1, Ordering::Relaxed);
                break;
            }
            Ok(Control::Pause) => {
                #[cfg(debug_assertions)]
                println!("[{}] Pausing meta data", now_string());
                paused = true;
                ui_sched_id.fetch_add(1, Ordering::Relaxed); // invalidate any pending scheduled sends
            }
            Ok(Control::Resume) => {
                #[cfg(debug_assertions)]
                println!("[{}] Resuming meta data", now_string());
                paused = false;
                ui_sched_id.fetch_add(1, Ordering::Relaxed); // invalidate timers from before pause

                // Snap UI to the track that matches buffered playback time.
                let lag = lag_ms.load(Ordering::Relaxed);
                #[cfg(debug_assertions)]
                if let Some(t) = pick_track_for_playback(&history, lag) {
                    println!("[{}] ui snap: {} - {}", now_string(), t.artist, t.title);
                }
                // Immediately snap UI to what playback should be on resume
                if let Some(correct) = pick_track_for_playback(&history, lag) {
                    let _ = sender.send(correct);
                }
                // Also schedule the next switch that should happen after resume
                schedule_next_from_history(sender.clone(), &history, lag, ui_sched_id.clone());
            }
            Err(mpsc::TryRecvError::Empty) => {}
        }

        // Heartbeat: if we know an interval, send a heartbeat when it elapses.
        if let (Some(interval), Some(last)) = (heartbeat_dur, last_heartbeat.as_mut()) {
            if last.elapsed() >= interval {
                if let Err(err) = ws.send(Message::Text(r#"{"op":9}"#.into())) {
                    eprintln!("Gateway heartbeat send error: {err}");
                    break;
                }
                *last = Instant::now();
            }
        }

        // Incoming messages.
        let msg = match ws.read() {
            Ok(msg) => msg,
            Err(tungstenite::Error::ConnectionClosed) => break,
            Err(tungstenite::Error::Io(ref e))
                if e.kind() == std::io::ErrorKind::WouldBlock
                    || e.kind() == std::io::ErrorKind::TimedOut =>
            {
                continue; // No websocket message right now; loop again so the process can pause
            }
            Err(err) => return Err(Box::new(err)),
        };

        if !msg.is_text() {
            continue;
        }

        let txt = msg.into_text()?;
        let env: GatewayEnvelope = match serde_json::from_str(&txt) {
            Ok(env) => env,
            Err(err) => {
                eprintln!("Gateway JSON parse error: {err}");
                continue;
            }
        };

        match (env.op, env.t.as_deref()) {
            (OP_HEARTBEAT_ACK, _) => {
                #[cfg(debug_assertions)]
                println!("[{}] Gateway heartbeat", now_string());
            }
            (OP_DISPATCH, Some(EVENT_TRACK_UPDATE)) => {
                if let Some(info) = parse_track_info(&env.d) {
                    #[cfg(debug_assertions)]
                    println!(
                        "[{}] live track update: {} - {} (duration={})",
                        now_string(),
                        info.artist,
                        info.title,
                        info.duration_secs
                    );
                    if history.len() == 32 {
                        history.pop_front();
                    }
                    history.push_back(info);

                    if !paused {
                        let lag = lag_ms.load(Ordering::Relaxed);
                        let my_id = ui_sched_id.fetch_add(1, Ordering::Relaxed) + 1;
                        #[cfg(debug_assertions)]
                        println!(
                            "[{}] ui {} scheduled: {} - {} (lag_ms={})",
                            now_string(),
                            my_id,
                            history.back().unwrap().artist,
                            history.back().unwrap().title,
                            lag
                        );
                        // Schedule the *new* track to appear when playback reaches it
                        schedule_ui_switch(
                            sender.clone(),
                            history.back().unwrap().clone(),
                            lag,
                            ui_sched_id.clone(),
                            my_id,
                        );
                    }
                }
            }
            _ => {}
        }
    }

    Ok(())
}

/// Read the initial hello and extract the heartbeat interval (if any).
fn read_hello_heartbeat<S>(ws: &mut WebSocket<S>) -> MetaResult<Option<u64>>
where
    S: Read + Write,
{
    match ws.read() {
        Ok(msg) => {
            if msg.is_text() {
                let txt = msg.into_text()?;
                let env: GatewayEnvelope = serde_json::from_str(&txt)?;

                if env.op == OP_HELLO {
                    let hello: GatewayHello = serde_json::from_value(env.d)?;
                    return Ok(Some(hello.heartbeat));
                }
            }
            Ok(None)
        }
        Err(tungstenite::Error::ConnectionClosed) => Ok(None),
        Err(err) => Err(Box::new(err)),
    }
}

/// Extract artist(s) + title from the gateway payload.
fn parse_track_info(d: &Value) -> Option<TrackInfo> {
    let payload: GatewaySongPayload = serde_json::from_value(d.clone()).ok()?;
    let Song {
        title,
        artists,
        albums,
        duration,
    } = payload.song;

    let start_time_utc = parse_rfc3339_system_time(&payload.start_time)?;
    let duration_secs = duration.unwrap_or(0);

    let title = title.unwrap_or_else(|| "unknown title".to_owned());

    let artist = if artists.is_empty() {
        "Unknown artist".to_owned()
    } else {
        artists
            .iter()
            .filter_map(|a| a.name.as_deref())
            .map(str::to_owned)
            .collect::<Vec<_>>()
            .join(", ")
    };

    let album_cover = albums
        .first()
        .and_then(|album| album.image.as_deref())
        .map(|name| format!("{ALBUM_COVER_BASE}{name}"));

    let artist_image = artists
        .first()
        .and_then(|a| a.image.as_deref())
        .map(|name| format!("{ARTIST_IMAGE_BASE}{name}"));

    Some(TrackInfo {
        artist,
        title,
        album_cover,
        artist_image,
        start_time_utc,
        duration_secs,
    })
}

fn parse_rfc3339_system_time(s: &str) -> Option<SystemTime> {
    let odt = OffsetDateTime::parse(s, &Rfc3339).ok()?;
    let unix = odt.unix_timestamp(); // seconds
    let nanos = odt.nanosecond(); // 0..1e9

    let t = if unix >= 0 {
        SystemTime::UNIX_EPOCH
            .checked_add(Duration::from_secs(unix as u64))?
            .checked_add(Duration::from_nanos(nanos as u64))?
    } else {
        SystemTime::UNIX_EPOCH
            .checked_sub(Duration::from_secs((-unix) as u64))?
            .checked_add(Duration::from_nanos(nanos as u64))?
    };

    Some(t)
}

fn pick_track_for_playback(history: &VecDeque<TrackInfo>, lag_ms: u64) -> Option<TrackInfo> {
    let playback_now = SystemTime::now().checked_sub(Duration::from_millis(lag_ms))?;

    // Prefer a proper [start, end) window when duration is known and > 0.
    if let Some(hit) = history.iter().rev().find(|t| {
        if t.duration_secs == 0 {
            return false;
        }
        let start = t.start_time_utc;
        let end = start.checked_add(Duration::from_secs(t.duration_secs as u64));
        end.map(|end| playback_now >= start && playback_now < end)
            .unwrap_or(false)
    }) {
        return Some(hit.clone());
    }

    // Fallback: duration is missing/0 => pick the latest track that started before playback_now.
    history
        .iter()
        .rev()
        .find(|t| playback_now >= t.start_time_utc)
        .cloned()
}

fn set_maybe_tls_read_timeout(
    stream: &mut MaybeTlsStream<std::net::TcpStream>,
    dur: std::time::Duration,
) -> std::io::Result<()> {
    match stream {
        MaybeTlsStream::Plain(tcp) => tcp.set_read_timeout(Some(dur)),
        MaybeTlsStream::Rustls(tls) => tls.get_mut().set_read_timeout(Some(dur)),
        _ => Ok(()),
    }
}

fn schedule_ui_switch(
    sender: mpsc::Sender<TrackInfo>,
    track: TrackInfo,
    lag_ms: u64,
    ui_sched_id: Arc<AtomicU64>,
    my_id: u64,
) {
    thread::spawn(move || {
        let lag = Duration::from_millis(lag_ms);
        let target = track.start_time_utc.checked_add(lag);
        if let Some(target) = target {
            if let Ok(wait) = target.duration_since(SystemTime::now()) {
                thread::sleep(wait);
            }
        }
        if ui_sched_id.load(Ordering::Relaxed) == my_id {
            let _ = sender.send(track);
        }
    });
}

fn schedule_next_from_history(
    sender: mpsc::Sender<TrackInfo>,
    history: &VecDeque<TrackInfo>,
    lag_ms: u64,
    ui_sched_id: Arc<AtomicU64>,
) {
    let playback_now = match SystemTime::now().checked_sub(Duration::from_millis(lag_ms)) {
        Some(t) => t,
        None => return,
    };

    // Find the earliest track whose (start_time_utc) is still in the future for playback time.
    // i.e. playback_now < track.start_time_utc
    let next = history
        .iter()
        .filter(|t| playback_now < t.start_time_utc)
        .min_by_key(|t| t.start_time_utc)
        .cloned();

    let Some(next) = next else { return };

    let my_id = ui_sched_id.fetch_add(1, Ordering::Relaxed) + 1;

    #[cfg(debug_assertions)]
    println!(
        "[{}] ui {} resched-next: {} - {} (lag_ms={})",
        now_string(),
        my_id,
        next.artist,
        next.title,
        lag_ms
    );

    schedule_ui_switch(sender, next, lag_ms, ui_sched_id, my_id);
}
