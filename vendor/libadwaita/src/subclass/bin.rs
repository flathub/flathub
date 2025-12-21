use crate::Bin;
use glib::prelude::*;
use glib::subclass::prelude::*;
use gtk::subclass::prelude::WidgetImpl;

pub trait BinImpl: WidgetImpl + ObjectSubclass<Type: IsA<Bin>> {}

unsafe impl<T: BinImpl> IsSubclassable<T> for Bin {}
