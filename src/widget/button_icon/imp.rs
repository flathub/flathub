use gtk::{glib, subclass::prelude::*};

#[derive(Debug, Default, gtk::CompositeTemplate)]
#[template(resource = "/io/github/plrigaux/sysd-manager/button_icon.ui")]
pub struct ButtonIcon {
    #[template_child]
    pub button_icon: TemplateChild<gtk::Image>,

    #[template_child]
    pub button_label: TemplateChild<gtk::Label>,
}

// The central trait for subclassing a GObject
#[glib::object_subclass]
impl ObjectSubclass for ButtonIcon {
    const NAME: &'static str = "ButtonIcon";
    type Type = super::ButtonIcon;
    type ParentType = gtk::Button;

    fn class_init(klass: &mut Self::Class) {
        // The layout manager determines how child widgets are laid out.
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

impl ObjectImpl for ButtonIcon {}
impl WidgetImpl for ButtonIcon {}
impl ButtonImpl for ButtonIcon {}
