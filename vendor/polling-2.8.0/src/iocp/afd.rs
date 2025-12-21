//! Safe wrapper around \Device\Afd

use super::port::{Completion, CompletionHandle};

use std::cell::UnsafeCell;
use std::fmt;
use std::io;
use std::marker::{PhantomData, PhantomPinned};
use std::mem::{size_of, transmute, MaybeUninit};
use std::os::windows::prelude::{AsRawHandle, RawHandle, RawSocket};
use std::pin::Pin;
use std::ptr;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Once;

use windows_sys::Win32::Foundation::{
    CloseHandle, HANDLE, HMODULE, NTSTATUS, STATUS_NOT_FOUND, STATUS_PENDING, STATUS_SUCCESS,
    UNICODE_STRING,
};
use windows_sys::Win32::Networking::WinSock::{
    WSAIoctl, SIO_BASE_HANDLE, SIO_BSP_HANDLE_POLL, SOCKET_ERROR,
};
use windows_sys::Win32::Storage::FileSystem::{
    FILE_OPEN, FILE_SHARE_READ, FILE_SHARE_WRITE, SYNCHRONIZE,
};
use windows_sys::Win32::System::LibraryLoader::{GetModuleHandleW, GetProcAddress};
use windows_sys::Win32::System::WindowsProgramming::{IO_STATUS_BLOCK, OBJECT_ATTRIBUTES};

#[derive(Default)]
#[repr(C)]
pub(super) struct AfdPollInfo {
    /// The timeout for this poll.
    timeout: i64,

    /// The number of handles being polled.
    handle_count: u32,

    /// Whether or not this poll is exclusive for this handle.
    exclusive: u32,

    /// The handles to poll.
    handles: [AfdPollHandleInfo; 1],
}

#[derive(Default)]
#[repr(C)]
struct AfdPollHandleInfo {
    /// The handle to poll.
    handle: HANDLE,

    /// The events to poll for.
    events: AfdPollMask,

    /// The status of the poll.
    status: NTSTATUS,
}

impl AfdPollInfo {
    pub(super) fn handle_count(&self) -> u32 {
        self.handle_count
    }

    pub(super) fn events(&self) -> AfdPollMask {
        self.handles[0].events
    }
}

bitflags::bitflags! {
    #[derive(Default)]
    #[repr(transparent)]
    pub(super) struct AfdPollMask: u32 {
        const RECEIVE = 0x001;
        const RECEIVE_EXPEDITED = 0x002;
        const SEND = 0x004;
        const DISCONNECT = 0x008;
        const ABORT = 0x010;
        const LOCAL_CLOSE = 0x020;
        const ACCEPT = 0x080;
        const CONNECT_FAIL = 0x100;
    }
}

pub(super) trait HasAfdInfo {
    fn afd_info(self: Pin<&Self>) -> Pin<&UnsafeCell<AfdPollInfo>>;
}

