// Take a look at the license at the top of the repository in the LICENSE file.

use crate::TabBar;
use glib::translate::*;

impl TabBar {
    #[doc(alias = "adw_tab_bar_setup_extra_drop_target")]
    pub fn setup_extra_drop_target(&self, actions: gdk::DragAction, types: &[glib::Type]) {
        unsafe {
            ffi::adw_tab_bar_setup_extra_drop_target(
                self.to_glib_none().0,
                actions.into_glib(),
                types.to_glib_none().0,
                types.len(),
            )
        }
    }
}
