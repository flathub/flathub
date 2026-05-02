use std::{
    path::PathBuf,
    sync::atomic::{AtomicU64, Ordering},
    time::{SystemTime, UNIX_EPOCH},
};

static TEMP_PATH_COUNTER: AtomicU64 = AtomicU64::new(0);

pub fn unique_temp_path(prefix: &str) -> PathBuf {
    // Parallel integration tests were colliding on timestamp-only names and deleting each
    // other's fixtures. A per-process counter keeps temp paths unique even when the clock
    // resolution is coarser than the test scheduler.
    let sequence = TEMP_PATH_COUNTER.fetch_add(1, Ordering::Relaxed);
    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_nanos();

    std::env::temp_dir().join(format!(
        "openkara-{prefix}-{pid}-{timestamp}-{sequence}",
        pid = std::process::id()
    ))
}
