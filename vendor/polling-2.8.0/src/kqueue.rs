//! Bindings to kqueue (macOS, iOS, tvOS, watchOS, FreeBSD, NetBSD, OpenBSD, DragonFly BSD).

use std::io;
use std::mem;
use std::os::unix::io::{AsRawFd, RawFd};
use std::ptr;
use std::time::Duration;

#[cfg(not(polling_no_io_safety))]
use std::os::unix::io::{AsFd, BorrowedFd};

use crate::{Event, PollMode};

/// Interface to kqueue.
#[derive(Debug)]
pub struct Poller {
    /// File descriptor for the kqueue instance.
    kqueue_fd: RawFd,

    /// Notification pipe for waking up the poller.
    ///
    /// On platforms that support `EVFILT_USER`, this uses that to wake up the poller. Otherwise, it
    /// uses a pipe.
    notify: notify::Notify,
}

impl Poller {
    /// Creates a new poller.
    pub fn new() -> io::Result<Poller> {
        // Create a kqueue instance.
        let kqueue_fd = syscall!(kqueue())?;
        syscall!(fcntl(kqueue_fd, libc::F_SETFD, libc::FD_CLOEXEC))?;

        let poller = Poller {
            kqueue_fd,
            notify: notify::Notify::new()?,
        };

        // Register the notification pipe.
        poller.notify.register(&poller)?;

        log::trace!("new: kqueue_fd={}", kqueue_fd,);
        Ok(poller)
    }

    /// Whether this poller supports level-triggered events.
    pub fn supports_level(&self) -> bool {
        true
    }

    /// Whether this poller supports edge-triggered events.
    pub fn supports_edge(&self) -> bool {
        true
    }

    /// Adds a new file descriptor.
    pub fn add(&self, fd: RawFd, ev: Event, mode: PollMode) -> io::Result<()> {
        // File descriptors don't need to be added explicitly, so just modify the interest.
        self.modify(fd, ev, mode)
    }

    /// Modifies an existing file descriptor.
    pub fn modify(&self, fd: RawFd, ev: Event, mode: PollMode) -> io::Result<()> {
        if !self.notify.has_fd(fd) {
            log::trace!("add: kqueue_fd={}, fd={}, ev={:?}", self.kqueue_fd, fd, ev);
        }

        let mode_flags = mode_to_flags(mode);

        let read_flags = if ev.readable {
            libc::EV_ADD | mode_flags
        } else {
            libc::EV_DELETE
        };
        let write_flags = if ev.writable {
            libc::EV_ADD | mode_flags
        } else {
            libc::EV_DELETE
        };

        // A list of changes for kqueue.
        let changelist = [
            libc::kevent {
                ident: fd as _,
                filter: libc::EVFILT_READ,
                flags: read_flags | libc::EV_RECEIPT,
                udata: ev.key as _,
                ..unsafe { mem::zeroed() }
            },
            libc::kevent {
                ident: fd as _,
                filter: libc::EVFILT_WRITE,
                flags: write_flags | libc::EV_RECEIPT,
                udata: ev.key as _,
                ..unsafe { mem::zeroed() }
            },
        ];

        // Apply changes.
        self.submit_changes(changelist)
    }

    /// Submit one or more changes to the kernel queue and check to see if they succeeded.
    pub(crate) fn submit_changes<A>(&self, changelist: A) -> io::Result<()>
    where
        A: Copy + AsRef<[libc::kevent]> + AsMut<[libc::kevent]>,
    {
        let mut eventlist = changelist;

        // Apply changes.
        {
            let changelist = changelist.as_ref();
            let eventlist = eventlist.as_mut();

            syscall!(kevent(
                self.kqueue_fd,
                changelist.as_ptr() as *const libc::kevent,
                changelist.len() as _,
                eventlist.as_mut_ptr() as *mut libc::kevent,
                eventlist.len() as _,
                ptr::null(),
            ))?;
        }

        // Check for errors.
        for &ev in eventlist.as_ref() {
            // Explanation for ignoring EPIPE: https://github.com/tokio-rs/mio/issues/582
            if (ev.flags & libc::EV_ERROR) != 0
                && ev.data != 0
                && ev.data != libc::ENOENT as _
                && ev.data != libc::EPIPE as _
            {
                return Err(io::Error::from_raw_os_error(ev.data as _));
            }
        }

        Ok(())
    }

    /// Deletes a file descriptor.
    pub fn delete(&self, fd: RawFd) -> io::Result<()> {
        // Simply delete interest in the file descriptor.
        self.modify(fd, Event::none(0), PollMode::Oneshot)
    }

