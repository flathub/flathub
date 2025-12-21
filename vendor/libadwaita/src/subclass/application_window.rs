use crate::ApplicationWindow;
use glib::prelude::*;
use gtk::subclass::prelude::*;

pub trait AdwApplicationWindowImpl:
    ApplicationWindowImpl + ObjectSubclass<Type: IsA<ApplicationWindow>>
{
}

unsafe impl<T: AdwApplicationWindowImpl> IsSubclassable<T> for ApplicationWindow {}
