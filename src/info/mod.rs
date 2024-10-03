

use gtk::gio;
use gtk::{prelude::*, ListBox};

use crate::systemd;
use log::error;

pub mod rowitem;

use rowitem::Metadata;

pub fn build_systemd_info() -> gtk::Window {
    let systemd_info = build_systemd_info_data();

    let window = gtk::Window::builder()
        .title("Systemd Info")
        .default_height(600)
        .default_width(600)
        .child(&systemd_info)
        .build();

    window
}

fn build_systemd_info_data() -> gtk::ScrolledWindow {
    let store = gio::ListStore::new::<rowitem::Metadata>();

    let Ok(map) = systemd::fetch_system_info() else {
        let unit_analyse_scrolled_window = gtk::ScrolledWindow::builder()
            .vexpand(true)
            .focusable(true)
            .build();

        return unit_analyse_scrolled_window;
    };

    for (key, value) in map {
        store.append(&rowitem::Metadata::new(key, value));
    }

    let no_selection = gtk::SingleSelection::new(Some(store));

    let list_box = ListBox::builder().build();

    list_box.bind_model(Some(&no_selection), |object| {
        let meta = match object.downcast_ref::<Metadata>() {
            Some(any_objet) => any_objet,
            None => {
                error!("No linked object");
                let list_box_row = gtk::ListBoxRow::new();
                return list_box_row.upcast::<gtk::Widget>();
            }
        };

        let box_ = gtk::Box::new(gtk::Orientation::Horizontal, 15);

        const SIZE: usize = 30;

        let mut tmp = String::new();
        let mut long_text = false;
        let key_label = if meta.col1().chars().count() > SIZE {
            long_text = true;
            tmp.push_str(&meta.col1()[..(SIZE - 3)]);
            tmp.push_str("...");
            &tmp
        } else {
            tmp.push_str(meta.col1().as_str());
            &tmp
        };

        let l1 = gtk::Label::builder()
            .label(key_label)
            .width_chars(SIZE as i32)
            .xalign(0.0)
            .max_width_chars(30)
            .single_line_mode(true)
            .build();

        if long_text {
            l1.set_tooltip_text(Some(&meta.col1()));
        }

        let l2 = gtk::Label::new(Some(&meta.col2()));

        box_.append(&l1);
        box_.append(&l2);

        box_.upcast::<gtk::Widget>()
    });
    //list_box.set_mo

    let unit_analyse_scrolled_window = gtk::ScrolledWindow::builder()
        .vexpand(true)
        .focusable(true)
        .child(&list_box)
        .build();

    unit_analyse_scrolled_window
}
