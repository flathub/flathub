public class Objects.Radio : GLib.Object {
    public int    id       { get; set; default = 0; }
    public string name     { get; set; default = ""; }
    public string url      { get; set; default = ""; }
    public string homepage { get; set; default = ""; }
    public string tags     { get; set; default = ""; }
    public string favicon  { get; set; default = ""; }
    public string country  { get; set; default = ""; }
    public string state    { get; set; default = ""; }
    public string votes    { get; set; default = ""; }

    public Radio () {
    }
    
    string ? _file = null;
    public string ? file {
        get {
            if (_file == null) {
                _file = get_stream_file ();
            }

            return _file;
        }
    }

    private string ? get_stream_file () {
        string ? return_value = null;

        string ? content = get_stream_content ();
        if (content != null) {
            return_value = get_file_from_m3u (content);
            if (return_value == null) {
                return_value = get_file_from_pls (content);
            }
        } else {
            return_value = url;
        }

        return return_value;
    }

    private string ? get_stream_content () {
        string ? return_value = null;
        var session = new Soup.Session ();
        var msg = new Soup.Message ("GET", url);

        try {
            session.send (msg, null);
        }
        catch (Error err) {
            warning (err.message);
            return return_value;
        }
        var content_type = msg.response_headers.get_one ("Content-Type");
        if (content_type != null && !content_type.has_prefix ("audio/mpeg") && !content_type.has_prefix ("audio/aac")) {
            session.send_message (msg);
            var data = (string)msg.response_body.data;
            if (msg.status_code == 200) {
                return_value = data;
            }
        }
        return return_value;
    }

    private string ? get_file_from_m3u (string content) {
        string[] lines = content.split ("\n");
        foreach (unowned string line in lines) {
            if (line.has_prefix ("http") && line.index_of ("#") == -1) {
                return line;
            }
        }
        return null;
    }

    private string ? get_file_from_pls (string content) {
        string group = "playlist";

        var file = new KeyFile ();
        try {
            file.load_from_data (content, -1, KeyFileFlags.NONE);
        } catch (Error err) {
            warning (err.message);
        }

        if (!file.has_group (group)) {
            return null;
        }

        try {
            foreach (unowned string key in file.get_keys (group)) {
                string val = file.get_value (group, key);
                if (key.down ().has_prefix ("file")) {
                    return val;
                }
            }
        } catch (Error err) {
            warning (err.message);
        }

        return null;
    }
}