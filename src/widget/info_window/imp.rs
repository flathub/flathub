use gtk::prelude::*;
use gtk::subclass::prelude::*;
use gtk::{gio, glib};
use std::cell::RefCell;

use crate::info::rowitem;

// ANCHOR: imp
#[derive(Debug, Default, gtk::CompositeTemplate)]
#[template(resource = "/io/github/plrigaux/sysd-manager/unit_info.ui")]
pub struct InfoWindowImp {
    //pub settings: OnceCell<Settings>,
    #[template_child]
    pub unit_properties: TemplateChild<gtk::ListBox>,

    pub(super) store: RefCell<Option<gio::ListStore>>,
}

#[gtk::template_callbacks]
impl InfoWindowImp {
    #[template_callback]
    fn handle_copy_click(&self, _button: &gtk::Button) {
        let clipboard = _button.clipboard();

        let unit_prop_store = &self.store;
        //unit_prop_store.borrow()
        if let Some(store) = unit_prop_store.borrow().as_ref() {
            let n_item = store.n_items();

            let mut data = String::new();
            for i in 0..n_item {
                if let Some(object) = store.item(i) {
                    if let Ok(x) = object.downcast::<rowitem::Metadata>() {
                        data.push_str(&x.col1());
                        data.push('\t');
                        data.push_str(&x.col2());
                        data.push('\n')
                    }
                }
            }
            clipboard.set_text(&data)
        }
    }
}

#[glib::object_subclass]
impl ObjectSubclass for InfoWindowImp {
    const NAME: &'static str = "InfoWindow";
    type Type = super::InfoWindow;
    type ParentType = gtk::Window;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
        klass.bind_template_callbacks();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

impl ObjectImpl for InfoWindowImp {
    fn constructed(&self) {
        self.parent_constructed();
        // Load latest window state
        let obj = self.obj();
        // obj.setup_settings();
        // obj.load_window_size();
        obj.load_dark_mode();
    }
}
impl WidgetImpl for InfoWindowImp {}
impl WindowImpl for InfoWindowImp {
    // Save window state right before the window will be closed
    fn close_request(&self) -> glib::Propagation {
        // Save window size
        log::debug!("Close window");
        /*         self.obj()
        .save_window_size()
        .expect("Failed to save window state"); */
        // Allow to invoke other event handlers
        glib::Propagation::Proceed
    }
}
impl ApplicationWindowImpl for InfoWindowImp {}
// ANCHOR_END: imp