    /// Waits for I/O events with an optional timeout.
    pub fn wait(&self, events: &mut Events, timeout: Option<Duration>) -> io::Result<()> {
        log::trace!("wait: kqueue_fd={}, timeout={:?}", self.kqueue_fd, timeout);

        // Convert the `Duration` to `libc::timespec`.
        let timeout = timeout.map(|t| libc::timespec {
            tv_sec: t.as_secs() as libc::time_t,
            tv_nsec: t.subsec_nanos() as libc::c_long,
        });

        // Wait for I/O events.
        let changelist = [];
        let eventlist = &mut events.list;
        let res = syscall!(kevent(
            self.kqueue_fd,
            changelist.as_ptr() as *const libc::kevent,
            changelist.len() as _,
            eventlist.as_mut_ptr() as *mut libc::kevent,
            eventlist.len() as _,
            match &timeout {
                None => ptr::null(),
                Some(t) => t,
            }
        ))?;
        events.len = res as usize;
        log::trace!("new events: kqueue_fd={}, res={}", self.kqueue_fd, res);

        // Clear the notification (if received) and re-register interest in it.
        self.notify.reregister(self)?;

        Ok(())
    }

    /// Sends a notification to wake up the current or next `wait()` call.
    pub fn notify(&self) -> io::Result<()> {
        log::trace!("notify: kqueue_fd={}", self.kqueue_fd);
        self.notify.notify(self).ok();
        Ok(())
    }
}

impl AsRawFd for Poller {
    fn as_raw_fd(&self) -> RawFd {
        self.kqueue_fd
    }
}

#[cfg(not(polling_no_io_safety))]
impl AsFd for Poller {
    fn as_fd(&self) -> BorrowedFd<'_> {
        // SAFETY: lifetime is bound by "self"
        unsafe { BorrowedFd::borrow_raw(self.kqueue_fd) }
    }
}

impl Drop for Poller {
    fn drop(&mut self) {
        log::trace!("drop: kqueue_fd={}", self.kqueue_fd);
        let _ = self.notify.deregister(self);
        let _ = syscall!(close(self.kqueue_fd));
    }
}

/// A list of reported I/O events.
pub struct Events {
    list: Box<[libc::kevent; 1024]>,
    len: usize,
}

unsafe impl Send for Events {}

impl Events {
    /// Creates an empty list.
    pub fn new() -> Events {
        let ev: libc::kevent = unsafe { mem::zeroed() };
        let list = Box::new([ev; 1024]);
        let len = 0;
        Events { list, len }
    }

    /// Iterates over I/O events.
    pub fn iter(&self) -> impl Iterator<Item = Event> + '_ {
        const READABLES: &[FilterName] = &[
            libc::EVFILT_READ,
            libc::EVFILT_VNODE,
            libc::EVFILT_PROC,
            libc::EVFILT_SIGNAL,
            libc::EVFILT_TIMER,
        ];

        // On some platforms, closing the read end of a pipe wakes up writers, but the
        // event is reported as EVFILT_READ with the EV_EOF flag.
        //
        // https://github.com/golang/go/commit/23aad448b1e3f7c3b4ba2af90120bde91ac865b4
        self.list[..self.len].iter().map(|ev| Event {
            key: ev.udata as usize,
            readable: READABLES.contains(&ev.filter),
            writable: ev.filter == libc::EVFILT_WRITE
                || (ev.filter == libc::EVFILT_READ && (ev.flags & libc::EV_EOF) != 0),
        })
    }
}

pub(crate) fn mode_to_flags(mode: PollMode) -> FilterFlags {
    match mode {
        PollMode::Oneshot => libc::EV_ONESHOT,
        PollMode::Level => 0,
        PollMode::Edge => libc::EV_CLEAR,
        PollMode::EdgeOneshot => libc::EV_ONESHOT | libc::EV_CLEAR,
    }
}

#[cfg(target_os = "netbsd")]
pub(crate) type FilterFlags = u32;

#[cfg(not(target_os = "netbsd"))]
pub(crate) type FilterFlags = libc::c_ushort;

#[cfg(target_os = "netbsd")]
pub(crate) type FilterName = u32;

#[cfg(not(target_os = "netbsd"))]
pub(crate) type FilterName = libc::c_short;

#[cfg(any(
    target_os = "freebsd",
    target_os = "dragonfly",
    target_os = "macos",
    target_os = "ios",
    target_os = "tvos",
    target_os = "watchos",
))]
mod notify {
    use super::Poller;
    use std::os::unix::io::RawFd;
    use std::{io, mem};

    /// A notification pipe.
    ///
    /// This implementation uses `EVFILT_USER` to avoid allocating a pipe.
    #[derive(Debug)]
    pub(super) struct Notify;

