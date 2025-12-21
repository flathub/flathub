use super::action_row::ActionRowImpl;
use crate::ComboRow;
use glib::prelude::*;
use glib::subclass::prelude::*;

pub trait ComboRowImpl: ActionRowImpl + ObjectSubclass<Type: IsA<ComboRow>> {}

unsafe impl<T: ComboRowImpl> IsSubclassable<T> for ComboRow {}
