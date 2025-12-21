use reqwest::blocking::Client;
use rodio::{buffer::SamplesBuffer, OutputStreamBuilder, Sink};
use std::cell::RefCell;
use std::error::Error;
use std::rc::Rc;
use std::sync::{
    atomic::{AtomicU64, Ordering},
    mpsc, Arc,
};
use std::thread;
use std::time::Instant;
use symphonia::core::audio::SampleBuffer;
use symphonia::core::codecs::{DecoderOptions, CODEC_TYPE_NULL};
use symphonia::core::errors::Error as SymphoniaError;
use symphonia::core::formats::FormatOptions;
use symphonia::core::io::MediaSourceStream;
use symphonia::core::meta::MetadataOptions;
use symphonia::core::probe::Hint;

use crate::http_source::HttpSource;
#[cfg(debug_assertions)]
use crate::log::now_string;
use crate::station::Station;

type DynError = Box<dyn Error + Send + Sync + 'static>;
type Result<T> = std::result::Result<T, DynError>;

#[derive(Debug)]
enum Control {
    Stop,
    Pause,
    Resume,
}

#[derive(Debug)]
enum State {
    Stopped,
    Paused { tx: mpsc::Sender<Control> },
    Playing { tx: mpsc::Sender<Control> },
}

#[derive(Debug)]
struct Inner {
    station: Station,
    state: State,
}

#[derive(Debug)]
pub struct Listen {
    inner: RefCell<Inner>,
    lag_ms: Arc<AtomicU64>,
    pause_started: RefCell<Option<Instant>>,
}

impl Listen {
    pub fn new(station: Station) -> Rc<Self> {
        Rc::new(Self {
            inner: RefCell::new(Inner {
                station,
                state: State::Stopped,
            }),
            lag_ms: Arc::new(AtomicU64::new(0)),
            pause_started: RefCell::new(None),
        })
    }

    pub fn lag_ms(&self) -> Arc<AtomicU64> {
        self.lag_ms.clone()
    }

    pub fn get_station(&self) -> Station {
        self.inner.borrow_mut().station
    }

    pub fn set_station(&self, station: Station) {
        let mut inner = self.inner.borrow_mut();
        let was_playing_or_paused = matches!(inner.state, State::Playing { .. } | State::Paused { .. });
        if was_playing_or_paused {
            Self::stop_inner(&mut inner);
        }
        inner.station = station;
        if was_playing_or_paused {
            Self::start_inner(&mut inner);
        }
    }

    pub fn start(&self) {
        if matches!(self.inner.borrow().state, State::Paused { .. }) {
            if let Some(t0) = self.pause_started.borrow_mut().take() {
                let add = t0.elapsed().as_millis() as u64;
                self.lag_ms.fetch_add(add, Ordering::Relaxed);
            }
        }
        let mut inner = self.inner.borrow_mut();
        Self::start_inner(&mut inner);
    }

    pub fn pause(&self) {
        let mut inner = self.inner.borrow_mut();
        match &inner.state {
            State::Playing { tx } => {
                let _ = tx.send(Control::Pause);
                inner.state = State::Paused { tx: tx.clone() };
            }
            _ => {}
        }
        *self.pause_started.borrow_mut() = Some(Instant::now());
    }

    pub fn stop(&self) {
        let mut inner = self.inner.borrow_mut();
        Self::stop_inner(&mut inner);
    }

    fn start_inner(inner: &mut Inner) {
        match &inner.state {
            State::Playing { .. } => {
                // already playing
                return;
            }
            State::Paused { tx } => {
                let _ = tx.send(Control::Resume);
                inner.state = State::Playing { tx: tx.clone() };
                return;
            }
            State::Stopped => {
                let (tx, rx) = mpsc::channel::<Control>();
                let station = inner.station;

                inner.state = State::Playing { tx: tx.clone() };

                // detached worker thread; will exit on Stop or error
                thread::spawn(move || {
                    if let Err(err) = run_listenmoe_stream(station, rx) {
                        eprintln!("stream error: {err}");
                    }
                });
            }
        }
    }

    fn stop_inner(inner: &mut Inner) {
        if let State::Playing { tx } | State::Paused { tx } = &inner.state {
            let _ = tx.send(Control::Stop);
        }
        inner.state = State::Stopped;
    }
}

impl Drop for Listen {
    fn drop(&mut self) {
        // Best-effort cleanup
        let mut inner = self.inner.borrow_mut();
        Self::stop_inner(&mut inner);
    }
}

