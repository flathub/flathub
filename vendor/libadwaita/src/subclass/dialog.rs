use glib::subclass::prelude::*;
use glib::translate::*;

use crate::{prelude::*, Dialog};
use gtk::subclass::prelude::WidgetImpl;

pub trait AdwDialogImpl: WidgetImpl + ObjectSubclass<Type: IsA<Dialog>> {
    fn close_attempt(&self) {
        AdwDialogImplExt::parent_close_attempt(self)
    }

    fn closed(&self) {
        AdwDialogImplExt::parent_closed(self)
    }
}

pub trait AdwDialogImplExt: AdwDialogImpl {
    fn parent_close_attempt(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwDialogClass;
            if let Some(f) = (*parent_class).close_attempt {
                f(self.obj().unsafe_cast_ref::<Dialog>().to_glib_none().0)
            }
        }
    }

    fn parent_closed(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwDialogClass;
            if let Some(f) = (*parent_class).closed {
                f(self.obj().unsafe_cast_ref::<Dialog>().to_glib_none().0)
            }
        }
    }
}

impl<T: AdwDialogImpl> AdwDialogImplExt for T {}

unsafe impl<T: AdwDialogImpl> IsSubclassable<T> for Dialog {
    fn class_init(class: &mut glib::Class<Self>) {
        Self::parent_class_init::<T>(class);

        let klass = class.as_mut();
        klass.close_attempt = Some(close_attempt::<T>);
        klass.closed = Some(closed::<T>);
    }
}

unsafe extern "C" fn close_attempt<T: AdwDialogImpl>(ptr: *mut ffi::AdwDialog) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    AdwDialogImpl::close_attempt(imp)
}

unsafe extern "C" fn closed<T: AdwDialogImpl>(ptr: *mut ffi::AdwDialog) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    AdwDialogImpl::closed(imp)
}
