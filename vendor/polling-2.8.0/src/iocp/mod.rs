//! Bindings to Windows I/O Completion Ports.
//!
//! I/O Completion Ports is a completion-based API rather than a polling-based API, like
//! epoll or kqueue. Therefore, we have to adapt the IOCP API to the crate's API.
//!
//! WinSock is powered by the Auxillary Function Driver (AFD) subsystem, which can be
//! accessed directly by using unstable `ntdll` functions. AFD exposes features that are not
//! available through the normal WinSock interface, such as IOCTL_AFD_POLL. This function is
//! similar to the exposed `WSAPoll` method. However, once the targeted socket is "ready",
//! a completion packet is queued to an I/O completion port.
//!
//! We take advantage of IOCTL_AFD_POLL to "translate" this crate's polling-based API
//! to the one Windows expects. When a device is added to the `Poller`, an IOCTL_AFD_POLL
//! operation is started and queued to the IOCP. To modify a currently registered device
//! (e.g. with `modify()` or `delete()`), the ongoing POLL is cancelled and then restarted
//! with new parameters. Whn the POLL eventually completes, the packet is posted to the IOCP.
//! From here it's a simple matter of using `GetQueuedCompletionStatusEx` to read the packets
//! from the IOCP and react accordingly. Notifying the poller is trivial, because we can
//! simply post a packet to the IOCP to wake it up.
//!
//! The main disadvantage of this strategy is that it relies on unstable Windows APIs.
//! However, as `libuv` (the backing I/O library for Node.JS) relies on the same unstable
//! AFD strategy, it is unlikely to be broken without plenty of advanced warning.
//!
//! Previously, this crate used the `wepoll` library for polling. `wepoll` uses a similar
//! AFD-based strategy for polling.

mod afd;
mod port;

use afd::{base_socket, Afd, AfdPollInfo, AfdPollMask, HasAfdInfo, IoStatusBlock};
use port::{IoCompletionPort, OverlappedEntry};
use windows_sys::Win32::Foundation::{ERROR_INVALID_HANDLE, ERROR_IO_PENDING, STATUS_CANCELLED};

use crate::{Event, PollMode};

use concurrent_queue::ConcurrentQueue;
use pin_project_lite::pin_project;

use std::cell::UnsafeCell;
use std::collections::hash_map::{Entry, HashMap};
use std::fmt;
use std::io;
use std::marker::PhantomPinned;
use std::os::windows::io::{AsRawHandle, RawHandle, RawSocket};
use std::pin::Pin;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, MutexGuard, RwLock, Weak};
use std::time::{Duration, Instant};

#[cfg(not(polling_no_io_safety))]
use std::os::windows::io::{AsHandle, BorrowedHandle};

/// Macro to lock and ignore lock poisoning.
macro_rules! lock {
    ($lock_result:expr) => {{
        $lock_result.unwrap_or_else(|e| e.into_inner())
    }};
}

/// Interface to I/O completion ports.
#[derive(Debug)]
pub(super) struct Poller {
    /// The I/O completion port.
    port: IoCompletionPort<Packet>,

    /// List of currently active AFD instances.
    ///
    /// Weak references are kept here so that the AFD handle is automatically dropped
    /// when the last associated socket is dropped.
    afd: Mutex<Vec<Weak<Afd<Packet>>>>,

    /// The state of the sources registered with this poller.
    sources: RwLock<HashMap<RawSocket, Packet>>,

    /// Sockets with pending updates.
    pending_updates: ConcurrentQueue<Packet>,

    /// Are we currently polling?
    polling: AtomicBool,

    /// A list of completion packets.
    packets: Mutex<Vec<OverlappedEntry<Packet>>>,

    /// The packet used to notify the poller.
    notifier: Packet,
}

unsafe impl Send for Poller {}
unsafe impl Sync for Poller {}

