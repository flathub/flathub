#![allow(clippy::module_inception)]
pub use self::platform::*;

#[cfg(target_os = "windows")]
#[path = "windows/mod.rs"]
mod platform;

#[cfg(any(target_os = "macos", target_os = "ios"))]
#[path = "macos/mod.rs"]
mod platform;

#[cfg(all(
    unix,
    not(any(target_os = "macos", target_os = "ios", target_os = "android"))
))]
#[path = "mpris/mod.rs"]
mod platform;

#[cfg(all(
    not(target_os = "linux"),
    not(target_os = "netbsd"),
    not(target_os = "freebsd"),
    not(target_os = "openbsd"),
    not(target_os = "dragonfly"),
    not(target_os = "windows"),
    not(target_os = "macos"),
    not(target_os = "ios")
))]
#[path = "empty/mod.rs"]
mod platform;
