#[cfg(not(feature = "tokio"))]
pub(crate) use async_lock::{Mutex, RwLock, RwLockReadGuard, RwLockWriteGuard};
#[cfg(feature = "tokio")]
pub(crate) use tokio::sync::{Mutex, RwLock, RwLockReadGuard, RwLockWriteGuard};
