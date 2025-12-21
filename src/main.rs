#![cfg_attr(all(not(debug_assertions), target_os = "windows"), windows_subsystem = "windows")]

mod http_source;
mod listen;
mod locale;
#[cfg(debug_assertions)]
mod log;
mod meta;
mod metainfo;
mod station;
mod ui;

#[cfg(debug_assertions)]
const APP_ID: &str = "io.github.noobping.listenmoe_develop";
#[cfg(not(debug_assertions))]
const APP_ID: &str = "io.github.noobping.listenmoe";
#[cfg(feature = "icon")]
const RESOURCE_ID: &str = "/io/github/noobping/listenmoe";
#[cfg(feature = "icon")]
use adw::gtk::{gdk::Display, IconTheme};
use adw::prelude::*;
use adw::Application;

fn main() {
    locale::init_i18n();

    // Register resources compiled into the binary. If this fails, the app cannot find its assets.
    #[cfg(feature = "icon")]
    adw::gtk::gio::resources_register_include!("compiled.gresource")
        .expect("Failed to register resources");

    // Initialize libadwaita/GTK. This must be called before any UI code.
    adw::init().expect("Failed to initialize libadwaita");

    // Load the icon theme from the embedded resources so that icons resolve correctly even outside a installed environment.
    #[cfg(feature = "icon")]
    if let Some(display) = Display::default() {
        let theme = IconTheme::for_display(&display);
        theme.add_resource_path(RESOURCE_ID);
    }

    // Create the GTK application. The application ID must be unique and corresponds to the desktop file name.
    let app = Application::builder().application_id(APP_ID).build();
    app.connect_activate(ui::build_ui); // Build the UI when the application is activated.
    app.run(); // Run the application. This function does not return until the last window is closed.
}
