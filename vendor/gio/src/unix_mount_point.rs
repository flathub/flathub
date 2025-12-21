// Take a look at the license at the top of the repository in the LICENSE file.

use std::mem;

use glib::translate::*;

use crate::{ffi, UnixMountPoint};

impl UnixMountPoint {
    #[cfg(unix)]
    #[doc(alias = "g_unix_mount_points_get")]
    #[doc(alias = "get_mount_points")]
    pub fn mount_points() -> (Vec<UnixMountPoint>, u64) {
        unsafe {
            let mut time_read = mem::MaybeUninit::uninit();
            let ret = FromGlibPtrContainer::from_glib_full(ffi::g_unix_mount_points_get(
                time_read.as_mut_ptr(),
            ));
            let time_read = time_read.assume_init();
            (ret, time_read)
        }
    }

    #[doc(alias = "g_unix_mount_points_changed_since")]
    pub fn is_changed_since(time: u64) -> bool {
        unsafe { from_glib(ffi::g_unix_mount_points_changed_since(time)) }
    }
}
