use openkara_lib::{cache, library::Song};
use rusqlite::Connection;

fn sample_song(hash: &str, title: &str, artist: &str, imported_at: i64) -> Song {
    Song {
        hash: hash.to_owned(),
        file_path: format!("/music/{hash}.mp3"),
        cdg_path: None,
        media_g_container: None,
        title: Some(title.to_owned()),
        artist: Some(artist.to_owned()),
        album: Some("Fixture Album".to_owned()),
        duration_ms: 1_000,
        cover_art: Some(vec![1, 2, 3, 4]),
        imported_at,
        original_ext: None,
    }
}

#[test]
fn inserts_and_lists_songs_in_reverse_import_order() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    cache::upsert_song(
        &connection,
        &sample_song("hash-a", "First Song", "Artist A", 10),
    )
    .expect("first insert should succeed");
    cache::upsert_song(
        &connection,
        &sample_song("hash-b", "Second Song", "Artist B", 20),
    )
    .expect("second insert should succeed");

    let songs = cache::list_songs(&connection).expect("song listing should succeed");

    assert_eq!(songs.len(), 2);
    assert_eq!(songs[0].hash, "hash-b");
    assert_eq!(songs[0].title.as_deref(), Some("Second Song"));
    assert_eq!(songs[0].cover_art.as_deref(), Some(&[1, 2, 3, 4][..]));
    assert_eq!(songs[1].hash, "hash-a");
}

#[test]
fn search_matches_title_and_artist_case_insensitively() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    cache::upsert_song(&connection, &sample_song("hash-a", "Starlight", "Muse", 10))
        .expect("first insert should succeed");
    cache::upsert_song(
        &connection,
        &sample_song("hash-b", "Knights", "MUSE tribute", 20),
    )
    .expect("second insert should succeed");

    let title_results =
        cache::search_songs(&connection, "star").expect("title search should succeed");
    let artist_results =
        cache::search_songs(&connection, "muse").expect("artist search should succeed");

    assert_eq!(title_results.len(), 1);
    assert_eq!(title_results[0].hash, "hash-a");
    assert_eq!(artist_results.len(), 2);
}

#[test]
fn upsert_replaces_existing_song_record_by_hash() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    cache::upsert_song(
        &connection,
        &sample_song("hash-a", "Original Title", "Artist A", 10),
    )
    .expect("first insert should succeed");
    cache::upsert_song(
        &connection,
        &sample_song("hash-a", "Updated Title", "Artist A", 30),
    )
    .expect("upsert should succeed");

    let songs = cache::list_songs(&connection).expect("song listing should succeed");

    assert_eq!(songs.len(), 1);
    assert_eq!(songs[0].title.as_deref(), Some("Updated Title"));
    assert_eq!(songs[0].imported_at, 30);
}