impl Poller {
    /// Creates a new poller.
    pub(super) fn new() -> io::Result<Self> {
        // Make sure AFD is able to be used.
        if let Err(e) = afd::NtdllImports::force_load() {
            return Err(crate::unsupported_error(format!(
                "Failed to initialize unstable Windows functions: {}\nThis usually only happens for old Windows or Wine.",
                e
            )));
        }

        // Create and destroy a single AFD to test if we support it.
        Afd::<Packet>::new().map_err(|e| crate::unsupported_error(format!(
            "Failed to initialize \\Device\\Afd: {}\nThis usually only happens for old Windows or Wine.",
            e,
        )))?;

        let port = IoCompletionPort::new(0)?;

        log::trace!("new: handle={:?}", &port);

        Ok(Poller {
            port,
            afd: Mutex::new(vec![]),
            sources: RwLock::new(HashMap::new()),
            pending_updates: ConcurrentQueue::bounded(1024),
            polling: AtomicBool::new(false),
            packets: Mutex::new(Vec::with_capacity(1024)),
            notifier: Arc::pin(
                PacketInner::Wakeup {
                    _pinned: PhantomPinned,
                }
                .into(),
            ),
        })
    }

    /// Whether this poller supports level-triggered events.
    pub(super) fn supports_level(&self) -> bool {
        true
    }

    /// Whether this poller supports edge-triggered events.
    pub(super) fn supports_edge(&self) -> bool {
        false
    }

