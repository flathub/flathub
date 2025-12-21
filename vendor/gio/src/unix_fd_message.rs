// Take a look at the license at the top of the repository in the LICENSE file.

#[cfg(unix)]
use std::os::unix::io::{AsRawFd, RawFd};
use std::{mem, ptr};

use glib::{prelude::*, translate::*};
#[cfg(all(not(unix), docsrs))]
use socket::{AsRawFd, RawFd};

use crate::{ffi, UnixFDMessage};

pub trait UnixFDMessageExtManual: IsA<UnixFDMessage> + Sized {
    #[doc(alias = "g_unix_fd_message_append_fd")]
    fn append_fd<T: AsRawFd>(&self, fd: T) -> Result<(), glib::Error> {
        unsafe {
            let mut error = ptr::null_mut();
            ffi::g_unix_fd_message_append_fd(
                self.as_ref().to_glib_none().0,
                fd.as_raw_fd(),
                &mut error,
            );
            if error.is_null() {
                Ok(())
            } else {
                Err(from_glib_full(error))
            }
        }
    }
    #[doc(alias = "g_unix_fd_message_steal_fds")]
    fn steal_fds(&self) -> Vec<RawFd> {
        unsafe {
            let mut length = mem::MaybeUninit::uninit();
            let ret = FromGlibContainer::from_glib_full_num(
                ffi::g_unix_fd_message_steal_fds(
                    self.as_ref().to_glib_none().0,
                    length.as_mut_ptr(),
                ),
                length.assume_init() as usize,
            );
            ret
        }
    }
}

impl<O: IsA<UnixFDMessage>> UnixFDMessageExtManual for O {}
