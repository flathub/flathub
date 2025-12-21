use glib::subclass::prelude::*;
use glib::translate::*;

use super::dialog::AdwDialogImpl;
use crate::{prelude::*, AlertDialog};

pub trait AdwAlertDialogImpl: AdwDialogImpl + ObjectSubclass<Type: IsA<AlertDialog>> {
    fn response(&self, response: &str) {
        AdwAlertDialogImplExt::parent_response(self, response)
    }
}

pub trait AdwAlertDialogImplExt: AdwAlertDialogImpl {
    fn parent_response(&self, response: &str) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwAlertDialogClass;
            if let Some(f) = (*parent_class).response {
                f(
                    self.obj().unsafe_cast_ref::<AlertDialog>().to_glib_none().0,
                    response.to_glib_none().0,
                )
            }
        }
    }
}

impl<T: AdwAlertDialogImpl> AdwAlertDialogImplExt for T {}

unsafe impl<T: AdwAlertDialogImpl> IsSubclassable<T> for AlertDialog {
    fn class_init(class: &mut glib::Class<Self>) {
        Self::parent_class_init::<T>(class);

        let klass = class.as_mut();
        klass.response = Some(response::<T>);
    }
}

unsafe extern "C" fn response<T: AdwAlertDialogImpl>(
    ptr: *mut ffi::AdwAlertDialog,
    response: *const libc::c_char,
) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    let response: Borrowed<glib::GString> = unsafe { from_glib_borrow(response) };

    imp.response(response.as_ref())
}
