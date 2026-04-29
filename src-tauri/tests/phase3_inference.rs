use std::{
    fs,
    path::{Path, PathBuf},
};

mod support;

use openkara_lib::{
    audio::decode,
    config::ExecutionProviderPreference,
    separator::{inference, model},
};

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

fn unique_output_dir() -> PathBuf {
    support::unique_temp_path("phase3-inference")
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary output directory should be removable");
    }
}

#[test]
fn separates_fixture_audio_into_named_stems_and_writes_wavs() {
    let mut loaded_model = model::load_from_path(
        &PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("models")
            .join("htdemucs.onnx"),
        ExecutionProviderPreference::Cpu
    )
    .expect("demucs model should load");
    let decoded = decode::decode_file(&fixture_path("audio", "fixture.wav"))
        .expect("wav fixture should decode");

    let separated =
        inference::separate_audio(&mut loaded_model, &decoded, |_, _| {}, None, "fixture")
        .expect("fixture audio should separate into stems");

    assert_eq!(separated.stems.len(), 4);
    assert_eq!(
        separated
            .stems
            .iter()
            .map(|stem| stem.name.as_str())
            .collect::<Vec<_>>(),
        vec!["drums", "bass", "other", "vocals"]
    );
    assert!(separated
        .stems
        .iter()
        .all(|stem| stem.audio.sample_rate == decoded.sample_rate));
    assert!(separated
        .stems
        .iter()
        .all(|stem| stem.audio.channels == decoded.channels));
    assert!(separated
        .stems
        .iter()
        .all(|stem| stem.audio.samples.len() == decoded.samples.len()));

    let output_dir = unique_output_dir();
    cleanup_dir(&output_dir);

    let written_paths = inference::write_stems_to_directory(&separated, &output_dir)
        .expect("stem ogg files should be written");

    assert_eq!(written_paths.len(), 4);
    for stem_name in ["drums", "bass", "other", "vocals"] {
        let stem_path = output_dir.join(format!("{stem_name}.ogg"));
        assert!(stem_path.exists(), "{} should exist", stem_path.display());
    }

    cleanup_dir(&output_dir);
}

#[test]
fn separates_audio_longer_than_a_single_demucs_window() {
    let mut loaded_model = model::load_from_path(
        &PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("models")
            .join("htdemucs.onnx"),
        ExecutionProviderPreference::Cpu
    )
    .expect("demucs model should load");
    let fixture = decode::decode_file(&fixture_path("audio", "fixture.wav"))
        .expect("wav fixture should decode");

    let mut long_audio = fixture.clone();
    long_audio.samples = fixture.samples.repeat(8);

    let separated =
        inference::separate_audio(&mut loaded_model, &long_audio, |_, _| {}, None, "fixture-long")
        .expect("audio longer than one model window should separate");

    assert_eq!(separated.stems.len(), 4);
    assert!(separated
        .stems
        .iter()
        .all(|stem| stem.audio.samples.len() == long_audio.samples.len()));
}
