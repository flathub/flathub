public class Widgets.Popovers.NewPlaylist : Gtk.Popover {
    public Gtk.Entry title_entry;
    private Gtk.TextView note_text;
    private Widgets.Cover image_cover;

    private string cover_path;
    public NewPlaylist (Gtk.Widget relative) {
        Object (
            relative_to: relative,
            modal: true,
            position: Gtk.PositionType.BOTTOM
        );
    }

    construct {
        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);

        title_entry = new Gtk.Entry ();
        title_entry.placeholder_text = _("New Playlist");
        title_entry.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        title_entry.valign = Gtk.Align.START;
        title_entry.margin_top = 6;

        note_text = new Gtk.TextView ();
        note_text.height_request = 35;
        note_text.wrap_mode = Gtk.WrapMode.WORD_CHAR;
        note_text.hexpand = true;
        note_text.margin_start = 6;
        note_text.margin_end = 6;
        
        var note_placeholder = new Gtk.Label (_("Add Description"));
        note_placeholder.opacity = 0.6;
        note_text.add (note_placeholder);

        var camera_image = new Gtk.Image ();
        camera_image.gicon = new ThemedIcon ("camera-photo-symbolic");
        camera_image.get_style_context ().add_class ("playing-ani-color");
        camera_image.pixel_size = 16;
        camera_image.halign = Gtk.Align.END;
        camera_image.valign = Gtk.Align.END;

        image_cover = new Widgets.Cover.with_default_icon (64, "playlist");

        var cover_button = new Gtk.Button ();
        cover_button.can_focus = false;
        cover_button.get_style_context ().add_class ("no-padding");
        cover_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        cover_button.add (image_cover);

        var cover_overlay = new Gtk.Overlay ();
        cover_overlay.halign = Gtk.Align.START;
        cover_overlay.valign = Gtk.Align.START;
        cover_overlay.add_overlay (camera_image);
        cover_overlay.add (cover_button); 

        var add_button = new Gtk.Button.with_label (_("Add"));
        add_button.can_focus = false;
        add_button.margin = 6;

        var box_01 = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        box_01.pack_start (cover_overlay, false, false, 0);
        box_01.pack_start (title_entry, false, false, 0);

        var main_grid = new Gtk.Grid ();
        main_grid.orientation = Gtk.Orientation.VERTICAL;
        main_grid.margin = 3;
        main_grid.column_spacing = 6;
        main_grid.row_spacing = 3;

        main_grid.add (box_01);
        main_grid.add (note_text);
        main_grid.add (add_button);

        add (main_grid);
        title_entry.grab_focus ();

        title_entry.activate.connect (add_playlist);

        add_button.clicked.connect (add_playlist);

        cover_button.clicked.connect (() => {
            var new_cover = Byte.utils.choose_new_cover ();
            if (new_cover != null) {
                cover_path = new_cover;
                try {
                    var pixbuf = Byte.utils.align_and_scale_pixbuf (
                        new Gdk.Pixbuf.from_file (cover_path),
                        64
                    );

                    image_cover.pixbuf = pixbuf;
                } catch (Error err) { 
                    warning (err.message);
                }
            }
        });

        note_text.focus_in_event.connect (() => {
            note_placeholder.visible = false;
            return false;
        });

        note_text.focus_out_event.connect (() => {
            if (note_text.buffer.text == "") {
                note_placeholder.visible = true;
            }
            return false;
        });
    }

    private void add_playlist () {
        if (title_entry.text != "") {
            var playlist = new Objects.Playlist ();
            playlist.title = title_entry.text;
            playlist.note = note_text.buffer.text;

            int id = Byte.database.insert_playlist (playlist);

            if (cover_path != null && id != 0) {
                try {
                    var pixbuf = Byte.utils.align_and_scale_pixbuf (
                        new Gdk.Pixbuf.from_file (cover_path),
                        256
                    );

                    string file = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("playlist-%i.jpg").printf (id));

                    try {
                        pixbuf.save (file, "jpeg", "quality", "100");
                    } catch (Error err) {
                        warning (err.message);
                    }
                } catch (Error err) { 
                    warning (err.message);
                }
            }

            cover_path = "";
            title_entry.text = "";
            note_text.buffer.text = "";
            image_cover.set_with_default_icon (64, "playlist");
            
            popdown ();
        }
    }
}   