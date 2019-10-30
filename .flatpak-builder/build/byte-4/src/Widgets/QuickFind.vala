public class Widgets.QuickFind : Gtk.Revealer {
    private Gtk.ListBox radios_listbox;
    public Widgets.SearchEntry search_entry;
    public bool reveal {
        set {
            if (value) {
                reveal_child = true;
                search_entry.grab_focus ();
            } else {
                reveal_child = false;
            }
        }
    }
    public QuickFind () {
        transition_type = Gtk.RevealerTransitionType.SLIDE_DOWN;
        margin_top = 12;
        valign = Gtk.Align.START;
        halign = Gtk.Align.CENTER;
        reveal_child = false;
        //transition_duration = 300;
    }

    construct {
        var toast = new Granite.Widgets.Toast (_("The radio station was added correctly"));

        search_entry = new Widgets.SearchEntry ();
        search_entry.margin = 0;
        search_entry.get_style_context ().add_class ("search-entry");
        search_entry.placeholder_text = _("Quick find");

        var cancel_button = new Gtk.Button.with_label (_("Cancel"));
        cancel_button.can_focus = false;
        cancel_button.valign = Gtk.Align.CENTER;
        cancel_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        cancel_button.get_style_context ().add_class ("quick-find-cancel");

        var top_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        top_box.margin = 6;
        top_box.pack_start (search_entry, true, true, 0);
        top_box.pack_end (cancel_button, false, false, 0);

        var mode_button = new Granite.Widgets.ModeButton ();
        mode_button.get_style_context ().add_class ("quick-find-modebutton");
        mode_button.margin = 6;
        mode_button.valign = Gtk.Align.CENTER;
        mode_button.append_text (_("Library"));
        mode_button.append_text (_("Radios"));
        //mode_button.append_text (_("Podcasts"));
        mode_button.selected = 0;

        // Radios View
        radios_listbox = new Gtk.ListBox ();
        radios_listbox.expand = true;

        var radios_spinner = new Gtk.Spinner ();
        radios_spinner.halign = Gtk.Align.CENTER;
        radios_spinner.valign = Gtk.Align.CENTER;
        radios_spinner.expand = true;
        radios_spinner.active = true;
        radios_spinner.start ();

        var alert_view = new Widgets.AlertView (
            _("Discover…"),
            _("Search your favorite radios"),
            "edit-find-symbolic"
        );

        var radio_stack = new Gtk.Stack ();
        radio_stack.expand = true;
        radio_stack.transition_type = Gtk.StackTransitionType.CROSSFADE;

        radio_stack.add_named (alert_view, "radio_alert_grid");
        radio_stack.add_named (radios_spinner, "radios_spinner");
        radio_stack.add_named (radios_listbox, "radios_listbox");
        
        var radio_scrolled = new Gtk.ScrolledWindow (null, null);
        radio_scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        radio_scrolled.expand = true;
        radio_scrolled.add (radio_stack);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.height_request = 450;
        main_box.width_request = 325;
        main_box.get_style_context ().add_class ("quick-find");
        main_box.pack_start (top_box, false, false, 0);
        //main_box.pack_start (mode_button, false, false, 0);
        main_box.pack_start (radio_scrolled, true, true, 0);
        main_box.pack_end (toast, false, false, 0);

        add (main_box);

        cancel_button.clicked.connect (() => {
            reveal = false;
        });

        search_entry.activate.connect (() => {
        //search_entry.search_changed.connect (() => {
            Byte.radio_browser.get_radios_by_name (search_entry.text);
        });

        Byte.radio_browser.item_loaded.connect ((item) => {
            print (item.name + "\n");

            var row = new Widgets.RadioSearchRow (item);

            row.send_notification_error.connect (() => {
                toast.title = _("The radio station is already added");
                toast.send_notification ();
            });

            radios_listbox.add (row);
            radios_listbox.show_all ();
        });

        Byte.radio_browser.started.connect (() => {
            radio_stack.visible_child_name = "radios_spinner";

            radios_listbox.foreach((widget) => {
                widget.destroy (); 
            });
        });

        Byte.radio_browser.finished.connect (() => {
            int c = 0;

            radios_listbox.foreach ((widget) => {
                c++;
            });

            if (c > 0) {
                radio_stack.visible_child_name = "radios_listbox";
            } else {
                radio_stack.visible_child_name = "radio_alert_grid";
            }
        });

        Byte.database.adden_new_radio.connect ((radio) => {
            toast.title = _("The radio station was added correctly");
            toast.send_notification ();
        });
    }
}
