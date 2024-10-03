mod imp;

use crate::gtk::{glib, subclass::prelude::*};
use std::collections::{HashMap, HashSet};

glib::wrapper! {
    pub struct ExMenuButton(ObjectSubclass<imp::ExMenuButton>)
        @extends gtk::Widget,
        @implements gtk::Buildable;
}

impl ExMenuButton {
    pub fn new(label: &str) -> Self {
        let obj : ExMenuButton = glib::Object::new();
        obj.set_button_label(label);

        let imp = obj.imp();
        imp.check_boxes.replace(HashMap::new());
        imp.filter_set.replace(HashSet::new());
     
        obj
    }

    pub fn set_button_label(&self, label: &str) {
        self.imp().button_label.set_label(label);
    }

    pub fn add_item(&mut self, label: &str) {
        let binding = self.imp();

        binding.add_item(label);
    }

    pub fn contains_value(&self, value : &Option<String>) -> bool {
        let imp = self.imp();
        let set = imp.filter_set.borrow();
        
        if set.is_empty() {
            return true;
        }

        match value {
            Some(v) => set.contains(v),
            None => set.is_empty(),
        }
       
    }

    pub fn set_filter(&self, filter : gtk::CustomFilter) {
        let imp = self.imp();
        imp.filter.replace(filter);
    }

}
