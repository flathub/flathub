// Take a look at the license at the top of the repository in the LICENSE file.

#[cfg(all(unix, feature = "v2_58"))]
use std::boxed::Box as Box_;
#[cfg(all(unix, feature = "v2_58"))]
use std::os::unix::io::{AsFd, AsRawFd};
#[cfg(all(unix, feature = "v2_58"))]
use std::ptr;

#[cfg(all(feature = "v2_58", unix))]
use glib::{prelude::*, Error};
use glib::{translate::*, GString};

#[cfg(all(feature = "v2_58", unix))]
use crate::AppLaunchContext;
use crate::{ffi, DesktopAppInfo};

impl DesktopAppInfo {
    #[doc(alias = "g_desktop_app_info_search")]
    pub fn search(search_string: &str) -> Vec<Vec<GString>> {
        unsafe {
            let out = ffi::g_desktop_app_info_search(search_string.to_glib_none().0);

            if out.is_null() {
                return Vec::new();
            }

            let mut ret = Vec::new();
            let mut it = 0;
            loop {
                let tmp: *mut *mut libc::c_char = *out.offset(it);

                if tmp.is_null() {
                    break;
                }
                let v: Vec<GString> = FromGlibPtrContainer::from_glib_full(tmp);
                ret.push(v);
                it += 1;
            }

            glib::ffi::g_free(out as *mut libc::c_void);
            ret
        }
    }
}

#[cfg(all(feature = "v2_58", unix))]
pub trait DesktopAppInfoExtManual: IsA<DesktopAppInfo> {
    #[cfg_attr(docsrs, doc(cfg(all(feature = "v2_58", unix))))]
    #[doc(alias = "g_desktop_app_info_launch_uris_as_manager_with_fds")]
    fn launch_uris_as_manager_with_fds<P: IsA<AppLaunchContext>>(
        &self,
        uris: &[&str],
        launch_context: Option<&P>,
        spawn_flags: glib::SpawnFlags,
        user_setup: Option<Box_<dyn FnOnce() + 'static>>,
        pid_callback: Option<&mut dyn FnMut(&DesktopAppInfo, glib::Pid)>,
        stdin_fd: Option<impl AsFd>,
        stdout_fd: Option<impl AsFd>,
        stderr_fd: Option<impl AsFd>,
    ) -> Result<(), Error> {
        let user_setup_data: Box_<Option<Box_<dyn FnOnce() + 'static>>> = Box_::new(user_setup);
        unsafe extern "C" fn user_setup_func(user_data: glib::ffi::gpointer) {
            let callback: Box_<Option<Box_<dyn FnOnce() + 'static>>> =
                Box_::from_raw(user_data as *mut _);
            let callback = (*callback).expect("cannot get closure...");
            callback()
        }
        let user_setup = if user_setup_data.is_some() {
            Some(user_setup_func as _)
        } else {
            None
        };
        let pid_callback_data: Option<&mut dyn FnMut(&DesktopAppInfo, glib::Pid)> = pid_callback;
        unsafe extern "C" fn pid_callback_func(
            appinfo: *mut ffi::GDesktopAppInfo,
            pid: glib::ffi::GPid,
            user_data: glib::ffi::gpointer,
        ) {
            let appinfo = from_glib_borrow(appinfo);
            let pid = from_glib(pid);
            let callback = user_data as *mut Option<&mut dyn FnMut(&DesktopAppInfo, glib::Pid)>;
            if let Some(ref mut callback) = *callback {
                callback(&appinfo, pid)
            } else {
                panic!("cannot get closure...")
            };
        }
        let pid_callback = if pid_callback_data.is_some() {
            Some(pid_callback_func as _)
        } else {
            None
        };
        let super_callback0: Box_<Option<Box_<dyn FnOnce() + 'static>>> = user_setup_data;
        let super_callback1: &Option<&mut dyn FnMut(&DesktopAppInfo, glib::Pid)> =
            &pid_callback_data;

        let stdin_raw_fd = stdin_fd.map_or(-1, |fd| fd.as_fd().as_raw_fd());
        let stdout_raw_fd = stdout_fd.map_or(-1, |fd| fd.as_fd().as_raw_fd());
        let stderr_raw_fd = stderr_fd.map_or(-1, |fd| fd.as_fd().as_raw_fd());
        unsafe {
            let mut error = ptr::null_mut();
            let _ = ffi::g_desktop_app_info_launch_uris_as_manager_with_fds(
                self.as_ref().to_glib_none().0,
                uris.to_glib_none().0,
                launch_context.map(|p| p.as_ref()).to_glib_none().0,
                spawn_flags.into_glib(),
                user_setup,
                Box_::into_raw(super_callback0) as *mut _,
                pid_callback,
                super_callback1 as *const _ as *mut _,
                stdin_raw_fd,
                stdout_raw_fd,
                stderr_raw_fd,
                &mut error,
            );
            if error.is_null() {
                Ok(())
            } else {
                Err(from_glib_full(error))
            }
        }
    }
}

#[cfg(all(feature = "v2_58", unix))]
impl<O: IsA<DesktopAppInfo>> DesktopAppInfoExtManual for O {}
