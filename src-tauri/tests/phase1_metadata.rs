use std::path::PathBuf;
use std::{fs, path::Path};

use openkara_lib::metadata;

fn fixture_path(filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join("metadata")
        .join(filename)
}

fn copy_fixture_to_temp(temp_dir: &Path, source_name: &str, target_name: &str) -> PathBuf {
    let source = fixture_path(source_name);
    let destination = temp_dir.join(target_name);
    fs::copy(source, &destination).expect("fixture should copy");
    destination
}

#[test]
fn reads_mp3_fixture_metadata() {
    let metadata = metadata::read_from_path(&fixture_path("fixture.mp3"))
        .expect("fixture mp3 metadata should parse");

    assert_eq!(metadata.title.as_deref(), Some("Fixture Song MP3"));
    assert_eq!(metadata.artist.as_deref(), Some("Fixture Artist"));
    assert_eq!(metadata.album.as_deref(), Some("Fixture Album"));
    assert!(metadata.duration_ms > 0);
}

#[test]
fn reads_flac_fixture_metadata() {
    let metadata = metadata::read_from_path(&fixture_path("fixture.flac"))
        .expect("fixture flac metadata should parse");

    assert_eq!(metadata.title.as_deref(), Some("Fixture Song FLAC"));
    assert_eq!(metadata.artist.as_deref(), Some("Fixture Artist"));
    assert_eq!(metadata.album.as_deref(), Some("Fixture Album"));
    assert!(metadata.duration_ms > 0);
}

#[test]
fn reads_m4a_fixture_metadata() {
    let metadata = metadata::read_from_path(&fixture_path("fixture.m4a"))
        .expect("fixture m4a metadata should parse");

    assert_eq!(metadata.title.as_deref(), Some("Fixture Song M4A"));
    assert_eq!(metadata.artist.as_deref(), Some("Fixture Artist"));
    assert_eq!(metadata.album.as_deref(), Some("Fixture Album"));
    assert!(metadata.duration_ms > 0);
}

#[test]
fn reads_mp4_audio_metadata_even_when_extension_is_aac() {
    let temp_dir = tempfile::tempdir().expect("temp dir should create");
    let aac_path = copy_fixture_to_temp(temp_dir.path(), "fixture.m4a", "fixture.aac");

    let metadata =
        metadata::read_from_path(&aac_path).expect("mp4 audio with .aac extension should parse");

    assert_eq!(metadata.title.as_deref(), Some("Fixture Song M4A"));
    assert_eq!(metadata.artist.as_deref(), Some("Fixture Artist"));
    assert_eq!(metadata.album.as_deref(), Some("Fixture Album"));
    assert!(metadata.duration_ms > 0);
}
