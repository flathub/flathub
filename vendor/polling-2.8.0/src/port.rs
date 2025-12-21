//! Bindings to event port (illumos, Solaris).

use std::io;
use std::os::unix::io::{AsRawFd, RawFd};
use std::ptr;
use std::time::Duration;

#[cfg(not(polling_no_io_safety))]
use std::os::unix::io::{AsFd, BorrowedFd};

use crate::{Event, PollMode};

/// Interface to event ports.
#[derive(Debug)]
pub struct Poller {
    /// File descriptor for the port instance.
    port_fd: RawFd,
}

impl Poller {
    /// Creates a new poller.
    pub fn new() -> io::Result<Poller> {
        let port_fd = syscall!(port_create())?;
        let flags = syscall!(fcntl(port_fd, libc::F_GETFD))?;
        syscall!(fcntl(port_fd, libc::F_SETFD, flags | libc::FD_CLOEXEC))?;

        Ok(Poller { port_fd })
    }

    /// Whether this poller supports level-triggered events.
    pub fn supports_level(&self) -> bool {
        false
    }

    /// Whether this poller supports edge-triggered events.
    pub fn supports_edge(&self) -> bool {
        false
    }

    /// Adds a file descriptor.
    pub fn add(&self, fd: RawFd, ev: Event, mode: PollMode) -> io::Result<()> {
        // File descriptors don't need to be added explicitly, so just modify the interest.
        self.modify(fd, ev, mode)
    }

    /// Modifies an existing file descriptor.
    pub fn modify(&self, fd: RawFd, ev: Event, mode: PollMode) -> io::Result<()> {
        let mut flags = 0;
        if ev.readable {
            flags |= libc::POLLIN;
        }
        if ev.writable {
            flags |= libc::POLLOUT;
        }

        if mode != PollMode::Oneshot {
            return Err(crate::unsupported_error(
                "this kind of event is not supported with event ports",
            ));
        }

        syscall!(port_associate(
            self.port_fd,
            libc::PORT_SOURCE_FD,
            fd as _,
            flags as _,
            ev.key as _,
        ))?;

        Ok(())
    }

    /// Deletes a file descriptor.
    pub fn delete(&self, fd: RawFd) -> io::Result<()> {
        if let Err(e) = syscall!(port_dissociate(
            self.port_fd,
            libc::PORT_SOURCE_FD,
            fd as usize,
        )) {
            match e.raw_os_error().unwrap() {
                libc::ENOENT => return Ok(()),
                _ => return Err(e),
            }
        }

        Ok(())
    }

    /// Waits for I/O events with an optional timeout.
    pub fn wait(&self, events: &mut Events, timeout: Option<Duration>) -> io::Result<()> {
        let mut timeout = timeout.map(|t| libc::timespec {
            tv_sec: t.as_secs() as libc::time_t,
            tv_nsec: t.subsec_nanos() as libc::c_long,
        });
        let mut nget: u32 = 1;

        // Wait for I/O events.
        let res = syscall!(port_getn(
            self.port_fd,
            events.list.as_mut_ptr() as *mut libc::port_event,
            events.list.len() as libc::c_uint,
            &mut nget as _,
            match &mut timeout {
                None => ptr::null_mut(),
                Some(t) => t,
            }
        ));

        // Event ports sets the return value to -1 and returns ETIME on timer expire. The number of
        // returned events is stored in nget, but in our case it should always be 0 since we set
        // nget to 1 initially.
        let nevents = match res {
            Err(e) => match e.raw_os_error().unwrap() {
                libc::ETIME => 0,
                _ => return Err(e),
            },
            Ok(_) => nget as usize,
        };
        events.len = nevents;

        Ok(())
    }

    /// Sends a notification to wake up the current or next `wait()` call.
    pub fn notify(&self) -> io::Result<()> {
        // Use port_send to send a notification to the port.
        syscall!(port_send(
            self.port_fd,
            libc::PORT_SOURCE_USER,
            crate::NOTIFY_KEY as _
        ))?;

        Ok(())
    }
}

impl AsRawFd for Poller {
    fn as_raw_fd(&self) -> RawFd {
        self.port_fd
    }
}

#[cfg(not(polling_no_io_safety))]
impl AsFd for Poller {
    fn as_fd(&self) -> BorrowedFd<'_> {
        // SAFETY: lifetime is bound by self
        unsafe { BorrowedFd::borrow_raw(self.port_fd) }
    }
}

impl Drop for Poller {
    fn drop(&mut self) {
        let _ = syscall!(close(self.port_fd));
    }
}

/// Poll flags for all possible readability events.
fn read_flags() -> libc::c_short {
    libc::POLLIN | libc::POLLHUP | libc::POLLERR | libc::POLLPRI
}

/// Poll flags for all possible writability events.
fn write_flags() -> libc::c_short {
    libc::POLLOUT | libc::POLLHUP | libc::POLLERR
}

/// A list of reported I/O events.
pub struct Events {
    list: Box<[libc::port_event; 1024]>,
    len: usize,
}

unsafe impl Send for Events {}

impl Events {
    /// Creates an empty list.
    pub fn new() -> Events {
        let ev = libc::port_event {
            portev_events: 0,
            portev_source: 0,
            portev_pad: 0,
            portev_object: 0,
            portev_user: 0 as _,
        };
        let list = Box::new([ev; 1024]);
        let len = 0;
        Events { list, len }
    }

    /// Iterates over I/O events.
    pub fn iter(&self) -> impl Iterator<Item = Event> + '_ {
        self.list[..self.len].iter().map(|ev| Event {
            key: ev.portev_user as _,
            readable: (ev.portev_events & read_flags() as libc::c_int) != 0,
            writable: (ev.portev_events & write_flags() as libc::c_int) != 0,
        })
    }
}
