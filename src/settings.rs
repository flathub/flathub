use std::sync::OnceLock;

use crate::gtk::prelude::SettingsExt;
use crate::widget::window::Window;
use ashpd::desktop::settings::ColorScheme;

use gtk::prelude::*;

use gtk::gio;
use log::info;
use log::warn;
use tokio::runtime::Runtime;

/* const MATE: &str = "/org/mate/desktop/interface/gtk-theme";
const GNOME: &str = "/org/gnome/desktop/interface/gtk-theme";
const CINNAMON: &str = "/org/cinnamon/desktop/interface/gtk-theme"; */

/*
TODO check https://github.com/bilelmoussaoui/ashpd for the rust interface on https://github.com/flatpak/xdg-desktop-portal
*/
pub fn set_color_scheme(win: &Window) {
    //client: Arc<Mutex<Client>>,
    let rt = runtime();
    let value = rt.block_on(get_color_scheme());
    let win_settings: gtk::Settings = WidgetExt::settings(win);
    if let Some(color_scheme) = value {
        match color_scheme {
            ColorScheme::NoPreference => {}
            ColorScheme::PreferDark => {
                set_application_prefer_dark_theme(&win_settings, true);
            }
            ColorScheme::PreferLight => {
                set_application_prefer_dark_theme(&win_settings, false);
            }
        }
    } else {
        const SCHEMA: &str = "org.gnome.desktop.interface";
        const KEY: &str = "color-scheme";

        let Some(settings_schema_source) = gio::SettingsSchemaSource::default() else {
            warn!("Can't intanciate SettingsSchemaSource");
            return;
        };

        let Some(setting_schema) = settings_schema_source.lookup(SCHEMA, true) else {
            warn!("Schema '{}' not found", SCHEMA);
            return;
        };

        if !setting_schema.has_key(KEY) {
            warn!("Key '{}' not found on schema '{}'", KEY, SCHEMA);
        }

        let gio_settings = gio::Settings::new(SCHEMA);

        let color_scheme_value = gio_settings.value(KEY);

        let Some(color_scheme) = color_scheme_value.str() else {
            warn!("Key '{}' not a string", KEY);
            return;
        };

        set_dark_theme(color_scheme, &win_settings);
    }
}

fn set_dark_theme(color_scheme: &str, settings: &gtk::Settings) {
    info!("color-scheme value: '{}'", color_scheme);

    if color_scheme.contains("prefer-dark") {
        set_application_prefer_dark_theme(settings, true);
    } else if color_scheme.contains("default") {
        set_application_prefer_dark_theme(settings, false);
    }
}

fn set_application_prefer_dark_theme(settings: &gtk::Settings, prefer_dark_theme: bool) {
    info!("set_gtk_application_prefer_dark_theme {prefer_dark_theme}");
    settings.set_gtk_application_prefer_dark_theme(prefer_dark_theme);
}

// ANCHOR: tokio_runtime
fn runtime() -> &'static Runtime {
    static RUNTIME: OnceLock<Runtime> = OnceLock::new();
    RUNTIME.get_or_init(|| Runtime::new().expect("Setting up tokio runtime needs to succeed."))
}

async fn get_color_scheme() -> Option<ColorScheme> {
    let ashpd_settings = match ashpd::desktop::settings::Settings::new().await {
        Ok(settings) => settings,
        Err(e) => {
            warn!("Error: {:#?}", e);
            return None;
        }
    };

    let color_scheme: ashpd::desktop::settings::ColorScheme =
        match ashpd_settings.color_scheme().await {
            Ok(cs) => cs,
            Err(e) => {
                warn!("Error: {:#?}", e);
                return None;
            }
        };

    info!("ColorScheme: {:?}", color_scheme);

    return Some(color_scheme);
}
