#![cfg_attr(docsrs, feature(doc_cfg))]
#![allow(clippy::needless_doctest_main)]
#![doc(
    html_logo_url = "https://gitlab.gnome.org/GNOME/libadwaita/-/raw/main/doc/libadwaita.svg",
    html_favicon_url = "https://gitlab.gnome.org/GNOME/libadwaita/-/raw/main/demo/data/org.gnome.Adwaita1.Demo-symbolic.svg"
)]
//! # Rust Adwaita bindings
//!
//! This library contains safe Rust bindings for [Adwaita](https://gitlab.gnome.org/GNOME/libadwaita), a library that offers
//! building blocks for modern GNOME applications.
//!
//! See also
//!
//! - [GTK 4 Rust bindings documentation](mod@gtk)
//! - [Libadwaita documentation](https://gnome.pages.gitlab.gnome.org/libadwaita/)
//! - [gtk-rs project overview](https://gtk-rs.org/)
//! - [Report bindings related issues](https://gitlab.gnome.org/World/Rust/libadwaita-rs)
//! - [Report upstream libadwaita issues](https://gitlab.gnome.org/GNOME/libadwaita/)
//!
//! # Example
//!
//! Adwaita needs to be initialized before use.
//! This can be done by either:
//! - using [`adw::Application`](struct@Application) instead of [`gtk::Application`](struct@gtk::Application), or by
//! - calling [`fn@init`] on [`startup`](fn@gio::prelude::ApplicationExt::connect_startup).
//!
//! The [`libadwaita`](mod@crate) crate is usually renamed to `adw`. You can
//! do this globally in your `Cargo.toml` file:
//!
//! ```toml
//! [dependencies.adw]
//! package = "libadwaita"
//! version = "0.x.y"
//! ```
//!
//! ```no_run
//! # use libadwaita as adw;
//! use adw::prelude::*;
//!
//! use adw::{ActionRow, Application, ApplicationWindow, HeaderBar};
//! use gtk::{Box, ListBox, Orientation, SelectionMode};
//!
//! fn main() {
//!     let application = Application::builder()
//!         .application_id("com.example.FirstAdwaitaApp")
//!         .build();
//!
//!     application.connect_activate(|app| {
//!         // ActionRows are only available in Adwaita
//!         let row = ActionRow::builder()
//!             .activatable(true)
//!             .title("Click me")
//!             .build();
//!         row.connect_activated(|_| {
//!             eprintln!("Clicked!");
//!         });
//!
//!         let list = ListBox::builder()
//!             .margin_top(32)
//!             .margin_end(32)
//!             .margin_bottom(32)
//!             .margin_start(32)
//!             .selection_mode(SelectionMode::None)
//!             // makes the list look nicer
//!             .css_classes(vec![String::from("boxed-list")])
//!             .build();
//!         list.append(&row);
//!
//!         // Combine the content in a box
//!         let content = Box::new(Orientation::Vertical, 0);
//!         // Adwaitas' ApplicationWindow does not include a HeaderBar
//!         content.append(&HeaderBar::new());
//!         content.append(&list);
//!
//!         let window = ApplicationWindow::builder()
//!             .application(app)
//!             .title("First App")
//!             .default_width(350)
//!             // add content to window
//!             .content(&content)
//!             .build();
//!         window.present();
//!     });
//!
//!     application.run();
//! }
//! ```

// Re-export the -sys bindings
pub use ffi;
pub use gdk;
pub use gio;
pub use glib;
pub use gtk;

/// Asserts that this is the main thread and `gtk::init` has been called.
macro_rules! assert_initialized_main_thread {
    () => {
        if !::gtk::is_initialized_main_thread() {
            if ::gtk::is_initialized() {
                panic!("libadwaita may only be used from the main thread.");
            } else {
                panic!("Gtk has to be initialized before using libadwaita.");
            }
        }
    };
}

macro_rules! skip_assert_initialized {
    () => {};
}

#[allow(unused_imports)]
#[allow(clippy::let_and_return)]
#[allow(clippy::type_complexity)]
mod auto;

#[cfg(feature = "v1_5")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_5")))]
mod alert_dialog;
mod application;
#[cfg(feature = "v1_4")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_4")))]
mod breakpoint;
mod carousel;
mod functions;
#[cfg(feature = "v1_2")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_2")))]
mod message_dialog;
#[cfg(feature = "v1_9")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_9")))]
mod sidebar;
#[cfg(feature = "v1_9")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_9")))]
mod sidebar_section;
#[cfg(feature = "v1_4")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_4")))]
mod spin_row;
mod tab_bar;
#[cfg(feature = "v1_3")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_3")))]
mod tab_overview;
mod tab_view;
mod toast;

pub use auto::functions::*;
pub use auto::*;
pub use functions::*;

pub mod builders;
pub mod prelude;
pub mod subclass;
