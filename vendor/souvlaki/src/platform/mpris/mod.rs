#![cfg(all(unix, not(target_os = "macos")))]

#[cfg(not(any(feature = "dbus", feature = "zbus")))]
compile_error!("either feature \"dbus\" or feature \"zbus\" are required");

#[cfg(all(feature = "dbus", feature = "zbus"))]
compile_error!("feature \"dbus\" and feature \"zbus\" are mutually exclusive");

#[cfg(feature = "zbus")]
mod zbus;
#[cfg(feature = "zbus")]
pub use self::zbus::*;
#[cfg(feature = "zbus")]
extern crate zbus as zbus_crate;

#[cfg(feature = "dbus")]
mod dbus;
#[cfg(feature = "dbus")]
pub use self::dbus::*;
#[cfg(feature = "dbus")]
extern crate dbus as dbus_crate;

/// A platform-specific error.
#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("internal D-Bus error: {0}")]
    #[cfg(feature = "dbus")]
    DbusError(#[from] dbus_crate::Error),
    #[error("internal D-Bus error: {0}")]
    #[cfg(feature = "zbus")]
    DbusError(#[from] zbus_crate::Error),
    #[error("D-bus service thread not running. Run MediaControls::attach()")]
    ThreadNotRunning,
    // NOTE: For now this error is not very descriptive. For now we can't do much about it
    // since the panic message returned by JoinHandle::join does not implement Debug/Display,
    // thus we cannot print it, though perhaps there is another way. I will leave this error here,
    // to at least be able to catch it, but it is preferable to have this thread *not panic* at all.
    #[error("D-Bus service thread panicked")]
    ThreadPanicked,
}
