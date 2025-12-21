// Take a look at the license at the top of the repository in the LICENSE file.

use crate::TabOverview;
use glib::translate::*;

impl TabOverview {
    #[doc(alias = "adw_tab_overview_setup_extra_drop_target")]
    pub fn setup_extra_drop_target(&self, actions: gdk::DragAction, types: &[glib::Type]) {
        unsafe {
            ffi::adw_tab_overview_setup_extra_drop_target(
                self.to_glib_none().0,
                actions.into_glib(),
                types.to_glib_none().0,
                types.len(),
            )
        }
    }
}
