public class Widgets.HomeButton : Gtk.Button {
    private Gtk.Label name_label;
    private Gtk.Image icon_image;

    public string primary_icon {
        set {
            icon_image.gicon = new ThemedIcon (value);
        }
    }

    public string primary_name {
        set {
            name_label.label = value;
        }
    }

    public HomeButton (string name, string icon) {
        Object (
            primary_name: name,
            primary_icon: icon
        );
    }

    construct {
        hexpand = true;
        get_style_context ().add_class ("home-button");
        get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        name_label = new Gtk.Label (null);

        icon_image = new Gtk.Image ();
        icon_image.pixel_size = 16;

        var main_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        main_box.margin = 3;
        main_box.pack_start (name_label, false, false, 0);
        main_box.pack_end (icon_image, false, false, 0);

        add (main_box);
    }
}