    /// Add a new source to the poller.
    pub(super) fn add(&self, socket: RawSocket, interest: Event, mode: PollMode) -> io::Result<()> {
        log::trace!(
            "add: handle={:?}, sock={}, ev={:?}",
            self.port,
            socket,
            interest
        );

        // We don't support edge-triggered events.
        if matches!(mode, PollMode::Edge | PollMode::EdgeOneshot) {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "edge-triggered events are not supported",
            ));
        }

        // Create a new packet.
        let socket_state = {
            let state = SocketState {
                socket,
                base_socket: base_socket(socket)?,
                interest,
                interest_error: true,
                afd: self.afd_handle()?,
                mode,
                waiting_on_delete: false,
                status: SocketStatus::Idle,
            };

            Arc::pin(IoStatusBlock::from(PacketInner::Socket {
                packet: UnsafeCell::new(AfdPollInfo::default()),
                socket: Mutex::new(state),
            }))
        };

        // Keep track of the source in the poller.
        {
            let mut sources = lock!(self.sources.write());

            match sources.entry(socket) {
                Entry::Vacant(v) => {
                    v.insert(Pin::<Arc<_>>::clone(&socket_state));
                }

                Entry::Occupied(_) => {
                    return Err(io::Error::from(io::ErrorKind::AlreadyExists));
                }
            }
        }

        // Update the packet.
        self.update_packet(socket_state)
    }

    /// Update a source in the poller.
    pub(super) fn modify(
        &self,
        socket: RawSocket,
        interest: Event,
        mode: PollMode,
    ) -> io::Result<()> {
        log::trace!(
            "modify: handle={:?}, sock={}, ev={:?}",
            self.port,
            socket,
            interest
        );

        // We don't support edge-triggered events.
        if matches!(mode, PollMode::Edge | PollMode::EdgeOneshot) {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "edge-triggered events are not supported",
            ));
        }

        // Get a reference to the source.
        let source = {
            let sources = lock!(self.sources.read());

            sources
                .get(&socket)
                .cloned()
                .ok_or_else(|| io::Error::from(io::ErrorKind::NotFound))?
        };

        // Set the new event.
        if source.as_ref().set_events(interest, mode) {
            self.update_packet(source)?;
        }

        Ok(())
    }

    /// Delete a source from the poller.
    pub(super) fn delete(&self, socket: RawSocket) -> io::Result<()> {
        log::trace!("remove: handle={:?}, sock={}", self.port, socket);

        // Get a reference to the source.
        let source = {
            let mut sources = lock!(self.sources.write());

            match sources.remove(&socket) {
                Some(s) => s,
                None => {
                    // If the source has already been removed, then we can just return.
                    return Ok(());
                }
            }
        };

        // Indicate to the source that it is being deleted.
        // This cancels any ongoing AFD_IOCTL_POLL operations.
        source.begin_delete()
    }

    /// Wait for events.
    pub(super) fn wait(&self, events: &mut Events, timeout: Option<Duration>) -> io::Result<()> {
        log::trace!("wait: handle={:?}, timeout={:?}", self.port, timeout);

        let deadline = timeout.and_then(|timeout| Instant::now().checked_add(timeout));
        let mut packets = lock!(self.packets.lock());
        let mut notified = false;
        events.packets.clear();

        loop {
            let mut new_events = 0;

            // Indicate that we are now polling.
            let was_polling = self.polling.swap(true, Ordering::SeqCst);
            debug_assert!(!was_polling);

            let guard = CallOnDrop(|| {
                let was_polling = self.polling.swap(false, Ordering::SeqCst);
                debug_assert!(was_polling);
            });

            // Process every entry in the queue before we start polling.
            self.drain_update_queue(false)?;

            // Get the time to wait for.
            let timeout = deadline.map(|t| t.saturating_duration_since(Instant::now()));

            // Wait for I/O events.
            let len = self.port.wait(&mut packets, timeout)?;
            log::trace!("new events: handle={:?}, len={}", self.port, len);

            // We are no longer polling.
            drop(guard);

            // Process all of the events.
            for entry in packets.drain(..) {
                let packet = entry.into_packet();

                // Feed the event into the packet.
                match packet.feed_event(self)? {
                    FeedEventResult::NoEvent => {}
                    FeedEventResult::Event(event) => {
                        events.packets.push(event);
                        new_events += 1;
                    }
                    FeedEventResult::Notified => {
                        notified = true;
                    }
                }
            }

            // Break if there was a notification or at least one event, or if deadline is reached.
            let timeout_is_empty =
                timeout.map_or(false, |t| t.as_secs() == 0 && t.subsec_nanos() == 0);
            if notified || new_events > 0 || timeout_is_empty {
                break;
            }

            log::trace!("wait: no events found, re-entering polling loop");
        }

        Ok(())
    }

    /// Notify this poller.
    pub(super) fn notify(&self) -> io::Result<()> {
        // Push the notify packet into the IOCP.
        self.port.post(0, 0, self.notifier.clone())
    }

    /// Push an IOCP packet into the queue.
    pub(super) fn post(&self, packet: CompletionPacket) -> io::Result<()> {
        self.port.post(0, 0, packet.0)
    }

    /// Run an update on a packet.
    fn update_packet(&self, mut packet: Packet) -> io::Result<()> {
        loop {
            // If we are currently polling, we need to update the packet immediately.
            if self.polling.load(Ordering::Acquire) {
                packet.update()?;
                return Ok(());
            }

            // Try to queue the update.
            match self.pending_updates.push(packet) {
                Ok(()) => return Ok(()),
                Err(p) => packet = p.into_inner(),
            }

            // If we failed to queue the update, we need to drain the queue first.
            self.drain_update_queue(true)?;
        }
    }

    /// Drain the update queue.
    fn drain_update_queue(&self, limit: bool) -> io::Result<()> {
        let max = if limit {
            self.pending_updates.capacity().unwrap()
        } else {
            std::usize::MAX
        };

        // Only drain the queue's capacity, since this could in theory run forever.
        self.pending_updates
            .try_iter()
            .take(max)
            .try_for_each(|packet| packet.update())
    }

    /// Get a handle to the AFD reference.
    fn afd_handle(&self) -> io::Result<Arc<Afd<Packet>>> {
        const AFD_MAX_SIZE: usize = 32;

        // Crawl the list and see if there are any existing AFD instances that we can use.
        // Remove any unused AFD pointers.
        let mut afd_handles = lock!(self.afd.lock());
        let mut i = 0;
        while i < afd_handles.len() {
            // Get the reference count of the AFD instance.
            let refcount = Weak::strong_count(&afd_handles[i]);

            match refcount {
                0 => {
                    // Prune the AFD pointer if it has no references.
                    afd_handles.swap_remove(i);
                }

                refcount if refcount >= AFD_MAX_SIZE => {
                    // Skip this one, since it is already at the maximum size.
                    i += 1;
                }

                _ => {
                    // We can use this AFD instance.
                    match afd_handles[i].upgrade() {
                        Some(afd) => return Ok(afd),
                        None => {
                            // The last socket dropped the AFD before we could acquire it.
                            // Prune the AFD pointer and continue.
                            afd_handles.swap_remove(i);
                        }
                    }
                }
            }
        }

        // No available handles, create a new AFD instance.
        let afd = Arc::new(Afd::new()?);

        // Register the AFD instance with the I/O completion port.
        self.port.register(&*afd, true)?;

        // Insert a weak pointer to the AFD instance into the list.
        afd_handles.push(Arc::downgrade(&afd));

        Ok(afd)
    }
}

