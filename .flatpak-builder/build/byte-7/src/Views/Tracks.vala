public class Views.Tracks : Gtk.EventBox {
    private Widgets.SearchEntry search_entry;
    private Gtk.ListBox listbox;
    private Gtk.Stack stack;
    private Widgets.AlertView alert_view;

    public signal void go_back ();

    private int item_index;
    private int item_max;
    private Gee.ArrayList<Objects.Track?> all_tracks;
    public Tracks () {} 

    construct {
        item_index = 0;
        item_max = 25;

        all_tracks = Byte.database.get_all_tracks_order_by (
            Byte.settings.get_enum ("track-sort"), 
            Byte.settings.get_boolean ("track-order-reverse")
        );

        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);
        get_style_context ().add_class ("w-round");

        /*
            Alert View
        */

        alert_view = new Widgets.AlertView (
            _("No Results"),
            null,
            "edit-find-symbolic"
        );

        var back_button = new Gtk.Button.from_icon_name ("byte-arrow-back-symbolic", Gtk.IconSize.MENU);
        back_button.can_focus = false;
        back_button.margin = 3;
        back_button.margin_bottom = 6;
        back_button.margin_top = 6;
        back_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        back_button.get_style_context ().add_class ("label-color-primary");

        var search_button = new Gtk.Button.from_icon_name ("edit-find-symbolic", Gtk.IconSize.MENU);
        search_button.label = _("Songs");
        search_button.can_focus = false;
        search_button.image_position = Gtk.PositionType.LEFT;
        search_button.valign = Gtk.Align.CENTER;
        search_button.halign = Gtk.Align.CENTER;
        search_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        search_button.get_style_context ().add_class ("h3");
        search_button.get_style_context ().add_class ("label-color-primary");
        search_button.always_show_image = true;
        search_button.tooltip_text = _("Search by title, artist and album");

        search_entry = new Widgets.SearchEntry ();
        search_entry.tooltip_text = _("Search by title, artist and album");
        search_entry.placeholder_text = _("Search by title, artist and album");

        var search_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        search_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        search_box.add (search_entry);
        search_box.add (new Gtk.Separator (Gtk.Orientation.HORIZONTAL));

        var search_revealer = new Gtk.Revealer ();
        search_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        search_revealer.add (search_box);
        search_revealer.reveal_child = false;

        var sort_button = new Gtk.ToggleButton ();
        sort_button.margin = 3;
        sort_button.can_focus = false;
        sort_button.add (new Gtk.Image.from_icon_name ("byte-sort-symbolic", Gtk.IconSize.MENU));
        sort_button.tooltip_text = _("Sort");
        sort_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        sort_button.get_style_context ().add_class ("sort-button");

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        header_box.pack_start (back_button, false, false, 0);
        header_box.set_center_widget (search_button);
        header_box.pack_end (sort_button, false, false, 0);

        var sort_popover = new Widgets.Popovers.Sort (sort_button);
        sort_popover.selected = Byte.settings.get_enum ("track-sort");
        sort_popover.reverse = Byte.settings.get_boolean ("track-order-reverse");
        sort_popover.radio_01_label = _("Name");
        sort_popover.radio_02_label = _("Artist");
        sort_popover.radio_03_label = _("Album");
        sort_popover.radio_04_label = _("Date Added");
        sort_popover.radio_05_label = _("Play Count");

        listbox = new Gtk.ListBox (); 
        listbox.expand = true;

        var play_button = new Gtk.Button.from_icon_name ("media-playback-start-symbolic", Gtk.IconSize.MENU);
        play_button.always_show_image = true;
        play_button.label = _("Play");
        play_button.hexpand = true;
        play_button.margin = 6;
        play_button.margin_end = 0;
        play_button.get_style_context ().add_class ("home-button");
        play_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var shuffle_button = new Gtk.Button.from_icon_name ("media-playlist-shuffle-symbolic", Gtk.IconSize.MENU);
        shuffle_button.always_show_image = true;
        shuffle_button.label = _("Shuffle");
        shuffle_button.hexpand = true;
        shuffle_button.margin = 6;
        shuffle_button.margin_start = 0;
        shuffle_button.get_style_context ().add_class ("home-button");
        shuffle_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var action_grid = new Gtk.Grid ();
        action_grid.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        action_grid.column_spacing = 6;
        action_grid.add (play_button);
        action_grid.add (shuffle_button);

        var listbox_scrolled = new Gtk.ScrolledWindow (null, null);
        listbox_scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        listbox_scrolled.expand = true;
        listbox_scrolled.add (listbox);

        stack = new Gtk.Stack ();
        stack.expand = true;
        stack.transition_type = Gtk.StackTransitionType.CROSSFADE;

        stack.add_named (listbox_scrolled, "listbox_scrolled");
        stack.add_named (alert_view, "alert_view");

        var separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
        separator.margin_start = 14;
        separator.margin_end = 9;

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.margin_bottom = 3;
        main_box.expand = true;
        main_box.pack_start (header_box, false, false, 0);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (action_grid, false, false);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (search_revealer, false, false, 0);
        main_box.pack_start (stack, true, true, 0);
        
        add (main_box);
        add_all_tracks ();
        get_realtive_time ();

        Timeout.add (200, () => {
            check_size ();
            return false;
        });

        back_button.clicked.connect (() => {
            go_back ();
        });

        search_button.clicked.connect (() => {
            if (search_revealer.reveal_child) {
                search_revealer.reveal_child = false;
                search_entry.text = "";
            } else {
                search_revealer.reveal_child = true;
                search_entry.grab_focus ();
            }            
        });

        search_entry.key_release_event.connect ((key) => {
            if (key.keyval == 65307) {
                search_revealer.reveal_child = false;
                search_entry.text = "";
            }

            return false;
        });

        search_entry.activate.connect (start_search);
        search_entry.search_changed.connect (start_search);

        sort_button.toggled.connect (() => {
            if (sort_button.active) {
                sort_popover.show_all ();
            }
        });

        sort_popover.closed.connect (() => {
            sort_button.active = false;
        });

        sort_popover.mode_changed.connect ((mode) => {
            Byte.settings.set_enum ("track-sort", mode);

            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_tracks = Byte.database.get_all_tracks_order_by (mode, Byte.settings.get_boolean ("track-order-reverse"));

            add_all_tracks ();
        });

        sort_popover.order_reverse.connect ((reverse) => {
            Byte.settings.set_boolean ("track-order-reverse", reverse); 

            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_tracks = Byte.database.get_all_tracks_order_by (
                Byte.settings.get_enum ("track-sort"), 
                Byte.settings.get_boolean ("track-order-reverse")
            );

            add_all_tracks ();
        });

        play_button.clicked.connect (() => {
            Byte.utils.set_items (
                all_tracks,
                false,
                null
            );
        });

        shuffle_button.clicked.connect (() => {
            Byte.utils.set_items (
                all_tracks,
                true,
                null
            );
        });

        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.TrackRow;
            
            Byte.utils.set_items (
                all_tracks,
                Byte.settings.get_boolean ("shuffle-mode"),
                item.track
            );
        });

        Byte.database.adden_new_track.connect ((track) => {
            Idle.add (() => {
                if (track != null) {
                    add_track (track);
                    stack.visible_child_name = "listbox_scrolled";
                }
                
                return false;
            });
        });

        listbox_scrolled.edge_reached.connect((pos)=> {
            if (pos == Gtk.PositionType.BOTTOM) {
                
                item_index = item_max;
                item_max = item_max + 100;

                if (item_max > all_tracks.size) {
                    item_max = all_tracks.size;
                }

                add_all_tracks ();
            }
        });

        Byte.database.reset_library.connect (() => {
            listbox.foreach ((widget) => {
                Idle.add (() => {
                    widget.destroy (); 
    
                    return false;
                });
            });
        });

        Byte.scan_service.sync_started.connect (() => {
            sort_button.sensitive = false;
            search_entry.sensitive = false;
        });

        Byte.scan_service.sync_finished.connect (() => {
            sort_button.sensitive = true;
            search_entry.sensitive = true;
        });
    }

    private void add_track (Objects.Track track) {
        if (track.id != 0) {
            var row = new Widgets.TrackRow (track);
        
            all_tracks.add (track);
            listbox.add (row);
            listbox.show_all ();
        }
    }

    private void start_search () {
        if (search_entry.text != "") {
            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_tracks = Byte.database.get_all_tracks_search (
                search_entry.text.down ()
            );

            if (all_tracks.size > 0) {
                stack.visible_child_name = "listbox_scrolled";
            } else {
                alert_view.description = _(
                    "No results for '%s' in Tracks. Try again.".printf (
                        search_entry.text
                    )
                );
                stack.visible_child_name = "alert_view";
            }

            add_all_tracks ();
        } else {
            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_tracks = Byte.database.get_all_tracks_order_by (
                Byte.settings.get_enum ("track-sort"), 
                Byte.settings.get_boolean ("track-order-reverse")
            );

            check_size ();

            add_all_tracks ();
        }
    }

    public void check_size () {
        if (all_tracks.size > 0) {
            stack.visible_child_name = "listbox_scrolled";
        } else {
            alert_view.title = _("No Songs");
            alert_view.description = _("Add some tracks to your selected music folder.");
            alert_view.icon_name = "dialog-information-symbolic";

            stack.visible_child_name = "alert_view";
        }
    }

    public void add_all_tracks () {
        if (item_max > all_tracks.size) {
            item_max = all_tracks.size;
        }

        for (int i = item_index; i < item_max; i++) {
            var row = new Widgets.TrackRow (all_tracks [i]);

            listbox.add (row);
            listbox.show_all ();
        }   
    }

    private void get_realtive_time () {
        /* 
        tracks_number = all_tracks.size;

        foreach (var item in all_tracks) {
            tracks_time = tracks_time + item.duration;
        }

        time_label.label = "%i Tracks - %s".printf (tracks_number, Byte.utils.get_relative_duration (tracks_time));

        print ("tracks_time: %s\n".printf (Byte.utils.get_relative_duration (tracks_time)));
        print ("size: %s\n".printf (tracks_number.to_string ()));
        */
    }
}