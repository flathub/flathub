//! Raw, unsafe reader-writer locking implementation,
//! doesn't depend on the data protected by the lock.
//! [`RwLock`](super::RwLock) is implemented in terms of this.
//!
//! Splitting the implementation this way allows instantiating
//! the locking code only once, and also lets us make
//! [`RwLockReadGuard`](super::RwLockReadGuard) covariant in `T`.

use std::future::Future;
use std::mem::forget;
use std::pin::Pin;
use std::process;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::task::{Context, Poll};

use event_listener::{Event, EventListener};

use crate::futures::Lock;
use crate::Mutex;

const WRITER_BIT: usize = 1;
const ONE_READER: usize = 2;

/// A "raw" RwLock that doesn't hold any data.
pub(super) struct RawRwLock {
    /// Acquired by the writer.
    mutex: Mutex<()>,

    /// Event triggered when the last reader is dropped.
    no_readers: Event,

    /// Event triggered when the writer is dropped.
    no_writer: Event,

    /// Current state of the lock.
    ///
    /// The least significant bit (`WRITER_BIT`) is set to 1 when a writer is holding the lock or
    /// trying to acquire it.
    ///
    /// The upper bits contain the number of currently active readers. Each active reader
    /// increments the state by `ONE_READER`.
    state: AtomicUsize,
}

impl RawRwLock {
    #[inline]
    pub(super) const fn new() -> Self {
        RawRwLock {
            mutex: Mutex::new(()),
            no_readers: Event::new(),
            no_writer: Event::new(),
            state: AtomicUsize::new(0),
        }
    }

    /// Returns `true` iff a read lock was successfully acquired.

    pub(super) fn try_read(&self) -> bool {
        let mut state = self.state.load(Ordering::Acquire);

        loop {
            // If there's a writer holding the lock or attempting to acquire it, we cannot acquire
            // a read lock here.
            if state & WRITER_BIT != 0 {
                return false;
            }

            // Make sure the number of readers doesn't overflow.
            if state > std::isize::MAX as usize {
                process::abort();
            }

            // Increment the number of readers.
            match self.state.compare_exchange(
                state,
                state + ONE_READER,
                Ordering::AcqRel,
                Ordering::Acquire,
            ) {
                Ok(_) => return true,
                Err(s) => state = s,
            }
        }
    }

