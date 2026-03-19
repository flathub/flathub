use std::{
    fs,
    path::{Path, PathBuf},
};

mod support;

use openkara_lib::{
    library::Song,
    lyrics::{
        fetch::{fetch_lyrics_for_song, LyricsFetchResult, LyricsSource},
        lrclib::LrcLibClient,
    },
};

fn metadata_fixture_path(filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join("metadata")
        .join(filename)
}

fn unique_fixture_dir() -> PathBuf {
    support::unique_temp_path("phase4-fetch")
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary fixture directory should be removable");
    }
}

fn fixture_song(file_path: &Path) -> Song {
    Song {
        hash: "fixture-song".to_owned(),
        file_path: file_path.display().to_string(),
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
fn fetch_chain_prefers_lrclib_synced_lyrics_over_sidecar() {
    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");
    fs::write(audio_path.with_extension("lrc"), "[00:10.00] from sidecar")
        .expect("sidecar should write");

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
                "syncedLyrics": "[00:35.66] from lrclib"
            }"#,
        )
        .create();

    let fetched = fetch_lyrics_for_song(
        &LrcLibClient::new(server.url()),
        &fixture_song(&audio_path),
        &audio_path,
    )
    .expect("fetch chain should succeed")
    .expect("lyrics should be returned");

    assert_eq!(
        fetched,
        LyricsFetchResult {
            source: LyricsSource::LrcLib,
            raw_lrc: "[00:35.66] from lrclib".to_owned(),
        }
    );

    mock.assert();
    cleanup_dir(&fixture_dir);
}

#[test]
fn fetch_chain_falls_back_to_sidecar_when_lrclib_misses() {
    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");
    fs::write(audio_path.with_extension("lrc"), "[00:10.00] from sidecar")
        .expect("sidecar should write");

    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::Any)
        .with_status(404)
        .create();

    let fetched = fetch_lyrics_for_song(
        &LrcLibClient::new(server.url()),
        &fixture_song(&audio_path),
        &audio_path,
    )
    .expect("fetch chain should succeed")
    .expect("sidecar lyrics should be returned");

    assert_eq!(
        fetched,
        LyricsFetchResult {
            source: LyricsSource::Sidecar,
            raw_lrc: "[00:10.00] from sidecar".to_owned(),
        }
    );

    mock.assert();
    cleanup_dir(&fixture_dir);
}
