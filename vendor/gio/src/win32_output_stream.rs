// Take a look at the license at the top of the repository in the LICENSE file.

use std::os::windows::io::{AsRawHandle, FromRawHandle, IntoRawHandle, RawHandle};

use glib::{prelude::*, translate::*};

use crate::{ffi, OutputStream};

glib::wrapper! {
    pub struct Win32OutputStream(Object<ffi::GWin32OutputStream, ffi::GWin32OutputStreamClass>) @extends OutputStream;

    match fn {
        type_ => || ffi::g_win32_output_stream_get_type(),
    }
}

impl Win32OutputStream {
    pub const NONE: Option<&'static Win32OutputStream> = None;

    // rustdoc-stripper-ignore-next
    /// Creates a new [`Self`] that takes ownership of the passed in handle.
    ///
    /// # Safety
    /// You must not close the handle unless you've previously called [`Win32OutputStreamExtManual::set_close_handle`]
    /// with `true` on this stream. At which point you may only do so when all references to this
    /// stream have been dropped.
    #[doc(alias = "g_win32_output_stream_new")]
    pub unsafe fn take_handle(handle: impl IntoRawHandle) -> Win32OutputStream {
        let handle = handle.into_raw_handle();
        let close_handle = true.into_glib();
        OutputStream::from_glib_full(ffi::g_win32_output_stream_new(handle, close_handle))
            .unsafe_cast()
    }

    // rustdoc-stripper-ignore-next
    /// Creates a new [`Self`] that does not take ownership of the passed in handle.
    ///
    /// # Safety
    /// You may only close the handle if all references to this stream have been dropped.
    #[doc(alias = "g_win32_output_stream_new")]
    pub unsafe fn with_handle<T: AsRawHandle>(handle: T) -> Win32OutputStream {
        let handle = handle.as_raw_handle();
        let close_handle = false.into_glib();
        OutputStream::from_glib_full(ffi::g_win32_output_stream_new(handle, close_handle))
            .unsafe_cast()
    }
}

impl AsRawHandle for Win32OutputStream {
    fn as_raw_handle(&self) -> RawHandle {
        unsafe { ffi::g_win32_output_stream_get_handle(self.to_glib_none().0) as _ }
    }
}

pub trait Win32OutputStreamExt: IsA<Win32OutputStream> + Sized {
    #[doc(alias = "g_win32_output_stream_get_close_handle")]
    #[doc(alias = "get_close_handle")]
    fn closes_handle(&self) -> bool {
        unsafe {
            from_glib(ffi::g_win32_output_stream_get_close_handle(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    #[doc(alias = "g_win32_output_stream_get_handle")]
    #[doc(alias = "get_handle")]
    fn handle<T: FromRawHandle>(&self) -> T {
        unsafe {
            T::from_raw_handle(ffi::g_win32_output_stream_get_handle(
                self.as_ref().to_glib_none().0,
            ))
        }
    }

    // rustdoc-stripper-ignore-next
    /// Sets whether the handle of this stream will be closed when the stream is closed.
    ///
    /// # Safety
    /// If you pass in `false` as the parameter, you may only close the handle if the all references
    /// to the stream have been dropped. If you pass in `true`, you must never call close.
    #[doc(alias = "g_win32_output_stream_set_close_handle")]
    unsafe fn set_close_handle(&self, close_handle: bool) {
        ffi::g_win32_output_stream_set_close_handle(
            self.as_ref().to_glib_none().0,
            close_handle.into_glib(),
        );
    }
}

impl<O: IsA<Win32OutputStream>> Win32OutputStreamExt for O {}
