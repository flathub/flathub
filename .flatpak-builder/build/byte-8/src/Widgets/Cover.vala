public class Widgets.Cover : Gtk.EventBox {
    private string DEFAULT_ICON = "/usr/share/com.github.alainm23.byte/track-default-cover.svg";
    private const int EXTRA_MARGIN = 4;
    private bool draw_theme_background = true;

    private bool is_default = false;
    private string? orig_filename = null;
    private int? orig_pixel_size = null;
    private string? orig_type = null;

    public Gdk.Pixbuf? pixbuf { get; set; }
    
    public Cover () {
    }

    public Cover.from_pixbuf (Gdk.Pixbuf pixbuf) {
        Object (pixbuf: pixbuf);
    }

    public Cover.from_file (string filepath, int pixel_size, string type) {
        set_default (type);
        
        load_image (filepath, pixel_size);

        orig_filename = filepath;
        orig_pixel_size = pixel_size;
        orig_type = type;
    }

    public void set_from_file (string filepath, int pixel_size, string type) {
        set_default (type);
        
        load_image (filepath, pixel_size);

        orig_filename = filepath;
        orig_pixel_size = pixel_size;
        orig_type = type;
    }

    public Cover.from_url_async (string uri, int pixel_size, bool preserve_aspect_ratio, string type, Cancellable? cancellable = null) {
        set_default (type);
        show_default (pixel_size);

        var file = GLib.File.new_for_uri (uri);
        
        set_from_file_async (file, pixel_size, preserve_aspect_ratio);

        orig_filename = uri;
        orig_pixel_size = pixel_size;
        orig_type = type;
    }

    public void set_from_url_async (string uri, int pixel_size, bool preserve_aspect_ratio, string type, Cancellable? cancellable = null) {
        set_default (type);
        show_default (pixel_size);

        var file = GLib.File.new_for_uri (uri);
        
        set_from_file_async (file, pixel_size, preserve_aspect_ratio);

        orig_filename = uri;
        orig_pixel_size = pixel_size;
        orig_type = type;
    }

    private async void set_from_file_async (GLib.File file, int pixel_size, bool preserve_aspect_ratio, Cancellable? cancellable = null)  throws Error {
        try {
            var stream = yield file.read_async ();
            pixbuf = yield new Gdk.Pixbuf.from_stream_at_scale_async (stream, pixel_size * get_scale_factor (), pixel_size * get_scale_factor (), preserve_aspect_ratio, cancellable);
        } catch (Error e) {
            show_default (pixel_size);
            throw e;
        }
    }

    private void load_image (string filepath, int pixel_size) {
        try {
            var size = pixel_size * get_scale_factor ();
            pixbuf = new Gdk.Pixbuf.from_file_at_size (filepath, size, size);
        } catch (Error e) {
            show_default (pixel_size);
        }
    }

    public Cover.with_default_icon (int pixel_size, string type) {
        set_default (type);
        
        show_default (pixel_size);
        orig_pixel_size = pixel_size;
    }

    public void set_with_default_icon (int pixel_size, string type) {
        set_default (type);
        
        show_default (pixel_size);
        orig_pixel_size = pixel_size;
    }

    construct {
        valign = Gtk.Align.CENTER;
        halign = Gtk.Align.CENTER;
        visible_window = false;

        notify["pixbuf"].connect (refresh_size_request);
        Gdk.Screen.get_default ().monitors_changed.connect (dpi_change);
    }

    ~Cover () {
        notify["pixbuf"].disconnect (refresh_size_request);
        Gdk.Screen.get_default ().monitors_changed.disconnect (dpi_change);
    }

    private void refresh_size_request () {
        if (pixbuf != null) {
            var scale_factor = get_scale_factor ();
            set_size_request (pixbuf.width / scale_factor + EXTRA_MARGIN * 2, pixbuf.height / scale_factor + EXTRA_MARGIN * 2);
            draw_theme_background = true;
        } else {
            set_size_request (0, 0);
        }

        queue_draw ();
    }

    private void dpi_change () {
        if (is_default && orig_pixel_size != null) {
            show_default (orig_pixel_size);
        } else {
            if (orig_filename != null && orig_pixel_size != null) {
                load_image (orig_filename, orig_pixel_size);
            }
        }
    }

    private void set_default (string type) {
        if (type == "album") {
            DEFAULT_ICON = "/usr/share/com.github.alainm23.byte/album-default-cover.svg";
            get_style_context ().add_class ("album-cover");
        } else if (type == "artist") {
            DEFAULT_ICON = "/usr/share/com.github.alainm23.byte/artist-default-cover.svg";
            get_style_context ().add_class ("artist-cover");
        } else if (type == "radio") {
            DEFAULT_ICON = "/usr/share/com.github.alainm23.byte/radio-default-cover.svg";
            get_style_context ().add_class ("playlist-cover");
        } else if (type == "track") {
            DEFAULT_ICON = "/usr/share/com.github.alainm23.byte/track-default-cover.svg";
            get_style_context ().add_class ("track-cover");
        } else {
            DEFAULT_ICON = "/usr/share/com.github.alainm23.byte/playlist-default-cover.svg";
            get_style_context ().add_class ("playlist-cover");
        }
    }

    public void show_default (int pixel_size) {
        try {
            var size = pixel_size * get_scale_factor ();
            pixbuf = new Gdk.Pixbuf.from_file_at_size (DEFAULT_ICON, size, size);
        } catch (Error e) {
            stderr.printf ("Error setting default avatar icon: %s ", e.message);
        }
    }

    public override bool draw (Cairo.Context cr) {
        if (pixbuf == null) {
            return base.draw (cr);
        }

        unowned Gtk.StyleContext style_context = get_style_context ();
        var width = get_allocated_width () - EXTRA_MARGIN * 2;
        var height = get_allocated_height () - EXTRA_MARGIN * 2;
        var scale_factor = get_scale_factor ();

        if (draw_theme_background) {
            var border_radius = style_context.get_property (Gtk.STYLE_PROPERTY_BORDER_RADIUS, style_context.get_state ()).get_int ();
            var crop_radius = int.min (width / 2, border_radius * width / 100);

            Granite.Drawing.Utilities.cairo_rounded_rectangle (cr, EXTRA_MARGIN, EXTRA_MARGIN, width, height, crop_radius);
            cr.save ();
            cr.scale (1.0 / scale_factor, 1.0 / scale_factor);
            Gdk.cairo_set_source_pixbuf (cr, pixbuf, EXTRA_MARGIN * scale_factor, EXTRA_MARGIN * scale_factor);
            cr.fill_preserve ();
            cr.restore ();
            style_context.render_background (cr, EXTRA_MARGIN, EXTRA_MARGIN, width, height);
            style_context.render_frame (cr, EXTRA_MARGIN, EXTRA_MARGIN, width, height);

        } else {
            cr.save ();
            cr.scale (1.0 / scale_factor, 1.0 / scale_factor);
            style_context.render_icon (cr, pixbuf, EXTRA_MARGIN, EXTRA_MARGIN);
            cr.restore ();
        }

        return Gdk.EVENT_STOP;
    }
}
