public class Widgets.MediaControl : Gtk.Revealer {
    private Granite.SeekBar timeline;
    private Gtk.Label title_label;
    private Gtk.Label subtitle_label;

    private Gtk.Image icon_favorite;
    private Gtk.Image icon_no_favorite;

    Gtk.Menu playlists;
    private Gtk.Menu menu;

    public MediaControl () {

    }

    construct {
        icon_favorite = new Gtk.Image.from_icon_name ("byte-favorite-symbolic", Gtk.IconSize.MENU);
        icon_no_favorite = new Gtk.Image.from_icon_name ("byte-no-favorite-symbolic", Gtk.IconSize.MENU);

        timeline = new Granite.SeekBar (0);
        timeline.margin_start = 6;
        timeline.margin_top = 9;
        timeline.margin_end = 6;
        timeline.get_style_context ().remove_class ("seek-bar");
        timeline.get_style_context ().add_class ("byte-seekbar");

        var timeline_revealer = new Gtk.Revealer ();
        timeline_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        timeline_revealer.add (timeline);
        timeline_revealer.reveal_child = false;

        title_label = new Gtk.Label (null);
        title_label.get_style_context ().add_class ("font-bold");
        title_label.ellipsize = Pango.EllipsizeMode.END;
        title_label.halign = Gtk.Align.CENTER;
        title_label.selectable = true;

        subtitle_label = new Gtk.Label (null);
        subtitle_label.halign = Gtk.Align.CENTER;
        subtitle_label.ellipsize = Pango.EllipsizeMode.END;
        subtitle_label.selectable = true;

        var options_button = new Gtk.Button.from_icon_name ("view-more-horizontal-symbolic", Gtk.IconSize.MENU);
        options_button.valign = Gtk.Align.CENTER;
        options_button.can_focus = false;
        options_button.margin_end = 6;
        options_button.tooltip_text = _("Options");
        options_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        options_button.get_style_context ().add_class ("options-button");
        options_button.get_style_context ().add_class ("button-color");
        options_button.get_style_context ().remove_class ("button");

        var favorite_revealer = new Gtk.Revealer ();
        favorite_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_LEFT;
        favorite_revealer.add (options_button);
        favorite_revealer.reveal_child = false;

        var metainfo_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        metainfo_box.margin_start = 6;
        metainfo_box.margin_end = 6;
        metainfo_box.valign = Gtk.Align.CENTER;
        metainfo_box.add (title_label);
        metainfo_box.add (subtitle_label);

        var image_cover = new Widgets.Cover ();

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.margin = 3;
        header_box.margin_start = 4;
        header_box.margin_end = 3;
        header_box.pack_start (image_cover, false, false, 0);
        header_box.set_center_widget (metainfo_box);
        header_box.pack_end (favorite_revealer, false, false, 0);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.pack_start (timeline_revealer, false, false, 0);
        main_box.pack_start (header_box, false, false, 0);

        add (main_box);

        Byte.player.current_progress_changed.connect ((progress) => {
            timeline.playback_progress = progress;
            if (timeline.playback_duration == 0) {
                timeline.playback_duration = Byte.player.duration / Gst.SECOND;
            }
        });

        Byte.player.current_duration_changed.connect ((duration) => {
            timeline.playback_duration = duration / Gst.SECOND;
        });

        Byte.player.current_track_changed.connect ((track) => {
            title_label.label = track.title;
            subtitle_label.label = "%s — %s".printf (track.artist_name, track.album_title);

            string cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track.id));
            image_cover.set_from_file (cover_path, 32, "track");
        });

        Byte.player.current_radio_changed.connect ((radio) => {
            title_label.label = radio.name;
            reveal_child = true;
        });

        Byte.player.current_radio_title_changed.connect ((title) => {
            if (Byte.player.mode == "radio") {
                subtitle_label.label = title;
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

        Byte.player.mode_changed.connect ((mode) => {
            if (mode == "radio") {
                timeline_revealer.reveal_child = false;
                favorite_revealer.reveal_child = false;
            } else {
                timeline_revealer.reveal_child = true;
                if (Byte.scan_service.is_sync == false) {
                    favorite_revealer.reveal_child = true;
                }
            }
        });

        Byte.lastfm_service.radio_cover_track_found.connect ((track_url) => {
            print ("URL: %s\n".printf (track_url));
            image_cover.set_from_url_async (track_url, 32, true, "track");
        });

        Byte.database.updated_track_cover.connect ((track_id) => {
            Idle.add (() => {
                if (Byte.player.current_track != null && track_id == Byte.player.current_track.id) {
                    try {
                        image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (
                            GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track_id)),
                            32,
                            32);
                    } catch (Error e) {
                        stderr.printf ("Error setting default avatar icon: %s ", e.message);
                    }
                }

                return false;
            });
        });

        timeline.scale.change_value.connect ((scroll, new_value) => {
            Byte.player.seek_to_progress (new_value);
            return true;
        });

        Byte.scan_service.sync_started.connect (() => {
            favorite_revealer.reveal_child = false;
        });

        Byte.scan_service.sync_finished.connect (() => {
            favorite_revealer.reveal_child = true;
        });

        options_button.clicked.connect (() => {
            print ("Click\n");

            if (Byte.player.current_track != null) {
                activate_menu (Byte.player.current_track);
            }
        });
    }

    private void activate_menu (Objects.Track track) {
        build_context_menu (track);

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
            var message_dialog = new Granite.MessageDialog.with_image_from_icon_name (
                _("Delete from library?"),
                _("Are you sure you want to delete <b>%s</b> from your library?").printf (track.title),
                "dialog-warning",
                Gtk.ButtonsType.CANCEL
            );

            var set_button = new Gtk.Button.with_label (_("Delete"));
            set_button.get_style_context ().add_class (Gtk.STYLE_CLASS_DESTRUCTIVE_ACTION);
            message_dialog.add_action_widget (set_button, Gtk.ResponseType.ACCEPT);

            message_dialog.show_all ();

            if (message_dialog.run () == Gtk.ResponseType.ACCEPT) {
                Byte.database.remove_from_library (track);
            }

            message_dialog.destroy ();
        });

        remove_file_menu.activate.connect (() => {

        });

        remove_playlist_menu.activate.connect (() => {
            if (Byte.database.remove_from_playlist (track)) {
                destroy ();
            }
        });
    }
}