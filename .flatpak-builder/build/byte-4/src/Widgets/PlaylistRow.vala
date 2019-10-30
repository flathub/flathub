public class Widgets.PlaylistRow : Gtk.ListBoxRow {
    public Objects.Playlist playlist { get; construct; }

    private Gtk.Label title_label;
    private Widgets.Cover image_cover;
    private string cover_path;
    public PlaylistRow (Objects.Playlist playlist) {
        Object (
            playlist: playlist
        );
    }

    construct {
        tooltip_text = playlist.title;
        get_style_context ().add_class ("album-row");

        title_label = new Gtk.Label (playlist.title);
        title_label.get_style_context ().add_class ("font-bold");
        title_label.get_style_context ().add_class ("h2");
        title_label.ellipsize = Pango.EllipsizeMode.END;
        title_label.halign = Gtk.Align.START;
        title_label.valign = Gtk.Align.END;

        var tracks_label = new Gtk.Label (_("Updated %s").printf(Byte.utils.get_relative_datetime (playlist.date_updated)));
        tracks_label.valign = Gtk.Align.START;
        tracks_label.halign = Gtk.Align.START;

        cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("playlist-%i.jpg").printf (playlist.id));
        image_cover = new Widgets.Cover.from_file (cover_path, 64, "playlist");

        var main_grid = new Gtk.Grid ();
        main_grid.margin = 3;
        main_grid.column_spacing = 6;
        main_grid.attach (image_cover, 0, 0, 1, 2);
        main_grid.attach (title_label, 1, 0, 1, 1);
        main_grid.attach (tracks_label, 1, 1, 1, 1);

        add (main_grid);

        Byte.database.removed_playlist.connect ((id) => {
            if (playlist.id == id) {
                destroy ();
            }
        });

        Byte.database.updated_playlist.connect ((p) => {
            if (playlist.id == p.id) {
                title_label.label = p.title;
                tracks_label.label = _("Updated %s").printf(Byte.utils.get_relative_datetime (p.date_updated));
            }
        });

        Byte.database.updated_playlist_cover.connect ((id) => {
            if (playlist.id == id) {
                try {
                    image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (
                        GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("playlist-%i.jpg").printf (playlist.id)),
                        64,
                        64);
                } catch (Error e) {
                    stderr.printf ("Error setting default avatar icon: %s ", e.message);
                }
            }
        });
    }
}