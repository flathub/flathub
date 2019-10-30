public class Widgets.RadioSearchRow : Gtk.ListBoxRow {
    public Objects.Radio radio { get; construct; }

    private Gtk.Label name_label;
    private Gtk.Label secondary_label;
    private Widgets.Cover image_cover;

    public signal void send_notification_error ();

    public RadioSearchRow (Objects.Radio radio) {
        Object (
            radio: radio
        );
    }

    construct {
        get_style_context ().add_class ("album-row");
        tooltip_text = radio.name;

        name_label = new Gtk.Label (radio.name);
        name_label.margin_end = 6;
        name_label.get_style_context ().add_class ("font-bold");
        name_label.ellipsize = Pango.EllipsizeMode.END;
        name_label.halign = Gtk.Align.START;
        name_label.valign = Gtk.Align.END;

        secondary_label = new Gtk.Label ("👍️ %s - %s".printf (radio.votes, radio.country));
        secondary_label.halign = Gtk.Align.START;
        secondary_label.valign = Gtk.Align.START;
        secondary_label.max_width_chars = 35;
        secondary_label.ellipsize = Pango.EllipsizeMode.END;

        image_cover = new Widgets.Cover.from_url_async (radio.favicon, 32, true, "radio");

        var add_button = new Gtk.Button.from_icon_name ("list-add-symbolic", Gtk.IconSize.MENU);
        add_button.get_style_context ().add_class ("quick-find-add-radio");
        add_button.get_style_context ().add_class ("remove-button");
        add_button.hexpand = true;
        add_button.halign = Gtk.Align.END;
        add_button.valign = Gtk.Align.CENTER;

        var add_revealer = new Gtk.Revealer ();
        add_revealer.halign = Gtk.Align.END;
        add_revealer.valign = Gtk.Align.CENTER;
        add_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_LEFT;
        add_revealer.add (add_button);
        add_revealer.reveal_child = false;

        var main_grid = new Gtk.Grid ();
        main_grid.margin = 3;
        main_grid.margin_end = 12;
        main_grid.column_spacing = 6;
        main_grid.attach (image_cover, 0, 0, 1, 2);
        main_grid.attach (name_label, 1, 0, 1, 1);
        main_grid.attach (secondary_label, 1, 1, 1, 1);
        main_grid.attach (add_revealer, 2, 0, 2, 2);

        var eventbox = new Gtk.EventBox ();
        eventbox.add_events (Gdk.EventMask.ENTER_NOTIFY_MASK | Gdk.EventMask.LEAVE_NOTIFY_MASK);
        eventbox.add (main_grid);

        add (eventbox);

        add_button.clicked.connect (() => {
            if (Byte.database.radio_exists (radio.url)) {
                send_notification_error ();
            } else {
                Byte.database.insert_radio (radio);
            }
        });

        eventbox.enter_notify_event.connect ((event) => {
            add_revealer.reveal_child = true;
            add_button.get_style_context ().add_class ("closed");
            return false;
        });

        eventbox.leave_notify_event.connect ((event) => {
            if (event.detail == Gdk.NotifyType.INFERIOR) {
                return false;
            }

            add_revealer.reveal_child = false;
            add_button.get_style_context ().remove_class ("closed");
            return false;
        });
    }
}