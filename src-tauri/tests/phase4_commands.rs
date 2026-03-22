use std::{
    fs,
    io::Write,
    path::{Path, PathBuf},
};

use lofty::{
    config::WriteOptions,
    file::{AudioFile, TaggedFileExt},
    picture::{MimeType, Picture, PictureType},
    probe::Probe,
    tag::{Tag, TagType},
};
mod support;

use openkara_lib::{
    cache::{self, lyrics},
    commands::{
        import::{
            extract_embedded_cover_art_from_connection, set_songs_instrumental_in_connection,
        },
        lyrics::{fetch_lyrics_from_connection, set_lyrics_offset_in_connection},
    },
    library::Song,
    library_root::LibraryRoot,
    lyrics::{fetch::LyricsSource, lrcapi::LrcApiClient, lrclib::LrcLibClient},
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

fn unique_library() -> LibraryRoot {
    let path = support::unique_temp_path("phase4-lib");
    if path.exists() {
        fs::remove_dir_all(&path).ok();
    }
    LibraryRoot::create(&path).expect("library should create")
}

fn write_wav_without_cover(path: &Path) {
    let spec = hound::WavSpec {
        channels: 1,
        sample_rate: 44_100,
        bits_per_sample: 16,
        sample_format: hound::SampleFormat::Int,
    };
    let mut writer = hound::WavWriter::create(path, spec).expect("wav should create");
    writer.write_sample::<i16>(0).expect("sample should write");
    writer.finalize().expect("wav should finalize");
}

fn copy_mp3_with_embedded_cover(destination: &Path) {
    fs::copy(metadata_fixture_path("fixture.mp3"), destination).expect("fixture audio should copy");

    let mut tagged_file = Probe::open(destination)
        .expect("fixture should open")
        .read()
        .expect("fixture tags should read");
    let mut tag = Tag::new(TagType::Id3v2);
    tag.push_picture(
        Picture::unchecked(vec![0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a])
            .pic_type(PictureType::CoverFront)
            .mime_type(MimeType::Png)
            .build(),
    );
    tagged_file.insert_tag(tag);
    tagged_file
        .save_to_path(destination, WriteOptions::default())
        .expect("fixture cover art should save");
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
        &LrcApiClient::new("http://127.0.0.1:9"),
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
fn fetch_lyrics_fetches_remote_lrc_api_and_persists_it_in_cache() {
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
                    "id": "1",
                    "title": "Yellow",
                    "artist": "Coldplay",
                    "album": "Parachutes",
                    "score": 95.0,
                    "lrc": "[00:35.66] Look at the stars",
                    "lrc_ttml": "<tt>ignored</tt>",
                    "lyric_path": "/lyrics/yellow"
                }
            ]"#,
        )
        .create();

    let lrcapi_client = LrcApiClient::new(lrcapi_server.url());
    let payload = fetch_lyrics_from_connection(
        &connection,
        &library,
        &LrcLibClient::new(lrclib_server.url()),
        &lrcapi_client,
        &song.hash,
    )
    .expect("remote lyrics fetch should succeed");

    assert_eq!(payload.song_id, "song-b");
    assert_eq!(payload.offset_ms, 0);
    assert_eq!(payload.source, Some(LyricsSource::LrcApi));
    assert_eq!(payload.lines.len(), 1);
    assert_eq!(payload.lines[0].time_ms, 35_660);

    let cached = lyrics::get_lyrics_cache_entry(&connection, &song.hash)
        .expect("lyrics cache lookup should succeed")
        .expect("lyrics cache entry should exist after fetch");
    assert_eq!(cached.source, LyricsSource::LrcApi);
    assert_eq!(cached.lrc, "[00:35.66] Look at the stars");

    lrclib_mock.assert();
    lrcapi_mock.assert();
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

    let payload = fetch_lyrics_from_connection(
        &connection,
        &library,
        &LrcLibClient::new(lrclib_server.url()),
        &LrcApiClient::new(lrcapi_server.url()),
        &song.hash,
    )
    .expect("lyrics miss should still succeed");

    assert_eq!(payload.song_id, "song-c");
    assert!(payload.lines.is_empty());
    assert_eq!(payload.offset_ms, 0);
    assert_eq!(payload.source, None);
    assert!(lyrics::get_lyrics_cache_entry(&connection, &song.hash)
        .expect("lyrics cache lookup should succeed")
        .is_none());

    lrclib_mock.assert();
    lrcapi_mock.assert();
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
fn set_songs_instrumental_updates_every_requested_song() {
    let mut connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let song_a = fixture_song("song-a", &metadata_fixture_path("fixture.mp3"));
    let song_b = fixture_song("song-b", &metadata_fixture_path("fixture.mp3"));
    cache::upsert_song(&connection, &song_a).expect("first song insert should succeed");
    cache::upsert_song(&connection, &song_b).expect("second song insert should succeed");

    let updated = set_songs_instrumental_in_connection(
        &mut connection,
        &[song_a.hash.clone(), song_b.hash.clone()],
        true,
    )
    .expect("instrumental flag should update");

    assert_eq!(updated.len(), 2);
    assert!(updated.iter().all(|song| song.instrumental));
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

#[test]
fn extract_embedded_cover_art_updates_a_regular_song_and_persists_it() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library();

    let dest = library.media_path("song-cover", "mp3");
    copy_mp3_with_embedded_cover(&dest);

    let song = Song {
        hash: "song-cover".to_owned(),
        file_path: "media/song-cover.mp3".to_owned(),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        title: Some("Fixture Song MP3".to_owned()),
        artist: Some("Fixture Artist".to_owned()),
        album: Some("Fixture Album".to_owned()),
        duration_ms: 1_000,
        cover_art: None,
        imported_at: 1,
        original_ext: Some("mp3".to_owned()),
    };
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let result =
        extract_embedded_cover_art_from_connection(&connection, &library, &[song.hash.clone()]);

    assert_eq!(result.failed.len(), 0);
    assert_eq!(result.updated_songs.len(), 1);
    assert!(result.updated_songs[0]
        .cover_art
        .as_ref()
        .is_some_and(|bytes| !bytes.is_empty()));

    let persisted = cache::get_song_by_hash(&connection, &song.hash)
        .expect("song lookup should succeed")
        .expect("song should exist");
    assert_eq!(persisted.cover_art, result.updated_songs[0].cover_art);
}

