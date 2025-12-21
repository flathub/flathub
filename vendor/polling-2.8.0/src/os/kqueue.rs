//! Functionality that is only available for `kqueue`-based platforms.

use crate::sys::{mode_to_flags, FilterFlags};
use crate::{PollMode, Poller};

use std::convert::TryInto;
use std::process::Child;
use std::time::Duration;
use std::{io, mem};

use super::__private::PollerSealed;
use __private::FilterSealed;

// TODO(notgull): We should also have EVFILT_AIO, EVFILT_VNODE and EVFILT_USER. However, the current
// API makes it difficult to effectively express events from these filters. At the next breaking
// change, we should change `Event` to be a struct with private fields, and encode additional
// information in there.

/// Functionality that is only available for `kqueue`-based platforms.
///
/// `kqueue` is able to monitor much more than just read/write readiness on file descriptors. Using
/// this extension trait, you can monitor for signals, process exits, and more. See the implementors
/// of the [`Filter`] trait for more information.
pub trait PollerKqueueExt<F: Filter>: PollerSealed {
    /// Add a filter to the poller.
    ///
    /// This is similar to [`add`][Poller::add], but it allows you to specify a filter instead of
    /// a socket. See the implementors of the [`Filter`] trait for more information.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use polling::{Poller, PollMode};
    /// use polling::os::kqueue::{Filter, PollerKqueueExt, Signal};
    ///
    /// let poller = Poller::new().unwrap();
    ///
    /// // Register the SIGINT signal.
    /// poller.add_filter(Signal(libc::SIGINT), 0, PollMode::Oneshot).unwrap();
    ///
    /// // Wait for the signal.
    /// let mut events = vec![];
    /// poller.wait(&mut events, None).unwrap();
    /// # let _ = events;
    /// ```
    fn add_filter(&self, filter: F, key: usize, mode: PollMode) -> io::Result<()>;

    /// Modify a filter in the poller.
    ///
    /// This is similar to [`modify`][Poller::modify], but it allows you to specify a filter
    /// instead of a socket. See the implementors of the [`Filter`] trait for more information.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use polling::{Poller, PollMode};
    /// use polling::os::kqueue::{Filter, PollerKqueueExt, Signal};
    ///
    /// let poller = Poller::new().unwrap();
    ///
    /// // Register the SIGINT signal.
    /// poller.add_filter(Signal(libc::SIGINT), 0, PollMode::Oneshot).unwrap();
    ///
    /// // Re-register with a different key.
    /// poller.modify_filter(Signal(libc::SIGINT), 1, PollMode::Oneshot).unwrap();
    ///
    /// // Wait for the signal.
    /// let mut events = vec![];
    /// poller.wait(&mut events, None).unwrap();
    /// # let _ = events;
    /// ```
    fn modify_filter(&self, filter: F, key: usize, mode: PollMode) -> io::Result<()>;

    /// Remove a filter from the poller.
    ///
    /// This is used to remove filters that were previously added with
    /// [`add_filter`](PollerKqueueExt::add_filter).
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use polling::{Poller, PollMode};
    /// use polling::os::kqueue::{Filter, PollerKqueueExt, Signal};
    ///
    /// let poller = Poller::new().unwrap();
    ///
    /// // Register the SIGINT signal.
    /// poller.add_filter(Signal(libc::SIGINT), 0, PollMode::Oneshot).unwrap();
    ///
    /// // Remove the filter.
    /// poller.delete_filter(Signal(libc::SIGINT)).unwrap();
    /// ```
    fn delete_filter(&self, filter: F) -> io::Result<()>;
}

impl<F: Filter> PollerKqueueExt<F> for Poller {
    #[inline(always)]
    fn add_filter(&self, filter: F, key: usize, mode: PollMode) -> io::Result<()> {
        // No difference between adding and modifying in kqueue.
        self.modify_filter(filter, key, mode)
    }

    fn modify_filter(&self, filter: F, key: usize, mode: PollMode) -> io::Result<()> {
        // Convert the filter into a kevent.
        let event = filter.filter(libc::EV_ADD | mode_to_flags(mode), key);

        // Modify the filter.
        self.poller.submit_changes([event])
    }

    fn delete_filter(&self, filter: F) -> io::Result<()> {
        // Convert the filter into a kevent.
        let event = filter.filter(libc::EV_DELETE, 0);

        // Delete the filter.
        self.poller.submit_changes([event])
    }
}

/// A filter that can be registered into a `kqueue`.
pub trait Filter: FilterSealed {}

unsafe impl<T: FilterSealed + ?Sized> FilterSealed for &T {
    #[inline(always)]
    fn filter(&self, flags: FilterFlags, key: usize) -> libc::kevent {
        (**self).filter(flags, key)
    }
}

