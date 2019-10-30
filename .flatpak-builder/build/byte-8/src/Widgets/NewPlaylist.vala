public class Widgets.NewPlaylist : Gtk.EventBox {
    public NewPlaylist () {}
    
    construct {
        add_events (Gdk.EventMask.ENTER_NOTIFY_MASK | Gdk.EventMask.LEAVE_NOTIFY_MASK);

        // Add
        var add_image = new Widgets.Cover.from_file ("/usr/share/com.github.alainm23.byte/playlist-add.svg", 48, "playlist");

        var add_label = new Gtk.Label ("<b>%s</b>".printf (_("New Playlist")));
        add_label.valign = Gtk.Align.CENTER;
        add_label.use_markup = true;
        add_label.get_style_context ().add_class ("h3");

        var add_grid = new Gtk.Grid ();
        add_grid.get_style_context ().add_class ("new-task");
        add_grid.column_spacing = 9;
        add_grid.add (add_image);
        add_grid.add (add_label);

        var add_eventbox = new Gtk.EventBox ();
        add_eventbox.valign = Gtk.Align.CENTER;
        add_eventbox.add (add_grid);

        // Entry
        var image_cover = new Widgets.Cover.from_file ("/usr/share/com.github.alainm23.byte/playlist-add.svg", 48, "playlist");

        var title_entry = new Gtk.Entry ();
        title_entry.valign = Gtk.Align.CENTER;
        title_entry.hexpand = true;
        title_entry.get_style_context ().add_class ("add-playlist-entry");
        title_entry.placeholder_text = _("Project name");

        var playlist_grid = new Gtk.Grid ();
        playlist_grid.add (image_cover);
        playlist_grid.add (title_entry);

        var stack = new Gtk.Stack ();
        stack.margin_start = 3;
        stack.margin_end = 12;
        stack.valign = Gtk.Align.CENTER;
        stack.transition_type = Gtk.StackTransitionType.CROSSFADE;
        stack.transition_duration = 120;

        stack.add_named (add_eventbox, "add_eventbox");
        stack.add_named (playlist_grid, "playlist_grid");

        add (stack);

        add_eventbox.event.connect ((event) => {
            if (event.type == Gdk.EventType.BUTTON_PRESS) {
                stack.visible_child_name = "playlist_grid";
                title_entry.grab_focus ();
            }
        });

        title_entry.key_release_event.connect ((key) => {
            if (key.keyval == 65307) {
                stack.visible_child_name = "add_eventbox";
                title_entry.text = "";
            }

            return false;
        });

        title_entry.activate.connect (() => {
            if (title_entry.text != "") {
                var playlist = new Objects.Playlist ();
                playlist.title = title_entry.text;

                Byte.database.insert_playlist (playlist);

                stack.visible_child_name = "add_eventbox";
                title_entry.text = "";
            }
        });
    }
}