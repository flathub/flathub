use anyhow::{Context, Result};
use std::{
    collections::VecDeque,
    fs,
    io::Read,
    net::{Ipv4Addr, SocketAddrV4, TcpListener},
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

    pub fn drain_pending(&self) -> Vec<AirPlayAudioChunk> {
        let Ok(mut chunks) = self.chunks.lock() else {
            return Vec::new();
        };

        chunks.drain(..).collect()
    }
}

pub fn select_forwardable_audio_chunks(
    current_epoch: u64,
    chunks: Vec<AirPlayAudioChunk>,
) -> (u64, Vec<AirPlayAudioChunk>) {
    // RATIONALE: `epoch` is only a freshness boundary. It invalidates old PCM
    // after play/pause/seek/track changes, but it does not mean "forward only
    // the first chunk from that epoch". AirPlay must continue streaming every
    // subsequent chunk in the newest epoch or TV audio will fall silent after
    // the first short burst.
    let next_epoch = chunks
        .iter()
        .map(|chunk| chunk.epoch)
        .max()
        .map(|epoch| epoch.max(current_epoch))
        .unwrap_or(current_epoch);

    let forwardable = chunks
        .into_iter()
        .filter(|chunk| chunk.epoch == next_epoch)
        .collect();

    (next_epoch, forwardable)
}

#[derive(Debug)]
pub struct AirPlayHttpServer {
    root_dir: PathBuf,
    base_url: String,
    _thread: thread::JoinHandle<()>,
}

impl AirPlayHttpServer {
    pub fn bind(root_dir: &Path) -> Result<Self> {
        let published_ip = detect_airplay_publish_ip()?;
        Self::bind_with_publish_ip(root_dir, published_ip)
    }

