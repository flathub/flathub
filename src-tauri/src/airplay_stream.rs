use anyhow::{Context, Result};
use std::{
    collections::VecDeque,
    fs,
    io::Read,
    net::TcpListener,
    path::{Path, PathBuf},
    sync::{
        atomic::{AtomicU64, Ordering},
        Mutex,
    },
    thread,
    time::Duration,
};
use tiny_http::{Header, Method, Response, Server, StatusCode};

#[derive(Debug, Clone, PartialEq)]
pub struct AirPlayAudioChunk {
    pub epoch: u64,
    pub sample_rate: u32,
    pub channels: u16,
    pub samples: Vec<f32>,
}

#[derive(Debug)]
pub struct AirPlayAudioTap {
    epoch: AtomicU64,
    capacity: usize,
    chunks: Mutex<VecDeque<AirPlayAudioChunk>>,
}

impl AirPlayAudioTap {
    pub fn new(capacity: usize) -> Self {
        Self {
            epoch: AtomicU64::new(1),
            capacity: capacity.max(1),
            chunks: Mutex::new(VecDeque::with_capacity(capacity.max(1))),
        }
    }

    pub fn current_epoch(&self) -> u64 {
        self.epoch.load(Ordering::SeqCst)
    }

    pub fn bump_epoch(&self) -> u64 {
        self.epoch.fetch_add(1, Ordering::SeqCst) + 1
    }

    pub fn push_interleaved(&self, sample_rate: u32, channels: u16, samples: &[f32]) {
        if samples.is_empty() {
            return;
        }

        let Ok(mut chunks) = self.chunks.try_lock() else {
            return;
        };

        while chunks.len() >= self.capacity {
            chunks.pop_front();
        }

        chunks.push_back(AirPlayAudioChunk {
            epoch: self.current_epoch(),
            sample_rate,
            channels,
            samples: samples.to_vec(),
        });
    }

    pub fn drain_newer_than(&self, epoch: u64) -> Vec<AirPlayAudioChunk> {
        let Ok(mut chunks) = self.chunks.lock() else {
            return Vec::new();
        };

        let mut drained = Vec::with_capacity(chunks.len());
        while let Some(chunk) = chunks.pop_front() {
            if chunk.epoch > epoch {
                drained.push(chunk);
            }
        }

        drained
    }
}

#[derive(Debug)]
pub struct AirPlayHttpServer {
    root_dir: PathBuf,
    base_url: String,
    _thread: thread::JoinHandle<()>,
}

impl AirPlayHttpServer {
    pub fn bind(root_dir: &Path) -> Result<Self> {
        fs::create_dir_all(root_dir)
            .with_context(|| format!("failed to create airplay root dir {}", root_dir.display()))?;

        let listener =
            TcpListener::bind("127.0.0.1:0").context("failed to bind airplay loopback server")?;
        let address = listener
            .local_addr()
            .context("failed to read airplay loopback address")?;
        listener
            .set_nonblocking(false)
            .context("failed to configure airplay listener")?;

        let server = Server::from_listener(listener, None)
            .map_err(|error| anyhow::anyhow!("failed to start tiny_http server: {error}"))?;
        let root_dir = root_dir.to_path_buf();
        let server_root = root_dir.clone();

        let thread = thread::spawn(move || {
            loop {
                let Ok(Some(request)) = server.recv_timeout(Duration::from_millis(100)) else {
                    continue;
                };
                if request.method() != &Method::Get {
                    let _ = request.respond(Response::empty(StatusCode(405)));
                    continue;
                }

                let Some(path) = sanitize_request_path(request.url()) else {
                    let _ = request.respond(Response::empty(StatusCode(400)));
                    continue;
                };
                let file_path = server_root.join(path);

                match read_response_bytes(&file_path) {
                    Ok((body, content_type)) => {
                        let mut response = Response::from_data(body);
                        if let Ok(header) =
                            Header::from_bytes("Content-Type", content_type.as_bytes())
                        {
                            response = response.with_header(header);
                        }
                        let _ = request.respond(response);
                    }
                    Err(_) => {
                        let _ = request.respond(Response::empty(StatusCode(404)));
                    }
                }
            }
        });

        Ok(Self {
            root_dir,
            base_url: format!("http://127.0.0.1:{}", address.port()),
            _thread: thread,
        })
    }

    pub fn base_url(&self) -> &str {
        &self.base_url
    }

    pub fn root_dir(&self) -> &Path {
        &self.root_dir
    }
}

fn sanitize_request_path(url: &str) -> Option<PathBuf> {
    let trimmed = url.split('?').next()?.trim_start_matches('/');
    if trimmed.is_empty() {
        return Some(PathBuf::from("playlist.m3u8"));
    }
    if trimmed.split('/').any(|segment| segment == ".." || segment.is_empty()) {
        return None;
    }
    Some(PathBuf::from(trimmed))
}

fn read_response_bytes(file_path: &Path) -> Result<(Vec<u8>, &'static str)> {
    let mut file = fs::File::open(file_path)
        .with_context(|| format!("failed to open requested asset {}", file_path.display()))?;
    let mut body = Vec::new();
    file.read_to_end(&mut body)
        .with_context(|| format!("failed to read requested asset {}", file_path.display()))?;

    let content_type = match file_path.extension().and_then(|extension| extension.to_str()) {
        Some("m3u8") => "application/vnd.apple.mpegurl",
        Some("mp4") => "video/mp4",
        Some("m4s") => "video/iso.segment",
        _ => "application/octet-stream",
    };

    Ok((body, content_type))
}

pub fn default_stream_root(root: &Path) -> PathBuf {
    root.join("airplay").join("live")
}

pub fn stream_tick_interval() -> Duration {
    Duration::from_millis(33)
}

pub fn spawn_audio_forwarder(tap: std::sync::Arc<AirPlayAudioTap>) {
    #[cfg(target_os = "macos")]
    {
        thread::spawn(move || {
            let mut last_epoch = 0;

            loop {
                let chunks = tap.drain_newer_than(last_epoch);
                if chunks.is_empty() {
                    thread::sleep(Duration::from_millis(5));
                    continue;
                }

                for chunk in chunks {
                    last_epoch = last_epoch.max(chunk.epoch);
                    unsafe {
                        ok_airplay_push_audio_samples(
                            chunk.samples.as_ptr(),
                            chunk.samples.len(),
                            chunk.sample_rate,
                            chunk.channels,
                            chunk.epoch,
                        );
                    }
                }
            }
        });
    }

    #[cfg(not(target_os = "macos"))]
    {
        let _ = tap;
    }
}

pub fn notify_audio_epoch(epoch: u64) {
    #[cfg(target_os = "macos")]
    unsafe {
        ok_airplay_set_audio_epoch(epoch);
    }

    #[cfg(not(target_os = "macos"))]
    let _ = epoch;
}

#[cfg(target_os = "macos")]
unsafe extern "C" {
    fn ok_airplay_push_audio_samples(
        samples: *const f32,
        sample_count: usize,
        sample_rate: u32,
        channels: u16,
        epoch: u64,
    );
    fn ok_airplay_set_audio_epoch(epoch: u64);
}
