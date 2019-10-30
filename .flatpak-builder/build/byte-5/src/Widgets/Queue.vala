public class Widgets.Queue : Gtk.Revealer {
    private Widgets.Cover image_cover;
    private Gtk.ListBox listbox;
    private Gee.ArrayList<Objects.Track?> items;
    private int item_index;
    private int item_max;
    public Queue () {
        transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        valign = Gtk.Align.END;
        halign = Gtk.Align.CENTER;
        reveal_child = false;
    }

    construct {
        items = new Gee.ArrayList<Objects.Track?> ();

        image_cover = new Widgets.Cover.with_default_icon (24, "track");

        var next_track_label = new Gtk.Label ("<small>%s</small>".printf (_("Next track")));
        next_track_label.valign = Gtk.Align.END;
        next_track_label.halign = Gtk.Align.START;
        next_track_label.use_markup = true;
        next_track_label.get_style_context ().add_class ("label-color-primary");
        next_track_label.get_style_context ().add_class ("font-bold");

        var next_track_name = new Gtk.Label (null);
        next_track_name.valign = Gtk.Align.START;
        next_track_name.halign = Gtk.Align.START;
        next_track_name.use_markup = true;
        next_track_name.max_width_chars = 31;
        next_track_name.ellipsize = Pango.EllipsizeMode.END;

        var view_button = new Gtk.Button.with_label (_("View all"));
        view_button.hexpand = true;
        view_button.halign = Gtk.Align.END;
        view_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        view_button.get_style_context ().add_class ("button-color");
        view_button.can_focus = false;
        view_button.valign = Gtk.Align.CENTER;

        var next_track_grid = new Gtk.Grid ();
        next_track_grid.column_spacing = 3;
        next_track_grid.attach (image_cover, 0, 0, 1, 2);
        next_track_grid.attach (next_track_label, 1, 0, 1, 1);
        next_track_grid.attach (next_track_name, 1, 1, 1, 1);
        next_track_grid.attach (view_button, 2, 0, 2, 2);

        var top_eventbox = new Gtk.EventBox ();
        top_eventbox.add (next_track_grid);

        /*
            Notifications
        */

        var notification_image = new Gtk.Image ();
        notification_image.gicon = new ThemedIcon ("byte-favorite-symbolic");
        notification_image.pixel_size = 16;
        notification_image.margin_top = 1;
        notification_image.valign = Gtk.Align.CENTER;
        notification_image.halign = Gtk.Align.CENTER;
        notification_image.get_style_context ().add_class ("label-color-primary");

        var notification_primary_label = new Gtk.Label (null);
        notification_primary_label.margin_start = 6;
        notification_primary_label.valign = Gtk.Align.END;
        notification_primary_label.halign = Gtk.Align.START;
        notification_primary_label.use_markup = true;
        notification_primary_label.get_style_context ().add_class ("label-color-primary");
        notification_primary_label.get_style_context ().add_class ("font-bold");

        var notification_secondary_label = new Gtk.Label (null);
        notification_secondary_label.margin_start = 6;
        notification_secondary_label.valign = Gtk.Align.START;
        notification_secondary_label.halign = Gtk.Align.START;
        notification_secondary_label.use_markup = true;
        notification_secondary_label.max_width_chars = 31;
        notification_secondary_label.ellipsize = Pango.EllipsizeMode.END;

        var notification_grid = new Gtk.Grid ();
        notification_grid.margin_start = 6;
        notification_grid.column_spacing = 3;
        notification_grid.attach (notification_image, 0, 0, 1, 2);
        notification_grid.attach (notification_primary_label, 1, 0, 1, 1);
        notification_grid.attach (notification_secondary_label, 1, 1, 1, 1);

        /*
            Sync
        */

        var sync_image = new Gtk.Image ();
        sync_image.gicon = new ThemedIcon ("emblem-synchronizing-symbolic");
        sync_image.pixel_size = 24;
        sync_image.margin_top = 1;
        sync_image.valign = Gtk.Align.CENTER;
        sync_image.halign = Gtk.Align.CENTER;
        sync_image.get_style_context ().add_class ("sync-image");

        var sync_label = new Gtk.Label ("<small>%s</small>".printf (_("Syncing Library…")));
        sync_label.valign = Gtk.Align.END;
        sync_label.halign = Gtk.Align.START;
        sync_label.use_markup = true;
        sync_label.get_style_context ().add_class ("search-title");
        sync_label.get_style_context ().add_class ("font-bold");

        var sync_progressbar = new Gtk.ProgressBar ();
        sync_progressbar.valign = Gtk.Align.START;
        sync_progressbar.hexpand = true;
        sync_progressbar.get_style_context ().add_class ("label-white");

        var sync_grid = new Gtk.Grid ();
        sync_grid.valign = Gtk.Align.CENTER;
        sync_grid.row_spacing = 3;
        sync_grid.column_spacing = 6;
        sync_grid.margin_start = 6;
        sync_grid.margin_end = 6;
        sync_grid.attach (sync_image, 0, 0, 1, 2);
        sync_grid.attach (sync_label, 1, 0, 1, 1);
        sync_grid.attach (sync_progressbar, 1, 1, 1, 1);

        var top_stack = new Gtk.Stack ();
        top_stack.expand = true;
        top_stack.transition_type = Gtk.StackTransitionType.SLIDE_DOWN;

        top_stack.add_named (top_eventbox, "top_eventbox");
        top_stack.add_named (sync_grid, "sync_grid");
        top_stack.add_named (notification_grid, "notification_grid");

        var top_revealer = new Gtk.Revealer ();
        top_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_DOWN;
        top_revealer.expand = true;
        top_revealer.add (top_stack);
        top_revealer.reveal_child = true;

        var title_label = new Gtk.Label ("Up Next");
        title_label.halign = Gtk.Align.START;

        var hide_button = new Gtk.Button.with_label (_("Hide"));
        hide_button.can_focus = false;
        hide_button.valign = Gtk.Align.CENTER;
        hide_button.valign = Gtk.Align.CENTER;
        hide_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var mode_button = new Granite.Widgets.ModeButton ();
        mode_button.get_style_context ().add_class ("mode-button");
        mode_button.margin = 3;
        mode_button.append_text (_("Up Next"));
        //mode_button.append_text (_("History"));
        //mode_button.append_text (_("Lyrics"));
        mode_button.selected = 0;

        var title_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        title_box.get_style_context ().add_class ("queue-title");
        title_box.set_center_widget (mode_button);
        title_box.pack_end (hide_button, false, false, 0);

        var title_eventbox = new Gtk.EventBox ();
        title_eventbox.add (title_box);

        var title_revealer = new Gtk.Revealer ();
        title_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        title_revealer.expand = true;
        title_revealer.add (title_eventbox);
        title_revealer.reveal_child = false;

        listbox = new Gtk.ListBox ();
        listbox.expand = true;

        var queue_scrolled = new Gtk.ScrolledWindow (null, null);
        queue_scrolled.margin_bottom = 6;
        queue_scrolled.margin_top = 3;
        queue_scrolled.height_request = 275;
        queue_scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        queue_scrolled.expand = true;
        queue_scrolled.add (listbox);

        var tracks_revealer = new Gtk.Revealer ();
        tracks_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        tracks_revealer.expand = true;
        tracks_revealer.add (queue_scrolled);
        tracks_revealer.reveal_child = false;

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.margin_bottom = 6;
        main_box.width_request = 325;
        main_box.get_style_context ().add_class ("queue");
        main_box.pack_start (title_revealer, true, false, 0);
        main_box.pack_start (top_revealer, false, false, 0);
        main_box.pack_start (tracks_revealer, true, true, 0);

        //var stack = new Gtk.Stack ();
        //stack.expand = true;
        //stack.transition_type = Gtk.StackTransitionType.SLIDE_LEFT_RIGHT;

        //stack.add_named (welcome_scrolled, "welcome_view");
        //stack.add_named (library_view, "library_view");

        add (main_box);

        Byte.utils.play_items.connect ((_items, _track) => {
            listbox.foreach ((widget) => {
                widget.destroy ();
            });

            items = _items;

            item_index = 0;
            item_max = 50;

            if (item_max > items.size) {
                item_max = items.size;
            }

            add_all_items (items);

            if (_track == null) {
                Byte.player.set_track (items [0]);
            } else {
                Byte.player.set_track (_track);

                int current_index = Byte.utils.get_track_index_by_id (_track.id, items);

                listbox.set_filter_func ((row) => {
                    var index = row.get_index ();

                    return index >= current_index;
                });
            }

            if (Byte.scan_service.is_sync) {
                top_stack.visible_child_name = "sync_grid";
            } else {
                top_stack.visible_child_name = "top_eventbox";
            }
        });

        Byte.player.current_track_changed.connect ((track) => {
            int current_index = Byte.utils.get_track_index_by_id (track.id, items);

            listbox.set_filter_func ((row) => {
                var index = row.get_index ();
                return index >= current_index;
            });

            Objects.Track? next_track = Byte.utils.get_next_track (track);

            if (next_track != null) {
                reveal_child = true;

                next_track_name.label = "%s <b>by</b> %s".printf (next_track.title, next_track.artist_name);
                next_track_grid.tooltip_text = "%s - %s".printf (next_track.artist_name, next_track.title);

                try {
                    var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (next_track.id));
                    image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (cover_path, 27, 27);
                } catch (Error e) {
                    image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size ("/usr/share/com.github.alainm23.byte/track-default-cover.svg", 27, 27);
                    stderr.printf ("Error setting default avatar icon: %s ", e.message);
                }
            } else {
                reveal_child = false;
            }
        });

        Byte.player.state_changed.connect ((state) => {
            if (state == Gst.State.PLAYING) {
                if (Byte.player.current_track != null) {
                    reveal_child = true;
                }
            } else if (state == Gst.State.NULL) {
                reveal_child = false;
            }
        });

        Byte.utils.update_next_track.connect (() => {
            var next_track = Byte.utils.get_next_track (Byte.player.current_track);

            if (next_track != null) {
                next_track_name.label = _("%s <b>by</b> %s".printf (next_track.title, next_track.artist_name));
                next_track_grid.tooltip_text = "%s - %s".printf (next_track.artist_name, next_track.title);

                try {
                    var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (next_track.id));
                    image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (cover_path, 27, 27);
                } catch (Error e) {
                    image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size ("/usr/share/com.github.alainm23.byte/track-default-cover.svg", 27, 27);
                    stderr.printf ("Error setting default avatar icon: %s ", e.message);
                }
            } else {
                reveal_child = false;
            }
        });

        Byte.player.mode_changed.connect ((mode) => {
            if (mode == "radio") {
                reveal_child = false;
            } else {
                reveal_child = true;
            }
        });

        Byte.utils.add_next_track.connect ((_items) => {
            listbox.foreach ((widget) => {
                widget.destroy ();
            });

            items = _items;

            item_index = 0;
            item_max = 50;

            if (item_max > items.size) {
                item_max = items.size;
            }

            add_all_items (items);

            Byte.utils.update_next_track ();
        });

        Byte.utils.add_last_track.connect ((_items) => {
            listbox.foreach ((widget) => {
                widget.destroy ();
            });

            items = _items;

            item_index = 0;
            item_max = 50;

            if (item_max > items.size) {
                item_max = items.size;
            }

            add_all_items (items);

            Byte.utils.update_next_track ();
        });

        queue_scrolled.edge_reached.connect((pos)=> {
            if (pos == Gtk.PositionType.BOTTOM) {

                item_index = item_max;
                item_max = item_max + 50;

                if (item_max > items.size) {
                    item_max = items.size;
                }

                add_all_items (items);
            }
        });

        hide_button.clicked.connect (() => {
            title_revealer.reveal_child = false;
            top_revealer.reveal_child = true;
            tracks_revealer.reveal_child = false;
        });

        view_button.clicked.connect (() => {
            title_revealer.reveal_child = true;
            tracks_revealer.reveal_child = true;
            top_revealer.reveal_child = false;
        });

        top_eventbox.event.connect ((event) => {
            if (event.type == Gdk.EventType.BUTTON_PRESS) {
                title_revealer.reveal_child = true;
                tracks_revealer.reveal_child = true;
                top_revealer.reveal_child = false;
            }

            return false;
        });

        title_eventbox.event.connect ((event) => {
            if (event.type == Gdk.EventType.BUTTON_PRESS) {
                title_revealer.reveal_child = false;
                top_revealer.reveal_child = true;
                tracks_revealer.reveal_child = false;
            }

            return false;
        });

        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.TrackQueueRow;

            Byte.player.set_track (item.track);
            int current_index = Byte.utils.get_track_index_by_id (item.track.id, items);

            listbox.set_filter_func ((row) => {
                var index = row.get_index ();

                return index >= current_index;
            });
        });

        Byte.database.updated_track_favorite.connect ((track, favorite) => {
            if (favorite == 1) {
                top_stack.visible_child_name = "notification_grid";

                notification_primary_label.label = "<small>"+ _("Add Favorite") + "</small>";
                notification_secondary_label.label = track.title;
                notification_image.get_style_context ().add_class ("active");

                Timeout.add (1000, () => {
                    top_stack.visible_child_name = "top_eventbox";
                    notification_image.get_style_context ().remove_class ("active");

                    return false;
                });
            }
        });

        Byte.scan_service.sync_started.connect (() => {
            top_stack.visible_child_name = "sync_grid";
            reveal_child = true;
        });

        Byte.scan_service.sync_finished.connect (() => {
            if (Byte.player.current_track != null) {
                top_stack.visible_child_name = "top_eventbox";
            } else {
                reveal_child = false;
            }
        });

        Byte.scan_service.sync_progress.connect ((fraction) => {
            sync_progressbar.fraction = fraction;
        });
    }

    private void add_all_items (Gee.ArrayList<Objects.Track?> items) {
        for (int i = item_index; i < item_max; i++) {
            var row = new Widgets.TrackQueueRow (items [i]);

            row.remove_track.connect ((id) => {
                Byte.utils.remove_track (id);

                GLib.Timeout.add (250, () => {
                    row.destroy ();
                    return GLib.Source.REMOVE;
                });
            });

            listbox.add (row);
            listbox.show_all ();
        }
    }
}
