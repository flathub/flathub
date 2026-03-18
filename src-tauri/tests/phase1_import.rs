use std::{
    fs,
    io::Write,
    path::{Path, PathBuf},
};

use openkara_lib::{
    cache,
    commands::{
        error::{ErrorCode, FallbackAction},
        import::{
            get_library_from_connection, import_songs_from_paths,
            import_songs_from_paths_with_options, ImportSongsOptions,
        },
    },
    library_root::LibraryRoot,
};
use rusqlite::Connection;

fn fixture_path(filename: &str) -> String {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join("metadata")
        .join(filename)
        .display()
        .to_string()
}

fn temp_library() -> (tempfile::TempDir, LibraryRoot) {
    let tmp = tempfile::tempdir().expect("temp dir should create");
    let lib = LibraryRoot::create(tmp.path().join("lib").as_path()).expect("library should create");
    (tmp, lib)
}

fn write_sample_cdg(path: &Path) {
    let mut packet = [0_u8; 24];
    packet[0] = 0x09;
    packet[1] = 0x01;
    fs::write(path, packet).expect("cdg fixture should write");
}

#[test]
fn imports_fixture_audio_and_persists_library_rows() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let (_tmp, library) = temp_library();

    let result = import_songs_from_paths(
        &connection,
        &library,
        &[fixture_path("fixture.mp3"), fixture_path("fixture.flac")],
    );

    assert_eq!(result.imported.len(), 2);
    assert!(result.failed.is_empty());
    assert_eq!(
        result.imported[0].title.as_deref(),
        Some("Fixture Song MP3")
    );
    assert_eq!(
        result.imported[1].title.as_deref(),
        Some("Fixture Song FLAC")
    );
    assert_eq!(result.imported[0].hash.len(), 64);

    let library = get_library_from_connection(&connection).expect("library listing should succeed");
    assert_eq!(library.len(), 2);
}

#[test]
fn reports_failures_without_aborting_other_imports() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let (_tmp, library) = temp_library();

    let missing_path = fixture_path("missing.mp3");
    let result = import_songs_from_paths(
        &connection,
        &library,
        &[fixture_path("fixture.m4a"), missing_path.clone()],
    );

    assert_eq!(result.imported.len(), 1);
    assert_eq!(result.failed.len(), 1);
    assert_eq!(result.failed[0].path, missing_path);
    assert_eq!(result.failed[0].error.code, ErrorCode::MediaReadFailed);
    assert_eq!(
        result.failed[0].error.fallback,
        FallbackAction::ReimportSong
    );
    assert!(!result.failed[0].error.retryable);

    let library_songs =
        get_library_from_connection(&connection).expect("library listing should succeed");
    assert_eq!(library_songs.len(), 1);
    assert_eq!(library_songs[0].title.as_deref(), Some("Fixture Song M4A"));
}

#[test]
fn imports_audio_and_matching_cdg_into_media_g_directory() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let (_tmp, library) = temp_library();
    let import_dir = tempfile::tempdir().expect("temp dir should create");
    let audio_path = import_dir.path().join("paired.mp3");
    let cdg_path = import_dir.path().join("paired.cdg");
    fs::copy(fixture_path("fixture.mp3"), &audio_path).expect("fixture audio should copy");
    write_sample_cdg(&cdg_path);

    let result =
        import_songs_from_paths(&connection, &library, &[audio_path.display().to_string()]);

    assert_eq!(result.imported.len(), 1);
    let song = &result.imported[0];
    assert_eq!(song.media_g_container.as_deref(), Some("paired"));
    let expected_cdg_path = format!("media-g/{}.cdg", song.hash);
    assert_eq!(song.cdg_path.as_deref(), Some(expected_cdg_path.as_str()));
    assert!(library.resolve(&song.file_path).exists());
    assert!(library.resolve(song.cdg_path.as_deref().unwrap()).exists());
}

#[test]
fn imports_mp3g_zip_without_unpacking_it() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let (_tmp, library) = temp_library();
    let zip_dir = tempfile::tempdir().expect("temp dir should create");
    let zip_path = zip_dir.path().join("fixture.zip");
    let file = fs::File::create(&zip_path).expect("zip should create");
    let mut zip = zip::ZipWriter::new(file);
    let options = zip::write::SimpleFileOptions::default();
    zip.start_file("fixture.mp3", options)
        .expect("audio entry should start");
    zip.write_all(&fs::read(fixture_path("fixture.mp3")).expect("fixture audio should read"))
        .expect("audio entry should write");
    zip.start_file("fixture.cdg", options)
        .expect("cdg entry should start");
    zip.write_all(&[0x09_u8; 24])
        .expect("cdg entry should write");
    zip.finish().expect("zip should finish");

    let result = import_songs_from_paths(&connection, &library, &[zip_path.display().to_string()]);

    assert_eq!(result.imported.len(), 1);
    let song = &result.imported[0];
    assert_eq!(song.media_g_container.as_deref(), Some("zip"));
    assert!(song.cdg_path.is_none());
    assert_eq!(song.file_path, format!("media-g/{}.zip", song.hash));
    assert!(library.resolve(&song.file_path).exists());
}

#[test]
fn rejects_standalone_cdg_files_without_matching_audio() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let (_tmp, library) = temp_library();
    let import_dir = tempfile::tempdir().expect("temp dir should create");
    let cdg_path = import_dir.path().join("orphan.cdg");
    write_sample_cdg(&cdg_path);

    let result = import_songs_from_paths(&connection, &library, &[cdg_path.display().to_string()]);

    assert!(result.imported.is_empty());
    assert_eq!(result.failed.len(), 1);
    assert!(result.failed[0]
        .error
        .message
        .contains("does not have a matching audio track"));
}

#[test]
fn explicit_cdg_selection_pairs_only_the_chosen_audio_when_multiple_candidates_exist() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let (_tmp, library) = temp_library();
    let import_dir = tempfile::tempdir().expect("temp dir should create");
    let mp3_path = import_dir.path().join("paired.mp3");
    let m4a_path = import_dir.path().join("paired.m4a");
    let cdg_path = import_dir.path().join("paired.cdg");

    fs::copy(fixture_path("fixture.mp3"), &mp3_path).expect("fixture mp3 should copy");
    fs::copy(fixture_path("fixture.m4a"), &m4a_path).expect("fixture m4a should copy");
    write_sample_cdg(&cdg_path);

    let result = import_songs_from_paths_with_options(
        &connection,
        &library,
        &[
            mp3_path.display().to_string(),
            m4a_path.display().to_string(),
            cdg_path.display().to_string(),
        ],
        &ImportSongsOptions {
            explicit_cdg_by_audio_path: std::collections::HashMap::from([(
                m4a_path.display().to_string(),
                cdg_path.display().to_string(),
            )]),
            skip_cdg_for_audio_paths: vec![mp3_path.display().to_string()],
        },
    );

    assert_eq!(result.imported.len(), 2);

    let paired_song = result
        .imported
        .iter()
        .find(|song| song.file_path.ends_with(".m4a"))
        .expect("m4a song should import");
    assert_eq!(paired_song.media_g_container.as_deref(), Some("paired"));

    let plain_song = result
        .imported
        .iter()
        .find(|song| song.file_path.ends_with(".mp3"))
        .expect("mp3 song should import");
    assert_eq!(plain_song.media_g_container, None);
    assert_eq!(plain_song.cdg_path, None);
}
