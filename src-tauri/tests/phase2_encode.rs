use std::path::PathBuf;

use openkara_lib::audio::{decode, encode};
use tempfile::tempdir;

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

#[test]
fn write_ogg_file_round_trips_audio_metadata() {
    let decoded = decode::decode_file(&fixture_path("audio", "fixture.wav"))
        .expect("wav fixture should decode");
    let temp_dir = tempdir().expect("temporary directory should be created");
    let output_path = temp_dir.path().join("round-trip.ogg");

    encode::write_ogg_file(&output_path, &decoded).expect("ogg encoding should succeed");

    let redecoded =
        decode::decode_file(&output_path).expect("encoded ogg should decode successfully");

    assert_eq!(redecoded.sample_rate, decoded.sample_rate);
    assert_eq!(redecoded.channels, decoded.channels);
    assert!(!redecoded.samples.is_empty());
}
