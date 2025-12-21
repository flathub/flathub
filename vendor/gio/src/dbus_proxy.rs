// Take a look at the license at the top of the repository in the LICENSE file.

use std::{boxed::Box as Box_, mem::transmute};

use glib::{prelude::*, signal::connect_raw, translate::*, SignalHandlerId};

use crate::{ffi, DBusProxy};

pub trait DBusProxyExtManual: IsA<DBusProxy> + 'static {
    #[cfg(feature = "v2_72")]
    #[doc(alias = "g-signal")]
    fn connect_g_signal<
        F: Fn(&Self, Option<&str>, &str, &glib::Variant) + Send + Sync + 'static,
    >(
        &self,
        detail: Option<&str>,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn g_signal_trampoline<
            P: IsA<DBusProxy>,
            F: Fn(&P, Option<&str>, &str, &glib::Variant) + Send + Sync + 'static,
        >(
            this: *mut ffi::GDBusProxy,
            sender_name: *mut libc::c_char,
            signal_name: *mut libc::c_char,
            parameters: *mut glib::ffi::GVariant,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                DBusProxy::from_glib_borrow(this).unsafe_cast_ref(),
                Option::<glib::GString>::from_glib_borrow(sender_name)
                    .as_ref()
                    .as_deref(),
                &glib::GString::from_glib_borrow(signal_name),
                &from_glib_borrow(parameters),
            )
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            let detailed_signal_name = detail.map(|name| format!("g-signal::{name}\0"));
            let signal_name: &[u8] = detailed_signal_name
                .as_ref()
                .map_or(&b"g-signal\0"[..], |n| n.as_bytes());
            connect_raw(
                self.as_ptr() as *mut _,
                signal_name.as_ptr() as *const _,
                Some(transmute::<*const (), unsafe extern "C" fn()>(
                    g_signal_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }

    #[cfg(not(feature = "v2_72"))]
    #[doc(alias = "g-signal")]
    fn connect_g_signal<
        F: Fn(&Self, Option<&str>, &str, &glib::Variant) + Send + Sync + 'static,
    >(
        &self,
        f: F,
    ) -> SignalHandlerId {
        unsafe extern "C" fn g_signal_trampoline<
            P: IsA<DBusProxy>,
            F: Fn(&P, Option<&str>, &str, &glib::Variant) + Send + Sync + 'static,
        >(
            this: *mut ffi::GDBusProxy,
            sender_name: *mut libc::c_char,
            signal_name: *mut libc::c_char,
            parameters: *mut glib::ffi::GVariant,
            f: glib::ffi::gpointer,
        ) {
            let f: &F = &*(f as *const F);
            f(
                DBusProxy::from_glib_borrow(this).unsafe_cast_ref(),
                Option::<glib::GString>::from_glib_borrow(sender_name)
                    .as_ref()
                    .as_deref(),
                &glib::GString::from_glib_borrow(signal_name),
                &from_glib_borrow(parameters),
            )
        }
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                b"g-signal\0".as_ptr() as *const _,
                Some(transmute::<*const (), unsafe extern "C" fn()>(
                    g_signal_trampoline::<Self, F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }
}

impl<O: IsA<DBusProxy>> DBusProxyExtManual for O {}
