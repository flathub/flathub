use std::sync::LazyLock;

use crate::gtk::prelude::*;

use crate::systemd_gui;
use glib::Object;
use gtk::gio;
use gtk::gio::Settings;
use gtk::glib;

use crate::gtk::subclass::prelude::*;

use log::info;
use log::warn;

pub static PREFERENCES: LazyLock<Preferences> = LazyLock::new(|| {
    let settings = get_settings();
    let pref = Preferences::new_with_setting(&settings);

    pref
});

const KEY_DBUS_LEVEL: &str = "dbus-level";

pub fn build_preferences() -> gtk::Window {
    let settings = get_settings();

    let gbox = gtk::Box::new(gtk::Orientation::Horizontal, 10);

    gbox.append(&gtk::Label::new(Some("DBus level")));
    gbox.set_vexpand(false);

    let model = gtk::StringList::new(&[DbusLevel::Session.as_str(), DbusLevel::System.as_str()]);
    //let level_dropdown = gtk::DropDown::new(Some(model), gtk::Expression::NONE);

    let level_dropdown = gtk::DropDown::builder().model(&model).build();

    level_dropdown.set_vexpand(false);
    {
        let settings = settings.clone();
        level_dropdown.connect_selected_notify(move |toggle_button| {
            let idx = toggle_button.selected();
            info!("Values Selected {:?}", idx);

            let level: DbusLevel = idx.into();

            //let settings = get_settings();
            if let Err(e) = set_dbus_level(&settings, level) {
                warn!("Error: {:?}", e);
                return;
            }
            info!(
                "Save setting '{KEY_DBUS_LEVEL}' with value {:?}",
                level.as_str()
            )
        });
    }
    gbox.append(&level_dropdown);

    let level = get_dbus_level_settings(&settings);

    level_dropdown.set_selected(level as u32);

    let window = gtk::Window::builder()
        .title("Preferences")
        .default_height(600)
        .default_width(600)
        .child(&gbox)
        .build();

    window
}

fn set_dbus_level(settings: &Settings, level: DbusLevel) -> Result<(), glib::BoolError> {
    let res = settings.set_string(KEY_DBUS_LEVEL, level.as_str());
    PREFERENCES.set_dbus_level(level as u32);

    res
}

fn get_settings() -> Settings {
    gio::Settings::new(systemd_gui::APP_ID)
}

fn get_dbus_level_settings(settings: &Settings) -> DbusLevel {
    let level: glib::GString = settings.string(KEY_DBUS_LEVEL);
    DbusLevel::from(level.as_str())
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum DbusLevel {
    #[default]
    Session = 0,
    System = 1,
}

impl DbusLevel {
    fn as_str(&self) -> &str {
        match self {
            DbusLevel::Session => "Session",
            DbusLevel::System => "System",
        }
    }
}

impl From<&str> for DbusLevel {
    fn from(level: &str) -> Self {
        if "System".eq(level) {
            DbusLevel::System
        } else {
            DbusLevel::Session
        }
    }
}

impl From<u32> for DbusLevel {
    fn from(level: u32) -> Self {
        match level {
            1 => DbusLevel::System,
            _ => DbusLevel::Session,
        }
    }
}

glib::wrapper! {
    pub struct Preferences(ObjectSubclass<imp::PreferencesImp>);
}

impl Preferences {
    pub fn new() -> Self {
        let settings = get_settings();
        Self::new_with_setting(&settings)
    }

    pub fn new_with_setting(settings: &Settings) -> Self {
        let this_object: Self = Object::builder().build();
        let imp: &imp::PreferencesImp = this_object.imp();

        let level = get_dbus_level_settings(settings);
        imp.set_dbus_level(level as u32);

        this_object
    }
}

pub mod imp {
    use std::sync::Mutex;

    use gtk::{glib, prelude::*, subclass::prelude::*};
    use log::{info, warn};

    #[derive(Debug, glib::Properties, Default)]
    #[properties(wrapper_type = super::Preferences)]
    pub struct PreferencesImp {
        #[property(get, set = Self::set_dbus_level )]
        pub(super) dbus_level: Mutex<u32>,

    }

    #[glib::object_subclass]
    impl ObjectSubclass for PreferencesImp {
        const NAME: &'static str = "PreferencesImp";
        type Type = super::Preferences;

        fn new() -> Self {
            Default::default()
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for PreferencesImp {
        fn constructed(&self) {
            self.parent_constructed();
        }
    }

    impl PreferencesImp {
        pub fn set_dbus_level(&self, dbus_level: u32) {
            info!("set_dbus_level: {dbus_level}");
            match self.dbus_level.lock() {
                Ok(mut a) => *a = dbus_level,
                Err(e) => warn!("Error {:?}", e),
            }
        }
    }
}

#[cfg(test)]
mod tests {

    use super::*;

    #[test]
    fn test_dbus_level_any_number() {
        assert_eq!(<u32 as Into<DbusLevel>>::into(1000), DbusLevel::Session)
    }

    #[test]
    fn test_dbus_level_int_mapping() {
        //assert_num_mapping(EnablementStatus::Unasigned);
        assert_num_mapping(DbusLevel::Session);
        assert_num_mapping(DbusLevel::System);
    }

    #[test]
    fn test_dbus_level_string_mapping() {
        //assert_num_mapping(EnablementStatus::Unasigned);
        assert_string_mapping(DbusLevel::Session, "Session");
        assert_string_mapping(DbusLevel::System, "System");
    }

    fn assert_num_mapping(level: DbusLevel) {
        let val = level as u32;
        let convert: DbusLevel = val.into();
        assert_eq!(convert, level)
    }

    fn assert_string_mapping(level: DbusLevel, key: &str) {
        let convert: DbusLevel = key.into();
        assert_eq!(convert, level)
    }
}
