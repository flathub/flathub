public class Widgets.SearchEntry : Gtk.SearchEntry {
    construct {
        margin = 6;
        valign = Gtk.Align.CENTER;
        hexpand = true;
        get_style_context ().add_class ("search-entry");

        focus_in_event.connect (() => {
            Byte.instance.toggle_playing_action_enabled (false);
            return false;
        });

        focus_out_event.connect (() => {
            Byte.instance.toggle_playing_action_enabled (true);
            return false;
        });

        search_changed.connect (() => {
            Byte.instance.toggle_playing_action_enabled (false);
        });
    }
}