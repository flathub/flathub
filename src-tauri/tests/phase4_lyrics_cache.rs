use openkara_lib::{
    cache::{self, lyrics},
    library::Song,
    lyrics::fetch::LyricsSource,
};
use rusqlite::Connection;

fn sample_song(hash: &str) -> Song {
    Song {
        hash: hash.to_owned(),
        file_path: format!("/music/{hash}.mp3"),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        title: Some("Yellow".to_owned()),
        artist: Some("Coldplay".to_owned()),
        album: Some("Parachutes".to_owned()),
        duration_ms: 267_000,
        cover_art: None,
        imported_at: 1,
        original_ext: None,
    }
}

#[test]
fn upserts_lyrics_cache_and_persists_offset_updates() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    cache::upsert_song(&connection, &sample_song("song-a")).expect("song insert should succeed");

    lyrics::upsert_lyrics_cache_entry(
        &connection,
        &lyrics::LyricsCacheEntry {
            song_hash: "song-a".to_owned(),
            lrc: "[00:10.00] Look at the stars".to_owned(),
            source: LyricsSource::LrcLib,
            offset_ms: 0,
            fetched_at: 10,
        },
    )
    .expect("lyrics cache insert should succeed");

    let cached = lyrics::get_lyrics_cache_entry(&connection, "song-a")
        .expect("lyrics cache lookup should succeed")
        .expect("lyrics cache entry should exist");
    assert_eq!(cached.source, LyricsSource::LrcLib);
    assert_eq!(cached.offset_ms, 0);

    lyrics::set_lyrics_offset(&connection, "song-a", 500).expect("offset update should succeed");

    let updated = lyrics::get_lyrics_cache_entry(&connection, "song-a")
        .expect("lyrics cache lookup should succeed")
        .expect("lyrics cache entry should exist");
    assert_eq!(updated.offset_ms, 500);
}

#[test]
fn serializes_and_deserializes_lrcapi_source_values() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    cache::upsert_song(&connection, &sample_song("song-b")).expect("song insert should succeed");

    lyrics::upsert_lyrics_cache_entry(
        &connection,
        &lyrics::LyricsCacheEntry {
            song_hash: "song-b".to_owned(),
            lrc: "[00:10.00] Look at the stars".to_owned(),
            source: LyricsSource::LrcApi,
            offset_ms: 0,
            fetched_at: 10,
        },
    )
    .expect("lyrics cache insert should succeed");

    let cached = lyrics::get_lyrics_cache_entry(&connection, "song-b")
        .expect("lyrics cache lookup should succeed")
        .expect("lyrics cache entry should exist");
    assert_eq!(cached.source, LyricsSource::LrcApi);
}
