use gtk::gio::ResourceLookupFlags;
use gtk::prelude::*;
use gtk::{gdk, gio, prelude::ActionMapExtManual};

use crate::analyze::build_analyze_window;
use crate::info;
use log::warn;

use super::preferences;


pub const APP_TITLE: &str = "SysD Manager";

fn build_popover_menu() -> gtk::PopoverMenu {
    let menu = gio::Menu::new();

    menu.append(Some("Analyze Blame"), Some("app.analyze_blame"));
    menu.append(Some("About"), Some("app.about"));
    menu.append(Some("Systemd Info"), Some("app.systemd_info"));
    menu.append(Some("Preferences"), Some("app.preferences"));

    let unit_menu_popover = gtk::PopoverMenu::builder().menu_model(&menu).build();

    unit_menu_popover
}

pub fn build_menu() -> gtk::MenuButton {
    let popover = build_popover_menu();
    let menu_button = gtk::MenuButton::builder()
        .focusable(true)
        .receives_default(true)
        .icon_name("open-menu-symbolic")
        .halign(gtk::Align::End)
        .direction(gtk::ArrowType::Down)
        .popover(&popover)
        .build();

    menu_button
}

pub fn on_startup(app: &gtk::Application) {
    let about = gio::ActionEntry::builder("about")
        .activate(|application: &gtk::Application, _, _| {
            let about = create_about();

            if let Some(first_window) = application.windows().first() {
                about.set_transient_for(Some(first_window));
                about.set_modal(true);
            }

            about.present();
        })
        .build();

    let analyze_blame = gio::ActionEntry::builder("analyze_blame")
        .activate(|application: &gtk::Application, _b, _c| {
            let analyze_blame_window = build_analyze_window();

            if let Some(first_window) = application.windows().first() {
                analyze_blame_window.set_transient_for(Some(first_window));
                analyze_blame_window.set_modal(true);
            }

            analyze_blame_window.present();
        })
        .build();

    let systemd_info = gio::ActionEntry::builder("systemd_info")
        .activate(|application: &gtk::Application, _, _| {
            let systemd_info_window = info::build_systemd_info();

            if let Some(first_window) = application.windows().first() {
                systemd_info_window.set_transient_for(Some(first_window));
                systemd_info_window.set_modal(true);
            }

            systemd_info_window.present();
        })
        .build();

    let preferences = gio::ActionEntry::builder("preferences")
        .activate(|application: &gtk::Application, _, _| {
            let preferences_window = preferences::build_preferences();

            if let Some(first_window) = application.windows().first() {
                preferences_window.set_transient_for(Some(first_window));
                preferences_window.set_modal(true);
            }

            preferences_window.present();
        })
        .build();

    app.add_action_entries([about, analyze_blame, systemd_info, preferences]);
}

fn create_about() -> gtk::AboutDialog {
    const VERSION: &str = env!("CARGO_PKG_VERSION");
    const CARGO_PKG_AUTHORS: &str = env!("CARGO_PKG_AUTHORS");
    const CARGO_PKG_DESCRIPTION: &str = env!("CARGO_PKG_DESCRIPTION");

    let authors: Vec<&str> = CARGO_PKG_AUTHORS.split(',').collect();

    let about = gtk::AboutDialog::builder()
        .authors(authors)
        .name("About")
        .program_name(APP_TITLE)
        .modal(true)
        .version(VERSION)
        .license_type(gtk::License::Gpl30)
        .comments(CARGO_PKG_DESCRIPTION)
        .website("https://github.com/plrigaux/sysd-manager")
        .build();

    //TODO create const for the path prefix
    match gio::functions::resources_lookup_data(
        "/io/github/plrigaux/sysd-manager/io.github.plrigaux.sysd-manager.svg",
        ResourceLookupFlags::NONE,
    ) {
        Ok(bytes) => {
            let logo = gdk::Texture::from_bytes(&bytes).expect("gtk-rs.svg to load");
            about.set_logo(Some(&logo));
        }
        Err(e) => warn!("Fail to load logo: {}", e),
    };

    about
}
