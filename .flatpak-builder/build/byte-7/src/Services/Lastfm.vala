public class Services.Lastfm : GLib.Object {
    private Soup.Session session;

    private const string API_KEY = "a33950e8cc5e7130f42697fa957ec42a";
    private const string ROOT_URL = "http://ws.audioscrobbler.com/2.0/";

    public signal void radio_cover_track_found (string url);
    public Lastfm () {
        session = new Soup.Session ();

        Byte.player.current_radio_title_changed.connect ((title) => {
            get_current_radio_cover (title);
        });

        Byte.player.current_track_changed.connect ((track) => {
            if (Byte.scan_service.is_sync == false) {
                var cover_path = File.new_for_path (GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track.id)));
            
                if (cover_path.query_exists () == false) {
                    if (Byte.settings.get_boolean ("auto-download-covers")) {
                        get_current_track_cover (track);
                    }
                }
            }
        });
    }

    public void get_current_track_cover (Objects.Track track) {
        if (track.title != "" && track.artist_name != "") {
            string url = ROOT_URL;
            url = url + "?method=track.getInfo";
            url = url + "&api_key=" + API_KEY;
            url = url + "&artist=" + track.artist_name;
            url = url + "&track=" + track.title;
            url = url + "&format=json";

            var message = new Soup.Message ("GET", url);

            session.queue_message (message, (sess, mess) => {
                if (mess.status_code == 200) {
                    var parser = new Json.Parser ();

                    try {
                    parser.load_from_data ((string) mess.response_body.flatten ().data, -1);

                        var node = parser.get_root ().get_object ();

                        var track_object = node.get_object_member ("track");
                        var album_object = track_object.get_object_member ("album");

                        var image = album_object.get_array_member ("image");

                        foreach (unowned Json.Node item in image.get_elements ()) {
                            var object = item.get_object();

                            if (object.get_string_member ("size") == "large") {
                                if (object.get_string_member ("#text") != null || object.get_string_member ("#text") != "") {
                                    download_cover (
                                        track, 
                                        object.get_string_member ("#text"));
                                }
                            }
                        }
                    } catch (Error e) {
                            
                    }
                } else {
                }
            });
        }
    }

    private void download_cover (Objects.Track track, string uri) {
        var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track.id));

        var file_path = File.new_for_path (cover_path);
        var file_from_uri = File.new_for_uri (uri);

        MainLoop loop = new MainLoop ();

        file_from_uri.copy_async.begin (file_path, 0, Priority.DEFAULT, null, (current_num_bytes, total_num_bytes) => {
            // Report copy-status:
            print ("%" + int64.FORMAT + " bytes of %" + int64.FORMAT + " bytes copied.\n", current_num_bytes, total_num_bytes);
        }, (obj, res) => {
            try {
                if (file_from_uri.copy_async.end (res)) {
                    print ("Cover %s was downloaded\n".printf (track.title));
                    Byte.database.updated_track_cover (track.id);

                    if (Byte.settings.get_boolean ("save-id3-tags")) {
                        add_id3_image (track, cover_path);
                    }
                }
            } catch (Error e) {
                print ("Error: %s\n", e.message);
            }

            loop.quit ();
        });

        loop.run ();
    }

    public void get_current_radio_cover (string? title) {
        if (title != null) {
            var media = title.split (" - ");
        
            string artist = "";
            string track = "";
            
            if (media [0] != null && media [1] != null) {
                artist = media [0];
                track = media [1];
                
                string url = ROOT_URL;
                url = url + "?method=track.getInfo";
                url = url + "&api_key=" + API_KEY;
                url = url + "&artist=" + artist;
                url = url + "&track=" + track;
                url = url + "&format=json";

                var message = new Soup.Message ("GET", url);

                session.queue_message (message, (sess, mess) => {
                    if (mess.status_code == 200) {
                        var parser = new Json.Parser ();

                        try {
                            parser.load_from_data ((string) mess.response_body.flatten ().data, -1);

                            var node = parser.get_root ().get_object ();

                            var track_object = node.get_object_member ("track");
                            var album_object = track_object.get_object_member ("album");

                            var image = album_object.get_array_member ("image");

                            foreach (unowned Json.Node item in image.get_elements ()) {
                                var object = item.get_object();

                                if (object.get_string_member ("size") == "large") {
                                    if (object.get_string_member ("#text") != null || object.get_string_member ("#text") != "") {
                                        radio_cover_track_found (object.get_string_member ("#text"));
                                    }
                                }
                            }
                        } catch (Error e) {
                          
                        }
                    } else {

                    }
                });
            }
        }
    }

    private void add_id3_image (Objects.Track track, string cover_path) {
        string track_path = track.path.substring (7).replace ("%20", " ");
        string response = "";
        string command = "eyeD3 --add-image='%s':FRONT_COVER '%s'".printf (cover_path, track_path);

        try {
            GLib.Process.spawn_command_line_sync (command, out response);

            print ("---------------------------------\n");
            print ("Error: %s\n".printf (response));
            print ("---------------------------------\n");
        } catch (SpawnError e) {
		    print ("Error: %s\n", e.message);
	    }
        /*
        if ("file not found" in response) {
            print ("Error: %s\n".printf (response));
        }
        */
    }
}