    #[inline]
    pub(super) fn read(&self) -> RawRead<'_> {
        RawRead {
            lock: self,
            state: self.state.load(Ordering::Acquire),
            listener: None,
        }
    }

    /// Returns `true` iff an upgradable read lock was successfully acquired.

    pub(super) fn try_upgradable_read(&self) -> bool {
        // First try grabbing the mutex.
        let lock = if let Some(lock) = self.mutex.try_lock() {
            lock
        } else {
            return false;
        };

        forget(lock);

        let mut state = self.state.load(Ordering::Acquire);

        // Make sure the number of readers doesn't overflow.
        if state > std::isize::MAX as usize {
            process::abort();
        }

        // Increment the number of readers.
        loop {
            match self.state.compare_exchange(
                state,
                state + ONE_READER,
                Ordering::AcqRel,
                Ordering::Acquire,
            ) {
                Ok(_) => return true,
                Err(s) => state = s,
            }
        }
    }

    #[inline]

    pub(super) fn upgradable_read(&self) -> RawUpgradableRead<'_> {
        RawUpgradableRead {
            lock: self,
            acquire: self.mutex.lock(),
        }
    }

    /// Returs `true` iff a write lock was successfully acquired.

    pub(super) fn try_write(&self) -> bool {
        // First try grabbing the mutex.
        let lock = if let Some(lock) = self.mutex.try_lock() {
            lock
        } else {
            return false;
        };

        // If there are no readers, grab the write lock.
        if self
            .state
            .compare_exchange(0, WRITER_BIT, Ordering::AcqRel, Ordering::Acquire)
            .is_ok()
        {
            forget(lock);
            true
        } else {
            drop(lock);
            false
        }
    }

    #[inline]

    pub(super) fn write(&self) -> RawWrite<'_> {
        RawWrite {
            lock: self,
            state: WriteState::Acquiring(self.mutex.lock()),
        }
    }

    /// Returns `true` iff a the upgradable read lock was successfully upgraded to a write lock.
    ///
    /// # Safety
    ///
    /// Caller must hold an upgradable read lock.
    /// This will attempt to upgrade it to a write lock.

    pub(super) unsafe fn try_upgrade(&self) -> bool {
        self.state
            .compare_exchange(ONE_READER, WRITER_BIT, Ordering::AcqRel, Ordering::Acquire)
            .is_ok()
    }

    /// # Safety
    ///
    /// Caller must hold an upgradable read lock.
    /// This will upgrade it to a write lock.

    pub(super) unsafe fn upgrade(&self) -> RawUpgrade<'_> {
        // Set `WRITER_BIT` and decrement the number of readers at the same time.
        self.state
            .fetch_sub(ONE_READER - WRITER_BIT, Ordering::SeqCst);

        RawUpgrade {
            lock: Some(self),
            listener: None,
        }
    }

    /// # Safety
    ///
    /// Caller must hold an upgradable read lock.
    /// This will downgrade it to a stadard read lock.
    #[inline]

    pub(super) unsafe fn downgrade_upgradable_read(&self) {
        self.mutex.unlock_unchecked();
    }

    /// # Safety
    ///
    /// Caller must hold a write lock.
    /// This will downgrade it to a read lock.

    pub(super) unsafe fn downgrade_write(&self) {
        // Atomically downgrade state.
        self.state
            .fetch_add(ONE_READER - WRITER_BIT, Ordering::SeqCst);

        // Release the writer mutex.
        self.mutex.unlock_unchecked();

        // Trigger the "no writer" event.
        self.no_writer.notify(1);
    }

    /// # Safety
    ///
    /// Caller must hold a write lock.
    /// This will downgrade it to an upgradable read lock.

    pub(super) unsafe fn downgrade_to_upgradable(&self) {
        // Atomically downgrade state.
        self.state
            .fetch_add(ONE_READER - WRITER_BIT, Ordering::SeqCst);
    }

    /// # Safety
    ///
    /// Caller must hold a read lock .
    /// This will unlock that lock.

    pub(super) unsafe fn read_unlock(&self) {
        // Decrement the number of readers.
        if self.state.fetch_sub(ONE_READER, Ordering::SeqCst) & !WRITER_BIT == ONE_READER {
            // If this was the last reader, trigger the "no readers" event.
            self.no_readers.notify(1);
        }
    }

    /// # Safety
    ///
    /// Caller must hold an upgradable read lock.
    /// This will unlock that lock.

    pub(super) unsafe fn upgradable_read_unlock(&self) {
        // Decrement the number of readers.
        if self.state.fetch_sub(ONE_READER, Ordering::SeqCst) & !WRITER_BIT == ONE_READER {
            // If this was the last reader, trigger the "no readers" event.
            self.no_readers.notify(1);
        }

        // SAFETY: upgradable read guards acquire the writer mutex upon creation.
        self.mutex.unlock_unchecked();
    }

    /// # Safety
    ///
    /// Caller must hold a write lock.
    /// This will unlock that lock.

    pub(super) unsafe fn write_unlock(&self) {
        // Unset `WRITER_BIT`.
        self.state.fetch_and(!WRITER_BIT, Ordering::SeqCst);
        // Trigger the "no writer" event.
        self.no_writer.notify(1);

        // Release the writer lock.
        // SAFETY: `RwLockWriteGuard` always holds a lock on writer mutex.
        self.mutex.unlock_unchecked();
    }
}

/// The future returned by [`RawRwLock::read`].

pub(super) struct RawRead<'a> {
    /// The lock that is being acquired.
    pub(super) lock: &'a RawRwLock,

    /// The last-observed state of the lock.
    state: usize,

    /// The listener for the "no writers" event.
    listener: Option<EventListener>,
}

impl<'a> Future for RawRead<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        let this = self.get_mut();

        loop {
            if this.state & WRITER_BIT == 0 {
                // Make sure the number of readers doesn't overflow.
                if this.state > std::isize::MAX as usize {
                    process::abort();
                }

                // If nobody is holding a write lock or attempting to acquire it, increment the
                // number of readers.
                match this.lock.state.compare_exchange(
                    this.state,
                    this.state + ONE_READER,
                    Ordering::AcqRel,
                    Ordering::Acquire,
                ) {
                    Ok(_) => return Poll::Ready(()),
                    Err(s) => this.state = s,
                }
            } else {
                // Start listening for "no writer" events.
                let load_ordering = match &mut this.listener {
                    None => {
                        this.listener = Some(this.lock.no_writer.listen());

                        // Make sure there really is no writer.
                        Ordering::SeqCst
                    }

                    Some(ref mut listener) => {
                        // Wait for the writer to finish.
                        ready!(Pin::new(listener).poll(cx));
                        this.listener = None;

                        // Notify the next reader waiting in list.
                        this.lock.no_writer.notify(1);

                        // Check the state again.
                        Ordering::Acquire
                    }
                };

                // Reload the state.
                this.state = this.lock.state.load(load_ordering);
            }
        }
    }
}

/// The future returned by [`RawRwLock::upgradable_read`].

pub(super) struct RawUpgradableRead<'a> {
    /// The lock that is being acquired.
    pub(super) lock: &'a RawRwLock,

    /// The mutex we are trying to acquire.
    acquire: Lock<'a, ()>,
}

