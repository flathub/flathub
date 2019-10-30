public class Dialogs.TrackEditor : Gtk.Dialog { 
    public Objects.Track track { get; construct; }

    private Gtk.Entry title_entry;
    private Gtk.Entry artist_entry;
    private Gtk.Entry album_artist_entry;
    private Gtk.Entry album_entry;
    private Gtk.Entry genre_entry;
    private Gtk.SpinButton track_spinbutton; 
    private Gtk.SpinButton disk_spinbutton;
    private Gtk.SpinButton year_spinbutton;

    private Gtk.TextView lyrics_textview;
    private Gtk.ScrolledWindow lyrics_scrolledwindow;

    private Widgets.Cover image_cover;
    public TrackEditor (Objects.Track track) {
        Object (
            transient_for: Byte.instance.main_window,
            deletable: false,
            resizable: true,
            destroy_with_parent: true,
            //window_position: Gtk.WindowPosition.CENTER_ON_PARENT,
            track: track
        );
	}

    construct {
        get_style_context ().add_class ("editor-titlebar");
        set_size_request (-1, 500);

        image_cover = new Widgets.Cover.from_file (
            GLib.Path.build_filename (Byte.utils.COVER_FOLDER, ("track-%i.jpg").printf (track.id)), 
            64, "track");

        var cover_button = new Gtk.Button ();
        cover_button.halign = Gtk.Align.START;
        cover_button.can_focus = false;
        cover_button.get_style_context ().add_class ("no-padding");
        cover_button.get_style_context ().add_class (Gtk.STYLE_CLASS_FLAT);

        cover_button.add (image_cover);

        title_entry = new Gtk.Entry ();
        title_entry.text = track.title;

        artist_entry = new Gtk.Entry ();
        artist_entry.text = track.artist_name;

        album_entry = new Gtk.Entry ();
        album_entry.text = track.album_title;

        album_artist_entry = new Gtk.Entry ();

        genre_entry = new Gtk.Entry ();
        
        var local_time = new DateTime.now_local ();
        year_spinbutton = new Gtk.SpinButton.with_range (0, local_time.get_year (), 1);

        track_spinbutton = new Gtk.SpinButton.with_range (0, 500, 1);
        //track_spinbutton.value = ;
        
        disk_spinbutton = new Gtk.SpinButton.with_range (0, 500, 1);

        lyrics_textview = new Gtk.TextView ();
        lyrics_textview.set_wrap_mode (Gtk.WrapMode.WORD_CHAR);
        lyrics_textview.get_style_context ().add_class ("editor-textview");
        lyrics_textview.buffer.text = track.lyrics;

        lyrics_scrolledwindow = new Gtk.ScrolledWindow (null, null);
        lyrics_scrolledwindow.set_policy (Gtk.PolicyType.EXTERNAL, Gtk.PolicyType.AUTOMATIC);
        lyrics_scrolledwindow.add (lyrics_textview);
        lyrics_scrolledwindow.expand = true;

        var grid = new Gtk.Grid ();
        grid.expand = true;
        grid.margin_start = 12;
        grid.margin_end = 12;
        grid.column_spacing = 12;

        grid.attach (cover_button,                                0, 0, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Title")),        0, 1, 1, 1);
        grid.attach (title_entry,                                 0, 2, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Artist")),       1, 1, 1, 1);
        grid.attach (artist_entry,                                1, 2, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Album")),        0, 3, 1, 1);
        grid.attach (album_entry,                                 0, 4, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Genre")),        1, 3, 1, 1);
        grid.attach (genre_entry,                                 1, 4, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Year")),         0, 5, 1, 1);
        grid.attach (year_spinbutton,                             0, 6, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Track")),        1, 5, 1, 1);
        grid.attach (track_spinbutton,                            1, 6, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Lyrics")),       0, 7, 1, 1);
        grid.attach (lyrics_scrolledwindow,                       0, 8, 2, 1);
        //
        //
        //
        //                            
        //
        //grid.attach (new Granite.HeaderLabel (_("Genre")),        0, 4, 1, 1);
        //grid.attach (genre_entry,                                 0, 5, 1, 1);
        
        ///grid.attach (new Granite.HeaderLabel (_("Disc")),        1, 6, 1, 1);
        //grid.attach (disk_spinbutton,                             1, 7, 1, 1);
        //grid.attach (new Granite.HeaderLabel (_("Cover")),        0, 8, 1, 1);
        //grid.attach (cover_button,                                0, 9, 1, 1);

        /*
        grid.attach (new Granite.HeaderLabel (_("Genre")), 0, 6, 1, 1);
        grid.attach (genre_entry, 0, 7, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Year")), 1, 6, 1, 1);
        grid.attach (year_spinbutton, 1, 7, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Track")), 1, 8, 1, 1);
        grid.attach (track_spinbutton, 1, 9, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Disc")), 1, 10, 1, 1);
        grid.attach (disk_spinbutton, 1, 11, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Rating")), 1, 12, 1, 1);
        grid.attach (rating_widget, 1, 13, 1, 1);
        grid.attach (new Granite.HeaderLabel (_("Comment")), 0, 8, 1, 1);
        grid.attach (comment_frame, 0, 9, 1, 5);
        */

        get_content_area ().add (grid);

        var previous_button = new Gtk.Button.from_icon_name ("go-previous-symbolic");
        var next_button = new Gtk.Button.from_icon_name ("go-next-symbolic");

        var arrows_grid = new Gtk.Grid ();
        arrows_grid.get_style_context ().add_class (Gtk.STYLE_CLASS_LINKED);
        arrows_grid.add (previous_button);
        arrows_grid.add (next_button);

        add_button (_("Close"), Gtk.ResponseType.CLOSE);

        var save_button = (Gtk.Button) add_button (_("Save"), Gtk.ResponseType.APPLY);
        save_button.has_default = true;
        save_button.get_style_context ().add_class (Gtk.STYLE_CLASS_SUGGESTED_ACTION);

        var action_area = (Gtk.ButtonBox) get_action_area ();
        action_area.margin = 5;
        action_area.margin_top = 14;
        action_area.pack_start (arrows_grid, false, false, 0);
        action_area.set_child_secondary (arrows_grid, true);
        action_area.set_child_non_homogeneous (arrows_grid, true);

        //previous_button.clicked.connect (previous_track);
        //next_button.clicked.connect (next_track);

        response.connect ((response_id) => {
            if (response_id == Gtk.ResponseType.APPLY) {
                //save_and_exit ();
            }

            destroy ();
        });
    }
}