#[test]
fn extract_embedded_cover_art_reads_cover_art_from_media_g_zip_audio_bytes() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library();

    let dest = library.media_g_zip_path("song-zip");
    let zip_dir = unique_fixture_dir();
    cleanup_dir(&zip_dir);
    fs::create_dir_all(&zip_dir).expect("zip fixture directory should create");
    let covered_mp3 = zip_dir.join("fixture.mp3");
    copy_mp3_with_embedded_cover(&covered_mp3);

    let file = fs::File::create(&dest).expect("zip should create");
    let mut zip = zip::ZipWriter::new(file);
    let options = zip::write::SimpleFileOptions::default();
    zip.start_file("fixture.mp3", options)
        .expect("audio entry should start");
    zip.write_all(&fs::read(&covered_mp3).expect("covered audio should read"))
        .expect("audio entry should write");
    zip.start_file("fixture.cdg", options)
        .expect("cdg entry should start");
    zip.write_all(&[0x09_u8; 24])
        .expect("cdg entry should write");
    zip.finish().expect("zip should finish");

    let song = Song {
        hash: "song-zip".to_owned(),
        file_path: "media-g/song-zip.zip".to_owned(),
        cdg_path: None,
        media_g_container: Some("zip".to_owned()),
        instrumental: false,
        title: Some("Fixture Song MP3".to_owned()),
        artist: Some("Fixture Artist".to_owned()),
        album: Some("Fixture Album".to_owned()),
        duration_ms: 1_000,
        cover_art: None,
        imported_at: 1,
        original_ext: Some("mp3".to_owned()),
    };
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let result =
        extract_embedded_cover_art_from_connection(&connection, &library, &[song.hash.clone()]);

    assert!(result.failed.is_empty());
    assert_eq!(result.updated_songs.len(), 1);
    assert!(result.updated_songs[0]
        .cover_art
        .as_ref()
        .is_some_and(|bytes| !bytes.is_empty()));
    cleanup_dir(&zip_dir);
}

#[test]
fn extract_embedded_cover_art_keeps_existing_cover_when_a_song_has_no_embedded_art() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library();

    let dest = library.media_path("song-no-cover", "wav");
    write_wav_without_cover(&dest);

    let song = Song {
        hash: "song-no-cover".to_owned(),
        file_path: "media/song-no-cover.wav".to_owned(),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        title: Some("No Cover".to_owned()),
        artist: Some("Fixture Artist".to_owned()),
        album: None,
        duration_ms: 1_000,
        cover_art: Some(vec![1, 2, 3, 4]),
        imported_at: 1,
        original_ext: Some("wav".to_owned()),
    };
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let result =
        extract_embedded_cover_art_from_connection(&connection, &library, &[song.hash.clone()]);

    assert!(result.updated_songs.is_empty());
    assert_eq!(result.failed.len(), 1);
    assert!(result.failed[0]
        .error
        .message
        .contains("does not contain embedded cover art"));

    let persisted = cache::get_song_by_hash(&connection, &song.hash)
        .expect("song lookup should succeed")
        .expect("song should exist");
    assert_eq!(persisted.cover_art, Some(vec![1, 2, 3, 4]));
}
