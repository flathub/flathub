use crate::PreferencesPage;
use glib::prelude::*;
use glib::subclass::prelude::*;
use gtk::subclass::widget::WidgetImpl;

pub trait PreferencesPageImpl: WidgetImpl + ObjectSubclass<Type: IsA<PreferencesPage>> {}

unsafe impl<T: PreferencesPageImpl> IsSubclassable<T> for PreferencesPage {}
