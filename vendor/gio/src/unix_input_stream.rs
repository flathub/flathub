// Take a look at the license at the top of the repository in the LICENSE file.

#[cfg(unix)]
use std::os::unix::io::{AsFd, AsRawFd, BorrowedFd, IntoRawFd, OwnedFd, RawFd};

use glib::{prelude::*, translate::*};
#[cfg(all(not(unix), docsrs))]
use socket::{AsFd, AsRawFd, BorrowedFd, FromRawFd, IntoRawFd, OwnedFd, RawFd};

use crate::{ffi, InputStream, UnixInputStream};

impl UnixInputStream {
    // rustdoc-stripper-ignore-next
    /// Creates a new [`Self`] that takes ownership of the passed in fd.
    #[doc(alias = "g_unix_input_stream_new")]
    pub fn take_fd(fd: OwnedFd) -> UnixInputStream {
        let fd = fd.into_raw_fd();
        let close_fd = true.into_glib();
        unsafe {
            InputStream::from_glib_full(ffi::g_unix_input_stream_new(fd, close_fd)).unsafe_cast()
        }
    }

    // rustdoc-stripper-ignore-next
    /// Creates a new [`Self`] that does not take ownership of the passed in fd.
    ///
    /// # Safety
    /// You may only close the fd if all references to this stream have been dropped.
    #[doc(alias = "g_unix_input_stream_new")]
    pub unsafe fn with_fd<T: AsRawFd>(fd: T) -> UnixInputStream {
        let fd = fd.as_raw_fd();
        let close_fd = false.into_glib();
        InputStream::from_glib_full(ffi::g_unix_input_stream_new(fd, close_fd)).unsafe_cast()
    }
}

impl AsRawFd for UnixInputStream {
    fn as_raw_fd(&self) -> RawFd {
        unsafe { ffi::g_unix_input_stream_get_fd(self.to_glib_none().0) as _ }
    }
}

impl AsFd for UnixInputStream {
    fn as_fd(&self) -> BorrowedFd<'_> {
        unsafe {
            let raw_fd = self.as_raw_fd();
            BorrowedFd::borrow_raw(raw_fd)
        }
    }
}

pub trait UnixInputStreamExtManual: IsA<UnixInputStream> + Sized {
    // rustdoc-stripper-ignore-next
    /// Sets whether the fd of this stream will be closed when the stream is closed.
    ///
    /// # Safety
    /// If you pass in `false` as the parameter, you may only close the fd if the all references
    /// to the stream have been dropped. If you pass in `true`, you must never call close.
    #[doc(alias = "g_unix_input_stream_set_close_fd")]
    unsafe fn set_close_fd(&self, close_fd: bool) {
        ffi::g_unix_input_stream_set_close_fd(self.as_ref().to_glib_none().0, close_fd.into_glib());
    }
}

impl<O: IsA<UnixInputStream>> UnixInputStreamExtManual for O {}
