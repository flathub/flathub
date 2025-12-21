use crate::{prelude::*, NavigationPage};
use glib::subclass::prelude::*;
use glib::translate::*;
use gtk::subclass::prelude::WidgetImpl;

pub trait NavigationPageImpl: WidgetImpl + ObjectSubclass<Type: IsA<NavigationPage>> {
    fn hidden(&self) {
        self.parent_hidden()
    }

    fn hiding(&self) {
        self.parent_hiding();
    }

    fn showing(&self) {
        self.parent_showing();
    }

    fn shown(&self) {
        self.parent_shown();
    }
}

pub trait NavigationPageImplExt: NavigationPageImpl {
    fn parent_hidden(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwNavigationPageClass;
            if let Some(f) = (*parent_class).hidden {
                f(self
                    .obj()
                    .unsafe_cast_ref::<NavigationPage>()
                    .to_glib_none()
                    .0);
            }
        }
    }

    fn parent_hiding(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwNavigationPageClass;
            if let Some(f) = (*parent_class).hiding {
                f(self
                    .obj()
                    .unsafe_cast_ref::<NavigationPage>()
                    .to_glib_none()
                    .0);
            }
        }
    }

    fn parent_showing(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwNavigationPageClass;
            if let Some(f) = (*parent_class).showing {
                f(self
                    .obj()
                    .unsafe_cast_ref::<NavigationPage>()
                    .to_glib_none()
                    .0);
            }
        }
    }

    fn parent_shown(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwNavigationPageClass;
            if let Some(f) = (*parent_class).shown {
                f(self
                    .obj()
                    .unsafe_cast_ref::<NavigationPage>()
                    .to_glib_none()
                    .0);
            }
        }
    }
}

impl<T: NavigationPageImpl> NavigationPageImplExt for T {}

unsafe impl<T: NavigationPageImpl> IsSubclassable<T> for NavigationPage {
    fn class_init(class: &mut glib::Class<Self>) {
        Self::parent_class_init::<T>(class);

        let klass = class.as_mut();
        klass.hidden = Some(navigation_page_hidden::<T>);
        klass.hiding = Some(navigation_page_hiding::<T>);
        klass.showing = Some(navigation_page_showing::<T>);
        klass.shown = Some(navigation_page_shown::<T>);
    }
}

unsafe extern "C" fn navigation_page_hidden<T: NavigationPageImpl>(
    ptr: *mut ffi::AdwNavigationPage,
) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    imp.hidden();
}

unsafe extern "C" fn navigation_page_hiding<T: NavigationPageImpl>(
    ptr: *mut ffi::AdwNavigationPage,
) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    imp.hiding();
}

unsafe extern "C" fn navigation_page_showing<T: NavigationPageImpl>(
    ptr: *mut ffi::AdwNavigationPage,
) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    imp.showing();
}

unsafe extern "C" fn navigation_page_shown<T: NavigationPageImpl>(
    ptr: *mut ffi::AdwNavigationPage,
) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    imp.shown();
}
