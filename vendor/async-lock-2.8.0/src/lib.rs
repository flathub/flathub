//! Async synchronization primitives.
//!
//! This crate provides the following primitives:
//!
//! * [`Barrier`] - enables tasks to synchronize all together at the same time.
//! * [`Mutex`] - a mutual exclusion lock.
//! * [`RwLock`] - a reader-writer lock, allowing any number of readers or a single writer.
//! * [`Semaphore`] - limits the number of concurrent operations.

#![warn(missing_docs, missing_debug_implementations, rust_2018_idioms)]
#![doc(
    html_favicon_url = "https://raw.githubusercontent.com/smol-rs/smol/master/assets/images/logo_fullsize_transparent.png"
)]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/smol-rs/smol/master/assets/images/logo_fullsize_transparent.png"
)]

/// Simple macro to extract the value of `Poll` or return `Pending`.
///
/// TODO: Drop in favor of `core::task::ready`, once MSRV is bumped to 1.64.
macro_rules! ready {
    ($e:expr) => {{
        use ::core::task::Poll;

        match $e {
            Poll::Ready(v) => v,
            Poll::Pending => return Poll::Pending,
        }
    }};
}

/// Pins a variable on the stack.
///
/// TODO: Drop in favor of `core::pin::pin`, once MSRV is bumped to 1.68.
macro_rules! pin {
    ($($x:ident),* $(,)?) => {
        $(
            let mut $x = $x;
            #[allow(unused_mut)]
            let mut $x = unsafe {
                std::pin::Pin::new_unchecked(&mut $x)
            };
        )*
    }
}

mod barrier;
mod mutex;
mod once_cell;
mod rwlock;
mod semaphore;

pub use barrier::{Barrier, BarrierWaitResult};
pub use mutex::{Mutex, MutexGuard, MutexGuardArc};
pub use once_cell::OnceCell;
pub use rwlock::{
    RwLock, RwLockReadGuard, RwLockReadGuardArc, RwLockUpgradableReadGuard,
    RwLockUpgradableReadGuardArc, RwLockWriteGuard, RwLockWriteGuardArc,
};
pub use semaphore::{Semaphore, SemaphoreGuard, SemaphoreGuardArc};

pub mod futures {
    //! Named futures for use with `async_lock` primitives.

    pub use crate::barrier::BarrierWait;
    pub use crate::mutex::{Lock, LockArc};
    pub use crate::rwlock::futures::{
        Read, ReadArc, UpgradableRead, UpgradableReadArc, Upgrade, UpgradeArc, Write, WriteArc,
    };
    pub use crate::semaphore::{Acquire, AcquireArc};
}
