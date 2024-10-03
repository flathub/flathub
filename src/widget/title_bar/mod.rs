mod menu;

use super::preferences;
use gtk::pango::{AttrInt, AttrList, Weight};
use gtk::prelude::*;
use crate::widget::button_icon::ButtonIcon;

pub fn build_title_bar(search_bar: &gtk::SearchBar) -> TitleBar {
    // ----------------------------------------------

    let title = gtk::Label::builder()
        .label(menu::APP_TITLE)
        .single_line_mode(true)
        .ellipsize(gtk::pango::EllipsizeMode::End)
        .width_chars(5)
        .css_classes(["title"])
        .build();

    let title_bar = gtk::HeaderBar::builder().title_widget(&title).build();

    let menu_button = menu::build_menu();

    title_bar.pack_end(&menu_button);

    let right_bar_label = gtk::Label::builder()
        .label("Service Name")
        .attributes(&{
            let attribute_list = AttrList::new();
            attribute_list.insert(AttrInt::new_weight(Weight::Bold));
            attribute_list
        })
        .build();

    let search_button = gtk::ToggleButton::new();
    search_button.set_icon_name("system-search-symbolic");
    search_button.set_tooltip_text(Some("Filter results"));
    title_bar.pack_start(&search_button);

    let refresh_button = ButtonIcon::new("Refresh", "view-refresh");
    refresh_button.set_tooltip_text(Some("Refresh results"));

    title_bar.pack_start(&refresh_button);

    title_bar.pack_start(&right_bar_label);

    search_button
        .bind_property("active", search_bar, "search-mode-enabled")
        .sync_create()
        .bidirectional()
        .build();

    TitleBar {
        title_bar,
        right_bar_label,
        search_button,
        refresh_button,
    }
}

pub fn on_startup(app: &gtk::Application) {
    menu::on_startup(app);
}

pub struct TitleBar {
    pub title_bar: gtk::HeaderBar,
    pub right_bar_label: gtk::Label,
    pub search_button: gtk::ToggleButton,
    pub refresh_button: ButtonIcon,
}