impl AsRawHandle for Poller {
    fn as_raw_handle(&self) -> RawHandle {
        self.port.as_raw_handle()
    }
}

#[cfg(not(polling_no_io_safety))]
impl AsHandle for Poller {
    fn as_handle(&self) -> BorrowedHandle<'_> {
        unsafe { BorrowedHandle::borrow_raw(self.as_raw_handle()) }
    }
}

/// The container for events.
pub(super) struct Events {
    /// List of IOCP packets.
    packets: Vec<Event>,
}

unsafe impl Send for Events {}

impl Events {
    /// Creates an empty list of events.
    pub(super) fn new() -> Events {
        Events {
            packets: Vec::with_capacity(1024),
        }
    }

    /// Iterate over I/O events.
    pub(super) fn iter(&self) -> impl Iterator<Item = Event> + '_ {
        self.packets.iter().copied()
    }
}

/// A packet used to wake up the poller with an event.
#[derive(Debug, Clone)]
pub struct CompletionPacket(Packet);

impl CompletionPacket {
    /// Create a new completion packet with a custom event.
    pub fn new(event: Event) -> Self {
        Self(Arc::pin(IoStatusBlock::from(PacketInner::Custom { event })))
    }

    /// Get the event associated with this packet.
    pub fn event(&self) -> &Event {
        let data = self.0.as_ref().data().project_ref();

        match data {
            PacketInnerProj::Custom { event } => event,
            _ => unreachable!(),
        }
    }
}

/// The type of our completion packet.
type Packet = Pin<Arc<PacketUnwrapped>>;
type PacketUnwrapped = IoStatusBlock<PacketInner>;

pin_project! {
    /// The inner type of the packet.
    #[project_ref = PacketInnerProj]
    #[project = PacketInnerProjMut]
    enum PacketInner {
        // A packet for a socket.
        Socket {
            // The AFD packet state.
            #[pin]
            packet: UnsafeCell<AfdPollInfo>,

            // The socket state.
            socket: Mutex<SocketState>
        },

        /// A custom event sent by the user.
        Custom {
            event: Event,
        },

        // A packet used to wake up the poller.
        Wakeup { #[pin] _pinned: PhantomPinned },
    }
}

impl fmt::Debug for PacketInner {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Wakeup { .. } => f.write_str("Wakeup { .. }"),
            Self::Custom { event } => f.debug_struct("Custom").field("event", event).finish(),
            Self::Socket { socket, .. } => f
                .debug_struct("Socket")
                .field("packet", &"..")
                .field("socket", socket)
                .finish(),
        }
    }
}

impl HasAfdInfo for PacketInner {
    fn afd_info(self: Pin<&Self>) -> Pin<&UnsafeCell<AfdPollInfo>> {
        match self.project_ref() {
            PacketInnerProj::Socket { packet, .. } => packet,
            _ => unreachable!(),
        }
    }
}

impl PacketUnwrapped {
    /// Set the new events that this socket is waiting on.
    ///
    /// Returns `true` if we need to be updated.
    fn set_events(self: Pin<&Self>, interest: Event, mode: PollMode) -> bool {
        let mut socket = match self.socket_state() {
            Some(s) => s,
            None => return false,
        };

        socket.interest = interest;
        socket.mode = mode;
        socket.interest_error = true;

        match socket.status {
            SocketStatus::Polling { readable, writable } => {
                (interest.readable && !readable) || (interest.writable && !writable)
            }
            _ => true,
        }
    }

