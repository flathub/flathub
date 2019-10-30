public class Services.Notification : GLib.Object {
    public Notification () {
        Byte.player.current_track_changed.connect ((track) => {
            if (Byte.settings.get_boolean ("notifications-enabled")) {
                send_track_notification (track);
            }
        });
    
        Byte.player.current_radio_title_changed.connect ((title) => {
            if (Byte.settings.get_boolean ("notifications-enabled")) {
                send_radio_notification (title);
            }     
        });
    }

    public void send_track_notification (Objects.Track track) {
        try {
            var notification = new GLib.Notification (track.title);
            notification.set_body (track.artist_name);
            notification.set_icon (GLib.Icon.new_for_string (Byte.utils.get_cover_file (track.id)));
            notification.set_priority (GLib.NotificationPriority.LOW);

            Byte.instance.send_notification (Byte.instance.application_id, notification);
        } catch (Error e) {
            stderr.printf ("Error setting default avatar icon: %s ", e.message);
        }
    }

    public void send_notification (string title, string body) {
        var notification = new GLib.Notification (title);
        notification.set_body (body);
        notification.set_icon (new ThemedIcon (Byte.instance.application_id));
        notification.set_priority (GLib.NotificationPriority.NORMAL);

        Byte.instance.send_notification (Byte.instance.application_id, notification);
    }
    
    public void send_radio_notification (string title) {
        try {
            if (title != null) {
                var media = title.split (" - ");
            
                string artist = "";
                string track = "";
                
                if (media [0] != null && media [1] != null) {
                    artist = media [0];
                    track = media [1];

                    var notification = new GLib.Notification (Byte.player.current_radio.name);
                    notification.set_body ("%s - %s".printf (artist, track));
                    notification.set_icon (GLib.Icon.new_for_string (Byte.utils.get_cover_radio_file (Byte.player.current_radio.id)));
                    notification.set_priority (GLib.NotificationPriority.LOW);

                    Byte.instance.send_notification (Byte.instance.application_id, notification);
                }
            }
        } catch (Error e) {
            stderr.printf ("Error setting default avatar icon: %s ", e.message);
        }
    }
}