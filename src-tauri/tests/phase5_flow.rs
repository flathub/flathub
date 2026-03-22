use std::{
    fs,
    path::{Path, PathBuf},
    sync::{Arc, Mutex},
};

mod support;

use openkara_lib::{
    audio::playback::PlaybackController,
    cache,
    commands::{
        import::import_songs_from_paths,
        lyrics::{fetch_lyrics_from_connection, set_lyrics_offset_in_connection},
        playback::play_song_from_library,
    },
    config::StemMode,
    library_root::LibraryRoot,
    lyrics::{lrcapi::LrcApiClient, lrclib::LrcLibClient},
    separator::{job, model, model_cache::ModelCache},
};
use rusqlite::Connection;

fn metadata_fixture_path(filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join("metadata")
        .join(filename)
}

fn unique_temp_dir(prefix: &str) -> PathBuf {
    support::unique_temp_path(prefix)
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary directory should be removable");
    }
}

#[test]
fn backend_karaoke_flow_imports_plays_separates_fetches_lyrics_and_switches_mode() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let fixture_dir = unique_temp_dir("phase5-fixture");
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");
    fs::write(
        audio_path.with_extension("lrc"),
        "[00:10.00] Look at the stars\n[00:20.00] Look how they shine for you",
    )
    .expect("sidecar lyrics should write");

    let lib_dir = unique_temp_dir("phase5-library");
    cleanup_dir(&lib_dir);
    let library = LibraryRoot::create(&lib_dir).expect("library should create");
    let import_result =
        import_songs_from_paths(&connection, &library, &[audio_path.display().to_string()]);
    assert_eq!(import_result.imported.len(), 1);
    assert!(import_result.failed.is_empty());
    let song_id = import_result.imported[0].hash.clone();

    // Write sidecar .lrc next to the imported media file inside the library
    let imported_media = library.resolve(&import_result.imported[0].file_path);
    fs::write(
        imported_media.with_extension("lrc"),
        "[00:10.00] Look at the stars\n[00:20.00] Look how they shine for you",
    )
    .expect("sidecar lyrics should write into library");

    let mut playback = PlaybackController::default();
    let started = play_song_from_library(&connection, &library, &mut playback, &song_id, 1_000)
        .expect("song should load into the playback controller");
    assert_eq!(started.song_id.as_deref(), Some(song_id.as_str()));
    assert!(!started.has_stems);

    let model_cache = Arc::new(Mutex::new(ModelCache::default()));
    let separation = job::separate_song_into_cache(
        &connection,
        &library,
        &model_cache,
        &model::default_model_path(),
        &song_id,
        StemMode::default(),
        "htdemucs",
        |_| {},
    )
    .expect("separation should succeed for the imported fixture");
    assert!(library.resolve(&separation.accomp_path).exists());

    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::Any)
        .with_status(404)
        .create();

    let lyrics = fetch_lyrics_from_connection(
        &connection,
        &library,
        &LrcLibClient::new(server.url()),
        &LrcApiClient::new("http://127.0.0.1:9"),
        &song_id,
    )
    .expect("lyrics fetch should fall back to the sidecar file");
    assert_eq!(
        lyrics.source,
        Some(openkara_lib::lyrics::fetch::LyricsSource::Sidecar)
    );
    assert_eq!(lyrics.lines.len(), 2);

    set_lyrics_offset_in_connection(&connection, &song_id, 500)
        .expect("offset should persist for fetched lyrics");
    let cached_lyrics = fetch_lyrics_from_connection(
        &connection,
        &library,
        &LrcLibClient::new("http://127.0.0.1:9"),
        &LrcApiClient::new("http://127.0.0.1:9"),
        &song_id,
    )
    .expect("second fetch should read lyrics from cache");
    assert_eq!(cached_lyrics.offset_ms, 500);

    mock.assert();
    cleanup_dir(&fixture_dir);
    cleanup_dir(&lib_dir);
}
