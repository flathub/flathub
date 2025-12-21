#[doc(hidden)]
pub use gtk::prelude::*;

pub use crate::auto::traits::*;

#[cfg(feature = "v1_5")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_5")))]
pub use crate::alert_dialog::AlertDialogExtManual;
#[cfg(feature = "v1_2")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_2")))]
pub use crate::message_dialog::MessageDialogExtManual;
