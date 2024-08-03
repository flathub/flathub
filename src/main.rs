extern crate env_logger;
extern crate gtk;
extern crate log;

mod systemd;
mod systemd_gui;

// Contains all of the heavy GUI-related work
use gtk::gio;
use gtk::glib;
mod analyze;
mod info;
mod settings;
mod widget;

use log::{info, warn};

extern crate dotenv;

use dotenv::dotenv;

fn main() -> glib::ExitCode {
    dotenv().ok();

    env_logger::init();

    info!("Program starting up");

    match gio::resources_register_include!("sysd-manager.gresource") {
        Ok(_) => (),
        Err(e) => warn!("Failed to register resources. Error: {:?}", e),
    }

    systemd_gui::launch()
}
