use crate::gtk::{glib, subclass::prelude::*};

mod imp;

glib::wrapper! {
    pub struct ButtonIcon(ObjectSubclass<imp::ButtonIcon>)
        @extends gtk::Button, gtk::Widget,
        @implements gtk::Accessible, gtk::Actionable,
                    gtk::Buildable, gtk::ConstraintTarget;
}

impl ButtonIcon {
    pub fn new(label: &str, icon_name: &str) -> Self {
        let obj: ButtonIcon = glib::Object::new();
        obj.set_button_icon(icon_name);
        obj.set_button_label(label);
       
        obj
    }

    pub fn set_button_label(&self, label: &str) {
        self.imp().button_label.set_label(label);
    }

    pub fn set_button_icon(&self, icon_name: &str) {
        self.imp().button_icon.set_icon_name(Some(icon_name));
    }
}
