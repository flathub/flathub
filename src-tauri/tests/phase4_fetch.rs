use std::{
    fs,
    path::{Path, PathBuf},
};

use lofty::{
    config::WriteOptions,
    tag::{ItemKey, Tag, TagExt, TagType},
};
mod support;

use openkara_lib::{
    library::Song,
    lyrics::{
        fetch::{
            fetch_lyrics_for_song, read_embedded_lyrics, LyricsFetchResult, LyricsSource,
            TimedLyricsProvider,
        },
        lrcapi::LrcApiClient,
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
        file_path: Some(file_path.display().to_string()),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        audio_source_kind: "original".to_owned(),
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

    let lrclib_client = LrcLibClient::new(server.url());
    let lrcapi = LrcApiClient::new("http://127.0.0.1:9");
    let providers = [
        TimedLyricsProvider::LrcLib(&lrclib_client),
        TimedLyricsProvider::LrcApi(&lrcapi),
    ];

    let fetched = fetch_lyrics_for_song(
        &providers,
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
fn fetch_chain_prefers_lrcapi_over_sidecar_when_lrclib_misses() {
    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");
    fs::write(audio_path.with_extension("lrc"), "[00:10.00] from sidecar")
        .expect("sidecar should write");

    let mut lrclib_server = mockito::Server::new();
    let lrclib_mock = lrclib_server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::Any)
        .with_status(404)
        .create();

    let mut lrcapi_server = mockito::Server::new();
    let lrcapi_mock = lrcapi_server
        .mock("GET", "/jsonapi")
        .match_query(mockito::Matcher::AllOf(vec![
            mockito::Matcher::UrlEncoded("title".into(), "Yellow".into()),
            mockito::Matcher::UrlEncoded("artist".into(), "Coldplay".into()),
            mockito::Matcher::UrlEncoded("album".into(), "Parachutes".into()),
        ]))
        .with_status(200)
        .with_header("content-type", "application/json")
        .with_body(
            r#"[
                {
                    "id": "2",
                    "title": "Yellow",
                    "artist": "Coldplay",
                    "album": "Parachutes",
                    "score": 99.0,
                    "lrc": "[00:33.64] from lrcapi",
                    "lrc_ttml": null,
                    "lyric_path": "/lyrics/yellow"
                }
            ]"#,
        )
        .create();

    let lrclib_client = LrcLibClient::new(lrclib_server.url());
    let lrcapi = LrcApiClient::new(lrcapi_server.url());
    let providers = [
        TimedLyricsProvider::LrcLib(&lrclib_client),
        TimedLyricsProvider::LrcApi(&lrcapi),
    ];

    let fetched = fetch_lyrics_for_song(
        &providers,
        &fixture_song(&audio_path),
        &audio_path,
    )
    .expect("fetch chain should succeed")
    .expect("LrcApi lyrics should be returned");

    assert_eq!(
        fetched,
        LyricsFetchResult {
            source: LyricsSource::LrcApi,
            raw_lrc: "[00:33.64] from lrcapi".to_owned(),
        }
    );

    lrclib_mock.assert();
    lrcapi_mock.assert();
    cleanup_dir(&fixture_dir);
}

#[test]
fn fetch_chain_falls_back_to_sidecar_when_online_sources_miss() {
    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let audio_path = fixture_dir.join("yellow.mp3");
    fs::copy(metadata_fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");
    fs::write(audio_path.with_extension("lrc"), "[00:10.00] from sidecar")
        .expect("sidecar should write");

    let mut lrclib_server = mockito::Server::new();
    let lrclib_mock = lrclib_server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::Any)
        .with_status(404)
        .create();

    let mut lrcapi_server = mockito::Server::new();
    let lrcapi_mock = lrcapi_server
        .mock("GET", "/jsonapi")
        .match_query(mockito::Matcher::Any)
        .with_status(200)
        .with_header("content-type", "application/json")
        .with_body(r#"{"message":"未找到歌词"}"#)
        .create();

    let lrclib_client = LrcLibClient::new(lrclib_server.url());
    let lrcapi = LrcApiClient::new(lrcapi_server.url());
    let providers = [
        TimedLyricsProvider::LrcLib(&lrclib_client),
        TimedLyricsProvider::LrcApi(&lrcapi),
    ];

    let fetched = fetch_lyrics_for_song(
        &providers,
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

    lrclib_mock.assert();
    lrcapi_mock.assert();
    cleanup_dir(&fixture_dir);
}

#[test]
fn reads_embedded_lyrics_from_mp4_audio_even_when_extension_is_aac() {
    let fixture_dir = unique_fixture_dir();
    cleanup_dir(&fixture_dir);
    fs::create_dir_all(&fixture_dir).expect("fixture directory should create");

    let tagged_m4a_path = fixture_dir.join("lyrics-source.m4a");
    fs::copy(metadata_fixture_path("fixture.m4a"), &tagged_m4a_path)
        .expect("fixture m4a should copy");

    let mut tag = Tag::new(TagType::Mp4Ilst);
    tag.insert_text(ItemKey::Lyrics, "[00:10.00] embedded line".to_owned());
    tag.save_to_path(&tagged_m4a_path, WriteOptions::default())
        .expect("lyrics tag should save");

    let disguised_aac_path = fixture_dir.join("lyrics-source.aac");
    fs::copy(&tagged_m4a_path, &disguised_aac_path).expect("tagged m4a should copy to .aac");

    let embedded =
        read_embedded_lyrics(&disguised_aac_path).expect("embedded lyrics read should succeed");

    assert_eq!(embedded.as_deref(), Some("[00:10.00] embedded line"));

    cleanup_dir(&fixture_dir);
}
