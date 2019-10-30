public class Widgets.TrackAlbumRow : Gtk.ListBoxRow {
    public Objects.Track track { get; construct; }

    private Gtk.Label track_label;
    private Gtk.Label title_label;
    private Gtk.Label duration_label;

    Gtk.Menu playlists;
    private Gtk.Menu menu = null;
    public TrackAlbumRow (Objects.Track track) {
        Object (
            track: track
        );
    }

    construct {
        get_style_context ().add_class ("album-row");

        track_label = new Gtk.Label ("%i".printf (track.track));
        track_label.get_style_context ().add_class ("label-color-primary");
        track_label.halign = Gtk.Align.START;
        track_label.width_chars = 4;

        var playing_icon = new Gtk.Image ();
        playing_icon.gicon = new ThemedIcon ("audio-volume-high-symbolic");
        playing_icon.get_style_context ().add_class ("label-color-primary");
        playing_icon.pixel_size = 14;

        var playing_stack = new Gtk.Stack ();
        playing_stack.transition_type = Gtk.StackTransitionType.CROSSFADE;
        playing_stack.add_named (track_label, "track_label");
        playing_stack.add_named (playing_icon, "playing_icon");

        title_label = new Gtk.Label (track.title);
        title_label.ellipsize = Pango.EllipsizeMode.END;
        title_label.max_width_chars = 40;
        title_label.halign = Gtk.Align.START;

        duration_label = new Gtk.Label (Byte.utils.get_formated_duration (track.duration));

        var options_button = new Gtk.ToggleButton ();
        options_button.can_focus = false;
        options_button.add (new Gtk.Image.from_icon_name ("view-more-horizontal-symbolic", Gtk.IconSize.MENU));
        options_button.tooltip_text = _("Options");
        options_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        options_button.get_style_context ().add_class ("options-button");
        options_button.get_style_context ().remove_class ("button");

        var options_stack = new Gtk.Stack ();
        options_stack.hexpand = true;
        options_stack.halign = Gtk.Align.END;
        options_stack.transition_type = Gtk.StackTransitionType.CROSSFADE;

        options_stack.add_named (duration_label, "duration_label");
        options_stack.add_named (options_button, "options_button");

        var main_grid = new Gtk.Grid ();
        main_grid.hexpand = true;
        main_grid.margin = 6;
        main_grid.margin_end = 12;
        main_grid.margin_start = 0;
        main_grid.column_spacing = 6;
        main_grid.add (playing_stack);
        main_grid.add (title_label);
        main_grid.add (options_stack);

        var separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
        separator.margin_start = 6;
        separator.margin_end = 6;

        var grid = new Gtk.Grid ();
        grid.hexpand = true;
        grid.orientation = Gtk.Orientation.VERTICAL;
        grid.add (separator);
        grid.add (main_grid);

        var eventbox = new Gtk.EventBox ();
        eventbox.add_events (Gdk.EventMask.ENTER_NOTIFY_MASK | Gdk.EventMask.LEAVE_NOTIFY_MASK);
        eventbox.add (grid);

        add (eventbox);

        if (Byte.player.current_track != null && track.id == Byte.player.current_track.id) {
            title_label.get_style_context ().add_class ("label-color-primary");
            duration_label.get_style_context ().add_class ("label-color-primary");

            Timeout.add (150, () => {
                playing_stack.visible_child_name = "playing_icon";
                return false;
            });
        }

        Byte.player.current_track_changed.connect ((current_track) => {
            if (track.id == current_track.id) {
                playing_stack.visible_child_name = "playing_icon";
                title_label.get_style_context ().add_class ("label-color-primary");
                duration_label.get_style_context ().add_class ("label-color-primary");
            } else {
                playing_stack.visible_child_name = "track_label";
                title_label.get_style_context ().remove_class ("label-color-primary");
                duration_label.get_style_context ().remove_class ("label-color-primary");
            }
        });

        Byte.database.updated_track_favorite.connect ((_track, favorite) => {
            if (track.id == _track.id) {
                track.is_favorite = favorite;
            }
        });

        eventbox.enter_notify_event.connect ((event) => {
            options_stack.visible_child_name = "options_button";

            return false;
        });

        eventbox.leave_notify_event.connect ((event) => {
            if (event.detail == Gdk.NotifyType.INFERIOR) {
                return false;
            }

            options_stack.visible_child_name = "duration_label";

            return false;
        });

        button_press_event.connect ((sender, evt) => {
            if (evt.type == Gdk.EventType.BUTTON_PRESS && evt.button == 3) {
                activate_menu ();
                return true;
            }

            return false;
        });

        options_button.clicked.connect (activate_menu);

        Byte.database.removed_track.connect ((track_id) => {
            if (track_id == track.id) {
                destroy ();
            }
        });
    }

    private void activate_menu () {
        if (menu == null) {
            build_context_menu (track);
        }

        foreach (var child in playlists.get_children ()) {
            child.destroy ();
        }

        if (Byte.scan_service.is_sync == false) {
            var all_items = Byte.database.get_all_playlists ();

            var item = new Gtk.MenuItem.with_label (_ ("Create New Playlist"));
            item.get_style_context ().add_class ("track-options");
            item.get_style_context ().add_class ("css-item");
            item.activate.connect (() => {
                var new_playlist = Byte.database.create_new_playlist ();
                Byte.database.insert_track_into_playlist (new_playlist, track.id);
            });
            playlists.add (item);

            foreach (var playlist in all_items) {
                item = new Gtk.MenuItem.with_label (playlist.title);
                item.get_style_context ().add_class ("track-options");
                item.get_style_context ().add_class ("css-item");
                item.activate.connect (() => {
                    Byte.database.insert_track_into_playlist (playlist, track.id);
                });
                playlists.add (item);
            }
            playlists.show_all ();
        }

        menu.popup_at_pointer (null);
    }

    private void build_context_menu (Objects.Track track) {
        menu = new Gtk.Menu ();
        menu.get_style_context ().add_class ("view");

        var primary_label = new Gtk.Label (track.title);
        primary_label.get_style_context ().add_class ("font-bold");
        primary_label.ellipsize = Pango.EllipsizeMode.END;
        primary_label.max_width_chars = 25;
        primary_label.halign = Gtk.Align.START;

        var secondary_label = new Gtk.Label ("%s - %s".printf (track.artist_name, track.album_title));
        secondary_label.halign = Gtk.Align.START;
        secondary_label.max_width_chars = 25;
        secondary_label.ellipsize = Pango.EllipsizeMode.END;

        var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track.id));
        var image_cover = new Gtk.Image ();
        image_cover.halign = Gtk.Align.START;
        image_cover.valign = Gtk.Align.START;
        try {
            image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (cover_path, 38, 38);
        } catch (Error e) {
            image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size ("/usr/share/com.github.alainm23.byte/track-default-cover.svg", 38, 38);
        }

        var track_grid = new Gtk.Grid ();
        track_grid.width_request = 185;
        track_grid.hexpand = false;
        track_grid.halign = Gtk.Align.START;
        track_grid.valign = Gtk.Align.CENTER;
        track_grid.column_spacing = 6;
        track_grid.attach (image_cover, 0, 0, 1, 2);
        track_grid.attach (primary_label, 1, 0, 1, 1);
        track_grid.attach (secondary_label, 1, 1, 1, 1);

        var track_menu = new Gtk.MenuItem ();
        track_menu.get_style_context ().add_class ("track-options");
        track_menu.get_style_context ().add_class ("css-item");
        track_menu.right_justified = true;
        track_menu.add (track_grid);

        var play_menu = new Widgets.MenuItem (_("Play"), "media-playback-start-symbolic", _("Play"));
        var play_next_menu = new Widgets.MenuItem (_("Play Next"), "byte-play-next-symbolic", _("Play Next"));
        var play_last_menu = new Widgets.MenuItem (_("Play Later"), "byte-play-later-symbolic", _("Play Later"));

        var add_playlist_menu = new Widgets.MenuItem (_("Add to Playlist"), "zoom-in-symbolic", _("Add to Playlist"));
        playlists = new Gtk.Menu ();
        playlists.get_style_context ().add_class ("view");
        add_playlist_menu.set_submenu (playlists);

        var edit_menu = new Widgets.MenuItem (_("Edit Song Info…"), "edit-symbolic", _("Edit Song Info…"));

        var favorite_menu = new Widgets.MenuItem (_("Love"), "byte-favorite-symbolic", _("Love"));
        var no_favorite_menu = new Widgets.MenuItem (_("Dislike"), "byte-no-favorite-symbolic", _("Dislike"));

        var remove_db_menu = new Widgets.MenuItem (_("Delete from library"), "user-trash-symbolic", _("Delete from library"));
        var remove_file_menu = new Widgets.MenuItem (_("Delete from file"), "user-trash-symbolic", _("Delete from file"));
        var remove_playlist_menu = new Widgets.MenuItem (_("Remove from playlist"), "zoom-out-symbolic", _("Remove from playlist"));

        menu.add (track_menu);
        menu.add (new Gtk.SeparatorMenuItem ());
        menu.add (play_menu);
        menu.add (play_next_menu);
        menu.add (play_last_menu);
        menu.add (new Gtk.SeparatorMenuItem ());
        menu.add (add_playlist_menu);
        //menu.add (edit_menu);
        menu.add (favorite_menu);
        menu.add (no_favorite_menu);
        menu.add (new Gtk.SeparatorMenuItem ());

        if (track.playlist != 0) {
            menu.add (remove_playlist_menu);
        }

        menu.add (remove_db_menu);

        menu.show_all ();

        track_menu.activate.connect (() => {
            this.activate ();
        });

        play_menu.activate.connect (() => {
            this.activate ();
        });

        play_next_menu.activate.connect (() => {
            Byte.utils.set_next_track (track);
        });

        play_last_menu.activate.connect (() => {
            Byte.utils.set_last_track (track);
        });

        favorite_menu.activate.connect (() => {
            if (Byte.scan_service.is_sync == false) {
                Byte.database.set_track_favorite (track, 1);
            }
        });

        no_favorite_menu.activate.connect (() => {
            if (Byte.scan_service.is_sync == false) {
                Byte.database.set_track_favorite (track, 0);
            }
        });

        add_playlist_menu.activate.connect (() => {

        });

        edit_menu.activate.connect (() => {
            var editor_dialog = new Dialogs.TrackEditor (track);
            editor_dialog.destroy.connect (Gtk.main_quit);
            editor_dialog.show_all ();
        });

        remove_db_menu.activate.connect (() => {
            Byte.database.remove_from_library (track);
        });

        remove_file_menu.activate.connect (() => {

        });

        remove_playlist_menu.activate.connect (() => {
            destroy ();
        });
    }
}
