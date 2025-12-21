// Take a look at the license at the top of the repository in the LICENSE file.

use crate::Carousel;
use glib::translate::*;

impl Carousel {
    #[doc(alias = "adw_carousel_get_nth_page")]
    #[doc(alias = "get_nth_page")]
    pub fn nth_page(&self, n: u32) -> gtk::Widget {
        assert!(n < self.n_pages());
        unsafe { from_glib_none(ffi::adw_carousel_get_nth_page(self.to_glib_none().0, n)) }
    }
}
