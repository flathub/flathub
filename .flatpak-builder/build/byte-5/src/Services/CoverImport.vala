public class Services.CoverImport : GLib.Object {
    private const int DISCOVERER_TIMEOUT = 5;

    private Gst.PbUtils.Discoverer discoverer;
    construct {
        try {
            discoverer = new Gst.PbUtils.Discoverer ((Gst.ClockTime) (DISCOVERER_TIMEOUT * Gst.SECOND));
        } catch (Error err) {
            critical ("Could not create Gst discoverer object: %s", err.message);
        }
    }

    public void import (Objects.Track track) {
        try {
            var info = discoverer.discover_uri (track.path);
            read_info (info, track);
        } catch (Error err) {
            critical ("%s - %s, Error while importing …".printf (
                track.artist_name, track.title
            ));
        }
    }

    private void read_info (Gst.PbUtils.DiscovererInfo info, Objects.Track track) {
        string uri = info.get_uri ();
        bool gstreamer_discovery_successful = false;
        switch (info.get_result ()) {
            case Gst.PbUtils.DiscovererResult.OK:
                gstreamer_discovery_successful = true;
            break;

            case Gst.PbUtils.DiscovererResult.URI_INVALID:
                warning ("GStreamer could not import '%s': invalid URI.", uri);
            break;

            case Gst.PbUtils.DiscovererResult.ERROR:
                warning ("GStreamer could not import '%s'", uri);
            break;

            case Gst.PbUtils.DiscovererResult.TIMEOUT:
                warning ("GStreamer could not import '%s': Discovery timed out.", uri);
            break;

            case Gst.PbUtils.DiscovererResult.BUSY:
                warning ("GStreamer could not import '%s': Already discovering a file.", uri);
            break;

            case Gst.PbUtils.DiscovererResult.MISSING_PLUGINS:
                warning ("GStreamer could not import '%s': Missing plugins.", uri);
            break;
        }

        if (gstreamer_discovery_successful) {
            Idle.add (() => {
                Gdk.Pixbuf pixbuf = null;
                var tag_list = info.get_tags ();
                var sample = get_cover_sample (tag_list);

                if (sample == null) {
                    tag_list.get_sample_index (Gst.Tags.PREVIEW_IMAGE, 0, out sample);
                }

                if (sample != null) {
                    var buffer = sample.get_buffer ();

                    if (buffer != null) {
                        pixbuf = get_pixbuf_from_buffer (buffer);
                        if (pixbuf != null) {
                            save_cover_pixbuf (pixbuf, track);
                        }
                    }

                    debug ("Final image buffer is NULL for '%s'", info.get_uri ());
                } else {
                    debug ("Image sample is NULL for '%s'", info.get_uri ());
                }

                return false;
            });
        }
    }

    private Gst.Sample? get_cover_sample (Gst.TagList tag_list) {
        Gst.Sample cover_sample = null;
        Gst.Sample sample;
        for (int i = 0; tag_list.get_sample_index (Gst.Tags.IMAGE, i, out sample); i++) {
            var caps = sample.get_caps ();
            unowned Gst.Structure caps_struct = caps.get_structure (0);
            int image_type = Gst.Tag.ImageType.UNDEFINED;
            caps_struct.get_enum ("image-type", typeof (Gst.Tag.ImageType), out image_type);
            if (image_type == Gst.Tag.ImageType.UNDEFINED && cover_sample == null) {
                cover_sample = sample;
            } else if (image_type == Gst.Tag.ImageType.FRONT_COVER) {
                return sample;
            }
        }

        return cover_sample;
    }

    private Gdk.Pixbuf? get_pixbuf_from_buffer (Gst.Buffer buffer) {
        Gst.MapInfo map_info;

        if (!buffer.map (out map_info, Gst.MapFlags.READ)) {
            warning ("Could not map memory buffer");
            return null;
        }

        Gdk.Pixbuf pix = null;

        try {
            var loader = new Gdk.PixbufLoader ();

            if (loader.write (map_info.data) && loader.close ())
                pix = loader.get_pixbuf ();
        } catch (Error err) {
            warning ("Error processing image data: %s", err.message);
        }

        buffer.unmap (map_info);

        return pix;
    }

    private void save_cover_pixbuf (Gdk.Pixbuf p, Objects.Track track) {
        Gdk.Pixbuf ? pixbuf = Byte.utils.align_and_scale_pixbuf (p, 256);

        try {
            string album_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, 
                ("album-%i.jpg").printf (track.album_id));

            string track_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, 
                ("track-%i.jpg").printf (track.id));

            if (pixbuf.save (album_path, "jpeg", "quality", "100")) {
                Byte.database.updated_album_cover (track.album_id);
            }

            if (pixbuf.save (track_path, "jpeg", "quality", "100")) {
                Byte.database.updated_track_cover (track.id);
            }
        } catch (Error err) {
            warning (err.message);
        }
    }
}
