public class Views.Albums : Gtk.EventBox {
    private Gtk.ListBox listbox;
    public signal void go_back ();
    public signal void go_album (Objects.Album album);

    private int item_index;
    private int item_max;

    private Gee.ArrayList<Objects.Album?> all_items;

    public Albums () {} 

    construct {
        item_index = 0;
        item_max = 25;

        all_items = Byte.database.get_all_albums_order_by (
            Byte.settings.get_enum ("album-sort"), 
            Byte.settings.get_boolean ("album-order-reverse")
        );

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
        search_button.label = _("Albums");
        search_button.can_focus = false;
        search_button.image_position = Gtk.PositionType.LEFT;
        search_button.valign = Gtk.Align.CENTER;
        search_button.halign = Gtk.Align.CENTER;
        search_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        search_button.get_style_context ().add_class ("h3");
        search_button.get_style_context ().add_class ("label-color-primary");
        search_button.always_show_image = true;
        search_button.tooltip_text = _("Search by title, artist, genre and year");

        var search_entry = new Widgets.SearchEntry ();
        search_entry.tooltip_text = _("Search by title, artist, genre and year");
        search_entry.placeholder_text = _("Search by title, artist, genre and year");

        var search_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        search_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        search_box.add (search_entry);
        search_box.add (new Gtk.Separator (Gtk.Orientation.HORIZONTAL));

        var search_revealer = new Gtk.Revealer ();
        search_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_UP;
        search_revealer.add (search_box);
        search_revealer.reveal_child = false;

        var sort_button = new Gtk.ToggleButton ();
        sort_button.margin = 3;
        sort_button.can_focus = false;
        sort_button.add (new Gtk.Image.from_icon_name ("byte-sort-symbolic", Gtk.IconSize.MENU));
        sort_button.tooltip_text = _("Sort");
        sort_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        sort_button.get_style_context ().add_class ("sort-button");

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        header_box.pack_start (back_button, false, false, 0);
        header_box.set_center_widget (search_button);
        header_box.pack_end (sort_button, false, false, 0);

        var sort_popover = new Widgets.Popovers.Sort (sort_button);
        sort_popover.selected = Byte.settings.get_enum ("album-sort");
        sort_popover.reverse = Byte.settings.get_boolean ("album-order-reverse");
        sort_popover.radio_01_label = _("Name");
        sort_popover.radio_02_label = _("Artist");
        sort_popover.radio_03_label = _("Year");
        sort_popover.radio_04_label = _("Genre");
        
        sort_popover.radio_05_visible = false;

        listbox = new Gtk.ListBox (); 
        listbox.expand = true;

        var scrolled = new Gtk.ScrolledWindow (null, null);
        scrolled.margin_top = 3;
        scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        scrolled.expand = true;
        scrolled.add (listbox);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.margin_bottom = 3;
        main_box.expand = true;
        main_box.pack_start (header_box, false, false, 0);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false);
        main_box.pack_start (search_revealer, false, false, 0);
        main_box.pack_start (scrolled, true, true, 0);
        
        add (main_box);
        add_all_items ();

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
        
        search_entry.search_changed.connect (() => {
            if (search_entry.text != "") {
                item_index = 0;
                item_max = 100;
                
                listbox.foreach ((widget) => {
                    widget.destroy (); 
                });

                all_items = Byte.database.get_all_albums_search (search_entry.text.down ());

                add_all_items ();
            } else {
                item_index = 0;
                item_max = 100;
                
                listbox.foreach ((widget) => {
                    widget.destroy (); 
                });

                all_items = Byte.database.get_all_albums_order_by (
                    Byte.settings.get_enum ("album-sort"), 
                    Byte.settings.get_boolean ("album-order-reverse")
                );

                add_all_items ();
            }
        });

        sort_button.toggled.connect (() => {
            if (sort_button.active) {
                sort_popover.show_all ();
            }
        });

        sort_popover.closed.connect (() => {
            sort_button.active = false;
        });

        sort_popover.mode_changed.connect ((mode) => {
            Byte.settings.set_enum ("album-sort", mode);

            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_items = Byte.database.get_all_albums_order_by (mode, Byte.settings.get_boolean ("album-order-reverse"));

            add_all_items ();
        });

        sort_popover.order_reverse.connect ((reverse) => {
            Byte.settings.set_boolean ("album-order-reverse", reverse); 

            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_items = Byte.database.get_all_albums_order_by (
                Byte.settings.get_enum ("album-sort"), 
                Byte.settings.get_boolean ("album-order-reverse")
            );

            add_all_items ();
        });
        
        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.AlbumRow;
            go_album (item.album);
        });

        Byte.database.added_new_album.connect ((album) => {
            Idle.add (() => {
                add_item (album);

                return false;
            });
        });

        scrolled.edge_reached.connect((pos)=> {
            if (pos == Gtk.PositionType.BOTTOM) {
                
                item_index = item_max;
                item_max = item_max + 200;

                if (item_max > all_items.size) {
                    item_max = all_items.size;
                }

                add_all_items ();
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
            sort_button.sensitive = false;
            search_entry.sensitive = false;
        });

        Byte.scan_service.sync_finished.connect (() => {
            sort_button.sensitive = true;
            search_entry.sensitive = true;
        });
    }

    private void add_item (Objects.Album album) {
        if (album.id != 0) {
            var row = new Widgets.AlbumRow (album);
            
            listbox.add (row);
            listbox.show_all ();
        }
    }

    public void add_all_items () {
        if (item_max > all_items.size) {
            item_max = all_items.size;
        }

        for (int i = item_index; i < item_max; i++) {
            var row = new Widgets.AlbumRow (all_items [i]);

            listbox.add (row);
            listbox.show_all ();
        }   
    }
}