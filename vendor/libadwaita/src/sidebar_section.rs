// Take a look at the license at the top of the repository in the LICENSE file.

use std::ptr;

use glib::translate::*;

use crate::{ffi, SidebarSection};

impl SidebarSection {
    #[doc(alias = "adw_prefences_group_bind_model")]
    #[doc(alias = "bind_model")]
    pub fn unbind_model(&self) {
        unsafe {
            ffi::adw_sidebar_section_bind_model(
                self.to_glib_none().0,
                ptr::null_mut(),
                None,
                ptr::null_mut(),
                None,
            )
        }
    }
}
