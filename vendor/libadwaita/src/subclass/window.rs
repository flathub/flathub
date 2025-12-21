use crate::Window;
use glib::prelude::*;
use gtk::subclass::prelude::*;

pub trait AdwWindowImpl: WindowImpl + ObjectSubclass<Type: IsA<Window>> {}

unsafe impl<T: AdwWindowImpl> IsSubclassable<T> for Window {}
