public class Views.Playlist : Gtk.EventBox {
    public Gtk.Entry title_entry;
    private Gtk.TextView note_text;
    private Gtk.Label note_placeholder;
    private Gtk.Stack right_stack;

    private Gtk.Label title_label;
    private Gtk.Label note_label;
    private Gtk.Label time_label;
    private Gtk.Label update_relative_label;

    private Gtk.ListBox listbox;

    private string cover_path;
    private Widgets.Cover image_cover;

    public signal void go_back (string page);
    public string back_page { set; get; }

    private Gee.ArrayList<Objects.Track?> all_tracks;

    public Objects.Playlist _playlist { get; set; }
    public Objects.Playlist playlist {
        set {
            if (value != null) {
                _playlist = value;

                title_label.label = _playlist.title;
                title_entry.text = _playlist.title;

                note_label.label = _playlist.note;
                note_text.buffer.text = _playlist.note;

                if (_playlist.note != "") {
                    note_placeholder.visible = false;
                }

                update_relative_label.label = Byte.utils.get_relative_datetime (_playlist.date_updated);

                if (_playlist.note == "") {
                    note_label.visible = false;
                }

                try {
                    cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("playlist-%i.jpg").printf (_playlist.id));
                    var pixbuf = new Gdk.Pixbuf.from_file_at_size (cover_path, 128, 128);
                    image_cover.pixbuf = pixbuf;
                } catch (Error e) {
                    var pixbuf = new Gdk.Pixbuf.from_file_at_size ("/usr/share/com.github.alainm23.byte/album-default-cover.svg", 128, 128);
                    image_cover.pixbuf = pixbuf;
                }

                listbox.foreach ((widget) => {
                    widget.destroy ();
                });

                if (Byte.scan_service.is_sync == false) {
                    all_tracks = new Gee.ArrayList<Objects.Track?> ();
                    all_tracks = Byte.database.get_all_tracks_by_playlist (
                        _playlist.id,
                        Byte.settings.get_enum ("playlist-sort"),
                        Byte.settings.get_boolean ("playlist-order-reverse")
                    );

                    foreach (var item in all_tracks) {
                        print ("Track: %s\n".printf (item.title));
                        var row = new Widgets.TrackRow (item);
                        listbox.add (row);
                    }

                    listbox.show_all ();

                    time_label.label = _("%i songs").printf (all_tracks.size);
                }
            }
        }
    }

    public Playlist () {}

    construct {
        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);
        get_style_context ().add_class ("w-round");

        var back_button = new Gtk.Button.from_icon_name ("byte-arrow-back-symbolic", Gtk.IconSize.MENU);
        back_button.can_focus = false;
        back_button.margin = 3;
        back_button.margin_bottom = 6;
        back_button.margin_top = 6;
        back_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        back_button.get_style_context ().add_class ("label-color-primary");

        var center_label = new Gtk.Label (_("Playlist"));
        center_label.use_markup = true;
        center_label.valign = Gtk.Align.CENTER;
        center_label.get_style_context ().add_class ("h3");
        center_label.get_style_context ().add_class ("label-color-primary");

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
        header_box.set_center_widget (center_label);
        header_box.pack_end (sort_button, false, false, 0);

        var sort_popover = new Widgets.Popovers.Sort (sort_button);
        sort_popover.selected = Byte.settings.get_enum ("playlist-sort");
        sort_popover.reverse = Byte.settings.get_boolean ("playlist-order-reverse");
        sort_popover.radio_01_label = _("Name");
        sort_popover.radio_02_label = _("Artist");
        sort_popover.radio_03_label = _("Album");
        sort_popover.radio_04_label = _("Date Added");
        sort_popover.radio_05_label = _("Play Count");

        image_cover = new Widgets.Cover.with_default_icon (128, "playlist");
        image_cover.halign = Gtk.Align.START;
        image_cover.valign = Gtk.Align.START;

        title_label = new Gtk.Label (null);
        title_label.wrap = true;
        title_label.wrap_mode = Pango.WrapMode.CHAR;
        title_label.justify = Gtk.Justification.FILL;
        title_label.get_style_context ().add_class ("font-bold");
        title_label.get_style_context ().add_class ("h2");
        title_label.halign = Gtk.Align.START;

        note_label = new Gtk.Label (null);
        note_label.wrap = true;
        note_label.margin_bottom = 6;
        note_label.margin_start = 12;
        note_label.margin_end = 12;
        note_label.wrap_mode = Pango.WrapMode.WORD;
        note_label.justify = Gtk.Justification.FILL;
        note_label.halign = Gtk.Align.START;

        time_label = new Gtk.Label (null);
        time_label.get_style_context ().add_class ("h3");
        time_label.wrap = true;
        time_label.justify = Gtk.Justification.FILL;
        time_label.wrap_mode = Pango.WrapMode.CHAR;
        time_label.halign = Gtk.Align.START;

        var menu_icon = new Gtk.Image ();
        menu_icon.gicon = new ThemedIcon ("view-more-symbolic");
        menu_icon.pixel_size = 14;

        var menu_button = new Gtk.MenuButton ();
        menu_button.can_focus = false;
        menu_button.valign = Gtk.Align.CENTER;
        menu_button.tooltip_text = _("Edit Name and Appearance");
        menu_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        menu_button.get_style_context ().add_class (Gtk.STYLE_CLASS_DIM_LABEL);
        menu_button.get_style_context ().add_class ("label-color-primary");
        menu_button.image = menu_icon;

        /* Items */
        var edit_menuitem = new Widgets.ModelButton (_("Edit Details"), "edit-symbolic", _("Edit Details"));
        var cover_menuitem = new Widgets.ModelButton (_("Set new Cover"), "image-x-generic-symbolic", _("Set new Cover"));
        var delete_menuitem = new Widgets.ModelButton (_("Delete"), "edit-delete-symbolic", _("Delete"));

        var menu_grid = new Gtk.Grid ();
        menu_grid.margin_top = 6;
        menu_grid.margin_bottom = 6;
        menu_grid.orientation = Gtk.Orientation.VERTICAL;
        menu_grid.width_request = 165;

        menu_grid.add (cover_menuitem);
        menu_grid.add (edit_menuitem);
        menu_grid.add (delete_menuitem);

        menu_grid.show_all ();

        var menu_popover = new Gtk.Popover (null);
        menu_popover.add (menu_grid);
        menu_button.popover = menu_popover;

        var h_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        h_box.hexpand = true;
        h_box.pack_start (time_label, false, false, 0);
        h_box.pack_end (menu_button, false, false, 0);

        var update_label = new Gtk.Label (_("Updated"));
        update_label.halign = Gtk.Align.START;
        update_label.ellipsize = Pango.EllipsizeMode.END;
        update_label.get_style_context ().add_class ("font-bold");

        update_relative_label = new Gtk.Label (null);
        update_relative_label.halign = Gtk.Align.START;
        update_relative_label.ellipsize = Pango.EllipsizeMode.END;

        var play_button = new Gtk.Button.from_icon_name ("media-playback-start-symbolic", Gtk.IconSize.MENU);
        play_button.always_show_image = true;
        play_button.label = _("Play");
        play_button.hexpand = true;
        play_button.get_style_context ().add_class ("home-button");
        play_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var shuffle_button = new Gtk.Button.from_icon_name ("media-playlist-shuffle-symbolic", Gtk.IconSize.MENU);
        shuffle_button.always_show_image = true;
        shuffle_button.label = _("Shuffle");
        shuffle_button.hexpand = true;
        shuffle_button.get_style_context ().add_class ("home-button");
        shuffle_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var action_grid = new Gtk.Grid ();
        action_grid.margin = 6;
        action_grid.column_spacing = 12;
        action_grid.add (play_button);
        action_grid.add (shuffle_button);

        var detail_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        detail_box.margin_bottom = 3;
        detail_box.get_style_context ().add_class (Granite.STYLE_CLASS_WELCOME);
        detail_box.pack_start (title_label, false, false, 6);
        detail_box.pack_start (update_label, false, false, 0);
        detail_box.pack_start (update_relative_label, false, false, 0);
        detail_box.pack_end (h_box, false, false, 6);

        // Edit view
        title_entry = new Gtk.Entry ();
        title_entry.margin_top = 6;
        title_entry.placeholder_text = _("New Playlist");
        title_entry.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        title_entry.valign = Gtk.Align.START;

        note_text = new Gtk.TextView ();
        note_text.height_request = 35;
        note_text.wrap_mode = Gtk.WrapMode.WORD_CHAR;
        note_text.hexpand = true;

        var note_scrolled = new Gtk.ScrolledWindow (null, null);
        note_scrolled.margin_start = 3;
        note_scrolled.add (note_text);

        note_placeholder = new Gtk.Label (_("Add Description"));
        note_placeholder.opacity = 0.6;
        note_text.add (note_placeholder);

        var update_button = new Gtk.Button.with_label (_("Save"));
        update_button.halign = Gtk.Align.END;
        update_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        update_button.get_style_context ().add_class ("quick-find-cancel");

        var edit_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        edit_box.pack_start (title_entry, false, false, 0);
        edit_box.pack_start (note_scrolled, true, true, 6);
        edit_box.pack_end (update_button, false, false, 0);

        right_stack = new Gtk.Stack ();
        right_stack.transition_type = Gtk.StackTransitionType.CROSSFADE;

        right_stack.add_named (detail_box, "detail_box");
        right_stack.add_named (edit_box, "edit_box");

        var album_grid = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        album_grid.hexpand = true;
        album_grid.margin = 6;
        album_grid.add (image_cover);
        album_grid.add (right_stack);

        listbox = new Gtk.ListBox ();
        listbox.expand = true;

        var separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
        separator.margin_start = 6;
        separator.margin_end = 6;

        var separator_2 = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
        separator_2.margin_start = 6;
        separator_2.margin_end = 6;

        var scrolled_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        scrolled_box.expand = true;
        scrolled_box.pack_start (album_grid, false, false, 0);
        scrolled_box.pack_start (note_label, false, false, 0);
        scrolled_box.pack_start (separator, false, false, 0);
        scrolled_box.pack_start (action_grid, false, false, 0);
        scrolled_box.pack_start (separator_2, false, false, 0);
        scrolled_box.pack_start (listbox, true, true, 0);

        var main_scrolled = new Gtk.ScrolledWindow (null, null);
        main_scrolled.margin_bottom = 48;
        main_scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        main_scrolled.expand = true;
        main_scrolled.add (scrolled_box);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.expand = true;
        main_box.pack_start (header_box, false, false, 0);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (main_scrolled, true, true, 0);

        add (main_box);

        back_button.clicked.connect (() => {
            go_back (back_page);
        });

        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.TrackRow;

            Byte.utils.set_items (
                all_tracks,
                Byte.settings.get_boolean ("shuffle-mode"),
                item.track
            );
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

        sort_button.toggled.connect (() => {
            if (sort_button.active) {
                sort_popover.show_all ();
            }
        });

        sort_popover.closed.connect (() => {
            sort_button.active = false;
        });

        sort_popover.mode_changed.connect ((mode) => {
            Byte.settings.set_enum ("playlist-sort", mode);

            listbox.foreach ((widget) => {
                widget.destroy ();
            });

            all_tracks = new Gee.ArrayList<Objects.Track?> ();
            all_tracks = Byte.database.get_all_tracks_by_playlist (
                _playlist.id,
                Byte.settings.get_enum ("playlist-sort"),
                Byte.settings.get_boolean ("playlist-order-reverse")
            );

            foreach (var item in all_tracks) {
                var row = new Widgets.TrackRow (item);
                listbox.add (row);
            }

            listbox.show_all ();
        });

        sort_popover.order_reverse.connect ((reverse) => {
            Byte.settings.set_boolean ("playlist-order-reverse", reverse);

            listbox.foreach ((widget) => {
                widget.destroy ();
            });

            all_tracks = new Gee.ArrayList<Objects.Track?> ();
            all_tracks = Byte.database.get_all_tracks_by_playlist (
                _playlist.id,
                Byte.settings.get_enum ("playlist-sort"),
                Byte.settings.get_boolean ("playlist-order-reverse")
            );

            foreach (var item in all_tracks) {
                var row = new Widgets.TrackRow (item);
                listbox.add (row);
            }

            listbox.show_all ();
        });

        delete_menuitem.clicked.connect (() => {
            menu_popover.popdown ();

            var message_dialog = new Granite.MessageDialog.with_image_from_icon_name (
                _("Delete from library?"),
                _("Are you sure you want to delete <b>%s</b> from your library?").printf (_playlist.title),
                "dialog-warning",
                Gtk.ButtonsType.CANCEL
            );

            var set_button = new Gtk.Button.with_label (_("Delete"));
            set_button.get_style_context ().add_class (Gtk.STYLE_CLASS_DESTRUCTIVE_ACTION);
            message_dialog.add_action_widget (set_button, Gtk.ResponseType.ACCEPT);

            message_dialog.show_all ();

            if (message_dialog.run () == Gtk.ResponseType.ACCEPT) {
                //Byte.database.remove_from_library (track);
                if (Byte.database.remove_playlist_from_library (_playlist)) {
                    go_back (back_page);
                }
            }

            message_dialog.destroy ();
        });

        edit_menuitem.clicked.connect (() => {
            menu_popover.popdown ();

            if (right_stack.visible_child_name  == "detail_box") {
                right_stack.visible_child_name = "edit_box";
                note_label.visible = false;
            } else {
                right_stack.visible_child_name = "detail_box";
                note_label.visible = true;
            }
        });

        cover_menuitem.clicked.connect (() => {
            menu_popover.popdown ();

            var new_cover = Byte.utils.choose_new_cover ();
            if (new_cover != null) {
                cover_path = new_cover;
                try {
                    var pixbuf = Byte.utils.align_and_scale_pixbuf (
                        new Gdk.Pixbuf.from_file (cover_path),
                        128
                    );

                    image_cover.pixbuf = pixbuf;
                    string playlist_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("playlist-%i.jpg").printf (_playlist.id));

                    if (pixbuf.save (playlist_path, "jpeg", "quality", "100")) {
                        Byte.database.updated_playlist_cover (_playlist.id);
                    }
                } catch (Error err) {
                    warning (err.message);
                }
            }
        });

        note_text.focus_in_event.connect (() => {
            Byte.instance.toggle_playing_action_enabled (false);
            note_placeholder.visible = false;
            return false;
        });

        note_text.focus_out_event.connect (() => {
            Byte.instance.toggle_playing_action_enabled (true);

            if (note_text.buffer.text == "") {
                note_placeholder.visible = true;
            }
            return false;
        });

        note_text.buffer.changed.connect (() => {
            Byte.instance.toggle_playing_action_enabled (false);
        });

        title_entry.focus_in_event.connect (() => {
            Byte.instance.toggle_playing_action_enabled (false);
            return false;
        });

        title_entry.focus_out_event.connect (() => {
            Byte.instance.toggle_playing_action_enabled (true);
            return false;
        });

        title_entry.changed.connect (() => {
            Byte.instance.toggle_playing_action_enabled (false);
        });
        
        title_entry.activate.connect (update);
        update_button.clicked.connect (update);
    }

    private void update () {
        if (title_entry.text != "") {
            _playlist.title = title_entry.text;
            _playlist.note = note_text.buffer.text;
            _playlist.date_updated = new GLib.DateTime.now_local ().to_string ();

            title_label.label = _playlist.title;
            note_label.label = _playlist.note;
            update_relative_label.label = Byte.utils.get_relative_datetime (_playlist.date_updated);

            right_stack.visible_child_name = "detail_box";
            
            if (_playlist.note != "") {
                note_label.visible = true;
            } else {
                note_label.visible = false;
            }

            Byte.database.update_playlist (_playlist);
        }
    }
}