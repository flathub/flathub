use crate::SidebarItem;
use glib::prelude::*;
use glib::subclass::prelude::*;

pub trait SidebarItemImpl: ObjectImpl + ObjectSubclass<Type: IsA<SidebarItem>> {}

unsafe impl<T: SidebarItemImpl> IsSubclassable<T> for SidebarItem {}
