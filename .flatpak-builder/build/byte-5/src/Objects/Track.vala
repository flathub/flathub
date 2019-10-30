public class Objects.Track : GLib.Object {
    public int track_order; 
    public int id;      
    public int album_id;          
    public int track;             
    public int disc;              
    public int play_count;        
    public int is_favorite;       
    public int bitrate;           
    public int bpm;               
    public int rating;            
    public int samplerate;        
    public int channels;          
    public int year;     
    public int playlist;         
    public uint64 duration;       
    public string path;           
    public string title;          
    public string favorite_added; 
    public string last_played;    
    public string album_title;    
    public string artist_name;    
    public string composer;       
    public string grouping;       
    public string comment;        
    public string lyrics;         
    public string genre;          
    public string album_artist;   
    public string date_added;     

    public Track (int track_order = 0,
                  int id = 0,
                  int album_id = 0,
                  int track = 0,
                  int disc = 0,
                  int play_count = 0,
                  int is_favorite = 0,
                  int bitrate = 0,
                  int bpm = 0,
                  int rating = 0,
                  int samplerate = 0,
                  int channels = 0,
                  int year = 0,
                  int playlist = 0,
                  uint64 duration = 0,
                  string path = "",
                  string title = "",
                  string favorite_added = "",
                  string last_played = "",
                  string album_title = "",
                  string artist_name = "",
                  string composer = "",
                  string grouping = "",
                  string comment = "",
                  string lyrics = "",
                  string genre = "",
                  string album_artist = "",
                  string date_added = new GLib.DateTime.now_local ().to_string ()) {
        this.track_order = track_order;
        this.id = id;
        this.album_id = album_id;
        this.track = track;
        this.disc = disc;
        this.play_count = play_count;
        this.is_favorite = is_favorite;
        this.bitrate = bitrate;
        this.bpm = bpm;
        this.rating = rating;
        this.samplerate = samplerate;
        this.channels = channels;
        this.year = year;
        this.duration = duration;
        this.path = path;
        this.title = title;
        this.favorite_added = favorite_added;
        this.last_played = last_played;
        this.album_title = album_title;
        this.artist_name =artist_name;
        this.composer = composer;
        this.grouping = grouping;
        this.comment = comment;
        this.lyrics = lyrics;
        this.genre = genre;
        this.album_artist = album_artist;
        this.date_added = date_added;
    }
}
