use crate::Application;

use glib::prelude::*;
use gtk::subclass::prelude::*;

pub trait AdwApplicationImpl: GtkApplicationImpl + ObjectSubclass<Type: IsA<Application>> {}

unsafe impl<T: AdwApplicationImpl> IsSubclassable<T> for Application {}
