public class Services.Player : GLib.Object {
    public signal void state_changed (Gst.State state);
    public signal void mode_changed (string mode);

    public signal void current_progress_changed (double percent);
    public signal void current_duration_changed (int64 duration);

    public signal void current_track_changed (Objects.Track? track);
    public signal void current_radio_changed (Objects.Radio? radio);

    public signal void toggle_playing (); 
    public signal void current_radio_title_changed (string? title);

    uint progress_timer = 0;

    public Objects.Track? current_track { get; set; }
    public Objects.Radio? current_radio { get; private set; }
    public string? current_radio_title { get; set; }
    public string? mode { get; set; }
    public Gst.State? player_state { get; set; }

    Gst.Format fmt = Gst.Format.TIME;
    dynamic Gst.Element playbin;
    Gst.Bus bus;

    public unowned int64 duration {
        get {
            int64 d = 0;
            playbin.query_duration (fmt, out d);
            return d;
        }
    }

    public unowned int64 position {
        get {
            int64 d = 0;
            playbin.query_position (fmt, out d);
            return d;
        }
    }

    public double target_progress { get; set; default = 0; }

    public Player () {
        playbin = Gst.ElementFactory.make ("playbin", "play");

        bus = playbin.get_bus ();
        bus.add_watch (0, bus_callback);
        bus.enable_sync_message_emission ();

        state_changed.connect ((state) => {
            player_state = state;
            stop_progress_signal ();

            if (state != Gst.State.NULL) {
                playbin.set_state (state);
            }
            
            switch (state) {
                case Gst.State.PLAYING:
                    start_progress_signal ();
                    break;
                case Gst.State.READY:
                    stop_progress_signal (true);
                    break;
                case Gst.State.PAUSED:
                    pause_progress_signal ();
                    break;
            }
        });

        current_track_changed.connect ((track) => {
            current_radio = null;
            
            if (track != null && Byte.scan_service.is_sync == false) {
                Byte.database.add_track_count (track);
            }
        });

        current_radio_changed.connect ((radio) => {
            current_track = null;
        });

        /*
        var simple_command = new Granite.Services.SimpleCommand (
            "/usr/bin/",
            "acpi_listen"
        );

        simple_command.run ();

        simple_command.output_changed.connect ((text) => {
            print ("%s\n".printf (text));

            if ("unplug" in text) {
                Byte.player.state_changed (Gst.State.PAUSED);
            } else if ("plug" in text) {
                Byte.player.state_changed (Gst.State.PLAYING);
            }
        });
        */
    }
    
    public void set_radio (Objects.Radio radio) {
        if (radio == current_radio || radio == null || radio.file == null) {
            return;
        } 
        
        mode_changed ("radio");
        mode = "radio";
        current_radio_changed (radio);

        current_radio = radio;

        stop ();
        playbin.uri = radio.file;
        playbin.set_state (Gst.State.PLAYING);
        state_changed (Gst.State.PLAYING);
        player_state = Gst.State.PLAYING;
        play ();
    }

    public void set_track (Objects.Track? track) {
        if (track == null) {
            current_duration_changed (0);
        }

        if (load_track (track)) {
            current_track_changed (track);
            mode_changed ("track");
            mode = "track";

            play ();
        }
    }

    public bool load_track (Objects.Track? track, double progress = 0) {
        if (track == current_track || track == null) {
            return false;
        }

        current_track = track;
        
        var last_state = get_state ();
        stop ();

        playbin.uri = current_track.path;
        playbin.set_state (Gst.State.PLAYING);
        state_changed (Gst.State.PLAYING);
        player_state = Gst.State.PLAYING;

        while (duration == 0) {};

        if (last_state != Gst.State.PLAYING) {
            pause ();
        }

        current_duration_changed (duration);

        if (progress > 0) {
            seek_to_progress (progress);
            current_progress_changed (progress);
        }
        
        return true;
    }

    public void seek_to_position (int64 position) {
        playbin.seek_simple (fmt, Gst.SeekFlags.FLUSH, position);
    }

    public void seek_to_progress (double percent) {
        seek_to_position ((int64)(percent * duration));
    }

    private unowned int64 get_position_sec () {
        int64 current = position;
        return current > 0 ? current / Gst.SECOND : -1;
    }
    
    public unowned double get_position_progress () {
        return (double) 1 / duration * position;
    }

    public Gst.State get_state () {
        Gst.State state = Gst.State.NULL;
        Gst.State pending;
        playbin.get_state (out state, out pending, (Gst.ClockTime) (Gst.SECOND));
        return state;
    }

    public void pause_progress_signal () {
        if (progress_timer != 0) {
            Source.remove (progress_timer);
            progress_timer = 0;
        }
    }
    
    public void stop_progress_signal (bool reset_timer = false) {
        pause_progress_signal ();
        if (reset_timer) {
            current_progress_changed (0);
        }
    }

    public void start_progress_signal () {
        pause_progress_signal ();
        progress_timer = GLib.Timeout.add (250, () => {
            current_progress_changed (get_position_progress ());
            return true;
        });
    }
    
    public void play () {
        if (current_track != null) {
            state_changed (Gst.State.PLAYING);
            player_state = Gst.State.PLAYING;
        }
    }

    public void pause () {
        state_changed (Gst.State.PAUSED);
        player_state = Gst.State.PAUSED;
    }

    public void stop () {
        state_changed (Gst.State.READY);
        player_state = Gst.State.READY;
    }

    public void next () {
        if (current_track == null) {
            return;
        }

        Objects.Track? next_track = null;

        var repeat_mode = Byte.settings.get_enum ("repeat-mode");
        if (repeat_mode == 2) {
            next_track = current_track;
            current_track = null;
        } else {
            next_track = Byte.utils.get_next_track (current_track);
        }

        if (next_track != null) {
            set_track (next_track);
        } else {
            state_changed (Gst.State.NULL);
            player_state = Gst.State.NULL;
        }
    }

    public void prev () {        
        if (current_track == null) {
            return;
        }

        if (get_position_sec () < 1) {
            Objects.Track? prev_track = null;

            prev_track = Byte.utils.get_prev_track (current_track);

            if (prev_track != null) {
                set_track (prev_track);
            }
        } else {
            stop ();
            play ();

            current_track_changed (current_track);
        }
    }

    private bool bus_callback (Gst.Bus bus, Gst.Message message) {
        switch (message.type) {
            case Gst.MessageType.ERROR:
                GLib.Error err;
                string debug;
                message.parse_error (out err, out debug);
                warning ("Error: %s\n%s\n", err.message, debug);

                if (mode == "radio") {

                } else {
                    next ();
                }

                break;
            case Gst.MessageType.EOS:
                next ();
                break;
            case Gst.MessageType.TAG: 
                Gst.TagList tag_list;
                string title;

                message.parse_tag (out tag_list);
                tag_list.get_string (Gst.Tags.TITLE, out title);
                tag_list = null;

                current_radio_title = title;

                if (current_radio_title != null && mode == "radio") {
                    print ("Radio current title: %s\n".printf (current_radio_title));
                    current_radio_title_changed (current_radio_title);
                }
                
                break;
            default:
                break;
            }

        return true;
    }
}
