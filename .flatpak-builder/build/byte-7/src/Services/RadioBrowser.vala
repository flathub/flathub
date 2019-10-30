public class Services.RadioBrowser : GLib.Object {
    private Json.Parser parser;
    private Soup.Session session;
    private Cancellable cancellable;
    public Datalist<string> parameters = Datalist<string>();

    public signal void started ();
    public signal void finished ();
    public signal void item_loaded (Objects.Radio radio);

    private string API_URL = "http://www.radio-browser.info/webservice/json/stations/byname/";

    public RadioBrowser () {
        session = new Soup.Session ();
        session.timeout = 10;

        parser = new Json.Parser ();
        parser.array_start.connect(() => {
            started ();
        });

        parser.array_element.connect((parse, array, index) => {
            item_loaded (
                Json.gobject_deserialize (typeof (Objects.Radio), array.get_element (index)) as Objects.Radio
            );
        });

        parser.array_end.connect(() => {
            finished ();
        });
    }

    public void cancel () {
        cancellable.cancel ();
    }

    public async void get_radios_by_name (string name) {
        started ();
        cancel ();
        cancellable = new Cancellable ();

        string url = API_URL + name + "?order=votes&reverse=true&limit=100";
        var message = new Soup.Message ("GET", url);
        try {
            var stream = yield session.send_async (message, cancellable);
            yield parser.load_from_stream_async (stream);
        } catch (Error e) {
            finished ();
            print("Error %s\n", e.message);
        }
    }
}