    pub fn bind_with_publish_ip(root_dir: &Path, published_ip: Ipv4Addr) -> Result<Self> {
        fs::create_dir_all(root_dir)
            .with_context(|| format!("failed to create airplay root dir {}", root_dir.display()))?;

        // AirPlay receivers may fetch the HLS URL directly, so loopback URLs are not valid here.
        let listener = TcpListener::bind(SocketAddrV4::new(Ipv4Addr::UNSPECIFIED, 0))
            .context("failed to bind airplay http server")?;
        let address = listener
            .local_addr()
            .context("failed to read airplay http server address")?;
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
                if request.method() != &Method::Get && request.method() != &Method::Head {
                    let _ = request.respond(Response::empty(StatusCode(405)));
                    continue;
                }

                let Some(path) = sanitize_request_path(request.url()) else {
                    let _ = request.respond(Response::empty(StatusCode(400)));
                    continue;
                };
                let file_path = server_root.join(path);
                let range_header = request
                    .headers()
                    .iter()
                    .find(|header| header.field.equiv("Range"))
                    .and_then(|header| Some(header.value.as_str().to_owned()));

                match build_file_response(
                    request.method() == &Method::Head,
                    range_header.as_deref(),
                    &file_path,
                ) {
                    Ok(response) => {
                        let _ = request.respond(response);
                    }
                    Err(_) => {
                        let _ = request.respond(Response::empty(StatusCode(404)));
                    }
                }
            }
        });

        let base_url = format!("http://{}:{}", published_ip, address.port());
        eprintln!(
            "OpenKara AirPlay HLS publishing on {} (serving {})",
            base_url,
            root_dir.display()
        );

        Ok(Self {
            root_dir,
            base_url,
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

fn build_file_response(
    head_only: bool,
    range_header: Option<&str>,
    file_path: &Path,
) -> Result<Response<std::io::Cursor<Vec<u8>>>> {
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

    let total_len = body.len() as u64;
    let requested_range = range_header.and_then(|header| parse_byte_range(header, total_len));
    let (status, response_body, content_range) = if let Some(range) = requested_range {
        let partial = body[range.start as usize..=range.end as usize].to_vec();
        (
            StatusCode(206),
            partial,
            Some(format!(
                "bytes {}-{}/{}",
                range.start, range.end, total_len
            )),
        )
    } else {
        (
            StatusCode(200),
            body,
            None,
        )
    };

    let mut response = Response::from_data(response_body).with_status_code(status);
    response.add_header(Header::from_bytes("Content-Type", content_type.as_bytes()).unwrap());
    response.add_header(Header::from_bytes("Accept-Ranges", "bytes").unwrap());
    if head_only {
        // tiny_http derives Content-Length from the response body size. Returning
        // the same framed payload for HEAD keeps byte-range probes deterministic
        // without manufacturing conflicting length headers.
        let _ = head_only;
    }
    if let Some(content_range) = content_range {
        response.add_header(Header::from_bytes("Content-Range", content_range.as_bytes()).unwrap());
    }

    Ok(response)
}

pub fn default_stream_root(root: &Path) -> PathBuf {
    root.join("airplay").join("live")
}

pub fn stream_tick_interval() -> Duration {
    Duration::from_millis(33)
}

fn detect_airplay_publish_ip() -> Result<Ipv4Addr> {
    let candidates = collect_publish_ip_candidates()?;
    pick_publish_ip(&candidates)
        .ok_or_else(|| anyhow::anyhow!("failed to determine a non-loopback ipv4 address for airplay"))
}

pub fn spawn_audio_forwarder(tap: std::sync::Arc<AirPlayAudioTap>) {
    #[cfg(target_os = "macos")]
    {
        thread::spawn(move || {
            let mut current_epoch = 0;

            loop {
                let chunks = tap.drain_pending();
                if chunks.is_empty() {
                    thread::sleep(Duration::from_millis(5));
                    continue;
                }

                let (next_epoch, forwardable) =
                    select_forwardable_audio_chunks(current_epoch, chunks);
                current_epoch = next_epoch;

                for chunk in forwardable {
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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct ByteRange {
    start: u64,
    end: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct PublishIpCandidate {
    name: String,
    ip: Ipv4Addr,
}

fn parse_byte_range(header: &str, total_len: u64) -> Option<ByteRange> {
    let value = header.strip_prefix("bytes=")?;
    let (start, end) = value.split_once('-')?;

    if total_len == 0 {
        return None;
    }

    if start.is_empty() {
        let suffix_len = end.parse::<u64>().ok()?;
        if suffix_len == 0 {
            return None;
        }
        let suffix_len = suffix_len.min(total_len);
        return Some(ByteRange {
            start: total_len - suffix_len,
            end: total_len - 1,
        });
    }

    let start = start.parse::<u64>().ok()?;
    let end = if end.is_empty() {
        total_len - 1
    } else {
        end.parse::<u64>().ok()?.min(total_len - 1)
    };

    if start > end || start >= total_len {
        return None;
    }

    Some(ByteRange { start, end })
}

#[cfg(unix)]
fn collect_publish_ip_candidates() -> Result<Vec<PublishIpCandidate>> {
    use std::{ffi::CStr, net::Ipv4Addr, ptr};

    let mut addresses = ptr::null_mut();
    // SAFETY: libc fills a linked list owned by the OS until freeifaddrs is called below.
    let result = unsafe { libc::getifaddrs(&mut addresses) };
    if result != 0 {
        return Err(anyhow::anyhow!("failed to enumerate local network interfaces"));
    }

    let mut candidates = Vec::new();
    let mut cursor = addresses;

    while !cursor.is_null() {
        // SAFETY: cursor walks the linked list returned by getifaddrs until null.
        let entry = unsafe { &*cursor };
        let addr = entry.ifa_addr;
        if !addr.is_null() {
            // SAFETY: ifa_name is a valid C string for the lifetime of the list.
            let name = unsafe { CStr::from_ptr(entry.ifa_name) }
                .to_string_lossy()
                .into_owned();
            // SAFETY: ifa_addr points to a valid sockaddr tagged by sa_family.
            let family = unsafe { (*addr).sa_family as i32 };
            let flags = entry.ifa_flags as i32;
            let is_up = flags & libc::IFF_UP != 0;
            let is_running = flags & libc::IFF_RUNNING != 0;
            let is_loopback = flags & libc::IFF_LOOPBACK != 0;
            let is_point_to_point = flags & libc::IFF_POINTOPOINT != 0;

            if family == libc::AF_INET && is_up && is_running && !is_loopback && !is_point_to_point
            {
                // SAFETY: AF_INET entries are sockaddr_in instances.
                let socket_addr = unsafe { &*(addr as *const libc::sockaddr_in) };
                let ip = Ipv4Addr::from(u32::from_be(socket_addr.sin_addr.s_addr));

                if is_eligible_publish_ip(&name, ip) {
                    candidates.push(PublishIpCandidate { name, ip });
                }
            }
        }

        cursor = entry.ifa_next;
    }

    // SAFETY: addresses came from getifaddrs above and must be freed once iteration completes.
    unsafe { libc::freeifaddrs(addresses) };

    Ok(candidates)
}

#[cfg(not(unix))]
fn collect_publish_ip_candidates() -> Result<Vec<PublishIpCandidate>> {
    Ok(Vec::new())
}

fn pick_publish_ip(candidates: &[PublishIpCandidate]) -> Option<Ipv4Addr> {
    let mut ranked: Vec<_> = candidates
        .iter()
        .filter_map(|candidate| rank_publish_ip_candidate(candidate).map(|rank| (rank, candidate.ip)))
        .collect();
    ranked.sort_by_key(|(rank, _)| *rank);
    ranked.first().map(|(_, ip)| *ip)
}

fn rank_publish_ip_candidate(candidate: &PublishIpCandidate) -> Option<(u8, u32)> {
    let name = candidate.name.as_str();

    if is_virtual_interface(name) {
        return None;
    }

    if let Some(index) = interface_index(name, "en") {
        // macOS exposes built-in Wi‑Fi as en0 on the common hardware path.
        // Preferring it ahead of other active en* interfaces keeps the AirPlay
        // publish address stable on laptops that also have docks/adapters.
        return Some(if index == 0 { (0, index) } else { (1, index) });
    }

    if let Some(index) = interface_index(name, "wlan")
        .or_else(|| interface_index(name, "wifi"))
        .or_else(|| interface_index(name, "wl"))
    {
        return Some((0, index));
    }

    if let Some(index) = interface_index(name, "eth") {
        return Some((1, index));
    }

    Some((2, u32::MAX))
}

fn interface_index(name: &str, prefix: &str) -> Option<u32> {
    let suffix = name.strip_prefix(prefix)?;
    suffix.parse::<u32>().ok()
}

fn is_virtual_interface(name: &str) -> bool {
    const VIRTUAL_PREFIXES: &[&str] = &[
        "lo", "utun", "awdl", "llw", "bridge", "ap", "p2p", "gif", "stf", "anpi", "vnic",
        "vmnet", "vboxnet", "docker", "tailscale", "tap", "tun", "wg",
    ];

    VIRTUAL_PREFIXES.iter().any(|prefix| name.starts_with(prefix))
}

fn is_eligible_publish_ip(name: &str, ip: Ipv4Addr) -> bool {
    if ip.is_loopback() || ip.is_unspecified() || is_virtual_interface(name) {
        return false;
    }

    let octets = ip.octets();
    !(octets[0] == 169 && octets[1] == 254)
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
