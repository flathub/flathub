public class Objects.Artist : GLib.Object {
    public int id;
    public string name;

    public Artist (int id = 0,
                  string name = "") {
        this.id = id;
        this.name = name;
    }
}