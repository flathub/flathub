[DBus (name = "org.gnome.SettingsDaemon.MediaKeys")]
public interface GnomeMediaKeys : GLib.Object {
    public abstract void GrabMediaPlayerKeys (string application, uint32 time) throws Error;
    public abstract void ReleaseMediaPlayerKeys (string application) throws Error;
    public signal void MediaPlayerKeyPressed (string application, string key);
}

public class Services.MediaKey : GLib.Object {
    public static Services.MediaKey instance { get; private set; }
    private GnomeMediaKeys? media_keys;

    construct {
        assert (media_keys == null);

        try {
            media_keys = Bus.get_proxy_sync (BusType.SESSION, "org.gnome.SettingsDaemon", "/org/gnome/SettingsDaemon/MediaKeys");
        } catch (Error e) {
            warning ("Mediakeys error: %s", e.message);
        }

        if (media_keys != null) {
            media_keys.MediaPlayerKeyPressed.connect (pressed_key);
            try {
                media_keys.GrabMediaPlayerKeys (Byte.instance.application_id, (uint32)0);
            }
            catch (Error err) {
                warning ("Could not grab media player keys: %s", err.message);
            }
        }
    }

    private MediaKey () {}

    public static void listen () {
        instance = new Services.MediaKey ();
    }

    private void pressed_key (dynamic Object bus, string application, string key) {
        if (application == (Byte.instance.application_id)) {
            if (key == "Previous") {
                Byte.player.prev ();
            } else if (key == "Play") { 
                Byte.player.toggle_playing ();
            } else if (key == "Next") {
                Byte.player.next ();
            } else if (key == "Pause") {
                Byte.player.pause ();
            }
        }
    }
}
