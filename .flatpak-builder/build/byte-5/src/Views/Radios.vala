public class Views.Radios : Gtk.EventBox {
    private Gtk.ListBox listbox;
    public signal void go_back ();
    private int item_index;
    private int item_max;
    private Gee.ArrayList<Objects.Radio?> all_radios;
    public signal void show_quick_find ();
    public Radios () {}

    construct {
        item_index = 0;
        item_max = 25;

        all_radios = new Gee.ArrayList<Objects.Radio?> ();
        all_radios = Byte.database.get_all_radios ();

        get_style_context ().add_class (Gtk.STYLE_CLASS_VIEW);
        get_style_context ().add_class ("w-round");
        
        var back_button = new Gtk.Button.from_icon_name ("byte-arrow-back-symbolic", Gtk.IconSize.MENU);
        back_button.can_focus = false;
        back_button.margin = 3;
        back_button.margin_bottom = 6;
        back_button.margin_top = 6;
        back_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        back_button.get_style_context ().add_class ("label-color-primary"); 

        var title_label = new Gtk.Label ("<b>%s</b>".printf (_("Radios")));
        title_label.use_markup = true;
        title_label.valign = Gtk.Align.CENTER;
        title_label.get_style_context ().add_class ("h3");

        var internet_radio_button = new Gtk.Button.from_icon_name ("internet-radio-symbolic", Gtk.IconSize.MENU);
        internet_radio_button.can_focus = false;
        internet_radio_button.tooltip_text = _("Search internet radios");
        internet_radio_button.margin = 3;
        internet_radio_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        internet_radio_button.get_style_context ().add_class ("label-color-primary");

        internet_radio_button.clicked.connect (() => {
            show_quick_find ();
        });

        var header_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        header_box.get_style_context ().add_class (Gtk.STYLE_CLASS_BACKGROUND);
        header_box.pack_start (back_button, false, false, 0);
        header_box.set_center_widget (title_label);
        header_box.pack_end (internet_radio_button, false, false, 0);

        listbox = new Gtk.ListBox (); 
        listbox.expand = true;

        var scrolled = new Gtk.ScrolledWindow (null, null);
        scrolled.hscrollbar_policy = Gtk.PolicyType.NEVER;
        scrolled.expand = true;
        scrolled.add (listbox);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        main_box.margin_bottom = 3;
        main_box.expand = true;
        main_box.pack_start (header_box, false, false, 0);
        main_box.pack_start (new Gtk.Separator (Gtk.Orientation.HORIZONTAL), false, false, 0);
        main_box.pack_start (scrolled, true, true, 0);
        
        add (main_box);
        add_all_items ();

        back_button.clicked.connect (() => {
            go_back ();
        });

        listbox.row_activated.connect ((row) => {
            var item = row as Widgets.RadioRow;
            Byte.player.set_radio (item.radio);
        });

        Byte.database.adden_new_radio.connect ((radio) => {
            Idle.add (() => {
                add_radio (radio);

                return false;
            });
        });

        scrolled.edge_reached.connect((pos)=> {
            if (pos == Gtk.PositionType.BOTTOM) {
                
                item_index = item_max;
                item_max = item_max + 100;

                if (item_max > all_radios.size) {
                    item_max = all_radios.size;
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
            internet_radio_button.sensitive = false;
        });

        Byte.scan_service.sync_finished.connect (() => {
            internet_radio_button.sensitive = true;
        });
    }

    private void add_radio (Objects.Radio radio) {
        var row = new Widgets.RadioRow (radio);
        
        listbox.add (row);
        listbox.show_all ();
    }

    public void add_all_items () {
        if (item_max > all_radios.size) {
            item_max = all_radios.size;
        }

        for (int i = item_index; i < item_max; i++) {
            var row = new Widgets.RadioRow (all_radios [i]);

            listbox.add (row);
            listbox.show_all ();
        }   
    }
}