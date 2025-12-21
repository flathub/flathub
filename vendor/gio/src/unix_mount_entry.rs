// Take a look at the license at the top of the repository in the LICENSE file.

use std::mem;

use glib::translate::*;

use crate::{ffi, UnixMountEntry};

impl UnixMountEntry {
    #[doc(alias = "g_unix_mounts_get")]
    #[doc(alias = "g_unix_mount_entries_get")]
    #[doc(alias = "get_mounts")]
    pub fn mounts() -> (Vec<UnixMountEntry>, u64) {
        unsafe {
            let mut time_read = mem::MaybeUninit::uninit();
            let ret = FromGlibPtrContainer::from_glib_full(ffi::g_unix_mount_entries_get(
                time_read.as_mut_ptr(),
            ));
            let time_read = time_read.assume_init();
            (ret, time_read)
        }
    }

    #[cfg(feature = "v2_82")]
    #[cfg_attr(docsrs, doc(cfg(feature = "v2_82")))]
    #[doc(alias = "g_unix_mounts_get")]
    #[doc(alias = "g_unix_mount_entries_get")]
    #[doc(alias = "get_mounts")]
    pub fn mounts_from_file(table_path: impl AsRef<std::path::Path>) -> (Vec<UnixMountEntry>, u64) {
        unsafe {
            let mut time_read = mem::MaybeUninit::uninit();
            let mut n_entries_out = mem::MaybeUninit::uninit();
            let ret = ffi::g_unix_mount_entries_get_from_file(
                table_path.as_ref().to_glib_none().0,
                time_read.as_mut_ptr(),
                n_entries_out.as_mut_ptr(),
            );
            let n_entries_out = n_entries_out.assume_init();
            let ret = FromGlibContainer::from_glib_full_num(ret, n_entries_out);
            let time_read = time_read.assume_init();
            (ret, time_read)
        }
    }

    #[doc(alias = "g_unix_mount_get_mount_path")]
    #[deprecated = "Use `mount_path()`"]
    pub fn unix_mount_get_mount_path(&self) -> std::path::PathBuf {
        self.mount_path()
    }

    #[doc(alias = "g_unix_mounts_changed_since")]
    #[doc(alias = "g_unix_mount_entries_changed_since")]
    pub fn is_changed_since(time: u64) -> bool {
        unsafe { from_glib(ffi::g_unix_mount_entries_changed_since(time)) }
    }
}
