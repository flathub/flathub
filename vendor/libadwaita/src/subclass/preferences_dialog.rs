use super::dialog::AdwDialogImpl;
use crate::PreferencesDialog;
use glib::prelude::*;
use glib::subclass::prelude::*;

pub trait PreferencesDialogImpl:
    AdwDialogImpl + ObjectSubclass<Type: IsA<PreferencesDialog>>
{
}

unsafe impl<T: PreferencesDialogImpl> IsSubclassable<T> for PreferencesDialog {}