impl<T: Filter + ?Sized> Filter for &T {}

/// Monitor this signal number.
///
/// No matter what `PollMode` is specified, this filter will always be
/// oneshot-only.
#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Signal(pub c_int);

/// Alias for `libc::c_int`.
#[allow(non_camel_case_types)]
pub type c_int = i32;

unsafe impl FilterSealed for Signal {
    #[inline(always)]
    fn filter(&self, flags: FilterFlags, key: usize) -> libc::kevent {
        libc::kevent {
            ident: self.0 as _,
            filter: libc::EVFILT_SIGNAL,
            flags: flags | libc::EV_RECEIPT,
            udata: key as _,
            ..unsafe { mem::zeroed() }
        }
    }
}

impl Filter for Signal {}

/// Monitor a child process.
#[derive(Debug)]
pub struct Process<'a> {
    /// The child process to monitor.
    child: &'a Child,

    /// The operation to monitor.
    ops: ProcessOps,
}

/// The operations that a monitored process can perform.
#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[non_exhaustive]
pub enum ProcessOps {
    /// The process exited.
    Exit,

    /// The process was forked.
    Fork,

    /// The process executed a new process.
    Exec,
}

impl<'a> Process<'a> {
    /// Monitor a child process.
    pub fn new(child: &'a Child, ops: ProcessOps) -> Self {
        Self { child, ops }
    }
}

unsafe impl FilterSealed for Process<'_> {
    #[inline(always)]
    fn filter(&self, flags: FilterFlags, key: usize) -> libc::kevent {
        let fflags = match self.ops {
            ProcessOps::Exit => libc::NOTE_EXIT,
            ProcessOps::Fork => libc::NOTE_FORK,
            ProcessOps::Exec => libc::NOTE_EXEC,
        };

        libc::kevent {
            ident: self.child.id() as _,
            filter: libc::EVFILT_PROC,
            flags: flags | libc::EV_RECEIPT,
            fflags,
            udata: key as _,
            ..unsafe { mem::zeroed() }
        }
    }
}

impl Filter for Process<'_> {}

/// Wait for a timeout to expire.
///
/// Modifying the timeout after it has been added to the poller will reset it.
#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Timer {
    /// Identifier for the timer.
    pub id: usize,

    /// The timeout to wait for.
    pub timeout: Duration,
}

unsafe impl FilterSealed for Timer {
    fn filter(&self, flags: FilterFlags, key: usize) -> libc::kevent {
        // Figure out the granularity of the timer.
        let (fflags, data) = {
            #[cfg(not(any(target_os = "dragonfly", target_os = "netbsd", target_os = "openbsd")))]
            {
                let subsec_nanos = self.timeout.subsec_nanos();

                match (subsec_nanos % 1_000, subsec_nanos % 1_000_000, subsec_nanos) {
                    (_, _, 0) => (
                        libc::NOTE_SECONDS,
                        self.timeout.as_secs().try_into().expect("too many seconds"),
                    ),
                    (_, 0, _) => (
                        // Note: 0 by default means milliseconds.
                        0,
                        self.timeout
                            .as_millis()
                            .try_into()
                            .expect("too many milliseconds"),
                    ),
                    (0, _, _) => (
                        libc::NOTE_USECONDS,
                        self.timeout
                            .as_micros()
                            .try_into()
                            .expect("too many microseconds"),
                    ),
                    (_, _, _) => (
                        libc::NOTE_NSECONDS,
                        self.timeout
                            .as_nanos()
                            .try_into()
                            .expect("too many nanoseconds"),
                    ),
                }
            }

            #[cfg(any(target_os = "dragonfly", target_os = "netbsd", target_os = "openbsd"))]
            {
                // OpenBSD/Dragonfly/NetBSD only supports milliseconds.
                // NetBSD 10 supports NOTE_SECONDS et al, once Rust drops support for
                // NetBSD 9 we can use the same code as above.
                // See also: https://github.com/rust-lang/libc/pull/3080
                (
                    0,
                    self.timeout
                        .as_millis()
                        .try_into()
                        .expect("too many milliseconds"),
                )
            }
        };

        #[allow(clippy::needless_update)]
        libc::kevent {
            ident: self.id as _,
            filter: libc::EVFILT_TIMER,
            flags: flags | libc::EV_RECEIPT,
            fflags,
            data,
            udata: key as _,
            ..unsafe { mem::zeroed() }
        }
    }
}

impl Filter for Timer {}

mod __private {
    use crate::sys::FilterFlags;

    #[doc(hidden)]
    pub unsafe trait FilterSealed {
        /// Get the filter for the given event.
        ///
        /// This filter's flags must have `EV_RECEIPT`.
        fn filter(&self, flags: FilterFlags, key: usize) -> libc::kevent;
    }
}
