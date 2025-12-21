use glib::prelude::*;
use glib::subclass::prelude::*;

use crate::subclass::prelude::PreferencesRowImpl;
use crate::EntryRow;

pub trait EntryRowImpl: PreferencesRowImpl + ObjectSubclass<Type: IsA<EntryRow>> {}

unsafe impl<T: EntryRowImpl> IsSubclassable<T> for EntryRow {}
