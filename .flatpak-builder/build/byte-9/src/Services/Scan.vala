public class Services.Scan : GLib.Object {
    public signal void sync_started ();
    public signal void sync_finished ();
    public signal void sync_progress (double fraction);
    
    public int counter = 0;
    public int counter_max = 0;
    public bool is_sync = false;
    construct {
        Byte.tg_manager.discovered_new_item.connect (discovered_new_local_item);
        
        Byte.database.adden_new_track.connect (() => {
            Idle.add (() => {
                counter--;
                print ("%i/%i\n".printf (counter, counter_max));
                sync_progress (((double) counter_max - (double) counter) / (double) counter_max);
                if (counter <= 0) {
                    sync_finished ();
                    is_sync = false;
                    counter_max = 0;
                }
                
                return false;
            });
        });
    }

    public void scan_local_files (string uri) {
        new Thread<void*> ("scan_local_files", () => {
            File directory = File.new_for_uri (uri.replace ("#", "%23"));
            try {
                var children = directory.enumerate_children ("standard::*," + FileAttribute.STANDARD_CONTENT_TYPE + "," + FileAttribute.STANDARD_IS_HIDDEN + "," + FileAttribute.STANDARD_IS_SYMLINK + "," + FileAttribute.STANDARD_SYMLINK_TARGET, GLib.FileQueryInfoFlags.NONE);
                FileInfo file_info = null;

                while ((file_info = children.next_file ()) != null) {
                    if (file_info.get_is_hidden ()) {
                        continue;
                    }

                    if (file_info.get_is_symlink ()) {
                        string target = file_info.get_symlink_target ();
                        var symlink = File.new_for_path (target);
                        var file_type = symlink.query_file_type (0);
                        if (file_type == FileType.DIRECTORY) {
                            scan_local_files (target);
                        }
                    } else if (file_info.get_file_type () == FileType.DIRECTORY) {
                        // Without usleep it crashes on smb:// protocol
                        if (!directory.get_uri ().has_prefix ("file://")) {
                            Thread.usleep (1000000);
                        }

                        scan_local_files (directory.get_uri () + "/" + file_info.get_name ());
                    } else {
                        string mime_type = file_info.get_content_type ();
                        if (is_audio_file (mime_type)) {
                            found_music_file (directory.get_uri () + "/" + file_info.get_name ().replace ("#", "%23"));
                        }
                    }
                }

                children.close ();
                children.dispose ();
            } catch (Error err) {
                warning ("%s\n%s", err.message, uri);
            }

            directory.dispose ();
            return null;
        });
    } 

    public void found_music_file (string uri) {
        print ("URI: %s\n".printf (uri));

        new Thread<void*> ("found_local_music_file", () => {
            if (Byte.database.music_file_exists (uri) == false && Byte.database.music_blacklist_exists (uri) == false) {
                Byte.tg_manager.add_discover_uri (uri);
            }
            
            return null;
        });
    }

    public string? choose_folder (MainWindow window) {
        string? return_value = null;

        Gtk.FileChooserDialog chooser = new Gtk.FileChooserDialog (
            _ ("Select a folder."), window, Gtk.FileChooserAction.SELECT_FOLDER,
            _ ("Cancel"), Gtk.ResponseType.CANCEL,
            _ ("Open"), Gtk.ResponseType.ACCEPT);

        var filter = new Gtk.FileFilter ();
        filter.set_filter_name (_ ("Folder"));
        filter.add_mime_type ("inode/directory");

        chooser.add_filter (filter);

        if (chooser.run () == Gtk.ResponseType.ACCEPT) {
            return_value = chooser.get_file ().get_uri ();
        }

        chooser.destroy ();
        return return_value;
    }

    public void discovered_new_local_item (Objects.Artist artist, Objects.Album album, Objects.Track track) {
        if (counter == 0) {
            sync_started ();
            is_sync = true;
        }
        
        counter++;
        counter_max++;

        new Thread<void*> ("discovered_new_local_item", () => {
            album.artist_id = Byte.database.insert_artist_if_not_exists (artist);
            album.artist_name = artist.name;

            track.album_id = Byte.database.insert_album_if_not_exists (album);
            track.artist_name = artist.name;
            track.album_title = album.title;

            Byte.database.insert_track (track);
            return null;
        });
    }

    public static bool is_audio_file (string mime_type) {
        return mime_type.has_prefix ("audio/") && !mime_type.contains ("x-mpegurl") && !mime_type.contains ("x-scpls");
    }
}