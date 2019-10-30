public class Widgets.RadioRow : Gtk.ListBoxRow {
    public Objects.Radio radio { get; construct; }

    private Gtk.Label name_label;
    private Gtk.Label country_state_label;

    public signal void send_notification_error ();

    public RadioRow (Objects.Radio radio) {
        Object (
            radio: radio
        );
    }

    construct {
        get_style_context ().add_class ("album-row");
        tooltip_text = radio.name;

        var playing_icon = new Gtk.Image ();
        playing_icon.gicon = new ThemedIcon ("audio-volume-medium-symbolic");
        playing_icon.get_style_context ().add_class ("playing-ani-color");
        playing_icon.pixel_size = 16;

        var playing_revealer = new Gtk.Revealer ();
        playing_revealer.halign = Gtk.Align.CENTER;
        playing_revealer.valign = Gtk.Align.CENTER;
        playing_revealer.transition_type = Gtk.RevealerTransitionType.CROSSFADE;
        playing_revealer.add (playing_icon);
        playing_revealer.reveal_child = false;

        name_label = new Gtk.Label (radio.name);
        name_label.margin_end = 6;
        name_label.get_style_context ().add_class ("font-bold");
        name_label.ellipsize = Pango.EllipsizeMode.END;
        name_label.halign = Gtk.Align.START;
        name_label.valign = Gtk.Align.END;

        country_state_label = new Gtk.Label (radio.country);
        country_state_label.halign = Gtk.Align.START;
        country_state_label.valign = Gtk.Align.START;
        country_state_label.max_width_chars = 45;
        country_state_label.ellipsize = Pango.EllipsizeMode.END;

        var cover_path = GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("radio-%i.jpg").printf (radio.id));
        var image_cover = new Widgets.Cover.from_file (cover_path, 48, "radio");
        image_cover.halign = Gtk.Align.START;
        image_cover.valign = Gtk.Align.START;

        var overlay = new Gtk.Overlay ();
        overlay.halign = Gtk.Align.START;
        overlay.valign = Gtk.Align.START;
        overlay.add_overlay (playing_revealer);
        overlay.add (image_cover);

        var remove_button = new Gtk.Button.from_icon_name ("edit-delete-symbolic", Gtk.IconSize.MENU);
        remove_button.can_focus = false;
        remove_button.valign = Gtk.Align.CENTER;
        remove_button.tooltip_text = _("Remove");
        remove_button.margin_end = 2;
        remove_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);
        remove_button.get_style_context ().add_class ("options-button");
        remove_button.get_style_context ().remove_class ("button");

        var remove_revealer = new Gtk.Revealer ();
        remove_revealer.halign = Gtk.Align.END;
        remove_revealer.hexpand = true;
        remove_revealer.transition_type = Gtk.RevealerTransitionType.SLIDE_LEFT;
        remove_revealer.add (remove_button);
        remove_revealer.reveal_child = false;

        var main_grid = new Gtk.Grid ();
        main_grid.margin = 3;
        main_grid.margin_end = 6;
        main_grid.column_spacing = 6;
        main_grid.attach (overlay, 0, 0, 1, 2);
        main_grid.attach (name_label, 1, 0, 1, 1);
        main_grid.attach (country_state_label, 1, 1, 1, 1);
        main_grid.attach (remove_revealer, 2, 0, 2, 2);

        var eventbox = new Gtk.EventBox ();
        eventbox.add_events (Gdk.EventMask.ENTER_NOTIFY_MASK | Gdk.EventMask.LEAVE_NOTIFY_MASK);
        eventbox.add (main_grid);

        add (eventbox);

        Byte.player.current_radio_changed.connect ((current_radio) => {
            if (radio.id == current_radio.id) {
                playing_revealer.reveal_child = true;
            } else {
                playing_revealer.reveal_child = false;
            }
        });

        eventbox.enter_notify_event.connect ((event) => {
            remove_revealer.reveal_child = true;
            return false;
        });

        eventbox.leave_notify_event.connect ((event) => {
            if (event.detail == Gdk.NotifyType.INFERIOR) {
                return false;
            }

            remove_revealer.reveal_child = false;
            return false;
        });

        remove_button.clicked.connect (() => {
            var message_dialog = new Granite.MessageDialog.with_image_from_icon_name (
                _("Delete from library?"),
                _("Are you sure you want to delete <b>%s</b> from your library?").printf (radio.name),
                "dialog-warning",
                Gtk.ButtonsType.CANCEL
            );

            var set_button = new Gtk.Button.with_label (_("Delete"));
            set_button.get_style_context ().add_class (Gtk.STYLE_CLASS_DESTRUCTIVE_ACTION);
            message_dialog.add_action_widget (set_button, Gtk.ResponseType.ACCEPT);

            message_dialog.show_all ();

            if (message_dialog.run () == Gtk.ResponseType.ACCEPT) {
                if (Byte.database.remove_radio_from_library (radio)) {
                    destroy ();
                }
            }

            message_dialog.destroy ();
        });

        Byte.utils.radio_image_downloaded.connect ((id) => {
            if (radio.id == id) {
                try {
                    image_cover.pixbuf = new Gdk.Pixbuf.from_file_at_size (
                        GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("radio-%i.jpg").printf (id)),
                        48,
                        48);
                } catch (Error e) {
                    stderr.printf ("Error setting default avatar icon: %s ", e.message);
                }
            }
        });
    }
}