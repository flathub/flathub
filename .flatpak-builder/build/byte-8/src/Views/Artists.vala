public class Views.Artists : Gtk.EventBox {
    private Gtk.ListBox listbox;
    private Widgets.SearchEntry search_entry;
    public signal void go_back ();  
    public signal void go_artist (Objects.Artist artist);
    
    private int item_index;
    private int item_max;

    private Gee.ArrayList<Objects.Artist?> all_items;
    
    public Artists () {

    }

    construct { 
        item_index = 0;
        item_max = 25;

        all_items = Byte.database.get_all_artists ();

        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);
        get_style_context ().add_class ("w-round");
 
        var back_button = new Gtk.Button.from_icon_name ("byte-arrow-back-symbolic", Gtk.IconSize.MENU);
        back_button.can_focus = false;
        back_button.margin = 3;
        back_button.margin_bottom = 6;
        back_button.margin_top = 6;
        back_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        back_button.get_style_context ().add_class ("label-color-primary");

        var title_label = new Gtk.Label ("<b>%s</b>".printf (_("Artists")));
        title_label.use_markup = true;
        title_label.valign = Gtk.Align.CENTER;
        title_label.get_style_context ().add_class ("h3");

        search_entry = new Widgets.SearchEntry ();
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

        var search_button = new Gtk.Button.from_icon_name ("edit-find-symbolic", Gtk.IconSize.MENU);
        search_button.margin = 3;
        search_button.can_focus = false;
        search_button.get_style_context ().add_class ("label-color-primary");
        search_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        header_box.pack_start (back_button, false, false, 0);
        header_box.set_center_widget (title_label);
        header_box.pack_end (search_button, false, false, 0);

        listbox = new Gtk.ListBox ();
        listbox.expand = true;

        var scrolled = new Gtk.ScrolledWindow (null, null);
        scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        scrolled.expand = true;
        scrolled.add (listbox);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.expand = true;
        main_box.pack_start (header_box, false, false, 0);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (search_revealer, false, false, 0);
        main_box.pack_start (scrolled, true, true, 0);
        
        add (main_box);
        add_all_items ();

        back_button.clicked.connect (() => {
            go_back ();
        });

        scrolled.edge_reached.connect((pos)=> {
            if (pos == Gtk.PositionType.BOTTOM) {
                
                item_index = item_max;
                item_max = item_max + 100;

                if (item_max > all_items.size) {
                    item_max = all_items.size;
                }

                add_all_items ();
            }
        });

        search_button.clicked.connect (() => {
            if (search_revealer.reveal_child) {
                search_revealer.reveal_child = false;
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

        search_entry.activate.connect (start_search);
        search_entry.search_changed.connect (start_search);

        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.ArtistRow;
            go_artist (item.artist);
        });

        Byte.database.added_new_artist.connect ((artist) => {
            Idle.add (() => {
                add_artist (artist);

                return false;
            });
        });

        Byte.database.reset_library.connect (() => {
            listbox.foreach ((widget) => {
                Idle.add (() => {
                    widget.destroy (); 
    
                    return false;
                });
            });
        });
    }

    private void start_search () {
        if (search_entry.text != "") {
            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_items = Byte.database.get_all_artists_search (
                search_entry.text.down ()
            );

            add_all_items ();
        } else {
            item_index = 0;
            item_max = 100;
            
            listbox.foreach ((widget) => {
                widget.destroy (); 
            });

            all_items = Byte.database.get_all_artists ();
            add_all_items ();
        }
    }

    private void add_artist (Objects.Artist artist) {
        if (artist.id != 0) {
            var row = new Widgets.ArtistRow (artist);
            listbox.add (row);
        
            listbox.show_all ();
        }
    }

    public void add_all_items () {
        if (item_max > all_items.size) {
            item_max = all_items.size;
        }

        for (int i = item_index; i < item_max; i++) {
            var row = new Widgets.ArtistRow (all_items [i]);

            listbox.add (row);
            listbox.show_all ();
        }   
    }
}