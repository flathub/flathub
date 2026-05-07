use openkara_lib::{
    audio::playback::PlaybackController,
    cache,
    library::Song,
    library_root::LibraryRoot,
    services::playback::play_song_from_library,
};
use std::fs;
use rusqlite::Connection;

#[test]
fn cache_round_trip_preserves_nullable_file_path_and_audio_source_kind() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let song = Song {
        hash: "remote-song".to_owned(),
        file_path: None,
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
            language: None,
        title: Some("Remote Song".to_owned()),
        artist: Some("Remote Artist".to_owned()),
        album: Some("Remote Album".to_owned()),
        duration_ms: 123_456,
        cover_art: None,
        imported_at: 1,
        original_ext: Some("mp3".to_owned()),
        audio_source_kind: "stems_remote".to_owned(),
    };

    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let stored = cache::get_song_by_hash(&connection, &song.hash)
        .expect("song lookup should succeed")
        .expect("song should be present after insert");

    assert_eq!(stored.file_path, None);
    assert_eq!(stored.audio_source_kind, "stems_remote");
}

#[test]
fn remote_stems_song_plays_from_cached_stems_without_a_local_file_path() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let tempdir = tempfile::tempdir().expect("temp dir should create");
    let library = LibraryRoot::create(tempdir.path().join("library").as_path())
        .expect("library should create");

    let song = Song {
        hash: "remote-stems".to_owned(),
        file_path: None,
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
            language: None,
        audio_source_kind: "stems_remote".to_owned(),
        title: Some("Remote Stems".to_owned()),
        artist: Some("Remote Artist".to_owned()),
        album: Some("Remote Album".to_owned()),
        duration_ms: 1_000,
        cover_art: None,
        imported_at: 1,
        original_ext: Some("ogg".to_owned()),
    };
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let stems_dir = library.stems_dir().join("remote-stems");
    fs::create_dir_all(&stems_dir).expect("stems directory should create");
    fs::copy(
        openkara_test_fixture("audio/fixture.ogg"),
        stems_dir.join("vocals.ogg"),
    )
    .expect("vocals stem should copy");
    fs::copy(
        openkara_test_fixture("audio/fixture.ogg"),
        stems_dir.join("accompaniment.ogg"),
    )
    .expect("accompaniment stem should copy");

    connection
        .execute(
            "INSERT INTO stems (
                song_hash,
                vocals_path,
                accomp_path,
                separated_at,
                model_variant
            ) VALUES (?1, ?2, ?3, ?4, ?5)",
            rusqlite::params![
                "remote-stems",
                "stems/remote-stems/vocals.ogg",
                "stems/remote-stems/accompaniment.ogg",
                1_i64,
                "htdemucs",
            ],
        )
        .expect("stem cache row should insert");

    let mut controller = PlaybackController::default();
    let snapshot = play_song_from_library(
        &connection,
        &library,
        &mut controller,
        "remote-stems",
        1_000,
    )
    .expect("remote stems playback should succeed");

    assert_eq!(snapshot.song_id.as_deref(), Some("remote-stems"));
    assert!(snapshot.has_stems);
    assert_eq!(snapshot.stem_mode.as_deref(), Some("two_stem"));
}

fn openkara_test_fixture(relative: &str) -> std::path::PathBuf {
    std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(relative)
}
