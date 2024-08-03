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
        followed_unit:&str,
        object_path: &str,
    ) -> Self {
        let this_object: Self = glib::Object::new();
        let imp: &imp::UnitInfo = this_object.imp();

        imp.set_primary(primary.to_owned());
        imp.description.replace(description.to_owned());
        imp.load_state.replace(load_state.to_owned());
        imp.active_state.replace(active_state as u32);
        imp.active_state_icon
            .replace(active_state.icon_name().to_string());
        imp.sub_state.replace(sub_state.to_owned());
        imp.followed_unit.replace(followed_unit.to_owned());
        imp.object_path.replace(object_path.to_owned());

        this_object
    }
}

pub mod imp {
    use std::cell::{Cell, RefCell};

    use gtk::{glib, prelude::*, subclass::prelude::*};

    #[derive(Debug, glib::Properties, Default)]
    #[properties(wrapper_type = super::UnitInfo)]
    pub struct UnitInfo {
        #[property(get, set = Self::set_primary )]
        pub(super) primary: RefCell<String>,
        #[property(get)]
        display_name: RefCell<String>,
        #[property(get)]
        unit_type: RefCell<String>,
        #[property(get)]
        pub(super) description: RefCell<String>,
        #[property(get)]
        pub(super) load_state: RefCell<String>,
        #[property(get, set)]
        pub(super) active_state: Cell<u32>,
        #[property(get, set)]
        pub(super) active_state_icon: RefCell<String>,
        #[property(get)]
        pub(super) sub_state: RefCell<String>,
        #[property(get)]
        pub(super) followed_unit: RefCell<String>,
        #[property(get)]
        pub(super) object_path: RefCell<String>,
        #[property(get, set)]
        pub(super) file_path: RefCell<Option<String>>,
        #[property(get, set, default = None)]
        pub(super) enable_status: RefCell<Option<String>>,
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

            self.display_name
                .replace((&primary[..split_char_index]).to_owned());
            self.unit_type
                .replace((&primary[(split_char_index + 1)..]).to_owned());
            self.primary.replace(primary);
        }
    }
}
