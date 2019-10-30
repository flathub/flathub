public class Widgets.Welcome : Gtk.EventBox {
    private Granite.Widgets.Welcome welcome;

    public signal void selected (int index);
    public Welcome () {
        Object (

        );
    }

    construct {
        welcome = new Granite.Widgets.Welcome (_("Library is Empty"), _("Add music to start jamming out"));

        welcome.append ("byte-folder-music", _("Load Music"), _("Load from the user's music directory"));
        welcome.append ("byte-folder-open", _("Change Music Folder"), _("Load music from folder"));
        
        welcome.get_style_context ().add_class ("w-round");

        add (welcome);

        welcome.activated.connect ((index) => {
            selected (index);
       });
    }
}
