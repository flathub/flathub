public class Widgets.AlbumArtistChild : Gtk.FlowBoxChild {
    public Objects.Album album { get; construct; }
    
    private Gtk.Label primary_label;
    private Gtk.Label secondary_label;
    private Widgets.Cover image_cover;

    public AlbumArtistChild (Objects.Album album) {
        Object (
            album: album
        );
    }

    construct {
        tooltip_text = album.title;

        primary_label = new Gtk.Label (album.title);
        primary_label.get_style_context ().add_class ("font-bold");
        primary_label.ellipsize = Pango.EllipsizeMode.END;
        primary_label.halign = Gtk.Align.START;
        primary_label.max_width_chars = 45;
        primary_label.valign = Gtk.Align.END;

        secondary_label = new Gtk.Label ("%i".printf (album.year));
        secondary_label.halign = Gtk.Align.START;
        secondary_label.valign = Gtk.Align.START;
        secondary_label.ellipsize = Pango.EllipsizeMode.END;

        image_cover = new Widgets.Cover.from_file (
            GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("album-%i.jpg").printf (album.id)), 32, "track");
        image_cover.halign = Gtk.Align.START;
        image_cover.valign = Gtk.Align.START;

        var main_grid = new Gtk.Grid ();
        main_grid.margin_top = 1;
        main_grid.margin_start = 3;
        main_grid.margin_end = 9;
        main_grid.column_spacing = 3; 
        main_grid.attach (image_cover, 0, 0, 1, 2);
        main_grid.attach (primary_label, 1, 0, 1, 1);
        main_grid.attach (secondary_label, 1, 1, 1, 1);

        add (main_grid);

        Byte.database.updated_album_cover.connect ((album_id) => {
            Idle.add (() => {
                if (album_id == album.id) {
                    try {
                        image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (
                            GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("album-%i.jpg").printf (album_id)), 
                            64, 
                            64);
                    } catch (Error e) {
                        stderr.printf ("Error setting default avatar icon: %s ", e.message);
                    }
                }
                
                return false;
            });
        });
    }
}