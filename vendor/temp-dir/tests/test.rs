use core::sync::atomic::Ordering;
use std::io::ErrorKind;
use std::path::Path;
use std::sync::Mutex;
use temp_dir::{TempDir, INTERNAL_COUNTER, INTERNAL_RETRY};

// TODO: Move this file to tests/ dir.

// The error tests require all tests to run single-threaded.
static LOCK: Mutex<()> = Mutex::new(());

fn make_non_writable(path: &Path) {
    let metadata = std::fs::metadata(path).unwrap();
    let mut permissions = metadata.permissions();
    permissions.set_readonly(true);
    std::fs::set_permissions(path, permissions).unwrap();
}

fn make_writable(path: &Path) {
    let metadata = std::fs::metadata(path).unwrap();
    let mut permissions = metadata.permissions();
    #[allow(clippy::permissions_set_readonly_false)]
    permissions.set_readonly(false);
    std::fs::set_permissions(path, permissions).unwrap();
}

fn should_skip_cleanup_test() -> bool {
    // On Gitlab's shared CI runners, the cleanup always succeeds and the
    // test fails.  So we skip these tests when it's running on Gitlab CI.
    if std::env::current_dir().unwrap().starts_with("/builds/") {
        println!("Running on Gitlab CI.  Skipping test.");
        return true;
    }
    false
}

struct DisableRetryAndEnableOnDrop;
impl DisableRetryAndEnableOnDrop {
    pub fn new() -> Self {
        INTERNAL_RETRY.store(false, Ordering::SeqCst);
        Self {}
    }
}
impl Drop for DisableRetryAndEnableOnDrop {
    fn drop(&mut self) {
        INTERNAL_RETRY.store(true, Ordering::SeqCst);
    }
}

#[test]
fn new() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    println!("{temp_dir:?}");
    println!("{:?}", TempDir::new().unwrap());
    let metadata = std::fs::metadata(temp_dir.path()).unwrap();
    assert!(metadata.is_dir());
    let temp_dir2 = TempDir::new().unwrap();
    assert_ne!(temp_dir.path(), temp_dir2.path());
}

#[test]
fn new_error() {
    let _guard = LOCK.lock();
    let _disable_retry = DisableRetryAndEnableOnDrop::new();
    let previous_counter_value = INTERNAL_COUNTER.load(Ordering::SeqCst);
    let temp_dir = TempDir::new().unwrap();
    let dir_path = temp_dir.path().to_path_buf();
    INTERNAL_COUNTER.store(previous_counter_value, Ordering::SeqCst);
    let e = TempDir::new().unwrap_err();
    assert_eq!(ErrorKind::AlreadyExists, e.kind());
    assert!(
        e.to_string()
            .starts_with(&format!("error creating directory {dir_path:?}: ")),
        "unexpected error {e:?}",
    );
}

#[test]
fn already_exists() {
    let _guard = LOCK.lock();
    let previous_counter_value = INTERNAL_COUNTER.load(Ordering::SeqCst);
    let temp_dir1 = TempDir::new().unwrap();
    INTERNAL_COUNTER.store(previous_counter_value, Ordering::SeqCst);
    let temp_dir2 = TempDir::new().unwrap();
    assert_ne!(temp_dir1.path(), temp_dir2.path());
}

#[test]
fn with_prefix() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::with_prefix("prefix1").unwrap();
    let name = temp_dir.path().file_name().unwrap();
    assert!(
        name.to_str().unwrap().starts_with("prefix1"),
        "{temp_dir:?}",
    );
    let metadata = std::fs::metadata(temp_dir.path()).unwrap();
    assert!(metadata.is_dir());
    let temp_dir2 = TempDir::new().unwrap();
    assert_ne!(temp_dir.path(), temp_dir2.path());
}

#[test]
fn with_prefix_error() {
    let _guard = LOCK.lock();
    let _disable_retry = DisableRetryAndEnableOnDrop::new();
    let previous_counter_value = INTERNAL_COUNTER.load(Ordering::SeqCst);
    let temp_dir = TempDir::with_prefix("prefix1").unwrap();
    INTERNAL_COUNTER.store(previous_counter_value, Ordering::SeqCst);
    let e = TempDir::with_prefix("prefix1").unwrap_err();
    assert_eq!(ErrorKind::AlreadyExists, e.kind());
    assert!(
        e.to_string()
            .starts_with(&format!("error creating directory {:?}: ", temp_dir.path())),
        "unexpected error {e:?}",
    );
}

#[test]
fn child() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    let file1_path = temp_dir.child("file1");
    assert!(
        file1_path.ends_with("file1"),
        "{:?}",
        file1_path.to_string_lossy()
    );
    assert!(
        file1_path.starts_with(temp_dir.path()),
        "{:?}",
        file1_path.to_string_lossy()
    );
    std::fs::write(&file1_path, b"abc").unwrap();
}