fn run_listenmoe_stream(station: Station, rx: mpsc::Receiver<Control>) -> Result<()> {
    let url = station.stream_url();
    #[cfg(debug_assertions)]
    println!("[{}] Connecting to {url}…", now_string());

    let client = Client::new();
    let platform = if cfg!(target_os = "linux") {
        "linux"
    } else if cfg!(target_os = "windows") {
        "windows"
    } else {
        "other"
    };
    let useragent = format!(
        "{}-v{}-{}",
        env!("CARGO_PKG_NAME"),
        env!("CARGO_PKG_VERSION"),
        platform
    );
    let response = client.get(url).header("User-Agent", useragent).send()?;
    #[cfg(debug_assertions)]
    println!("[{}] HTTP status: {}", now_string(), response.status());
    if !response.status().is_success() {
        return Err(format!("HTTP status {}", response.status()).into());
    }

    let http_source = HttpSource { inner: response };
    let mss = MediaSourceStream::new(Box::new(http_source), Default::default());

    let mut hint = Hint::new();
    hint.with_extension("ogg");

    let format_opts: FormatOptions = Default::default();
    let metadata_opts: MetadataOptions = Default::default();
    let decoder_opts: DecoderOptions = Default::default();

    let probed = symphonia::default::get_probe().format(&hint, mss, &format_opts, &metadata_opts)?;
    let mut format = probed.format;
    let track = format
        .tracks()
        .iter()
        .find(|t| t.codec_params.codec != CODEC_TYPE_NULL)
        .ok_or_else(|| "no supported audio tracks".to_string())?;

    let mut track_id = track.id;
    let mut decoder = symphonia::default::get_codecs().make(&track.codec_params, &decoder_opts)?;

    let stream = OutputStreamBuilder::open_default_stream()?;
    let sink = Sink::connect_new(&stream.mixer());

    #[cfg(debug_assertions)]
    println!("[{}] Started decoding + playback.", now_string());

    let mut sample_buf: Option<SampleBuffer<f32>> = None;
    let mut channels: u16 = 0;
    let mut sample_rate: u32 = 0;
    let mut paused = false;

    loop {
        while let Ok(cmd) = rx.try_recv() {
            match cmd {
                Control::Stop => {
                    #[cfg(debug_assertions)]
                    println!("[{}] Stop requested, shutting down stream.", now_string());
                    sink.stop();
                    return Ok(());
                }
                Control::Pause => {
                    if !paused {
                        #[cfg(debug_assertions)]
                        println!("[{}] Pausing playback.", now_string());
                        paused = true;
                        sink.pause();
                    }
                }
                Control::Resume => {
                    if paused {
                        #[cfg(debug_assertions)]
                        println!("[{}] Resuming playback.", now_string());
                        paused = false;
                        sink.play();
                    }
                }
            }
        }
        let packet = match format.next_packet() {
            Ok(p) => p,
            Err(SymphoniaError::ResetRequired) => {
                #[cfg(debug_assertions)]
                println!("[{}] Stream reset, reconfiguring decoder…", now_string());
                let new_track = format
                    .tracks()
                    .iter()
                    .find(|t| t.codec_params.codec != CODEC_TYPE_NULL)
                    .ok_or_else(|| "no supported audio tracks after reset".to_string())?;

                track_id = new_track.id;
                decoder = symphonia::default::get_codecs()
                    .make(&new_track.codec_params, &decoder_opts)?;

                sample_buf = None;
                continue;
            }
            Err(err) => {
                return Err(format!("Error reading packet: {err:?}").into());
            }
        };

        if packet.track_id() != track_id {
            continue;
        }

        let decoded = match decoder.decode(&packet) {
            Ok(buf) => buf,
            Err(SymphoniaError::DecodeError(_)) => continue,
            Err(SymphoniaError::ResetRequired) => {
                #[cfg(debug_assertions)]
                println!("[{}] Decoder reset required, rebuilding decoder…", now_string());
                let new_track = format
                    .tracks()
                    .iter()
                    .find(|t| t.codec_params.codec != CODEC_TYPE_NULL)
                    .ok_or_else(|| "no supported audio tracks after decoder reset".to_string())?;

                track_id = new_track.id;
                decoder = symphonia::default::get_codecs()
                    .make(&new_track.codec_params, &decoder_opts)?;
                sample_buf = None;
                continue;
            }
            Err(err) => {
                return Err(format!("Fatal decode error: {err:?}").into());
            }
        };

        if sample_buf.is_none() {
            let spec = *decoded.spec();
            let duration = decoded.capacity() as u64;

            channels = spec.channels.count() as u16;
            sample_rate = spec.rate;

            sample_buf = Some(SampleBuffer::<f32>::new(duration, spec));
        }

        let buf = sample_buf.as_mut().expect("sample_buf just initialized");
        buf.copy_interleaved_ref(decoded);

        let samples = buf.samples().to_owned();
        let source = SamplesBuffer::new(channels, sample_rate, samples);
        sink.append(source);
    }
}
