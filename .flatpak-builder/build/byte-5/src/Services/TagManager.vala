public class Services.TagManager : GLib.Object {
    public signal void discovered_new_item (Objects.Artist artist, Objects.Album album, Objects.Track track);    
    private Gst.PbUtils.Discoverer discoverer;
    string unknown = _("Unknown");
     
    construct {
        try {
            discoverer = new Gst.PbUtils.Discoverer ((Gst.ClockTime) (5 * Gst.SECOND));
            discoverer.start ();
            discoverer.discovered.connect (discovered);
        } catch (Error err) {
            warning (err.message);
        }
    }

    private void discovered (Gst.PbUtils.DiscovererInfo info, Error? err) {
        new Thread<void*> (null, () => {
            string uri = info.get_uri ();

            if (info.get_result () != Gst.PbUtils.DiscovererResult.OK) {
                if (err != null) {
                    warning ("DISCOVER ERROR: '%d' %s %s\n(%s)", err.code, err.message, info.get_result ().to_string (), uri);
                }
            } else {
                var tags = info.get_tags ();

                if (tags != null) {
                    uint64 duration = info.get_duration ();
                    string o;
                    GLib.Date? d; 
                    Gst.DateTime? dt;
                    uint u;
                    double dou;

                    // TRACK OBJECT
                    var track = new Objects.Track ();
                    track.duration = duration;
                    track.path = uri;

                    if (tags.get_string (Gst.Tags.TITLE, out o)) {
                        track.title = o;
                    }

                    if (track.title.strip () == "") {
                        track.title = Path.get_basename (uri);
                    }
                    
                    if (tags.get_uint (Gst.Tags.TRACK_NUMBER, out u)) {
                        track.track = (int)u;
                    }
                    
                    if (tags.get_uint (Gst.Tags.ALBUM_VOLUME_NUMBER, out u)) {
                        track.disc = (int)u;
                    }

                    if (tags.get_string (Gst.Tags.COMPOSER, out o)) {
                        track.composer = o; 
                    }
                    
                    if (tags.get_string (Gst.Tags.GROUPING, out o)) {
                        track.grouping = o;
                    }
                    
                    if (tags.get_string (Gst.Tags.LYRICS, out o)) {
                        track.lyrics = o;
                    }

                    // BITRATE
                    var file = new TagLib.File (File.new_for_uri (uri).get_path ());
                    track.bitrate = file.audioproperties.bitrate;
                    track.samplerate = file.audioproperties.samplerate;
                    track.channels = file.audioproperties.channels;
                    
                    if (tags.get_uint (Gst.Tags.USER_RATING, out u)) {
                        print ("USER_RATING: %s\n".printf (u.to_string ()));
                        track.rating = (int) u;
                    }

                    if (tags.get_double (Gst.Tags.BEATS_PER_MINUTE, out dou)) {
                        track.bpm = (int) dou.clamp (0, dou);
                    }
    
                    // ALBUM OBJECT
                    var album = new Objects.Album ();
                    if (tags.get_string (Gst.Tags.ALBUM, out o)) {
                        album.title = o;
                    }

                    if (album.title.strip () == "") {
                        var dir = Path.get_dirname (uri);
                        if (dir != null) {
                            album.title = Path.get_basename (dir);
                        } else {
                            album.title = unknown;
                        }
                    }

                    if (tags.get_date_time (Gst.Tags.DATE_TIME, out dt)) {
                        if (dt != null) {
                            album.year = dt.get_year ();
                            track.year = dt.get_year ();
                        } else if (tags.get_date (Gst.Tags.DATE, out d)) {
                            if (d != null) {
                                album.year = dt.get_year ();
                                track.year = dt.get_year ();
                            }
                        }
                    }

                    if (tags.get_string (Gst.Tags.GENRE, out o)) {
                        album.genre = o;
                        track.genre = o;
                    }

                    // ARTIST OBJECT
                    var artist = new Objects.Artist ();
                    if (tags.get_string (Gst.Tags.ALBUM_ARTIST, out o)) {
                        track.album_artist = o;
                        artist.name = o;
                    } else if (tags.get_string (Gst.Tags.ARTIST, out o)) {
                        artist.name = o;
                    }

                    if (artist.name.strip () == "") {
                        var dir = Path.get_dirname (Path.get_dirname (uri));
                        if (dir != null) {
                            artist.name = Path.get_basename (dir);
                        } else {
                            artist.name = unknown;
                        }
                    }

                    discovered_new_item (artist, album, track);
                }
            }

            info.dispose ();
            return null;
        });
    }

    public void add_discover_uri (string uri) {
        discoverer.discover_uri_async (uri);
    }
}