impl<'a> Future for RawUpgradableRead<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        let this = self.get_mut();

        // Acquire the mutex.
        let mutex_guard = ready!(Pin::new(&mut this.acquire).poll(cx));
        forget(mutex_guard);

        let mut state = this.lock.state.load(Ordering::Acquire);

        // Make sure the number of readers doesn't overflow.
        if state > std::isize::MAX as usize {
            process::abort();
        }

        // Increment the number of readers.
        loop {
            match this.lock.state.compare_exchange(
                state,
                state + ONE_READER,
                Ordering::AcqRel,
                Ordering::Acquire,
            ) {
                Ok(_) => {
                    return Poll::Ready(());
                }
                Err(s) => state = s,
            }
        }
    }
}

/// The future returned by [`RawRwLock::write`].

pub(super) struct RawWrite<'a> {
    /// The lock that is being acquired.
    pub(super) lock: &'a RawRwLock,

    /// Current state fof this future.
    state: WriteState<'a>,
}

enum WriteState<'a> {
    /// We are currently acquiring the inner mutex.
    Acquiring(Lock<'a, ()>),

    /// We are currently waiting for readers to finish.
    WaitingReaders {
        /// The listener for the "no readers" event.
        listener: Option<EventListener>,
    },

    /// The future has completed.
    Acquired,
}

impl<'a> Future for RawWrite<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        let this = self.get_mut();

        loop {
            match &mut this.state {
                WriteState::Acquiring(lock) => {
                    // First grab the mutex.
                    let mutex_guard = ready!(Pin::new(lock).poll(cx));
                    forget(mutex_guard);

                    // Set `WRITER_BIT` and create a guard that unsets it in case this future is canceled.
                    let new_state = this.lock.state.fetch_or(WRITER_BIT, Ordering::SeqCst);

                    // If we just acquired the lock, return.
                    if new_state == WRITER_BIT {
                        this.state = WriteState::Acquired;
                        return Poll::Ready(());
                    }

                    // Start waiting for the readers to finish.
                    this.state = WriteState::WaitingReaders {
                        listener: Some(this.lock.no_readers.listen()),
                    };
                }

                WriteState::WaitingReaders { ref mut listener } => {
                    let load_ordering = if listener.is_some() {
                        Ordering::Acquire
                    } else {
                        Ordering::SeqCst
                    };

                    // Check the state again.
                    if this.lock.state.load(load_ordering) == WRITER_BIT {
                        // We are the only ones holding the lock, return `Ready`.
                        this.state = WriteState::Acquired;
                        return Poll::Ready(());
                    }

                    // Wait for the readers to finish.
                    match listener {
                        None => {
                            // Register a listener.
                            *listener = Some(this.lock.no_readers.listen());
                        }

                        Some(ref mut evl) => {
                            // Wait for the readers to finish.
                            ready!(Pin::new(evl).poll(cx));
                            *listener = None;
                        }
                    };
                }
                WriteState::Acquired => panic!("Write lock already acquired"),
            }
        }
    }
}

impl<'a> Drop for RawWrite<'a> {
    fn drop(&mut self) {
        if matches!(self.state, WriteState::WaitingReaders { .. }) {
            // Safety: we hold a write lock, more or less.
            unsafe {
                self.lock.write_unlock();
            }
        }
    }
}

/// The future returned by [`RawRwLock::upgrade`].

pub(super) struct RawUpgrade<'a> {
    lock: Option<&'a RawRwLock>,

    /// The event listener we are waiting on.
    listener: Option<EventListener>,
}

impl<'a> Future for RawUpgrade<'a> {
    type Output = &'a RawRwLock;

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<&'a RawRwLock> {
        let this = self.get_mut();
        let lock = this.lock.expect("cannot poll future after completion");

        // If there are readers, we need to wait for them to finish.
        loop {
            let load_ordering = if this.listener.is_some() {
                Ordering::Acquire
            } else {
                Ordering::SeqCst
            };

            // See if the number of readers is zero.
            let state = lock.state.load(load_ordering);
            if state == WRITER_BIT {
                break;
            }

            // If there are readers, wait for them to finish.
            match &mut this.listener {
                None => {
                    // Start listening for "no readers" events.
                    this.listener = Some(lock.no_readers.listen());
                }

                Some(ref mut listener) => {
                    // Wait for the readers to finish.
                    ready!(Pin::new(listener).poll(cx));
                    this.listener = None;
                }
            }
        }

        // We are done.
        Poll::Ready(this.lock.take().unwrap())
    }
}

impl<'a> Drop for RawUpgrade<'a> {
    #[inline]
    fn drop(&mut self) {
        if let Some(lock) = self.lock {
            // SAFETY: we are dropping the future that would give us a write lock,
            // so we don't need said lock anymore.
            unsafe {
                lock.write_unlock();
            }
        }
    }
}

impl<'a> RawUpgrade<'a> {
    /// Whether the future returned `Poll::Ready(..)` at some point.
    #[inline]
    pub(super) fn is_ready(&self) -> bool {
        self.lock.is_none()
    }
}
