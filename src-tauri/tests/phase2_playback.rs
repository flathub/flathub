use std::path::PathBuf;

use openkara_lib::{
    audio::{
        decode,
        playback::{PlaybackController, PlaybackStateSnapshot},
    },
    cache,
    commands::{import::import_songs_from_paths, playback::play_song_from_library},
    library_root::LibraryRoot,
};
use rusqlite::Connection;

fn fixture_path(directory: &str, filename: &str) -> String {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
        .display()
        .to_string()
}

fn assert_snapshot(
    snapshot: &PlaybackStateSnapshot,
    expected_song_id: Option<&str>,
    expected_playing: bool,
    expected_position_ms: u64,
) {
    assert_eq!(snapshot.song_id.as_deref(), expected_song_id);
    assert_eq!(snapshot.is_playing, expected_playing);
    assert_eq!(snapshot.position_ms, expected_position_ms);
}

#[test]
fn playback_controller_transitions_pause_seek_and_volume() {
    let decoded =
        decode::decode_file(PathBuf::from(fixture_path("audio", "fixture.wav")).as_path()).unwrap();
    let mut controller = PlaybackController::default();
    assert_eq!(controller.snapshot(0).volume, 1.0);

    let started = controller.start_track("song-a".into(), decoded, 1_000);
    assert_snapshot(&started, Some("song-a"), true, 0);

    let paused = controller.pause(1_250).expect("pause should succeed");
    assert_snapshot(&paused, Some("song-a"), false, 250);

    let resumed = controller.play(1_500).expect("resume should succeed");
    assert_snapshot(&resumed, Some("song-a"), true, 250);

    let sought = controller.seek(900, 1_700).expect("seek should succeed");
    assert_snapshot(&sought, Some("song-a"), true, 900);

    let clamped = controller
        .set_volume(1.5)
        .expect("set volume should succeed for loaded track");
    assert_eq!(clamped.volume, 1.0);

    let quiet = controller
        .set_volume(-0.25)
        .expect("volume clamp should allow values below zero");
    assert_eq!(quiet.volume, 0.0);
}

#[test]
fn playback_controller_advances_and_stops_at_track_end() {
    let decoded =
        decode::decode_file(PathBuf::from(fixture_path("audio", "fixture.wav")).as_path()).unwrap();
    let mut controller = PlaybackController::default();

    controller.start_track("song-a".into(), decoded, 5_000);

    let advanced = controller.snapshot(5_400);
    assert_snapshot(&advanced, Some("song-a"), true, 400);

    let ended = controller.snapshot(6_500);
    assert!(ended.duration_ms.is_some());
    assert_eq!(
        ended.position_ms,
        ended.duration_ms.expect("duration should exist")
    );
    assert!(!ended.is_playing);
}

#[test]
fn play_song_from_library_loads_track_by_hash() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let tmp = tempfile::tempdir().expect("temp dir should create");
    let library =
        LibraryRoot::create(tmp.path().join("lib").as_path()).expect("library should create");
    let import_result = import_songs_from_paths(
        &connection,
        &library,
        &[fixture_path("metadata", "fixture.mp3")],
    );
    assert_eq!(import_result.imported.len(), 1);

    let song_hash = import_result.imported[0].hash.clone();
    let mut controller = PlaybackController::default();

    let snapshot =
        play_song_from_library(&connection, &library, &mut controller, &song_hash, 10_000)
            .expect("play helper should load and start track");

    assert_snapshot(&snapshot, Some(song_hash.as_str()), true, 0);
    assert!(snapshot.duration_ms.is_some());
}
