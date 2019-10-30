public class Views.Favorites : Gtk.EventBox {
    private Gtk.ListBox listbox;
    public signal void go_back ();
    private int item_index;
    private int item_max;
    private Gee.ArrayList<Objects.Track?> all_tracks;

    public Favorites () {} 

    construct {
        item_index = 0;
        item_max = 25;

        all_tracks = Byte.database.get_all_tracks_favorites ();

        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);
        get_style_context ().add_class ("w-round");
        
        var back_button = new Gtk.Button.from_icon_name ("byte-arrow-back-symbolic", Gtk.IconSize.MENU);
        back_button.can_focus = false;
        back_button.margin = 3;
        back_button.margin_bottom = 6;
        back_button.margin_top = 6;
        back_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        back_button.get_style_context ().add_class ("label-color-primary");

        var search_button = new Gtk.Button.from_icon_name ("edit-find-symbolic", Gtk.IconSize.MENU);
        search_button.label = _("Favorites");
        search_button.can_focus = false;
        search_button.image_position = Gtk.PositionType.LEFT;
        search_button.valign = Gtk.Align.CENTER;
        search_button.halign = Gtk.Align.CENTER;
        search_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        search_button.get_style_context ().add_class ("h3");
        search_button.get_style_context ().add_class ("label-color-primary");
        search_button.always_show_image = true;
        search_button.tooltip_text = _("Search by title, artist and album");

        var search_entry = new Widgets.SearchEntry ();
        search_entry.get_style_context ().add_class ("search-entry");
        search_entry.tooltip_text = _("Search by title, artist and album");
        search_entry.placeholder_text = _("Search by title, artist and album");

        var search_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        search_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        search_box.add (search_entry);
        search_box.add (new Gtk.Separator (Gtk.Orientation.HORIZONTAL));

        var search_revealer = new Gtk.Revealer ();
        search_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        search_revealer.add (search_box);
        search_revealer.reveal_child = false;

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        header_box.pack_start (back_button, false, false, 0);
        header_box.set_center_widget (search_button);

        listbox = new Gtk.ListBox (); 
        listbox.expand = true;

        var play_button = new Gtk.Button.from_icon_name ("media-playback-start-symbolic", Gtk.IconSize.MENU);
        play_button.always_show_image = true;
        play_button.label = _("Play");
        play_button.hexpand = true;
        play_button.margin = 6;
        play_button.margin_end = 0;
        play_button.get_style_context ().add_class ("home-button");
        play_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var shuffle_button = new Gtk.Button.from_icon_name ("media-playlist-shuffle-symbolic", Gtk.IconSize.MENU);
        shuffle_button.always_show_image = true;
        shuffle_button.label = _("Shuffle");
        shuffle_button.hexpand = true;
        shuffle_button.margin = 6;
        shuffle_button.margin_start = 0;
        shuffle_button.get_style_context ().add_class ("home-button");
        shuffle_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var action_grid = new Gtk.Grid ();
        action_grid.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        action_grid.column_spacing = 6;
        action_grid.add (play_button);
        action_grid.add (shuffle_button);

        var scrolled = new Gtk.ScrolledWindow (null, null);
        scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        scrolled.expand = true;
        scrolled.add (listbox);

        var separator = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
        separator.margin_start = 14;
        separator.margin_end = 9;

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.margin_bottom = 3;
        main_box.expand = true;
        main_box.pack_start (header_box, false, false, 0);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (action_grid, false, false);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (search_revealer, false, false, 0);
        main_box.pack_start (scrolled, true, true, 0);
        
        add (main_box);
        add_all_tracks ();

        back_button.clicked.connect (() => {
            go_back ();
        });

        search_button.clicked.connect (() => {
            if (search_revealer.reveal_child) {
                search_revealer.reveal_child = false;
                search_entry.text = "";
            } else {
                search_revealer.reveal_child = true;
                search_entry.grab_focus ();
            }            
        });

        search_entry.key_release_event.connect ((key) => {
            if (key.keyval == 65307) {
                search_revealer.reveal_child = false;
                search_entry.text = "";
            }

            return false;
        });

        search_entry.activate.connect (() => {
            if (search_entry.text != "") {
                item_index = 0;
                item_max = 100;
                
                listbox.foreach ((widget) => {
                    widget.destroy (); 
                });

                all_tracks = Byte.database.get_all_tracks_favorites_search (search_entry.text.down ());

                add_all_tracks ();
            } else {
                item_index = 0;
                item_max = 100;
                
                listbox.foreach ((widget) => {
                    widget.destroy (); 
                });

                all_tracks = Byte.database.get_all_tracks_favorites ();

                add_all_tracks ();
            }
        });
        
        search_entry.search_changed.connect (() => {    
            if (search_entry.text != "") {
                item_index = 0;
                item_max = 100;
                
                listbox.foreach ((widget) => {
                    widget.destroy (); 
                });

                all_tracks = Byte.database.get_all_tracks_favorites_search (search_entry.text);

                add_all_tracks ();
            } else {
                item_index = 0;
                item_max = 100;
                
                listbox.foreach ((widget) => {
                    widget.destroy (); 
                });

                all_tracks = Byte.database.get_all_tracks_favorites ();

                add_all_tracks ();
            }
        });

        play_button.clicked.connect (() => {
            Byte.utils.set_items (
                all_tracks,
                false,
                null
            );
        });

        shuffle_button.clicked.connect (() => {
            Byte.utils.set_items (
                all_tracks,
                true,
                null
            );
        });

        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.TrackRow;
            
            Byte.utils.set_items (
                all_tracks,
                Byte.settings.get_boolean ("shuffle-mode"),
                item.track
            );
        });

        scrolled.edge_reached.connect((pos)=> {
            if (pos == Gtk.PositionType.BOTTOM) {
                
                item_index = item_max;
                item_max = item_max + 100;

                if (item_max > all_tracks.size) {
                    item_max = all_tracks.size;
                }

                add_all_tracks ();
            }
        });

        Byte.database.updated_track_favorite.connect ((track, favorite) => {
            if (track_exists (track) == false) {
                if (favorite == 1) {
                    track.track_order = all_tracks.size + 1;
                    all_tracks.add (track);

                    item_index = item_max;
                    item_max = item_max + 100;

                    if (item_max > all_tracks.size) {
                        item_max = all_tracks.size;
                    }

                    add_all_tracks ();
                }
            } else {
                if (favorite == 0) {
                    listbox.foreach ((widget) => {
                        var item = widget as Widgets.TrackRow;
                        if (item.track.id == track.id) {
                            widget.destroy (); 
                            all_tracks.remove (track);
                        }
                    });
                }
            }
        });

        Byte.database.reset_library.connect (() => {
            listbox.foreach ((widget) => {
                Idle.add (() => {
                    widget.destroy (); 
    
                    return false;
                });
            });
        });

        Byte.scan_service.sync_started.connect (() => {
            search_entry.sensitive = false;
        });

        Byte.scan_service.sync_finished.connect (() => {
            search_entry.sensitive = true;
        });
    }
    
    private bool track_exists (Objects.Track track) {
        foreach (var item in all_tracks) {
            if (item.id == track.id) {
                return true;
            }
        }

        return false;
    }
    public void add_all_tracks () {
        if (item_max > all_tracks.size) {
            item_max = all_tracks.size;
        }

        for (int i = item_index; i < item_max; i++) {
            var row = new Widgets.TrackRow (all_tracks [i]);

            listbox.add (row);
            listbox.show_all ();
        }   
    }
}