    /// Update the socket and install the new status in AFD.
    fn update(self: Pin<Arc<Self>>) -> io::Result<()> {
        let mut socket = match self.as_ref().socket_state() {
            Some(s) => s,
            None => return Err(io::Error::new(io::ErrorKind::Other, "invalid socket state")),
        };

        // If we are waiting on a delete, just return, dropping the packet.
        if socket.waiting_on_delete {
            return Ok(());
        }

        // Check the current status.
        match socket.status {
            SocketStatus::Polling { readable, writable } => {
                // If we need to poll for events aside from what we are currently polling, we need
                // to update the packet. Cancel the ongoing poll.
                if (socket.interest.readable && !readable)
                    || (socket.interest.writable && !writable)
                {
                    return self.cancel(socket);
                }

                // All events that we are currently waiting on are accounted for.
                Ok(())
            }

            SocketStatus::Cancelled => {
                // The ongoing operation was cancelled, and we're still waiting for it to return.
                // For now, wait until the top-level loop calls feed_event().
                Ok(())
            }

            SocketStatus::Idle => {
                // Start a new poll.
                let result = socket.afd.poll(
                    self.clone(),
                    socket.base_socket,
                    event_to_afd_mask(
                        socket.interest.readable,
                        socket.interest.writable,
                        socket.interest_error,
                    ),
                );

                match result {
                    Ok(()) => {}

                    Err(err)
                        if err.raw_os_error() == Some(ERROR_IO_PENDING as i32)
                            || err.kind() == io::ErrorKind::WouldBlock =>
                    {
                        // The operation is pending.
                    }

                    Err(err) if err.raw_os_error() == Some(ERROR_INVALID_HANDLE as i32) => {
                        // The socket was closed. We need to delete it.
                        // This should happen after we drop it here.
                    }

                    Err(err) => return Err(err),
                }

                // We are now polling for the current events.
                socket.status = SocketStatus::Polling {
                    readable: socket.interest.readable,
                    writable: socket.interest.writable,
                };

                Ok(())
            }
        }
    }

    /// This socket state was notified; see if we need to update it.
    fn feed_event(self: Pin<Arc<Self>>, poller: &Poller) -> io::Result<FeedEventResult> {
        let inner = self.as_ref().data().project_ref();

        let (afd_info, socket) = match inner {
            PacketInnerProj::Socket { packet, socket } => (packet, socket),
            PacketInnerProj::Custom { event } => {
                // This is a custom event.
                return Ok(FeedEventResult::Event(*event));
            }
            PacketInnerProj::Wakeup { .. } => {
                // The poller was notified.
                return Ok(FeedEventResult::Notified);
            }
        };

        let mut socket_state = lock!(socket.lock());
        let mut event = Event::none(socket_state.interest.key);

        // Put ourselves into the idle state.
        socket_state.status = SocketStatus::Idle;

        // If we are waiting to be deleted, just return and let the drop handler do their thing.
        if socket_state.waiting_on_delete {
            return Ok(FeedEventResult::NoEvent);
        }

        unsafe {
            // SAFETY: The packet is not in transit.
            let iosb = &mut *self.as_ref().iosb().get();

            // Check the status.
            match iosb.Anonymous.Status {
                STATUS_CANCELLED => {
                    // Poll request was cancelled.
                }

                status if status < 0 => {
                    // There was an error, so we signal both ends.
                    event.readable = true;
                    event.writable = true;
                }

                _ => {
                    // Check in on the AFD data.
                    let afd_data = &*afd_info.get();

                    if afd_data.handle_count() >= 1 {
                        let events = afd_data.events();

                        // If we closed the socket, remove it from being polled.
                        if events.contains(AfdPollMask::LOCAL_CLOSE) {
                            let source = lock!(poller.sources.write())
                                .remove(&socket_state.socket)
                                .unwrap();
                            return source.begin_delete().map(|()| FeedEventResult::NoEvent);
                        }

                        // Report socket-related events.
                        let (readable, writable) = afd_mask_to_event(events);
                        event.readable = readable;
                        event.writable = writable;
                    }
                }
            }
        }

        // Filter out events that the user didn't ask for.
        event.readable &= socket_state.interest.readable;
        event.writable &= socket_state.interest.writable;

        // If this event doesn't have anything that interests us, don't return or
        // update the oneshot state.
        let return_value = if event.readable || event.writable {
            // If we are in oneshot mode, remove the interest.
            if matches!(socket_state.mode, PollMode::Oneshot) {
                socket_state.interest = Event::none(socket_state.interest.key);
                socket_state.interest_error = false;
            }

            FeedEventResult::Event(event)
        } else {
            FeedEventResult::NoEvent
        };

        // Put ourselves in the update queue.
        drop(socket_state);
        poller.update_packet(self)?;

        // Return the event.
        Ok(return_value)
    }