macro_rules! define_ntdll_import {
    (
        $(
            $(#[$attr:meta])*
            fn $name:ident($($arg:ident: $arg_ty:ty),*) -> $ret:ty;
        )*
    ) => {
        /// Imported functions from ntdll.dll.
        #[allow(non_snake_case)]
        pub(super) struct NtdllImports {
            $(
                $(#[$attr])*
                $name: unsafe extern "system" fn($($arg_ty),*) -> $ret,
            )*
        }

        #[allow(non_snake_case)]
        impl NtdllImports {
            unsafe fn load(ntdll: HMODULE) -> io::Result<Self> {
                $(
                    let $name = {
                        const NAME: &str = concat!(stringify!($name), "\0");
                        let addr = GetProcAddress(ntdll, NAME.as_ptr() as *const _);

                        let addr = match addr {
                            Some(addr) => addr,
                            None => {
                                log::error!("Failed to load ntdll function {}", NAME);
                                return Err(io::Error::last_os_error());
                            },
                        };

                        transmute::<_, unsafe extern "system" fn($($arg_ty),*) -> $ret>(addr)
                    };
                )*

                Ok(Self {
                    $(
                        $name,
                    )*
                })
            }

            $(
                $(#[$attr])*
                unsafe fn $name(&self, $($arg: $arg_ty),*) -> $ret {
                    (self.$name)($($arg),*)
                }
            )*
        }
    };
}

define_ntdll_import! {
    /// Cancels an ongoing I/O operation.
    fn NtCancelIoFileEx(
        FileHandle: HANDLE,
        IoRequestToCancel: *mut IO_STATUS_BLOCK,
        IoStatusBlock: *mut IO_STATUS_BLOCK
    ) -> NTSTATUS;

    /// Opens or creates a file handle.
    #[allow(clippy::too_many_arguments)]
    fn NtCreateFile(
        FileHandle: *mut HANDLE,
        DesiredAccess: u32,
        ObjectAttributes: *mut OBJECT_ATTRIBUTES,
        IoStatusBlock: *mut IO_STATUS_BLOCK,
        AllocationSize: *mut i64,
        FileAttributes: u32,
        ShareAccess: u32,
        CreateDisposition: u32,
        CreateOptions: u32,
        EaBuffer: *mut (),
        EaLength: u32
    ) -> NTSTATUS;

    /// Runs an I/O control on a file handle.
    ///
    /// Practically equivalent to `ioctl`.
    #[allow(clippy::too_many_arguments)]
    fn NtDeviceIoControlFile(
        FileHandle: HANDLE,
        Event: HANDLE,
        ApcRoutine: *mut (),
        ApcContext: *mut (),
        IoStatusBlock: *mut IO_STATUS_BLOCK,
        IoControlCode: u32,
        InputBuffer: *mut (),
        InputBufferLength: u32,
        OutputBuffer: *mut (),
        OutputBufferLength: u32
    ) -> NTSTATUS;

    /// Converts `NTSTATUS` to a DOS error code.
    fn RtlNtStatusToDosError(
        Status: NTSTATUS
    ) -> u32;
}

impl NtdllImports {
    fn get() -> io::Result<&'static Self> {
        macro_rules! s {
            ($e:expr) => {{
                $e as u16
            }};
        }

        // ntdll.dll
        static NTDLL_NAME: &[u16] = &[
            s!('n'),
            s!('t'),
            s!('d'),
            s!('l'),
            s!('l'),
            s!('.'),
            s!('d'),
            s!('l'),
            s!('l'),
            s!('\0'),
        ];
        static NTDLL_IMPORTS: OnceCell<io::Result<NtdllImports>> = OnceCell::new();

        NTDLL_IMPORTS
            .get_or_init(|| unsafe {
                let ntdll = GetModuleHandleW(NTDLL_NAME.as_ptr() as *const _);

                if ntdll == 0 {
                    log::error!("Failed to load ntdll.dll");
                    return Err(io::Error::last_os_error());
                }

                NtdllImports::load(ntdll)
            })
            .as_ref()
            .map_err(|e| io::Error::from(e.kind()))
    }

    pub(super) fn force_load() -> io::Result<()> {
        Self::get()?;
        Ok(())
    }
}

/// The handle to the AFD device.
pub(super) struct Afd<T> {
    /// The handle to the AFD device.
    handle: HANDLE,

    /// We own `T`.
    _marker: PhantomData<T>,
}

impl<T> fmt::Debug for Afd<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        struct WriteAsHex(HANDLE);

        impl fmt::Debug for WriteAsHex {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                write!(f, "{:010x}", self.0)
            }
        }

        f.debug_struct("Afd")
            .field("handle", &WriteAsHex(self.handle))
            .finish()
    }
}

impl<T> Drop for Afd<T> {
    fn drop(&mut self) {
        unsafe {
            CloseHandle(self.handle);
        }
    }
}

impl<T> AsRawHandle for Afd<T> {
    fn as_raw_handle(&self) -> RawHandle {
        self.handle as _
    }
}

