public class Objects.Playlist : GLib.Object {
    public int id;
    public string title;
    public string note;
    public string date_added;
    public string date_updated;
    public int num_tracks;

    public Playlist (int id = 0,
                     string title = "",
                     string note = "",
                     string date_added = new GLib.DateTime.now_local ().to_string (),
                     string date_updated = new GLib.DateTime.now_local ().to_string (),
                     int num_tracks = 0) {
        this.id = id;
        this.title = title;
        this.note = note;
        this.date_added = date_added;
        this.date_updated = date_updated; 
        this.num_tracks = num_tracks;
    }
}