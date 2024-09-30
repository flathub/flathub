use super::enums::ActiveState;
use crate::gtk::subclass::prelude::ObjectSubclassIsExt;
use gtk::glib;

glib::wrapper! {
    pub struct UnitInfo(ObjectSubclass<imp::UnitInfo>);
}

impl UnitInfo {
    pub fn new(
        primary: &str,
        description: &str,
        load_state: &str,
        active_state: ActiveState,
        sub_state: &str,
        followed_unit: &str,
        object_path: &str,
    ) -> Self {
        let this_object: Self = glib::Object::new();
        let imp: &imp::UnitInfo = this_object.imp();

        imp.set_primary(primary.to_owned());
        *imp.description.write().unwrap() = description.to_owned();
        *imp.load_state.write().unwrap() = load_state.to_owned();
        *imp.active_state.write().unwrap() = active_state as u32;
        *imp.active_state_icon.write().unwrap() = active_state.icon_name().to_string();
        *imp.sub_state.write().unwrap() = sub_state.to_owned();
        *imp.followed_unit.write().unwrap() = followed_unit.to_owned();
        *imp.object_path.write().unwrap() = object_path.to_owned();

        this_object
    }
}

pub mod imp {
    use std::sync::RwLock;

    use gtk::{glib, prelude::*, subclass::prelude::*};

    #[derive(Debug, glib::Properties, Default)]
    #[properties(wrapper_type = super::UnitInfo)]
    pub struct UnitInfo {
        #[property(get, set = Self::set_primary )]
        pub(super) primary: RwLock<String>,
        #[property(get)]
        display_name: RwLock<String>,
        #[property(get)]
        unit_type: RwLock<String>,
        #[property(get)]
        pub(super) description: RwLock<String>,
        #[property(get)]
        pub(super) load_state: RwLock<String>,
        #[property(get, set)]
        pub(super) active_state: RwLock<u32>,
        #[property(get, set)]
        pub(super) active_state_icon: RwLock<String>,
        #[property(get)]
        pub(super) sub_state: RwLock<String>,
        #[property(get)]
        pub(super) followed_unit: RwLock<String>,
        #[property(get)]
        pub(super) object_path: RwLock<String>,
        #[property(get, set)]
        pub(super) file_path: RwLock<Option<String>>,
        #[property(get, set, default = None)]
        pub(super) enable_status: RwLock<Option<String>>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for UnitInfo {
        const NAME: &'static str = "UnitInfo";
        type Type = super::UnitInfo;

        fn new() -> Self {
            Default::default()
        }
    }

    #[glib::derived_properties]
    impl ObjectImpl for UnitInfo {}

    impl UnitInfo {
        pub fn set_primary(&self, primary: String) {
            let mut split_char_index = primary.len();
            for (i, c) in primary.chars().enumerate() {
                if c == '.' {
                    split_char_index = i;
                }
            }

            let display_name = (&primary[..split_char_index]).to_owned();
            *self.display_name.write().unwrap() = display_name;

            let unit_type = (&primary[(split_char_index + 1)..]).to_owned();
            *self.unit_type.write().unwrap() = unit_type;

            *self.primary.write().unwrap() = primary;
        }
    }
}