impl<T: CompletionHandle> Afd<T>
where
    T::Completion: AsIoStatusBlock + HasAfdInfo,
{
    /// Create a new AFD device.
    pub(super) fn new() -> io::Result<Self> {
        macro_rules! s {
            ($e:expr) => {
                ($e) as u16
            };
        }

        /// \Device\Afd\Smol
        const AFD_NAME: &[u16] = &[
            s!('\\'),
            s!('D'),
            s!('e'),
            s!('v'),
            s!('i'),
            s!('c'),
            s!('e'),
            s!('\\'),
            s!('A'),
            s!('f'),
            s!('d'),
            s!('\\'),
            s!('S'),
            s!('m'),
            s!('o'),
            s!('l'),
            s!('\0'),
        ];

        // Set up device attributes.
        let mut device_name = UNICODE_STRING {
            Length: (AFD_NAME.len() * size_of::<u16>()) as u16,
            MaximumLength: (AFD_NAME.len() * size_of::<u16>()) as u16,
            Buffer: AFD_NAME.as_ptr() as *mut _,
        };
        let mut device_attributes = OBJECT_ATTRIBUTES {
            Length: size_of::<OBJECT_ATTRIBUTES>() as u32,
            RootDirectory: 0,
            ObjectName: &mut device_name,
            Attributes: 0,
            SecurityDescriptor: ptr::null_mut(),
            SecurityQualityOfService: ptr::null_mut(),
        };

        let mut handle = MaybeUninit::<HANDLE>::uninit();
        let mut iosb = MaybeUninit::<IO_STATUS_BLOCK>::zeroed();
        let ntdll = NtdllImports::get()?;

        let result = unsafe {
            ntdll.NtCreateFile(
                handle.as_mut_ptr(),
                SYNCHRONIZE,
                &mut device_attributes,
                iosb.as_mut_ptr(),
                ptr::null_mut(),
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                0,
                ptr::null_mut(),
                0,
            )
        };

        if result != STATUS_SUCCESS {
            let real_code = unsafe { ntdll.RtlNtStatusToDosError(result) };

            return Err(io::Error::from_raw_os_error(real_code as i32));
        }

        let handle = unsafe { handle.assume_init() };

        Ok(Self {
            handle,
            _marker: PhantomData,
        })
    }

    /// Begin polling with the provided handle.
    pub(super) fn poll(
        &self,
        packet: T,
        base_socket: RawSocket,
        afd_events: AfdPollMask,
    ) -> io::Result<()> {
        const IOCTL_AFD_POLL: u32 = 0x00012024;

        // Lock the packet.
        if !packet.get().try_lock() {
            return Err(io::Error::new(
                io::ErrorKind::WouldBlock,
                "packet is already in use",
            ));
        }

        // Set up the AFD poll info.
        let poll_info = unsafe {
            let poll_info = Pin::into_inner_unchecked(packet.get().afd_info()).get();

            // Initialize the AFD poll info.
            (*poll_info).exclusive = false.into();
            (*poll_info).handle_count = 1;
            (*poll_info).timeout = std::i64::MAX;
            (*poll_info).handles[0].handle = base_socket as HANDLE;
            (*poll_info).handles[0].status = 0;
            (*poll_info).handles[0].events = afd_events;

            poll_info
        };

        let iosb = T::into_ptr(packet).cast::<IO_STATUS_BLOCK>();
        // Set Status to pending
        unsafe {
            (*iosb).Anonymous.Status = STATUS_PENDING;
        }

        let ntdll = NtdllImports::get()?;
        let result = unsafe {
            ntdll.NtDeviceIoControlFile(
                self.handle,
                0,
                ptr::null_mut(),
                iosb.cast(),
                iosb.cast(),
                IOCTL_AFD_POLL,
                poll_info.cast(),
                size_of::<AfdPollInfo>() as u32,
                poll_info.cast(),
                size_of::<AfdPollInfo>() as u32,
            )
        };

        match result {
            STATUS_SUCCESS => Ok(()),
            STATUS_PENDING => Err(io::ErrorKind::WouldBlock.into()),
            status => {
                let real_code = unsafe { ntdll.RtlNtStatusToDosError(status) };

                Err(io::Error::from_raw_os_error(real_code as i32))
            }
        }
    }

    /// Cancel an ongoing poll operation.
    ///
    /// # Safety
    ///
    /// The poll operation must currently be in progress for this AFD.
    pub(super) unsafe fn cancel(&self, packet: &T) -> io::Result<()> {
        let ntdll = NtdllImports::get()?;

        let result = {
            // First, check if the packet is still in use.
            let iosb = packet.as_ptr().cast::<IO_STATUS_BLOCK>();

            if (*iosb).Anonymous.Status != STATUS_PENDING {
                return Ok(());
            }

            // Cancel the packet.
            let mut cancel_iosb = MaybeUninit::<IO_STATUS_BLOCK>::zeroed();

            ntdll.NtCancelIoFileEx(self.handle, iosb, cancel_iosb.as_mut_ptr())
        };

        if result == STATUS_SUCCESS || result == STATUS_NOT_FOUND {
            Ok(())
        } else {
            let real_code = ntdll.RtlNtStatusToDosError(result);

            Err(io::Error::from_raw_os_error(real_code as i32))
        }
    }
}

/// A one-time initialization cell.
struct OnceCell<T> {
    /// The value.
    value: UnsafeCell<MaybeUninit<T>>,

    /// The one-time initialization.
    once: Once,
}

unsafe impl<T: Send + Sync> Send for OnceCell<T> {}
unsafe impl<T: Send + Sync> Sync for OnceCell<T> {}

