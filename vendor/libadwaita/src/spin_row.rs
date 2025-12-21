// Take a look at the license at the top of the repository in the LICENSE file.

use crate::SpinRow;
use glib::{
    signal::{connect_raw, SignalHandlerId},
    translate::*,
};
use gtk::{glib, prelude::*};
use libc::{c_double, c_int};
use std::{boxed::Box as Box_, mem::transmute};

impl SpinRow {
    pub fn connect_input<F>(&self, f: F) -> SignalHandlerId
    where
        F: Fn(&Self) -> Option<Result<f64, ()>> + 'static,
    {
        unsafe {
            let f: Box_<F> = Box_::new(f);
            connect_raw(
                self.as_ptr() as *mut _,
                c"input".as_ptr() as *const _,
                Some(transmute::<*const (), unsafe extern "C" fn()>(
                    input_trampoline::<F> as *const (),
                )),
                Box_::into_raw(f),
            )
        }
    }
}

unsafe extern "C" fn input_trampoline<F: Fn(&SpinRow) -> Option<Result<f64, ()>> + 'static>(
    this: *mut ffi::AdwSpinRow,
    new_value: *mut c_double,
    f: &F,
) -> c_int {
    unsafe {
        match f(SpinRow::from_glib_borrow(this).unsafe_cast_ref()) {
            Some(Ok(v)) => {
                *new_value = v;
                glib::ffi::GTRUE
            }
            Some(Err(_)) => gtk::ffi::GTK_INPUT_ERROR,
            None => glib::ffi::GFALSE,
        }
    }
}
