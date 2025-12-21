use crate::PreferencesGroup;
use glib::prelude::*;
use glib::subclass::prelude::*;
use gtk::subclass::widget::WidgetImpl;

pub trait PreferencesGroupImpl: WidgetImpl + ObjectSubclass<Type: IsA<PreferencesGroup>> {}

unsafe impl<T: PreferencesGroupImpl> IsSubclassable<T> for PreferencesGroup {}
