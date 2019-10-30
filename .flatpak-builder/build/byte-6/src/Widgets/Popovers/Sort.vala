/*
* Copyright © 2019 Alain M. (https://github.com/alainm23/planner)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public
* License along with this program; if not, write to the
* Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA
*
* Authored by: Alain M. <alain23@protonmail.com>
*/

public class Widgets.Popovers.Sort : Gtk.Popover {
    private Gtk.RadioButton radio_01;
    private Gtk.RadioButton radio_02;
    private Gtk.RadioButton radio_03;
    private Gtk.RadioButton radio_04;
    private Gtk.RadioButton radio_05;
    private Gtk.CheckButton order_reverse_button;

    public signal void mode_changed (int index);
    public signal void order_reverse (bool mode);
    
    public int selected {
        set {
            if (value == 0) {
                radio_01.active = true;
            } else if (value == 1) {
                radio_02.active = true;
            } else if (value == 2) {
                radio_03.active = true;
            } else if (value == 3) {
                radio_04.active = true;
            } else {
                radio_05.active = true;
            }
        }
    }
    public bool reverse {
        set {
            order_reverse_button.active = value;
        }
    }

    public string radio_01_label {
        set {
            radio_01.label = value;
        }
    }

    public string radio_02_label {
        set {
            radio_02.label = value;
        }
    }

    public string radio_03_label {
        set {
            radio_03.label = value;
        }
    }

    public string radio_04_label {
        set {
            radio_04.label = value;
        }
    }

    public string radio_05_label {
        set {
            radio_05.label = value;
        }
    }

    public bool radio_01_visible {
        set {
            radio_01.visible = value;
            radio_01.no_show_all = !value;
        }
    }

    public bool radio_02_visible {
        set {
            radio_02.visible = value;
            radio_02.no_show_all = !value;
        }
    }

    public bool radio_03_visible {
        set {
            radio_03.visible = value;
            radio_03.no_show_all = !value;
        }
    }

    public bool radio_04_visible {
        set {
            radio_04.visible = value;
            radio_04.no_show_all = !value;
        }
    }

    public bool radio_05_visible {
        set {
            radio_05.visible = value;
            radio_05.no_show_all = !value;
        }
    }

    public Sort (Gtk.Widget relative) {
        Object (
            relative_to: relative,
            modal: true,
            position: Gtk.PositionType.BOTTOM
        );
    }

    construct {
        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);

        var sort_label = new Granite.HeaderLabel (_("Sort by"));
        sort_label.margin_start = 12;
        sort_label.margin_top = 6;
        
        radio_01 = new Gtk.RadioButton.with_label_from_widget (null, "");
        radio_01.get_style_context ().add_class ("h3");
        radio_01.margin_start = 12;
        radio_01.margin_end = 12;

        radio_02 = new Gtk.RadioButton.with_label_from_widget (radio_01, "");
        radio_02.get_style_context ().add_class ("h3");
        radio_02.margin_start = 12;
        radio_02.margin_end = 12;

        radio_03 = new Gtk.RadioButton.with_label_from_widget (radio_01, "");
        radio_03.get_style_context ().add_class ("h3");
        radio_03.margin_start = 12;
        radio_03.margin_end = 12;

        radio_04 = new Gtk.RadioButton.with_label_from_widget (radio_01, "");
        radio_04.get_style_context ().add_class ("h3");
        radio_04.margin_start = 12;
        radio_04.margin_end = 12;

        radio_05 = new Gtk.RadioButton.with_label_from_widget (radio_01, "");
        radio_05.get_style_context ().add_class ("h3");
        radio_05.margin_start = 12;
        radio_05.margin_end = 12;

        order_reverse_button = new Gtk.CheckButton.with_label (_("Reversed order"));
        order_reverse_button.get_style_context ().add_class ("h3");
        order_reverse_button.margin_start = 12;
        order_reverse_button.margin_top = 3;
        order_reverse_button.margin_bottom = 6;
        order_reverse_button.margin_end = 12;

        var separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
        separator.margin_top = 3;

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 3);
        main_box.pack_start (sort_label, false, false, 0);
        main_box.pack_start (radio_01, false, false, 0);
        main_box.pack_start (radio_02, false, false, 0);
        main_box.pack_start (radio_03, false, false, 0);
        main_box.pack_start (radio_04, false, false, 0);
        main_box.pack_start (radio_05, false, false, 0);
        main_box.pack_start (separator, false, false, 0);
        main_box.pack_start (order_reverse_button, false, false, 0);

        add (main_box);

        radio_01.toggled.connect (() => {
            mode_changed (0);
        });

        radio_02.toggled.connect (() => {
            mode_changed (1);
        });

        radio_03.toggled.connect (() => {
            mode_changed (2);
        });

        radio_04.toggled.connect (() => {
            mode_changed (3);
        });

        radio_05.toggled.connect (() => {
            mode_changed (4);
        });

        order_reverse_button.toggled.connect (() => {
            order_reverse (order_reverse_button.active);
        });
    }
}