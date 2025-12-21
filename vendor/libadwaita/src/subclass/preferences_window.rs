use super::window::AdwWindowImpl;
use crate::PreferencesWindow;
use glib::prelude::*;
use glib::subclass::prelude::*;

pub trait PreferencesWindowImpl:
    AdwWindowImpl + ObjectSubclass<Type: IsA<PreferencesWindow>>
{
}

unsafe impl<T: PreferencesWindowImpl> IsSubclassable<T> for PreferencesWindow {}