#[test]
fn cleanup() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    std::fs::write(temp_dir.child("file1"), b"abc").unwrap();
    let dir_path = temp_dir.path().to_path_buf();
    std::fs::metadata(&dir_path).unwrap();
    temp_dir.cleanup().unwrap();
    assert_eq!(
        ErrorKind::NotFound,
        std::fs::metadata(&dir_path).unwrap_err().kind()
    );
}

#[test]
fn cleanup_already_deleted() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    std::fs::remove_dir_all(temp_dir.path()).unwrap();
    temp_dir.cleanup().unwrap();
}

#[cfg(unix)]
#[test]
fn cleanup_error() {
    if should_skip_cleanup_test() {
        return;
    }
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    let dir_path = temp_dir.path().to_path_buf();
    let file1_path = temp_dir.child("file1");
    std::fs::write(&file1_path, b"abc").unwrap();
    make_non_writable(&dir_path);
    let result = temp_dir.cleanup();
    std::fs::metadata(&dir_path).unwrap();
    std::fs::metadata(&file1_path).unwrap();
    make_writable(&dir_path);
    std::fs::remove_dir_all(&dir_path).unwrap();
    let e = result.unwrap_err();
    assert_eq!(ErrorKind::PermissionDenied, e.kind());
    assert!(
        e.to_string().starts_with(&format!(
            "error removing directory and contents {dir_path:?}: "
        )),
        "unexpected error {e:?}",
    );
}

#[test]
fn test_drop() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    let dir_path = temp_dir.path().to_path_buf();
    let file1_path = temp_dir.child("file1");
    std::fs::write(&file1_path, b"abc").unwrap();
    TempDir::new().unwrap();
    std::fs::metadata(&dir_path).unwrap();
    std::fs::metadata(&file1_path).unwrap();
    drop(temp_dir);
    assert_eq!(
        ErrorKind::NotFound,
        std::fs::metadata(&dir_path).unwrap_err().kind()
    );
    assert_eq!(
        ErrorKind::NotFound,
        std::fs::metadata(&file1_path).unwrap_err().kind()
    );
}

#[test]
fn drop_already_deleted() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap().panic_on_cleanup_error();
    std::fs::remove_dir(temp_dir.path()).unwrap();
}

#[cfg(unix)]
#[test]
fn drop_error_ignored() {
    if should_skip_cleanup_test() {
        return;
    }
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    let dir_path = temp_dir.path().to_path_buf();
    let file1_path = temp_dir.child("file1");
    std::fs::write(&file1_path, b"abc").unwrap();
    make_non_writable(&dir_path);
    drop(temp_dir);
    std::fs::metadata(&dir_path).unwrap();
    std::fs::metadata(&file1_path).unwrap();
    make_writable(&dir_path);
    std::fs::remove_dir_all(&dir_path).unwrap();
}

#[cfg(unix)]
#[test]
fn drop_error_panic() {
    if should_skip_cleanup_test() {
        return;
    }
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap().panic_on_cleanup_error();
    let dir_path = temp_dir.path().to_path_buf();
    let file1_path = temp_dir.child("file1");
    std::fs::write(&file1_path, b"abc").unwrap();
    make_non_writable(&dir_path);
    let result = std::panic::catch_unwind(move || drop(temp_dir));
    std::fs::metadata(&dir_path).unwrap();
    std::fs::metadata(&file1_path).unwrap();
    make_writable(&dir_path);
    std::fs::remove_dir_all(&dir_path).unwrap();
    let msg = result.unwrap_err().downcast::<String>().unwrap();
    assert!(
        msg.contains("error removing directory and contents ",),
        "unexpected panic message {msg:?}",
    );
}

#[test]
fn leak() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    let dir_path = temp_dir.path().to_path_buf();
    let file1_path = temp_dir.child("file1");
    std::fs::write(&file1_path, b"abc").unwrap();
    temp_dir.leak();
    std::fs::metadata(&dir_path).unwrap();
    std::fs::metadata(&file1_path).unwrap();
    std::fs::remove_dir_all(&dir_path).unwrap();
}

#[test]
fn dont_delete_on_drop() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap().dont_delete_on_drop();
    let dir_path = temp_dir.path().to_path_buf();
    let file1_path = temp_dir.child("file1");
    std::fs::write(&file1_path, b"abc").unwrap();
    drop(temp_dir);
    std::fs::metadata(&dir_path).unwrap();
    std::fs::metadata(&file1_path).unwrap();
    std::fs::remove_dir_all(&dir_path).unwrap();
}

#[test]
fn as_ref_path() {
    let _guard = LOCK.lock();
    let temp_dir = TempDir::new().unwrap();
    let path_ref: &Path = temp_dir.as_ref();
    assert_eq!(path_ref, temp_dir.path());
}
