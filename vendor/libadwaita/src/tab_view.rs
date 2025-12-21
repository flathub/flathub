// Take a look at the license at the top of the repository in the LICENSE file.

use crate::{TabPage, TabView};
use glib::translate::*;

impl TabView {
    #[doc(alias = "adw_tab_view_get_nth_page")]
    #[doc(alias = "get_nth_page")]
    pub fn nth_page(&self, position: i32) -> TabPage {
        assert!(position < self.n_pages());
        unsafe {
            from_glib_none(ffi::adw_tab_view_get_nth_page(
                self.to_glib_none().0,
                position,
            ))
        }
    }
}
