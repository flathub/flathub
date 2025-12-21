use super::preferences_row::PreferencesRowImpl;
use crate::ExpanderRow;
use glib::prelude::*;
use glib::subclass::prelude::*;

pub trait ExpanderRowImpl: PreferencesRowImpl + ObjectSubclass<Type: IsA<ExpanderRow>> {}

unsafe impl<T: ExpanderRowImpl> IsSubclassable<T> for ExpanderRow {}
