use glib::prelude::*;
use glib::subclass::prelude::*;

use crate::subclass::prelude::ActionRowImpl;
use crate::SpinRow;

pub trait SpinRowImpl: ActionRowImpl + ObjectSubclass<Type: IsA<SpinRow>> {}

unsafe impl<T: SpinRowImpl> IsSubclassable<T> for SpinRow {}
