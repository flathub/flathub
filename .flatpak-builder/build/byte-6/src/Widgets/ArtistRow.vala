public class Widgets.ArtistRow : Gtk.ListBoxRow {
    public Objects.Artist artist { get; construct; }

    private Gtk.Label name_label;
    private Widgets.Cover image_cover;
    private string cover_path;

    public ArtistRow (Objects.Artist artist) {
        Object (
            artist: artist
        );
    }

    construct {
        get_style_context ().add_class ("album-row");
        
        name_label = new Gtk.Label (artist.name);
        name_label.valign = Gtk.Align.CENTER;
        name_label.get_style_context ().add_class ("h3");
        name_label.ellipsize = Pango.EllipsizeMode.END;
        //name_label.max_width_chars = 36;

        cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("artist-%i.jpg").printf (artist.id));
        image_cover = new Widgets.Cover.from_file (cover_path, 48, "artist");
        image_cover.halign = Gtk.Align.START;
        image_cover.valign = Gtk.Align.START;

        var main_grid = new Gtk.Grid ();
        main_grid.margin = 3;
        main_grid.column_spacing = 6;
        main_grid.halign = Gtk.Align.START;
        main_grid.valign = Gtk.Align.START;
        main_grid.add (image_cover);
        main_grid.add (name_label);

        var event_box = new Gtk.EventBox ();
        event_box.add_events (Gdk.EventMask.ENTER_NOTIFY_MASK | Gdk.EventMask.LEAVE_NOTIFY_MASK);
        event_box.add (main_grid);

        add (event_box);
    }
}