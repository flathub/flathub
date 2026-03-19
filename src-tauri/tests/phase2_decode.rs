use std::path::PathBuf;
#[cfg(target_os = "macos")]
use std::process::Command;

use openkara_lib::audio::decode;

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

#[test]
fn decodes_wav_fixture_to_expected_sample_shape() {
    let decoded =
        decode::decode_file(&fixture_path("audio", "fixture.wav")).expect("wav should decode");

    assert_eq!(decoded.sample_rate, 44_100);
    assert_eq!(decoded.channels, 2);
    assert_eq!(decoded.samples.len(), 44_100 * 2);
    assert!(decoded.duration_ms >= 999);
    assert!(decoded.duration_ms <= 1_001);
}

#[test]
fn decodes_phase_fixtures_across_supported_formats() {
    for path in [
        fixture_path("metadata", "fixture.mp3"),
        fixture_path("metadata", "fixture.flac"),
        fixture_path("metadata", "fixture.m4a"),
        fixture_path("audio", "fixture.ogg"),
    ] {
        let decoded = decode::decode_file(&path)
            .unwrap_or_else(|error| panic!("{} should decode: {error:#}", path.display()));

        assert_eq!(decoded.channels, 2, "{} channel count", path.display());
        assert_eq!(
            decoded.sample_rate,
            44_100,
            "{} sample rate",
            path.display()
        );
        assert!(
            !decoded.samples.is_empty(),
            "{} has samples",
            path.display()
        );
        assert!(decoded.duration_ms > 0, "{} has duration", path.display());
    }
}

#[test]
fn decodes_mp4_audio_even_when_extension_is_aac() {
    let temp_dir = tempfile::tempdir().expect("temp dir should create");
    let aac_path = temp_dir.path().join("fixture.aac");
    std::fs::copy(fixture_path("metadata", "fixture.m4a"), &aac_path)
        .expect("fixture m4a should copy");

    let decoded =
        decode::decode_file(&aac_path).expect("mp4 audio with .aac extension should decode");

    assert_eq!(decoded.channels, 2);
    assert_eq!(decoded.sample_rate, 44_100);
    assert!(!decoded.samples.is_empty());
    assert!(decoded.duration_ms > 0);
}

#[cfg(target_os = "macos")]
#[test]
fn decodes_generated_alac_m4a() {
    let temp_dir = tempfile::tempdir().expect("temp dir should create");
    let alac_path = temp_dir.path().join("fixture-alac.m4a");
    let output = Command::new("/usr/bin/afconvert")
        .args(["-f", "m4af", "-d", "alac"])
        .arg(fixture_path("audio", "fixture.wav"))
        .arg(&alac_path)
        .output()
        .expect("afconvert should launch");
    if !output.status.success() {
        eprintln!(
            "skipping ALAC generation regression because afconvert could not encode it: {}",
            String::from_utf8_lossy(&output.stderr)
        );
        return;
    }

    let decoded =
        decode::decode_file(&alac_path).expect("generated ALAC m4a should decode successfully");

    assert_eq!(decoded.channels, 2);
    assert_eq!(decoded.sample_rate, 44_100);
    assert!(!decoded.samples.is_empty());
    assert!(decoded.duration_ms > 0);
}
