public class Views.Artist : Gtk.EventBox {
    private Gtk.Label name_label;
    private Widgets.Cover image_cover;
    private Gtk.ListBox listbox;
    private Gtk.FlowBox flowbox;

    public signal void go_back (string page);
    public string back_page { set; get; }

    private Gee.ArrayList<Objects.Track?> all_tracks;
    private Gee.ArrayList<Objects.Album?> all_albums;

    public Objects.Artist? _artist;
    public Objects.Artist artist {
        set {
            _artist = value;
            name_label.label = _artist.name;

            int item_max = 5;

            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            flowbox.foreach ((widget) => {
                widget.destroy (); 
            });
            
            if (Byte.scan_service.is_sync == false) {
                all_tracks = new Gee.ArrayList<Objects.Track?> ();
                all_tracks = Byte.database.get_all_tracks_by_artist (_artist.id);

                if (item_max > all_tracks.size) {
                    item_max = all_tracks.size;
                }
        
                for (int i = 0; i < item_max; i++) {
                    var row = new Widgets.TrackRow (all_tracks [i]);
        
                    listbox.add (row);
                    listbox.show_all ();
                }

                all_albums = new Gee.ArrayList<Objects.Album?> ();
                all_albums = Byte.database.get_all_albums_by_artist (_artist.id);

                foreach (var item in all_albums) {
                    var row = new Widgets.AlbumArtistChild (item);
                    flowbox.add (row);
                    flowbox.show_all ();
                }
            }

            try {
                var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("artist-%i.jpg").printf (_artist.id));
                var pixbuf = new Gdk.Pixbuf.from_file_at_size (cover_path, 64, 64);
                image_cover.pixbuf = pixbuf;
            } catch (Error e) {
                image_cover.set_with_default_icon (64, "artist");
            }
        }

        get {
            return _artist;
        }
    } 

    public Artist () {}

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

        var center_label = new Gtk.Label (_("Artist"));
        center_label.use_markup = true;
        center_label.valign = Gtk.Align.CENTER;
        center_label.get_style_context ().add_class ("h3");
        center_label.get_style_context ().add_class ("label-color-primary");

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        header_box.pack_start (back_button, false, false, 0);
        header_box.set_center_widget (center_label);

        image_cover = new Widgets.Cover.with_default_icon (128, "artist");
        image_cover.halign = Gtk.Align.CENTER;
        image_cover.valign = Gtk.Align.CENTER;

        name_label = new Gtk.Label (null);
        name_label.halign = Gtk.Align.CENTER;
        name_label.valign = Gtk.Align.CENTER;
        name_label.wrap = true;
        name_label.wrap_mode = Pango.WrapMode.CHAR; 
        name_label.justify = Gtk.Justification.FILL;
        name_label.get_style_context ().add_class ("font-bold");
        name_label.get_style_context ().add_class ("h3");

        var most_played_label = new Gtk.Label ("<b>%s</b>".printf (_("Most Played")));
        most_played_label.get_style_context ().add_class ("label-color-primary");
        most_played_label.get_style_context ().add_class ("h3");
        most_played_label.margin_start = 6;
        most_played_label.halign = Gtk.Align.START;
        most_played_label.use_markup = true;

        listbox = new Gtk.ListBox ();
        //listbox.hexpand = true;

        var albums_label = new Gtk.Label ("<b>%s</b>".printf (_("Albums")));
        albums_label.get_style_context ().add_class ("label-color-primary");
        albums_label.get_style_context ().add_class ("h3");
        albums_label.margin_start = 7;
        albums_label.halign = Gtk.Align.START;
        albums_label.use_markup = true;

        flowbox = new Gtk.FlowBox ();
        flowbox.min_children_per_line = 2;
        flowbox.max_children_per_line = 2;

        var detail_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        detail_box.get_style_context ().add_class (Granite.STYLE_CLASS_WELCOME);
        detail_box.pack_start (image_cover, false, false, 6);
        detail_box.pack_start (name_label, false, false, 6);
        detail_box.pack_start (most_played_label, false, false, 3);
        detail_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        detail_box.pack_start (listbox, false, false, 0);
        detail_box.pack_start (albums_label, false, false, 3);
        //detail_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        detail_box.pack_start (flowbox, false, false, 0);

        var main_scrolled = new Gtk.ScrolledWindow (null, null);
        main_scrolled.margin_bottom = 48;
        main_scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        main_scrolled.expand = true;
        main_scrolled.add (detail_box);

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
    }
}