impl<T> OnceCell<T> {
    /// Creates a new `OnceCell`.
    pub const fn new() -> Self {
        OnceCell {
            value: UnsafeCell::new(MaybeUninit::uninit()),
            once: Once::new(),
        }
    }

    /// Gets the value or initializes it.
    pub fn get_or_init<F>(&self, f: F) -> &T
    where
        F: FnOnce() -> T,
    {
        self.once.call_once(|| unsafe {
            let value = f();
            *self.value.get() = MaybeUninit::new(value);
        });

        unsafe { &*self.value.get().cast() }
    }
}

pin_project_lite::pin_project! {
    /// An I/O status block paired with some auxillary data.
    #[repr(C)]
    pub(super) struct IoStatusBlock<T> {
        // The I/O status block.
        iosb: UnsafeCell<IO_STATUS_BLOCK>,

        // Whether or not the block is in use.
        in_use: AtomicBool,

        // The auxillary data.
        #[pin]
        data: T,

        // This block is not allowed to move.
        #[pin]
        _marker: PhantomPinned,
    }
}

impl<T: fmt::Debug> fmt::Debug for IoStatusBlock<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("IoStatusBlock")
            .field("iosb", &"..")
            .field("in_use", &self.in_use)
            .field("data", &self.data)
            .finish()
    }
}

impl<T> From<T> for IoStatusBlock<T> {
    fn from(data: T) -> Self {
        Self {
            iosb: UnsafeCell::new(unsafe { std::mem::zeroed() }),
            in_use: AtomicBool::new(false),
            data,
            _marker: PhantomPinned,
        }
    }
}

impl<T> IoStatusBlock<T> {
    pub(super) fn iosb(self: Pin<&Self>) -> &UnsafeCell<IO_STATUS_BLOCK> {
        self.project_ref().iosb
    }

    pub(super) fn data(self: Pin<&Self>) -> Pin<&T> {
        self.project_ref().data
    }
}

impl<T: HasAfdInfo> HasAfdInfo for IoStatusBlock<T> {
    fn afd_info(self: Pin<&Self>) -> Pin<&UnsafeCell<AfdPollInfo>> {
        self.project_ref().data.afd_info()
    }
}

/// Can be transmuted to an I/O status block.
///
/// # Safety
///
/// A pointer to `T` must be able to be converted to a pointer to `IO_STATUS_BLOCK`
/// without any issues.
pub(super) unsafe trait AsIoStatusBlock {}

unsafe impl<T> AsIoStatusBlock for IoStatusBlock<T> {}
unsafe impl<T> Completion for IoStatusBlock<T> {
    fn try_lock(self: Pin<&Self>) -> bool {
        !self.in_use.swap(true, Ordering::SeqCst)
    }

    unsafe fn unlock(self: Pin<&Self>) {
        self.in_use.store(false, Ordering::SeqCst);
    }
}

/// Get the base socket associated with a socket.
pub(super) fn base_socket(sock: RawSocket) -> io::Result<RawSocket> {
    // First, try the SIO_BASE_HANDLE ioctl.
    let result = unsafe { try_socket_ioctl(sock, SIO_BASE_HANDLE) };

    match result {
        Ok(sock) => return Ok(sock),
        Err(e) if e.kind() == io::ErrorKind::InvalidInput => return Err(e),
        Err(_) => {}
    }

    // Some poorly coded LSPs may not handle SIO_BASE_HANDLE properly, but in some cases may
    // handle SIO_BSP_HANDLE_POLL better. Try that.
    let result = unsafe { try_socket_ioctl(sock, SIO_BSP_HANDLE_POLL)? };
    if result == sock {
        return Err(io::Error::from(io::ErrorKind::InvalidInput));
    }

    // Try `SIO_BASE_HANDLE` again, in case the LSP fixed itself.
    unsafe { try_socket_ioctl(result, SIO_BASE_HANDLE) }
}

/// Run an IOCTL on a socket and return a socket.
///
/// # Safety
///
/// The `ioctl` parameter must be a valid I/O control that returns a valid socket.
unsafe fn try_socket_ioctl(sock: RawSocket, ioctl: u32) -> io::Result<RawSocket> {
    let mut out = MaybeUninit::<RawSocket>::uninit();
    let mut bytes = 0u32;

    let result = WSAIoctl(
        sock as _,
        ioctl,
        ptr::null_mut(),
        0,
        out.as_mut_ptr().cast(),
        size_of::<RawSocket>() as u32,
        &mut bytes,
        ptr::null_mut(),
        None,
    );

    if result == SOCKET_ERROR {
        return Err(io::Error::last_os_error());
    }

    Ok(out.assume_init())
}