    impl Notify {
        /// Creates a new notification pipe.
        pub(super) fn new() -> io::Result<Self> {
            Ok(Self)
        }

        /// Registers this notification pipe in the `Poller`.
        pub(super) fn register(&self, poller: &Poller) -> io::Result<()> {
            // Register an EVFILT_USER event.
            poller.submit_changes([libc::kevent {
                ident: 0,
                filter: libc::EVFILT_USER,
                flags: libc::EV_ADD | libc::EV_RECEIPT | libc::EV_CLEAR,
                udata: crate::NOTIFY_KEY as _,
                ..unsafe { mem::zeroed() }
            }])
        }

        /// Reregister this notification pipe in the `Poller`.
        pub(super) fn reregister(&self, _poller: &Poller) -> io::Result<()> {
            // We don't need to do anything, it's already registered as EV_CLEAR.
            Ok(())
        }

        /// Notifies the `Poller`.
        pub(super) fn notify(&self, poller: &Poller) -> io::Result<()> {
            // Trigger the EVFILT_USER event.
            poller.submit_changes([libc::kevent {
                ident: 0,
                filter: libc::EVFILT_USER,
                flags: libc::EV_ADD | libc::EV_RECEIPT,
                fflags: libc::NOTE_TRIGGER,
                udata: crate::NOTIFY_KEY as _,
                ..unsafe { mem::zeroed() }
            }])?;

            Ok(())
        }

        /// Deregisters this notification pipe from the `Poller`.
        pub(super) fn deregister(&self, poller: &Poller) -> io::Result<()> {
            // Deregister the EVFILT_USER event.
            poller.submit_changes([libc::kevent {
                ident: 0,
                filter: libc::EVFILT_USER,
                flags: libc::EV_RECEIPT | libc::EV_DELETE,
                udata: crate::NOTIFY_KEY as _,
                ..unsafe { mem::zeroed() }
            }])
        }

        /// Whether this raw file descriptor is associated with this pipe.
        pub(super) fn has_fd(&self, _fd: RawFd) -> bool {
            false
        }
    }
}

#[cfg(not(any(
    target_os = "freebsd",
    target_os = "dragonfly",
    target_os = "macos",
    target_os = "ios",
    target_os = "tvos",
    target_os = "watchos",
)))]
mod notify {
    use super::Poller;
    use crate::{Event, PollMode, NOTIFY_KEY};
    use std::io::{self, prelude::*};
    use std::os::unix::{
        io::{AsRawFd, RawFd},
        net::UnixStream,
    };

    /// A notification pipe.
    ///
    /// This implementation uses a pipe to send notifications.
    #[derive(Debug)]
    pub(super) struct Notify {
        /// The read end of the pipe.
        read_stream: UnixStream,

        /// The write end of the pipe.
        write_stream: UnixStream,
    }

    impl Notify {
        /// Creates a new notification pipe.
        pub(super) fn new() -> io::Result<Self> {
            let (read_stream, write_stream) = UnixStream::pair()?;
            read_stream.set_nonblocking(true)?;
            write_stream.set_nonblocking(true)?;

            Ok(Self {
                read_stream,
                write_stream,
            })
        }

        /// Registers this notification pipe in the `Poller`.
        pub(super) fn register(&self, poller: &Poller) -> io::Result<()> {
            // Register the read end of this pipe.
            poller.add(
                self.read_stream.as_raw_fd(),
                Event::readable(NOTIFY_KEY),
                PollMode::Oneshot,
            )
        }

        /// Reregister this notification pipe in the `Poller`.
        pub(super) fn reregister(&self, poller: &Poller) -> io::Result<()> {
            // Clear out the notification.
            while (&self.read_stream).read(&mut [0; 64]).is_ok() {}

            // Reregister the read end of this pipe.
            poller.modify(
                self.read_stream.as_raw_fd(),
                Event::readable(NOTIFY_KEY),
                PollMode::Oneshot,
            )
        }

        /// Notifies the `Poller`.
        #[allow(clippy::unused_io_amount)]
        pub(super) fn notify(&self, _poller: &Poller) -> io::Result<()> {
            // Write to the write end of the pipe
            (&self.write_stream).write(&[1])?;

            Ok(())
        }

        /// Deregisters this notification pipe from the `Poller`.
        pub(super) fn deregister(&self, poller: &Poller) -> io::Result<()> {
            // Deregister the read end of the pipe.
            poller.delete(self.read_stream.as_raw_fd())
        }

        /// Whether this raw file descriptor is associated with this pipe.
        pub(super) fn has_fd(&self, fd: RawFd) -> bool {
            self.read_stream.as_raw_fd() == fd
        }
    }
}
