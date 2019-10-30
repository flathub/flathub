public class Widgets.ModelButton : Gtk.Button {
    private Gtk.Label _label;
    private Gtk.Image _image;

    public string icon {
        set {
            _image.gicon = new ThemedIcon (value);
        }
    }
    public string tooltip {
        set {
            tooltip_text = value;
        }
    }
    public string text { 
        set {
            _label.label = value;
        }
    }
    

    public ModelButton (string text, string icon, string tooltip) {
        Object (
            icon: icon,
            text: text,
            tooltip: tooltip,
            expand: true
        );
    }

    construct {
        can_focus = false;
        get_style_context ().add_class ("menuitem");
        get_style_context ().add_class ("flat");

        _label = new Gtk.Label (null);

        _image = new Gtk.Image ();
        _image.pixel_size = 16;
        
        var grid = new Gtk.Grid ();
        grid.column_spacing = 6;
        grid.add (_image);
        grid.add (_label);

        add (grid);
    }
}