    /// Begin deleting this socket.
    fn begin_delete(self: Pin<Arc<Self>>) -> io::Result<()> {
        // If we aren't already being deleted, start deleting.
        let mut socket = self
            .as_ref()
            .socket_state()
            .expect("can't delete packet that doesn't belong to a socket");
        if !socket.waiting_on_delete {
            socket.waiting_on_delete = true;

            if matches!(socket.status, SocketStatus::Polling { .. }) {
                // Cancel the ongoing poll.
                self.cancel(socket)?;
            }
        }

        // Either drop it now or wait for it to be dropped later.
        Ok(())
    }

    fn cancel(self: &Pin<Arc<Self>>, mut socket: MutexGuard<'_, SocketState>) -> io::Result<()> {
        assert!(matches!(socket.status, SocketStatus::Polling { .. }));

        // Send the cancel request.
        unsafe {
            socket.afd.cancel(self)?;
        }

        // Move state to cancelled.
        socket.status = SocketStatus::Cancelled;

        Ok(())
    }

    fn socket_state(self: Pin<&Self>) -> Option<MutexGuard<'_, SocketState>> {
        let inner = self.data().project_ref();

        let state = match inner {
            PacketInnerProj::Socket { socket, .. } => socket,
            _ => return None,
        };

        Some(lock!(state.lock()))
    }
}

/// Per-socket state.
#[derive(Debug)]
struct SocketState {
    /// The raw socket handle.
    socket: RawSocket,

    /// The base socket handle.
    base_socket: RawSocket,

    /// The event that this socket is currently waiting on.
    interest: Event,

    /// Whether to listen for error events.
    interest_error: bool,

    /// The current poll mode.
    mode: PollMode,

    /// The AFD instance that this socket is registered with.
    afd: Arc<Afd<Packet>>,

    /// Whether this socket is waiting to be deleted.
    waiting_on_delete: bool,

    /// The current status of the socket.
    status: SocketStatus,
}

/// The mode that a socket can be in.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
enum SocketStatus {
    /// We are currently not polling.
    Idle,

    /// We are currently polling these events.
    Polling {
        /// We are currently polling for readable events.
        readable: bool,

        /// We are currently polling for writable events.
        writable: bool,
    },

    /// The last poll operation was cancelled, and we're waiting for it to
    /// complete.
    Cancelled,
}

/// The result of calling `feed_event`.
#[derive(Debug)]
enum FeedEventResult {
    /// No event was yielded.
    NoEvent,

    /// An event was yielded.
    Event(Event),

    /// The poller has been notified.
    Notified,
}

fn event_to_afd_mask(readable: bool, writable: bool, error: bool) -> afd::AfdPollMask {
    use afd::AfdPollMask as AfdPoll;

    let mut mask = AfdPoll::empty();

    if error || readable || writable {
        mask |= AfdPoll::ABORT | AfdPoll::CONNECT_FAIL;
    }

    if readable {
        mask |=
            AfdPoll::RECEIVE | AfdPoll::ACCEPT | AfdPoll::DISCONNECT | AfdPoll::RECEIVE_EXPEDITED;
    }

    if writable {
        mask |= AfdPoll::SEND;
    }

    mask
}

fn afd_mask_to_event(mask: afd::AfdPollMask) -> (bool, bool) {
    use afd::AfdPollMask as AfdPoll;

    let mut readable = false;
    let mut writable = false;

    if mask.intersects(
        AfdPoll::RECEIVE | AfdPoll::ACCEPT | AfdPoll::DISCONNECT | AfdPoll::RECEIVE_EXPEDITED,
    ) {
        readable = true;
    }

    if mask.intersects(AfdPoll::SEND) {
        writable = true;
    }

    if mask.intersects(AfdPoll::ABORT | AfdPoll::CONNECT_FAIL) {
        readable = true;
        writable = true;
    }

    (readable, writable)
}

struct CallOnDrop<F: FnMut()>(F);

impl<F: FnMut()> Drop for CallOnDrop<F> {
    fn drop(&mut self) {
        (self.0)();
    }
}
