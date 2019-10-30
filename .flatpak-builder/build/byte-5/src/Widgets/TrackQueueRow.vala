public class Widgets.TrackQueueRow : Gtk.ListBoxRow {
    public Objects.Track track { get; construct; }

    public signal void remove_track (int id);

    private Gtk.Label track_title_label;
    private Gtk.Label artist_album_label;
    private Gtk.Button remove_button;

    private Widgets.Cover image_cover;
    private bool is_current_track;
    public TrackQueueRow (Objects.Track track) {
        Object (
            track: track
        );
    }

    construct {
        is_current_track = false;
        get_style_context ().add_class ("track-queue-row");
        
        var playing_icon = new Gtk.Image ();
        playing_icon.gicon = new ThemedIcon ("audio-volume-medium-symbolic");
        playing_icon.get_style_context ().add_class ("playing-ani-color");
        playing_icon.pixel_size = 12;

        var playing_revealer = new Gtk.Revealer ();
        playing_revealer.halign = Gtk.Align.CENTER;
        playing_revealer.valign = Gtk.Align.CENTER;
        playing_revealer.transition_type = Gtk.RevealerTransitionType.CROSSFADE;
        playing_revealer.add (playing_icon);
        playing_revealer.reveal_child = false;

        track_title_label = new Gtk.Label (track.title);
        track_title_label.get_style_context ().add_class ("font-bold");
        track_title_label.ellipsize = Pango.EllipsizeMode.END;
        track_title_label.max_width_chars = 40;
        track_title_label.halign = Gtk.Align.START;
        track_title_label.valign = Gtk.Align.END;

        artist_album_label = new Gtk.Label (track.artist_name);
        artist_album_label.halign = Gtk.Align.START;
        artist_album_label.use_markup = true;
        artist_album_label.valign = Gtk.Align.START;
        artist_album_label.max_width_chars = 40;
        artist_album_label.ellipsize = Pango.EllipsizeMode.END;
        artist_album_label.get_style_context ().add_class ("font-size-small");

        image_cover = new Widgets.Cover.from_file (
            GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track.id)), 
            27, 
            "track");
        image_cover.halign = Gtk.Align.START;
        image_cover.valign = Gtk.Align.START;

        remove_button = new Gtk.Button.from_icon_name ("window-close-symbolic", Gtk.IconSize.MENU);
        remove_button.valign = Gtk.Align.CENTER;
        remove_button.halign = Gtk.Align.END;
        remove_button.hexpand = true;
        remove_button.can_focus = false;
        remove_button.tooltip_text = _("Remove");
        remove_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        remove_button.get_style_context ().add_class ("remove-button");
        
        var remove_revealer = new Gtk.Revealer ();
        remove_revealer.halign = Gtk.Align.END;
        remove_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_LEFT;
        remove_revealer.add (remove_button);
        remove_revealer.reveal_child = false;

        var overlay = new Gtk.Overlay ();
        overlay.halign = Gtk.Align.START;
        overlay.valign = Gtk.Align.START;
        overlay.add_overlay (playing_revealer);
        overlay.add (image_cover); 

        var main_grid = new Gtk.Grid ();
        main_grid.margin_start = 3;
        main_grid.margin_end = 9;
        main_grid.column_spacing = 3;
        main_grid.attach (overlay, 0, 0, 1, 2);
        main_grid.attach (track_title_label, 1, 0, 1, 1);
        main_grid.attach (artist_album_label, 1, 1, 1, 1);
        main_grid.attach (remove_revealer, 2, 0, 2, 2);
        
        var eventbox = new Gtk.EventBox ();
        eventbox.add_events (Gdk.EventMask.ENTER_NOTIFY_MASK | Gdk.EventMask.LEAVE_NOTIFY_MASK);
        eventbox.add (main_grid);

        add (eventbox);
        
        eventbox.enter_notify_event.connect ((event) => {
            if (!is_current_track) {
                remove_revealer.reveal_child = true;
                remove_button.get_style_context ().add_class ("closed");
            }

            return false;
        });

        eventbox.leave_notify_event.connect ((event) => {
            if (event.detail == Gdk.NotifyType.INFERIOR) {
                return false;
            }

            if (!is_current_track) {
                remove_button.get_style_context ().remove_class ("closed");
                remove_revealer.reveal_child = false;
            }

            return false;
        });

        remove_button.clicked.connect (() => {
            remove_track (track.id);
        });

        Byte.player.current_track_changed.connect ((current_track) => {
            if (current_track.id == track.id) {
                is_current_track = true;
                playing_revealer.reveal_child =  true;

                main_grid.get_style_context ().add_class ("label-color-primary");
                grab_focus ();
            } else {
                is_current_track = false;
                playing_revealer.reveal_child =  false;
                main_grid.get_style_context ().remove_class ("label-color-primary");
            }
        });

        if (Byte.player.current_track != null && track.id == Byte.player.current_track.id) {
            playing_revealer.reveal_child = true;
            main_grid.get_style_context ().add_class ("label-color-primary");
        }

        Byte.database.removed_track.connect ((track_id) => {
            if (track_id == track.id) {
                destroy ();
            }
        });

        Byte.database.updated_track_cover.connect ((track_id) => {
            Idle.add (() => {
                if (track_id == track.id) {
                    try {
                        image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (
                            GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track_id)), 
                            32, 
                            32);
                    } catch (Error e) {
                        stderr.printf ("Error setting default avatar icon: %s ", e.message);
                    }
                }
                
                return false;
            });
        });
    }
}
