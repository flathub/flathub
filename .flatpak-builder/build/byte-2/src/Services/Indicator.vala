public class Services.Indicator : GLib.Object {
    SoundIndicatorPlayer player;
    SoundIndicatorRoot root;

    unowned DBusConnection conn;
    uint owner_id;
    uint root_id;
    uint player_id;
    
    public void initialize () {
        owner_id = Bus.own_name (BusType.SESSION, "org.mpris.MediaPlayer2.Byte", GLib.BusNameOwnerFlags.NONE, on_bus_acquired, on_name_acquired, on_name_lost);
        if (owner_id == 0) {
            warning ("Could not initialize MPRIS session.\n");
        }

        Byte.instance.main_window.destroy.connect (() => {
            conn.unregister_object (root_id);
            conn.unregister_object (player_id);
            Bus.unown_name (owner_id);
        });
    }

    private void on_bus_acquired (DBusConnection connection, string name) {
        this.conn = connection;
        try {
            root = new SoundIndicatorRoot ();
            root_id = connection.register_object ("/org/mpris/MediaPlayer2", root);

            player = new SoundIndicatorPlayer (connection);
            player_id = connection.register_object ("/org/mpris/MediaPlayer2", player);
        }
        catch(Error e) {
            warning ("could not create MPRIS player: %s\n", e.message);
        }
    }

    private void on_name_acquired (DBusConnection connection, string name) {
    }

    private void on_name_lost (DBusConnection connection, string name) {
    }
}

[DBus(name = "org.mpris.MediaPlayer2")]
public class SoundIndicatorRoot : GLib.Object {
    Byte app;

    construct {
        app = Byte.instance;
    }

    public string DesktopEntry {
        owned get {
            return app.application_id;
        }
    }
}

[DBus(name = "org.mpris.MediaPlayer2.Player")]
public class SoundIndicatorPlayer : GLib.Object {
    DBusConnection connection;
    Byte app;
    
    public SoundIndicatorPlayer (DBusConnection conn) {
        app = Byte.instance;
        connection = conn;
        
        Byte.player.current_track_changed.connect_after ((track) => {
            Variant property;
            if (Byte.player.player_state == Gst.State.PLAYING) {
                property = "Playing";
                var metadata = new HashTable<string, Variant> (null, null);

                if (track != null) {
                    metadata.insert ("mpris:artUrl", Byte.utils.get_cover_file (track.id));
                    metadata.insert ("xesam:title", track.title);
                    metadata.insert ("xesam:artist", get_simple_string_array (track.artist_name));
                }

                send_properties ("Metadata", metadata);
            } else if (Byte.player.player_state == Gst.State.PAUSED) {
                property = "Paused";
            } else {
                property = "Stopped";
                var metadata = new HashTable<string, Variant> (null, null);
                metadata.insert("mpris:artUrl", "");
                metadata.insert("xesam:title", "");
                metadata.insert("xesam:artist", new string [0]);
                send_properties ("Metadata", metadata);
            }

            send_properties ("PlaybackStatus", property);
        });

        Byte.player.current_radio_title_changed.connect ((title) => {
            if (Byte.player.mode == "radio") {
                Variant property;
                if (Byte.player.player_state == Gst.State.PLAYING) {
                    property = "Playing";
                    var metadata = new HashTable<string, Variant> (null, null);

                    if (Byte.player.current_radio != null) {
                        metadata.insert ("mpris:artUrl", Byte.utils.get_cover_radio_file (Byte.player.current_radio.id));
                        metadata.insert ("xesam:title", Byte.player.current_radio.name);
                        metadata.insert ("xesam:artist", get_simple_string_array (title));
                    }

                    send_properties ("Metadata", metadata);
                } else if (Byte.player.player_state == Gst.State.PAUSED) {
                    property = "Paused";
                } else {
                    property = "Stopped";
                    var metadata = new HashTable<string, Variant> (null, null);
                    metadata.insert("mpris:artUrl", "");
                    metadata.insert("xesam:title", "");
                    metadata.insert("xesam:artist", new string [0]);
                    send_properties ("Metadata", metadata);
                }
        
                send_properties ("PlaybackStatus", property);
            }
        });
    }

    private static string[] get_simple_string_array (string text) {
        string[] array = new string[0];
        array += text;
        return array;
    }

    private void send_properties (string property, Variant val) {
        var property_list = new HashTable<string,Variant> (str_hash, str_equal);
        property_list.insert (property, val);

        var builder = new GLib.VariantBuilder (VariantType.ARRAY);
        var invalidated_builder = new GLib.VariantBuilder (new VariantType("as"));

        foreach (string name in property_list.get_keys ()) {
            Variant variant = property_list.lookup (name);
            builder.add ("{sv}", name, variant);
        }

        try {
            connection.emit_signal (null,
                              "/org/mpris/MediaPlayer2",
                              "org.freedesktop.DBus.Properties",
                              "PropertiesChanged",
                              new Variant("(sa{sv}as)", "org.mpris.MediaPlayer2.Player", builder, invalidated_builder));
        } catch(Error e) {
            print("Could not send MPRIS property change: %s\n", e.message);
        }
    }

    public bool CanGoNext { get { return true; } }

    public bool CanGoPrevious { get { return true; } }

    public bool CanPlay { get { return true; } }

    public bool CanPause { get { return true; } }

    public void PlayPause () throws Error {
        Byte.player.toggle_playing ();
    }

    public void Next () throws Error {
        Byte.player.next ();
    }

    public void Previous () throws Error {
        Byte.player.prev ();
    }
}