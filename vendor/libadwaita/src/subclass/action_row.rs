use glib::subclass::prelude::*;
use glib::translate::*;

use crate::subclass::prelude::PreferencesRowImpl;
use crate::{prelude::*, ActionRow};

pub trait ActionRowImpl: PreferencesRowImpl + ObjectSubclass<Type: IsA<ActionRow>> {
    fn activate(&self) {
        ActionRowImplExt::parent_activate(self)
    }
}
pub trait ActionRowImplExt: ActionRowImpl {
    fn parent_activate(&self) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *mut ffi::AdwActionRowClass;
            if let Some(f) = (*parent_class).activate {
                f(self.obj().unsafe_cast_ref::<ActionRow>().to_glib_none().0)
            }
        }
    }
}

impl<T: ActionRowImpl> ActionRowImplExt for T {}

unsafe impl<T: ActionRowImpl> IsSubclassable<T> for ActionRow {
    fn class_init(class: &mut glib::Class<Self>) {
        Self::parent_class_init::<T>(class);

        let klass = class.as_mut();
        klass.activate = Some(row_activate::<T>);
    }
}

unsafe extern "C" fn row_activate<T: ActionRowImpl>(ptr: *mut ffi::AdwActionRow) {
    let instance = unsafe { &*(ptr as *mut T::Instance) };
    let imp = instance.imp();

    ActionRowImpl::activate(imp)
}
