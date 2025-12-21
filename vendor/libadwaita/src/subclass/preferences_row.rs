use crate::PreferencesRow;
use glib::prelude::*;
use glib::subclass::prelude::*;
use gtk::subclass::list_box_row::ListBoxRowImpl;

pub trait PreferencesRowImpl: ListBoxRowImpl + ObjectSubclass<Type: IsA<PreferencesRow>> {}

unsafe impl<T: PreferencesRowImpl> IsSubclassable<T> for PreferencesRow {}
