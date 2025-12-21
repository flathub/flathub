use crate::BreakpointBin;
use glib::prelude::*;
use glib::subclass::prelude::*;
use gtk::subclass::prelude::WidgetImpl;

pub trait BreakpointBinImpl: WidgetImpl + ObjectSubclass<Type: IsA<BreakpointBin>> {}

unsafe impl<T: BreakpointBinImpl> IsSubclassable<T> for BreakpointBin {}
