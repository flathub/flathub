use std::{
    fs,
    path::{Path, PathBuf},
};

mod support;

use openkara_lib::{
    cache::{self, lyrics},
    commands::lyrics::{fetch_lyrics_from_connection, set_lyrics_offset_in_connection},
    library::Song,
    library_root::LibraryRoot,
    lyrics::{fetch::LyricsSource, lrclib::LrcLibClient},
};
use rusqlite::Connection;

fn metadata_fixture_path(filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join("metadata")
        .join(filename)
}

fn unique_fixture_dir() -> PathBuf {
    support::unique_temp_path("phase4-commands")
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary fixture directory should be removable");
    }
}

fn fixture_song(hash: &str, file_path: &Path) -> Song {
    Song {
        hash: hash.to_owned(),
        file_path: file_path.display().to_string(),
        cdg_path: None,
        media_g_container: None,
        title: Some("Yellow".to_owned()),
        artist: Some("Coldplay".to_owned()),
        album: Some("Parachutes".to_owned()),
        duration_ms: 267_000,
        cover_art: None,
        imported_at: 1,
        original_ext: None,
    }
}

fn unique_library() -> LibraryRoot {
    let path = support::unique_temp_path("phase4-lib");
    if path.exists() {
        fs::remove_dir_all(&path).ok();
    }
    LibraryRoot::create(&path).expect("library should create")
}

#[test]
fn fetch_lyrics_reads_cached_lrc_before_attempting_remote_fetch() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library();

    let song = fixture_song("song-a", &metadata_fixture_path("fixture.mp3"));
    cache::upsert_song(&connection, &song).expect("song insert should succeed");
    lyrics::upsert_lyrics_cache_entry(
        &connection,
        &lyrics::LyricsCacheEntry {
            song_hash: song.hash.clone(),
            lrc: "[00:10.00] Look at the stars".to_owned(),
            source: LyricsSource::LrcLib,
            offset_ms: 250,
            fetched_at: 10,
        },
    )
    .expect("lyrics cache insert should succeed");

    let payload = fetch_lyrics_from_connection(
        &connection,
        &library,
        &LrcLibClient::new("http://127.0.0.1:9"),
        &song.hash,
    )
    .expect("cache-backed lyrics fetch should succeed");

    assert_eq!(payload.song_id, "song-a");
    assert_eq!(payload.offset_ms, 250);
    assert_eq!(payload.source, Some(LyricsSource::LrcLib));
    assert_eq!(payload.lines.len(), 1);
    assert_eq!(payload.lines[0].time_ms, 10_000);
    assert_eq!(payload.lines[0].text, "Look at the stars");
}

#[test]
fn fetch_lyrics_fetches_remote_lrc_and_persists_it_in_cache() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");

    // Create a library and copy the fixture into it so the resolved path works
    let library = unique_library();
    let dest = library.media_path("song-b", "mp3");
    fs::copy(&audio_path, &dest).expect("copy to library media");

    let song = fixture_song("song-b", Path::new("media/song-b.mp3"));
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::Any)
        .with_status(200)
        .with_header("content-type", "application/json")
        .with_body(
            r#"{
                "id": 1,
                "trackName": "Yellow",
                "artistName": "Coldplay",
                "albumName": "Parachutes",
                "duration": 267.0,
                "instrumental": false,
                "syncedLyrics": "[00:35.66] Look at the stars"
            }"#,
        )
        .create();

    let payload =
        fetch_lyrics_from_connection(&connection, &library, &LrcLibClient::new(server.url()), &song.hash)
            .expect("remote lyrics fetch should succeed");

    assert_eq!(payload.song_id, "song-b");
    assert_eq!(payload.offset_ms, 0);
    assert_eq!(payload.source, Some(LyricsSource::LrcLib));
    assert_eq!(payload.lines.len(), 1);
    assert_eq!(payload.lines[0].time_ms, 35_660);

    let cached = lyrics::get_lyrics_cache_entry(&connection, &song.hash)
        .expect("lyrics cache lookup should succeed")
        .expect("lyrics cache entry should exist after fetch");
    assert_eq!(cached.source, LyricsSource::LrcLib);
    assert_eq!(cached.lrc, "[00:35.66] Look at the stars");

    mock.assert();
    cleanup_dir(&fixture_dir);
}

#[test]
fn fetch_lyrics_returns_empty_payload_when_no_synced_source_exists() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");

    // Create a library and copy the fixture into it
    let library = unique_library();
    let dest = library.media_path("song-c", "mp3");
    fs::copy(&audio_path, &dest).expect("copy to library media");

    let song = fixture_song("song-c", Path::new("media/song-c.mp3"));
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::Any)
        .with_status(404)
        .create();

    let payload =
        fetch_lyrics_from_connection(&connection, &library, &LrcLibClient::new(server.url()), &song.hash)
            .expect("lyrics miss should still succeed");

    assert_eq!(payload.song_id, "song-c");
    assert!(payload.lines.is_empty());
    assert_eq!(payload.offset_ms, 0);
    assert_eq!(payload.source, None);
    assert!(lyrics::get_lyrics_cache_entry(&connection, &song.hash)
        .expect("lyrics cache lookup should succeed")
        .is_none());

    mock.assert();
    cleanup_dir(&fixture_dir);
}

#[test]
fn set_lyrics_offset_updates_existing_cached_lyrics() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let song = fixture_song("song-d", &metadata_fixture_path("fixture.mp3"));
    cache::upsert_song(&connection, &song).expect("song insert should succeed");
    lyrics::upsert_lyrics_cache_entry(
        &connection,
        &lyrics::LyricsCacheEntry {
            song_hash: song.hash.clone(),
            lrc: "[00:10.00] Look at the stars".to_owned(),
            source: LyricsSource::LrcLib,
            offset_ms: 0,
            fetched_at: 10,
        },
    )
    .expect("lyrics cache insert should succeed");

    set_lyrics_offset_in_connection(&connection, &song.hash, 500)
        .expect("offset update should succeed");

    let cached = lyrics::get_lyrics_cache_entry(&connection, &song.hash)
        .expect("lyrics cache lookup should succeed")
        .expect("lyrics cache entry should exist");
    assert_eq!(cached.offset_ms, 500);
}

#[test]
fn set_lyrics_offset_rejects_missing_cached_lyrics() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let song = fixture_song("song-e", &metadata_fixture_path("fixture.mp3"));
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let error = set_lyrics_offset_in_connection(&connection, &song.hash, 500)
        .expect_err("offset update should fail without cached lyrics");

    assert!(error.to_string().contains("does not have cached lyrics"));
}
