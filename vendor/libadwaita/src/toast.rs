// Take a look at the license at the top of the repository in the LICENSE file.

use crate::Toast;
use glib::translate::*;
use glib::variant::ToVariant;

impl Toast {
    #[doc(alias = "adw_toast_set_action_target")]
    #[doc(alias = "adw_toast_set_action_target_value")]
    pub fn set_action_target(&self, target: Option<&impl ToVariant>) {
        unsafe {
            ffi::adw_toast_set_action_target_value(
                self.to_glib_none().0,
                target.map(|v| v.to_variant()).to_glib_none().0,
            );
        }
    }
}
