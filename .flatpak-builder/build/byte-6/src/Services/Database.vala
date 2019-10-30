public class Services.Database : GLib.Object {
    private Sqlite.Database db; 
    private string db_path;

    public signal void adden_new_track (Objects.Track? track);
    public signal void added_new_artist (Objects.Artist artist);
    public signal void added_new_album (Objects.Album album);
    public signal void adden_new_radio (Objects.Radio radio);
    public signal void adden_new_playlist (Objects.Playlist playlist);
    
    public signal void removed_track (int id); 
    public signal void removed_playlist (int id);

    public signal void updated_album_cover (int album_id);
    public signal void updated_track_cover (int track_id);
    public signal void updated_playlist_cover (int playlist_id);

    public signal void updated_track_favorite (Objects.Track track, int favorite);
    public signal void updated_playlist (Objects.Playlist playlist);

    public signal void reset_library ();

    public Database (bool skip_tables = false) {
        int rc = 0;
        db_path = Environment.get_home_dir () + "/.local/share/com.github.alainm23.byte/database.db";

        if (!skip_tables) {
            if (create_tables () != Sqlite.OK) {
                stderr.printf ("Error creating db table: %d, %s\n", rc, db.errmsg ());
                Gtk.main_quit ();
            }
        }

        rc = Sqlite.Database.open (db_path, out db);

        if (rc != Sqlite.OK) {
            stderr.printf ("Can't open database: %d, %s\n", rc, db.errmsg ());
            Gtk.main_quit ();
        }
    }

    private int create_tables () {
        int rc;

        rc = Sqlite.Database.open (db_path, out db);

        if (rc != Sqlite.OK) {
            stderr.printf ("Can't open database: %d, %s\n", rc, db.errmsg ());
            Gtk.main_quit ();
        }

        rc = db.exec ("CREATE TABLE IF NOT EXISTS artists (" +
            "id             INTEGER PRIMARY KEY AUTOINCREMENT, " +
            "name           TEXT    NOT NULL," +
            "CONSTRAINT unique_artist UNIQUE (name))", null, null);
        debug ("Table artists created");

        rc = db.exec ("CREATE TABLE IF NOT EXISTS albums (" +
            "id             INTEGER PRIMARY KEY AUTOINCREMENT, " +
            "artist_id      INT     NOT NULL," + 
            "year           INT     NOT NULL," +
            "title          TEXT    NOT NULL," +
            "genre          TEXT    NOT NULL," +
            "CONSTRAINT unique_album UNIQUE (artist_id, title)," +
            "FOREIGN KEY (artist_id) REFERENCES artists (id) ON DELETE CASCADE)", null, null);
        debug ("Table albums created");

        rc = db.exec ("CREATE TABLE IF NOT EXISTS tracks (" +
            "id             INTEGER PRIMARY KEY AUTOINCREMENT," +
            "album_id       INT     NOT NULL," +
            "track          INT     NOT NULL," +
            "disc           INT     NOT NULL," +
            "play_count     INT     NOT NULL," +
            "is_favorite    INT     NOT NULL," +
            "duration       INT     NOT NULL," +
            "samplerate     INT     NOT NULL," +    
            "channels       INT     NOT NULL," +    
            "bitrate        INT     NOT NULL," +
            "bpm            INT     NOT NULL," +
            "rating         INT     NOT NULL," +
            "year           INT     NOT NULL," +
            "path           TEXT    NOT NULL," +
            "title          TEXT    NOT NULL," +
            "date_added     TEXT    NOT NULL," +
            "favorite_added TEXT    NOT NULL," +
            "last_played    TEXT    NOT NULL," +
            "composer       TEXT    NOT NULL," +
            "grouping       TEXT    NOT NULL," +
            "comment        TEXT    NOT NULL," +
            "lyrics         TEXT    NOT NULL," +
            "genre          TEXT    NOT NULL," +
            "album_artist   TEXT    NOT NULL," +
            "CONSTRAINT unique_track UNIQUE (path)," +
            "FOREIGN KEY (album_id) REFERENCES albums (id) ON DELETE CASCADE)", null, null);
        debug ("Table trackS created");

        rc = db.exec ("CREATE TABLE IF NOT EXISTS radios (" +
            "id         INTEGER PRIMARY KEY AUTOINCREMENT," +
            "name       TEXT," +
            "url        TEXT," +
            "homepage   TEXT," +
            "tags       TEXT," +
            "favicon    TEXT," +
            "country    TEXT," +
            "state      TEXT)", null, null);
        debug ("Table radios created");

        rc = db.exec ("CREATE TABLE IF NOT EXISTS playlists (" +
            "id           INTEGER PRIMARY KEY AUTOINCREMENT," +
            "title        TEXT," +
            "note         TEXT," +
            "date_added   TEXT," +
            "date_updated TEXT)", null, null);
        debug ("Table playlists created");

        rc = db.exec ("CREATE TABLE IF NOT EXISTS playlist_tracks (" +
            "id           INTEGER PRIMARY KEY AUTOINCREMENT," +
            "playlist_id  INT," +
            "track_id     INT," +
            "date_added   TEXT," +
            "CONSTRAINT unique_track UNIQUE (playlist_id, track_id)," +
            "FOREIGN KEY (track_id) REFERENCES tracks (id) ON DELETE CASCADE," +
            "FOREIGN KEY (playlist_id) REFERENCES playlists (id) ON DELETE CASCADE)", null, null);
        debug ("Table playlist_tracks created");

        rc = db.exec ("CREATE TABLE IF NOT EXISTS blacklist (" +
            "id     INTEGER PRIMARY KEY AUTOINCREMENT," +
            "path   TEXT NOT NULL)", null, null);
        debug ("Table blacklist created");

        rc = db.exec ("PRAGMA foreign_keys = ON;");

        return rc;
    }

    public bool music_file_exists (string uri) {
        bool file_exists = false;
        Sqlite.Statement stmt;

        int res = db.prepare_v2 ("SELECT COUNT (*) FROM tracks WHERE path = ?", -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, uri);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            file_exists = stmt.column_int (0) > 0;
        }

        return file_exists;
    }

    public bool music_blacklist_exists (string uri) {
        bool file_exists = false;
        Sqlite.Statement stmt;

        int res = db.prepare_v2 ("SELECT COUNT (*) FROM blacklist WHERE path = ?", -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, uri);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            file_exists = stmt.column_int (0) > 0;
        }

        return file_exists;
    }

    public bool radio_exists (string url) {
        bool exists = false;
        Sqlite.Statement stmt;

        int res = db.prepare_v2 ("SELECT COUNT (*) FROM radios WHERE url = ?", -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, url);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            exists = stmt.column_int (0) > 0;
        }

        return exists;
    }

    public bool is_database_empty () {
        bool empty = false;
        Sqlite.Statement stmt;

        int res = db.prepare_v2 ("SELECT COUNT (*) FROM tracks", -1, out stmt);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            empty = stmt.column_int (0) <= 0;
        }

        return empty;
    }

    public int get_id_if_artist_exists (Objects.Artist artist) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT COUNT (*) FROM artists WHERE name = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, artist.name);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            if (stmt.column_int (0) > 0) {
                stmt.reset ();

                sql = """
                    SELECT id FROM artists WHERE name = ?;
                """;

                res = db.prepare_v2 (sql, -1, out stmt);
                assert (res == Sqlite.OK);

                res = stmt.bind_text (1, artist.name);
                assert (res == Sqlite.OK);

                if (stmt.step () == Sqlite.ROW) {
                    return stmt.column_int (0);
                } else {
                    warning ("Error: %d: %s", db.errcode (), db.errmsg ());
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }

    public int insert_artist_if_not_exists (Objects.Artist artist) {
        Sqlite.Statement stmt;
        string sql;
        int id;

        id = get_id_if_artist_exists (artist);
        if (id == 0) {
            sql = """
                INSERT OR IGNORE INTO artists (name) VALUES (?);
            """;
            
            int res = db.prepare_v2 (sql, -1, out stmt);
            assert (res == Sqlite.OK);

            res = stmt.bind_text (1, artist.name);
            assert (res == Sqlite.OK);
            
            if (stmt.step () != Sqlite.DONE) {
                warning ("Error: %d: %s", db.errcode (), db.errmsg ());
            }

            artist.id = get_id_if_artist_exists (artist);
            added_new_artist (artist);

            return artist.id;
        } else {
            return id;
        }
    }

    public int get_id_if_album_exists (Objects.Album album) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT COUNT (*) FROM albums WHERE title = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, album.title);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            if (stmt.column_int (0) > 0) {
                stmt.reset ();

                sql = """
                    SELECT id FROM albums WHERE title = ?;
                """;

                res = db.prepare_v2 (sql, -1, out stmt);
                assert (res == Sqlite.OK);

                res = stmt.bind_text (1, album.title);
                assert (res == Sqlite.OK);

                if (stmt.step () == Sqlite.ROW) {
                    return stmt.column_int (0);
                } else {
                    warning ("Error: %d: %s", db.errcode (), db.errmsg ());
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }

    public int insert_album_if_not_exists (Objects.Album album) {
        Sqlite.Statement stmt;
        string sql;
        int id;

        id = get_id_if_album_exists (album);
        if (id == 0) {
            sql = """
                INSERT OR IGNORE INTO albums (artist_id, year, title, genre) VALUES (?, ?, ?, ?);
            """;
            
            int res = db.prepare_v2 (sql, -1, out stmt);
            assert (res == Sqlite.OK);

            res = stmt.bind_int (1, album.artist_id);
            assert (res == Sqlite.OK);

            res = stmt.bind_int (2, album.year);
            assert (res == Sqlite.OK);

            res = stmt.bind_text (3, album.title);
            assert (res == Sqlite.OK);

            res = stmt.bind_text (4, album.genre);
            assert (res == Sqlite.OK);
            
            if (stmt.step () != Sqlite.DONE) {
                warning ("Error: %d: %s", db.errcode (), db.errmsg ());
            }

            album.id = get_id_if_album_exists (album);
            //stdout.printf ("Album ID: %d - %s\n", album.id, album.title);

            added_new_album (album);
            return album.id;
        } else {
            return id;
        }
    }

    public void insert_track (Objects.Track track) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            INSERT OR IGNORE INTO tracks (album_id, track, disc, play_count, is_favorite,
                bitrate, bpm, rating, samplerate, channels, year, duration,
                path, title, favorite_added, last_played, composer, grouping, 
                comment, lyrics, genre, album_artist, date_added)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?); 
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, track.album_id);
        assert (res == Sqlite.OK);
        
        res = stmt.bind_int (2, track.track);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (3, track.disc);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (4, track.play_count);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (5, track.is_favorite);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (6, track.bitrate);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (7, track.bpm);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (8, track.rating);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (9, track.samplerate);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (10, track.channels);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (11, track.year);
        assert (res == Sqlite.OK);

        res = stmt.bind_int64 (12, (int64) track.duration);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (13, track.path);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (14, track.title);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (15, track.favorite_added);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (16, track.last_played);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (17, track.composer);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (18, track.grouping);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (19, track.comment);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (20, track.lyrics);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (21, track.genre);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (22, track.album_artist);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (23, track.date_added);
        assert (res == Sqlite.OK);

        if (stmt.step () != Sqlite.DONE) {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
        }

        stmt.reset ();

        sql = """
            SELECT id FROM tracks WHERE path = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, track.path);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            track.id = stmt.column_int (0);
            stdout.printf ("Track ID: %d - %s\n", track.id, track.title);
            
            adden_new_track (track);
            
            Idle.add (() => {
                Byte.cover_import.import (track);    
                return false;
            });
        } else {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
            adden_new_track (null);
        }

        stmt.reset ();
    }

    public Objects.Track? get_track_by_id (int id) {
        Sqlite.Statement stmt;
        int res;

        string sql = """
            SELECT tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.date_added, 
            tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id WHERE id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, id);
        assert (res == Sqlite.OK);

        var track = new Objects.Track ();

        if (stmt.step () == Sqlite.ROW) {
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.track = stmt.column_int (5);
            track.date_added = stmt.column_text (6);
            track.play_count = stmt.column_int (7);
            track.album_id = stmt.column_int (8);
            track.album_title = stmt.column_text (9);
            track.artist_name = stmt.column_text (10);
            track.favorite_added = stmt.column_text (11);
            track.last_played = stmt.column_text (12);
        }

        return track;
    }

    public Gee.ArrayList<Objects.Album?> get_all_albums_by_artist (int id) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT albums.id, albums.artist_id, albums.year, albums.title, albums.genre, artists.name from albums
            INNER JOIN artists ON artists.id = albums.artist_id WHERE artists.id = ? ORDER BY albums.year DESC;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);
        
        res = stmt.bind_int (1, id);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Album?> ();

        while ((res = stmt.step()) == Sqlite.ROW) {
            var album = new Objects.Album ();

            album.id = stmt.column_int (0);
            album.artist_id = stmt.column_int (1);
            album.year = stmt.column_int (2);
            album.title = stmt.column_text (3);
            album.genre = stmt.column_text (4);
            album.artist_name = stmt.column_text (5);
            
            all.add (album);
        }

        return all;
    }

    public Gee.ArrayList<Objects.Album?> get_all_albums () {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT albums.id, albums.artist_id, albums.year, albums.title, albums.genre, artists.name from albums
            INNER JOIN artists ON artists.id = albums.artist_id ORDER BY albums.title;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Album?> ();

        while ((res = stmt.step()) == Sqlite.ROW) {
            var album = new Objects.Album ();

            album.id = stmt.column_int (0);
            album.artist_id = stmt.column_int (1);
            album.year = stmt.column_int (2);
            album.title = stmt.column_text (3);
            album.genre = stmt.column_text (4);
            album.artist_name = stmt.column_text (5);
            
            all.add (album);
        }

        return all;
    }

    public Gee.ArrayList<Objects.Album?> get_all_albums_order_by (int item, bool is_reverse) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string order_mode = "albums.title";
        string reverse_mode = "DESC";
        
        if (item == 0) {
            order_mode = "albums.title";
        } else if (item == 1) {
            order_mode = "artists.name";
        } else if (item == 2) {
            order_mode = "albums.year";
        } else if (item == 3) {
            order_mode = "albums.genre";
        }

        if (is_reverse == false) {
            reverse_mode = "ASC";
        }

        sql = """
            SELECT albums.id, albums.artist_id, albums.year, albums.title, albums.genre, artists.name from albums
            INNER JOIN artists ON artists.id = albums.artist_id ORDER BY %s %s;
        """.printf (order_mode, reverse_mode);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Album?> ();

        while ((res = stmt.step()) == Sqlite.ROW) {
            var album = new Objects.Album ();

            album.id = stmt.column_int (0);
            album.artist_id = stmt.column_int (1);
            album.year = stmt.column_int (2);
            album.title = stmt.column_text (3);
            album.genre = stmt.column_text (4);
            album.artist_name = stmt.column_text (5);
            
            all.add (album);
        }

        return all;
    }

    public Gee.ArrayList<Objects.Album?> get_all_albums_search (string search_text) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string _search_text = "%" + search_text + "%";

        sql = """
            SELECT albums.id, albums.artist_id, albums.year, albums.title, albums.genre, artists.name from albums
            INNER JOIN artists ON artists.id = albums.artist_id
            WHERE albums.title LIKE '%s' OR
            artists.name LIKE '%s' OR
            albums.genre LIKE '%s' OR 
            albums.year LIKE '%s';
        """.printf (_search_text, _search_text, _search_text, _search_text);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Album?> ();

        while ((res = stmt.step()) == Sqlite.ROW) {
            var album = new Objects.Album ();

            album.id = stmt.column_int (0);
            album.artist_id = stmt.column_int (1);
            album.year = stmt.column_int (2);
            album.title = stmt.column_text (3);
            album.genre = stmt.column_text (4);
            album.artist_name = stmt.column_text (5);
            
            all.add (album);
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_tracks_recently_added () {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.date_added, 
            tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id ORDER BY tracks.date_added DESC LIMIT 100;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;

        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.date_added = stmt.column_text (5);
            track.album_id = stmt.column_int (6);
            track.album_title = stmt.column_text (7);
            track.artist_name = stmt.column_text (8);
            track.favorite_added = stmt.column_text (9);
            track.last_played = stmt.column_text (10);
            
            all.add (track);
            index = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_by_artist (int id) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.date_added, 
            tracks.album_id, albums.title, artists.id, artists.name, tracks.favorite_added, tracks.last_played, tracks.play_count FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id WHERE artists.id = ? ORDER BY tracks.play_count DESC;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, id);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;

        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.date_added = stmt.column_text (5);
            track.album_id = stmt.column_int (6);
            track.album_title = stmt.column_text (7);
            track.artist_name = stmt.column_text (9);
            track.favorite_added = stmt.column_text (10);
            track.last_played = stmt.column_text (11);
            track.play_count = stmt.column_int (12);
            
            all.add (track);
            index = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_order_by (int item, bool is_reverse) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string order_mode = "tracks.title";
        string reverse_mode = "DESC";
        
        if (item == 0) {
            order_mode = "tracks.title";
        } else if (item == 1) {
            order_mode = "artists.name";
        } else if (item == 2) {
            order_mode = "albums.title";
        } else if (item == 3) {
            order_mode = "tracks.date_added";
        } else {
            order_mode = "tracks.play_count";
        }

        if (is_reverse == false) {
            reverse_mode = "ASC";
        }

        sql = """
            SELECT  tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite,tracks.date_added, tracks.play_count, 
            tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id ORDER BY %s %s;
        """.printf (order_mode, reverse_mode);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;

        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.date_added = stmt.column_text (5);
            track.play_count = stmt.column_int (6);
            track.album_id = stmt.column_int (7);
            track.album_title = stmt.column_text (8);
            track.artist_name = stmt.column_text (9);
            track.favorite_added = stmt.column_text (10);
            track.last_played = stmt.column_text (11);
            
            all.add (track);
            index = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_favorites () {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT  tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.date_added, tracks.play_count, 
            tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id 
            WHERE tracks.is_favorite = 1 ORDER BY tracks.favorite_added;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;

        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.date_added = stmt.column_text (5);
            track.play_count = stmt.column_int (6);
            track.album_id = stmt.column_int (7);
            track.album_title = stmt.column_text (8);
            track.artist_name = stmt.column_text (9);
            track.favorite_added = stmt.column_text (10);
            track.last_played = stmt.column_text (11);

            all.add (track);
            index = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_favorites_search (string search_text) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string _search_text = "%" + search_text + "%";

        sql = """
            SELECT  tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.date_added, 
            tracks.play_count, tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id
            WHERE (tracks.title LIKE '%s' OR 
            artists.name LIKE '%s' OR 
            albums.title LIKE '%s') 
            AND tracks.is_favorite = 1;
        """.printf (_search_text, _search_text, _search_text);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;
        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.date_added = stmt.column_text (5);
            track.play_count = stmt.column_int (6);
            track.album_id = stmt.column_int (7);
            track.album_title = stmt.column_text (8);
            track.artist_name = stmt.column_text (9);
            track.favorite_added = stmt.column_text (10);
            track.last_played = stmt.column_text (11);
            
            all.add (track);
            index = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_search (string search_text) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string _search_text = "%" + search_text + "%";

        sql = """
            SELECT  tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.date_added, tracks.play_count, 
            tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks 
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id
            WHERE tracks.title LIKE '%s' OR 
            artists.name LIKE '%s' OR 
            albums.title LIKE '%s';
        """.printf (_search_text, _search_text, _search_text);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;
        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.date_added = stmt.column_text (5);
            track.play_count = stmt.column_int (6);
            track.album_id = stmt.column_int (7);
            track.album_title = stmt.column_text (8);
            track.artist_name = stmt.column_text (9);
            track.favorite_added = stmt.column_text (10);
            track.last_played = stmt.column_text (11);
            
            all.add (track);
            index = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_by_album (int id) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.track, tracks.date_added, tracks.play_count, 
            tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played FROM tracks
            INNER JOIN albums ON tracks.album_id = albums.id
            INNER JOIN artists ON albums.artist_id = artists.id WHERE tracks.album_id = %i ORDER BY tracks.track;
        """.printf (id);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;

        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.track = stmt.column_int (5);
            track.date_added = stmt.column_text (6);
            track.play_count = stmt.column_int (7);
            track.album_id = stmt.column_int (8);
            track.album_title = stmt.column_text (9);
            track.artist_name = stmt.column_text (10);
            track.favorite_added = stmt.column_text (11);
            track.last_played = stmt.column_text (12);
            
            all.add (track);
            index  = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Track?> get_all_tracks_by_playlist (int id, int sort, bool is_reverse) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string order_mode = "tracks.title";
        string reverse_mode = "DESC";
        
        if (sort == 0) {
            order_mode = "tracks.title";
        } else if (sort == 1) {
            order_mode = "artists.name";
        } else if (sort == 2) {
            order_mode = "albums.title";
        } else if (sort == 3) {
            order_mode = "playlist_tracks.date_added";
        } else {
            order_mode = "tracks.play_count";
        }

        if (is_reverse == false) {
            reverse_mode = "ASC";
        }

        sql = """
            SELECT tracks.id, tracks.path, tracks.title, tracks.duration, tracks.is_favorite, tracks.track, tracks.date_added, tracks.play_count, tracks.album_id, albums.title, artists.name, tracks.favorite_added, tracks.last_played, playlist_tracks.date_added FROM playlist_tracks 
            LEFT JOIN tracks ON playlist_tracks.track_id = tracks.id
            LEFT JOIN albums ON tracks.album_id = albums.id
            LEFT JOIN artists ON albums.artist_id = artists.id WHERE playlist_tracks.playlist_id = ? ORDER BY %s %s;
        """.printf (order_mode, reverse_mode);

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, id);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Track?> ();
        int index = 0;

        while ((res = stmt.step()) == Sqlite.ROW) {
            var track = new Objects.Track ();

            track.track_order = index;
            track.id = stmt.column_int (0);
            track.path = stmt.column_text (1);
            track.title = stmt.column_text (2);
            track.duration = stmt.column_int64 (3);
            track.is_favorite = stmt.column_int (4);
            track.track = stmt.column_int (5);
            track.date_added = stmt.column_text (6);
            track.play_count = stmt.column_int (7);
            track.album_id = stmt.column_int (8);
            track.album_title = stmt.column_text (9);
            track.artist_name = stmt.column_text (10);
            track.favorite_added = stmt.column_text (11);
            track.last_played = stmt.column_text (12);
            track.playlist = id;
            
            all.add (track);
            index  = index + 1;
        }

        return all;
    }

    public Gee.ArrayList<Objects.Artist?> get_all_artists () {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT artists.id, artists.name FROM artists ORDER BY artists.name;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Artist?> ();

        while ((res = stmt.step()) == Sqlite.ROW) {
            var artist = new Objects.Artist ();

            artist.id = stmt.column_int (0);
            artist.name = stmt.column_text (1);
            
            all.add (artist);
        }

        return all;
    }

    public Gee.ArrayList<Objects.Artist?> get_all_artists_search (string search_text) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT artists.id, artists.name FROM artists WHERE artists.name LIKE '%s';
        """.printf ("%" + search_text + "%");

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Artist?> ();

        while ((res = stmt.step()) == Sqlite.ROW) {
            var artist = new Objects.Artist ();

            artist.id = stmt.column_int (0);
            artist.name = stmt.column_text (1);
            
            all.add (artist);
        }

        return all;
    }

    public int get_tracks_number () {
        /*
        Sqlite.Statement stmt;
        int c = 0;

        int res = db.prepare_v2 ("SELECT COUNT (*) FROM tracks",
            -1, out stmt);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            c = stmt.column_int (0);
        }

        return c;
        */

        return 0;
    }

    public void insert_radio (Objects.Radio radio) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            INSERT OR IGNORE INTO radios (name, url, homepage, tags, favicon, country, state)
            VALUES (?, ?, ?, ?, ?, ?, ?);
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, radio.name);
        assert (res == Sqlite.OK);
        
        res = stmt.bind_text (2, radio.url);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (3, radio.homepage);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (4, radio.tags);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (5, radio.favicon);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (6, radio.country);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (7, radio.state);
        assert (res == Sqlite.OK);

        if (stmt.step () != Sqlite.DONE) {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
        }
        stmt.reset ();

        sql = """
            SELECT id FROM radios WHERE url = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, radio.url);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            radio.id = stmt.column_int (0);

            adden_new_radio (radio);
            
            Byte.utils.download_image ("radio", radio.id, radio.favicon);            
        } else {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
        }

        stmt.reset ();
    }

    public Gee.ArrayList<Objects.Radio?> get_all_radios () {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT * FROM radios;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Radio?> ();

        while ((res = stmt.step ()) == Sqlite.ROW) {
            var radio = new Objects.Radio ();

            radio.id = stmt.column_int (0);
            radio.name = stmt.column_text (1);
            radio.url = stmt.column_text (2);
            radio.homepage = stmt.column_text (3);
            radio.tags = stmt.column_text (4);
            radio.favicon = stmt.column_text (5);
            radio.country = stmt.column_text (6);
            radio.state = stmt.column_text (7);
            
            all.add (radio);
        }

        return all;
    }

    public Gee.ArrayList<Objects.Playlist?> get_all_playlists () {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT * FROM playlists;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        var all = new Gee.ArrayList<Objects.Playlist?> ();

        while ((res = stmt.step ()) == Sqlite.ROW) {
            var playlist = new Objects.Playlist ();

            playlist.id = stmt.column_int (0);
            playlist.title = stmt.column_text (1);
            playlist.note = stmt.column_text (2);
            playlist.date_added = stmt.column_text (3);
            playlist.date_updated = stmt.column_text (4);
            
            all.add (playlist);
        }

        return all;
    }

    public int insert_playlist (Objects.Playlist playlist) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            INSERT OR IGNORE INTO playlists (title, note, date_added, date_updated)
            VALUES (?, ?, ?, ?);
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, playlist.title);
        assert (res == Sqlite.OK);
        
        res = stmt.bind_text (2, playlist.note);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (3, playlist.date_added);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (4, playlist.date_updated);
        assert (res == Sqlite.OK);

        if (stmt.step () != Sqlite.DONE) {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
        }

        stmt.reset ();

        sql = """
            SELECT id FROM playlists WHERE date_added = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, playlist.date_added);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            playlist.id = stmt.column_int (0);
            stdout.printf ("Playlist ID: %d - %s\n", playlist.id, playlist.title);
            adden_new_playlist (playlist);
            return playlist.id;
        } else {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
            return 0;
        }
    }
    
    public void insert_track_into_playlist (Objects.Playlist playlist, int track_id) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            INSERT INTO playlist_tracks (playlist_id, track_id, date_added) VALUES (?, ?, ?);
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, playlist.id);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (2, track_id);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (3, new GLib.DateTime.now_local ().to_string ());
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.DONE) {
            print ("Track: %i agregado \n".printf (track_id));
            playlist.date_updated = new GLib.DateTime.now_local ().to_string ();
            Byte.database.update_playlist (playlist);
        }
    }

    public void add_track_count (Objects.Track track) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT play_count FROM tracks WHERE id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, track.id);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            var count = stmt.column_int (0) + 1;

            stmt.reset ();

            sql = """
                UPDATE tracks SET play_count = ?, last_played = ? WHERE id = ?;
            """;

            res = db.prepare_v2 (sql, -1, out stmt);
            assert (res == Sqlite.OK);

            res = stmt.bind_int (1, count);
            assert (res == Sqlite.OK);

            res = stmt.bind_text (2, new GLib.DateTime.now_local ().to_string ());
            assert (res == Sqlite.OK);

            res = stmt.bind_int (3, track.id);
            assert (res == Sqlite.OK);

            res = stmt.step ();

            if (res == Sqlite.DONE) {
                print ("Track: %s - Count: %i\n".printf (track.title, count));
            }
        } else {
            warning ("Error: %d: %s", db.errcode (), db.errmsg ());
        }
    }

    public void set_track_favorite (Objects.Track track, int favorite) {
        Sqlite.Statement stmt;
        string sql;
        int res;
        string favorite_added = "";

        sql = """
            UPDATE tracks SET is_favorite = ?, favorite_added = ? WHERE id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, favorite);
        assert (res == Sqlite.OK);

        if (favorite == 1) {
            favorite_added = new GLib.DateTime.now_local ().to_string ();
        }

        res = stmt.bind_text (2, favorite_added);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (3, track.id);
        assert (res == Sqlite.OK);

        res = stmt.step ();

        if (res == Sqlite.DONE) {
            print ("Track: %s - Favorite: %i\n".printf (track.title, favorite));
            updated_track_favorite (track, favorite);
        }
    }

    public int is_track_favorite (Objects.Track track) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            SELECT is_favorite FROM tracks WHERE id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, track.id);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.ROW) {
            return stmt.column_int (0);
        } else {
            return 0;
        }
    }

    /*
        Remove 
    */

    public void remove_from_library (Objects.Track track) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            DELETE FROM tracks where id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, track.id);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.DONE) {
            stmt.reset ();

            sql = """
                INSERT INTO blacklist (path) VALUES (?);
            """;

            res = db.prepare_v2 (sql, -1, out stmt);
            assert (res == Sqlite.OK);

            res = stmt.bind_text (1, track.path);
            assert (res == Sqlite.OK);
            
            if (stmt.step () == Sqlite.DONE) {
                removed_track (track.id);
            } else {
                warning ("Error: %d: %s", db.errcode (), db.errmsg ());
            }
        }
    }

    public bool remove_radio_from_library (Objects.Radio radio) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            DELETE FROM radios WHERE id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, radio.id);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.DONE) {
            return true;
        }

        return false;
    }

    public bool remove_playlist_from_library (Objects.Playlist playlist) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            DELETE FROM playlists WHERE id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, playlist.id);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.DONE) {
            stmt.reset ();

            sql = """
                DELETE FROM playlist_tracks WHERE playlist_id = ?; 
            """;

            res = db.prepare_v2 (sql, -1, out stmt);
            assert (res == Sqlite.OK);

            res = stmt.bind_int (1, playlist.id);
            assert (res == Sqlite.OK);
            
            if (stmt.step () == Sqlite.DONE) {
                removed_playlist (playlist.id);
                return true;
            } else {
                warning ("Error: %d: %s", db.errcode (), db.errmsg ());
                return false;
            }
        }

        return false;
    }

    public bool remove_from_playlist (Objects.Track track) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            DELETE FROM playlist_tracks where track_id = ? AND playlist_id = ?;
        """;

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (1, track.id);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (2, track.playlist);
        assert (res == Sqlite.OK);

        if (stmt.step () == Sqlite.DONE) {
            return true;
        } else {
            return false;
        }
    }

    public void reset_all_library () {
        File db_path = File.new_for_path (db_path);
        try {
            db_path.delete ();
        } catch (Error err) {
            warning (err.message);
        }

        create_tables ();
        reset_library ();

        File directory = File.new_for_path (Byte.utils.COVER_FOLDER);
        try {
            var children = directory.enumerate_children ("", 0);
            FileInfo file_info;
            while ((file_info = children.next_file ()) != null) {
                FileUtils.remove (GLib.Path.build_filename (Byte.utils.COVER_FOLDER, file_info.get_name ()));
            }
        
            children.close ();
            children.dispose ();
        } catch (Error err) {
            warning (err.message);
        }
        
        directory.dispose ();
    }

    public void update_playlist (Objects.Playlist playlist) {
        Sqlite.Statement stmt;
        string sql;
        int res;

        sql = """
            UPDATE playlists SET title = ?, note = ?, date_updated = ? WHERE id = ?;
        """; 

        res = db.prepare_v2 (sql, -1, out stmt);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (1, playlist.title);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (2, playlist.note);
        assert (res == Sqlite.OK);

        res = stmt.bind_text (3, playlist.date_updated);
        assert (res == Sqlite.OK);

        res = stmt.bind_int (4, playlist.id);
        assert (res == Sqlite.OK);

        res = stmt.step ();

        if (res == Sqlite.DONE) {
            updated_playlist (playlist);
        }
    }

    public Objects.Playlist? get_playlist_by_title (string title) {
        foreach (var playlist in Byte.database.get_all_playlists ()) {
            if (playlist.title == title) {
                return playlist;
            }
        }
        return null;
    }

    public Objects.Playlist create_new_playlist () {
        string new_title = _ ("New Playlist");
        string next_title = new_title;

        var playlist = get_playlist_by_title (next_title);
        for (int i = 1; playlist != null; i++) {
            next_title = "%s (%d)".printf (new_title, i);
            playlist = get_playlist_by_title (next_title);
        }

        playlist = new Objects.Playlist ();
        playlist.title = next_title;
        playlist.id = insert_playlist (playlist);
        
        return playlist;
    }
}
