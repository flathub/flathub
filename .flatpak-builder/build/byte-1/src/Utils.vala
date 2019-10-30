public class Utils : GLib.Object {
    public Gee.ArrayList<Objects.Track?> queue_playlist { set; get; }

    public signal void play_items (Gee.ArrayList<Objects.Track?> items, Objects.Track? track);

    public signal void update_next_track ();
    public signal void add_next_track (Gee.ArrayList<Objects.Track?> items);
    public signal void add_last_track (Gee.ArrayList<Objects.Track?> items);

    public signal void radio_image_downloaded (int id);

    public string MAIN_FOLDER;
    public string COVER_FOLDER;

    bool dark_mode;
    string colorPrimary;
    string colorAccent;
    string textColorPrimary;

    public Utils () {
        MAIN_FOLDER = Environment.get_home_dir () + "/.local/share/com.github.alainm23.byte";
        COVER_FOLDER = GLib.Path.build_filename (MAIN_FOLDER, "covers");
    }

    public void set_items (Gee.ArrayList<Objects.Track?> all_items, bool shuffle_mode, Objects.Track? track) {
        if (all_items.size > 0) {
            if (shuffle_mode) {
                queue_playlist = generate_shuffle (all_items);

                if (track != null) {
                    int index = get_track_index_by_id (track.id, queue_playlist);
                    queue_playlist.remove_at (index);
                    queue_playlist.insert (0, track);
                }

                Byte.settings.set_boolean ("shuffle-mode", true);
            } else {
                queue_playlist = playlist_order (all_items);
                Byte.settings.set_boolean ("shuffle-mode", false);
            }

            play_items (queue_playlist, track);
        }
    }

    public void shuffle_changed (bool shuffle_mode) {
        if (queue_playlist != null) {
            if (shuffle_mode) {
                queue_playlist = generate_shuffle (queue_playlist);

                if (Byte.player.current_track != null) {
                    int index = get_track_index_by_id (Byte.player.current_track.id, queue_playlist);
                    queue_playlist.remove_at (index);
                    queue_playlist.insert (0, Byte.player.current_track);
                }
            } else {
                queue_playlist = playlist_order (queue_playlist);
            }

            play_items (queue_playlist, Byte.player.current_track);
            update_next_track ();
        }
    }

    public int get_track_index_by_id (int id, Gee.ArrayList<Objects.Track?> queue_playlist) {
        int index = 0;
        foreach (var item in queue_playlist) {
            if (item.id == id) {
                return index;
            }

            index++;
        }

        return index;
    }

    private bool track_exists (int id, Gee.ArrayList<Objects.Track?> queue_playlist) {
        foreach (var item in queue_playlist) {
            if (item.id == id) {
                return true;
            }
        }

        return false;
    }

    public Gee.ArrayList<Objects.Track?> generate_shuffle (Gee.ArrayList<Objects.Track?> items) {
        for (int i = items.size - 1; i > 0; i--) {
            int random_index = GLib.Random.int_range (0, i);

            var tmp_track = items [random_index];
            items [random_index] = items [i];
            items [i] = tmp_track;
        }

        return items;
    }

    public Gee.ArrayList<Objects.Track?> playlist_order (Gee.ArrayList<Objects.Track?> items) {
        for (int j = 0; j < items.size; j++) {
            for (int i = 0; i < items.size - 1; i++) {
                if (items [i].track_order > items [i + 1].track_order) {
                    var tmp_track = items [i + 1];
                    items [i + 1] = items [i];
                    items [i] = tmp_track;
                }
            }
        }

        return items;
    }

    public Objects.Track? get_next_track (Objects.Track current_track) {
        int index = get_track_index_by_id (current_track.id, queue_playlist) + 1;
        Objects.Track? returned = null;
        var repeat_mode = Byte.settings.get_enum ("repeat-mode");

        if (index >= queue_playlist.size) {
            if (repeat_mode == 0) {
                returned = null;
            } else if (repeat_mode == 1) {
                returned = queue_playlist [0];
            } else {
                returned = null;
            }
        } else {
            returned = queue_playlist [index];
        }

        return returned;
    }

    public Objects.Track get_prev_track (Objects.Track current_track) {
        int index = get_track_index_by_id (current_track.id, queue_playlist) - 1;

        if (index < 0) {
            index = 0;
        }

        return queue_playlist [index];
    }

    public void remove_track (int id) {
        var index = get_track_index_by_id (id, queue_playlist);
        queue_playlist.remove_at (index);

        update_next_track ();
    }

    public void set_next_track (Objects.Track track) {
        if (track.id != Byte.player.current_track.id) {
            bool track_exists = track_exists (track.id, queue_playlist);
            if (track_exists) {
                int remove_index = get_track_index_by_id (track.id, queue_playlist);
                queue_playlist.remove_at (remove_index);
            }

            int index = get_track_index_by_id (Byte.player.current_track.id, queue_playlist) + 1;

            track.track_order = index;
            queue_playlist.insert (index, track);

            add_next_track (queue_playlist);
        }
    }

    public void set_last_track (Objects.Track track) {
        if (track.id != Byte.player.current_track.id) {
            bool track_exists = track_exists (track.id, queue_playlist);

            if (track_exists) {
                int remove_index = get_track_index_by_id (track.id, queue_playlist);
                queue_playlist.remove_at (remove_index);
            }

            track.track_order = queue_playlist.size + 1;
            queue_playlist.add (track);
            add_last_track (queue_playlist);
        }
    }

    public void download_image (string type, int id, string url) {
        // Create file
        var image_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("%s-%i.jpg").printf (type, id));

        var file_path = File.new_for_path (image_path);
        var file_from_uri = File.new_for_uri (url);

        MainLoop loop = new MainLoop ();

        file_from_uri.copy_async.begin (file_path, 0, Priority.DEFAULT, null, (current_num_bytes, total_num_bytes) => {
            // Report copy-status:
            print ("%" + int64.FORMAT + " bytes of %" + int64.FORMAT + " bytes copied.\n", current_num_bytes, total_num_bytes);
        }, (obj, res) => {
            try {
                if (file_from_uri.copy_async.end (res)) {
                    print ("Image Downloaded\n");
                    if (type == "radio") {
                        radio_image_downloaded (id);
                    }
                }
            } catch (Error e) {
                download_image (type, id, url);
                print ("Error: %s\n", e.message);
            }

            loop.quit ();
        });

        loop.run ();
    }

    public void create_dir_with_parents (string dir) {
        string path = Environment.get_home_dir () + dir;
        File tmp = File.new_for_path (path);
        if (tmp.query_file_type (0) != FileType.DIRECTORY) {
            GLib.DirUtils.create_with_parents (path, 0775);
        }
    }

    public string get_formated_duration (uint64 duration) {
        uint seconds = (uint) (duration / 1000000000);
        if (seconds < 3600) {
            uint minutes = seconds / 60;
            seconds -= minutes * 60;
            return "%u:%02u".printf (minutes, seconds);
        }

        uint hours = seconds / 3600;
        seconds -= hours * 3600;

        uint minutes = seconds / 60;
        seconds -= minutes * 60;

        return "%u:%02u:%02u".printf (hours, minutes, seconds);
    }

    public string get_relative_datetime (string date) {
        return Granite.DateTime.get_relative_datetime (
            new GLib.DateTime.from_iso8601 (date, new GLib.TimeZone.local ())
        );
    }

    public string get_relative_duration (uint64 duration) {
        uint temp_sec = (uint) (duration / 1000000000);
        uint sec = (uint) temp_sec % 60;
        uint min = (uint) ((temp_sec / 60) % 60);
        uint hour = (uint) ((temp_sec / (60 * 60)) % 24);
        uint day = (uint) ((temp_sec / (24 * 60 * 60)) % 24);

        if (day > 0) {
            return "%ud %uh %um %us".printf (day, hour, min, sec);
        } else {
            if (hour > 0) {
                return "%uh %um %us".printf (hour, min, sec);
            } else {
                if (min > 0) {
                    return "%um %us".printf (min, sec);
                } else {
                    if (sec > 0) {
                        return "%us".printf (sec);
                    } else {
                        return "";
                    }
                }
            }
        }
    }

    public string get_cover_file (int track_id) {
        var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track_id));
        if (File.new_for_path (cover_path).query_exists ()) {
            return "file://" + cover_path;
        }

        return "file:///usr/share/com.github.alainm23.byte/track-default-cover.svg";
    }

    public string get_cover_radio_file (int radio_id) {
        var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("radio-%i.jpg").printf (radio_id));
        if (File.new_for_path (cover_path).query_exists ()) {
            return "file://" + cover_path;
        }

        return "file:///usr/share/com.github.alainm23.byte/radio-default-cover.svg";
    }

    public string? choose_new_cover () {
        string? return_value = null;
        var chooser = new Gtk.FileChooserDialog (
            _("Choose an image…"), Byte.instance.main_window,
            Gtk.FileChooserAction.OPEN,
            _("Cancel"), Gtk.ResponseType.CANCEL,
            _("Open"), Gtk.ResponseType.ACCEPT);

        var filter = new Gtk.FileFilter ();
        filter.set_filter_name (_ ("Images"));
        filter.add_mime_type ("image/*");

        chooser.add_filter (filter);

        Gtk.Image preview_area = new Gtk.Image ();
        chooser.set_preview_widget (preview_area);
        chooser.set_use_preview_label (false);
        chooser.set_select_multiple (false);

        chooser.update_preview.connect (() => {
            string filename = chooser.get_preview_filename ();
            if (filename != null) {
                try {
                    Gdk.Pixbuf pixbuf = new Gdk.Pixbuf.from_file_at_scale (filename, 150, 150, true);
                    preview_area.set_from_pixbuf (pixbuf);
                    preview_area.show ();
                } catch (Error e) {
                    preview_area.hide ();
                }
            } else {
                preview_area.hide ();
            }
        });

        if (chooser.run () == Gtk.ResponseType.ACCEPT) {
            return_value = chooser.get_filename ();
        }

        chooser.destroy ();
        return return_value;
    }

    public Gdk.Pixbuf? align_and_scale_pixbuf (Gdk.Pixbuf p, int size) {
        Gdk.Pixbuf ? pixbuf = p;
        if (pixbuf.width != pixbuf.height) {
            if (pixbuf.width > pixbuf.height) {
                int dif = (pixbuf.width - pixbuf.height) / 2;
                pixbuf = new Gdk.Pixbuf.subpixbuf (pixbuf, dif, 0, pixbuf.height, pixbuf.height);
            } else {
                int dif = (pixbuf.height - pixbuf.width) / 2;
                pixbuf = new Gdk.Pixbuf.subpixbuf (pixbuf, 0, dif, pixbuf.width, pixbuf.width);
            }
        }

        pixbuf = pixbuf.scale_simple (size, size, Gdk.InterpType.BILINEAR);

        return pixbuf;
    }

    public void apply_theme (int id) {
        if (id == 1) {
            dark_mode = false;
            colorPrimary = "#fe2851";
            colorAccent = "#fe2851";
            textColorPrimary = "#fff";
        } else if (id == 2) {
            dark_mode = true;
            colorPrimary = "mix(@BLACK_500, @BLACK_300, 0.5)";
            colorAccent = "#fe2851";
            textColorPrimary = "#fe2851";
        } else if (id == 3) {
            dark_mode = true;
            colorPrimary = "#36E683";
            colorAccent = "#36E683";
            textColorPrimary = "#333";
        }

        Gtk.Settings.get_default ().gtk_application_prefer_dark_theme = dark_mode;

        string THEME_CSS = """
            @define-color colorPrimary %s;
            @define-color colorAccent %s;
            @define-color textColorPrimary %s;
        """;

        var provider = new Gtk.CssProvider ();

        try {
            var theme_css = THEME_CSS.printf (
                colorPrimary,
                colorAccent,
                textColorPrimary
            );

            provider.load_from_data (theme_css, theme_css.length);

            Gtk.StyleContext.add_provider_for_screen (Gdk.Screen.get_default (), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
        } catch (GLib.Error e) {
            return;
        }
    }
}
