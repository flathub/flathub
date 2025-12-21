// Take a look at the license at the top of the repository in the LICENSE file.

use crate::{prelude::*, Breakpoint};
use glib::translate::*;

impl Breakpoint {
    #[doc(alias = "adw_breakpoint_add_settersv")]
    #[doc(alias = "add_settersv")]
    pub fn add_setters(&self, additions: &[(&impl IsA<glib::Object>, &str, impl ToValue)]) {
        let n_setters = additions.len() as _;
        let objects = additions
            .iter()
            .map(|(o, _n, _v)| (*o).clone().upcast::<glib::Object>())
            .collect::<Vec<_>>();
        let names = additions.iter().map(|(_o, n, _v)| *n).collect::<Vec<_>>();
        let values = additions
            .iter()
            .map(|(_o, _n, v)| v.to_value())
            .collect::<Vec<_>>();

        unsafe {
            ffi::adw_breakpoint_add_settersv(
                self.to_glib_none().0,
                n_setters,
                objects.as_slice().to_glib_none().0,
                names.as_slice().to_glib_none().0,
                values.as_slice().to_glib_none().0,
            );
        }
    }
}
