// Take a look at the license at the top of the repository in the LICENSE file.

use crate::Sidebar;
use glib::translate::*;

impl Sidebar {
    #[doc(alias = "adw_sidebar_setup_drop_target")]
    pub fn setup_drop_target(&self, actions: gdk::DragAction, types: &[glib::Type]) {
        unsafe {
            ffi::adw_sidebar_setup_drop_target(
                self.to_glib_none().0,
                actions.into_glib(),
                types.to_glib_none().0,
                types.len(),
            )
        }
    }
}
