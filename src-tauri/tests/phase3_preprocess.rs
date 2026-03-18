use std::path::PathBuf;

use openkara_lib::{
    audio::decode,
    separator::{model, preprocess},
};

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

#[test]
fn preprocesses_stereo_audio_into_channels_first_model_tensor() {
    let loaded_model = model::load_from_path(
        &PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("models")
            .join("htdemucs.onnx"),
    )
    .expect("demucs model should load");
    let decoded = decode::decode_file(&fixture_path("audio", "fixture.wav"))
        .expect("wav fixture should decode");

    let prepared = preprocess::prepare_model_input(&loaded_model, &decoded)
        .expect("decoded audio should convert to model input");

    assert_eq!(prepared.shape, vec![1, 2, 343_980]);
    assert_eq!(prepared.samples.len(), 687_960);
    assert!(prepared.samples.iter().any(|sample| *sample != 0.0));

    assert!(prepared.samples[44_100..343_980]
        .iter()
        .all(|sample| *sample == 0.0));
    assert!(prepared.samples[388_080..]
        .iter()
        .all(|sample| *sample == 0.0));
}

#[test]
fn resamples_audio_with_a_non_demucs_sample_rate() {
    let mut decoded = decode::decode_file(&fixture_path("audio", "fixture.wav"))
        .expect("wav fixture should decode");
    let input_frame_count = decoded.samples.len() / decoded.channels;
    decoded.sample_rate = 48_000;
    decoded.duration_ms =
        ((input_frame_count as f64 / decoded.sample_rate as f64) * 1000.0).round() as u64;

    let normalized = preprocess::normalize_audio_for_model(&decoded)
        .expect("48k audio should be resampled for demucs");

    assert_eq!(normalized.sample_rate, preprocess::DEMUCS_SAMPLE_RATE);
    assert_eq!(normalized.channels, preprocess::DEMUCS_CHANNELS);

    let expected_frames = ((input_frame_count as f64 * preprocess::DEMUCS_SAMPLE_RATE as f64)
        / 48_000.0)
        .round() as usize;
    let actual_frames = normalized.samples.len() / normalized.channels;
    assert!(
        actual_frames.abs_diff(expected_frames) <= 2,
        "expected about {expected_frames} frames after resampling, got {actual_frames}",
    );
}

#[test]
fn prepares_model_input_from_pre_normalized_audio_without_re_normalizing() {
    let loaded_model = model::load_from_path(
        &PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("models")
            .join("htdemucs.onnx"),
    )
    .expect("demucs model should load");
    let decoded = decode::decode_file(&fixture_path("audio", "fixture.wav"))
        .expect("wav fixture should decode");
    let normalized = preprocess::normalize_audio_for_model(&decoded)
        .expect("fixture should normalize for demucs");

    let prepared = preprocess::prepare_model_input_from_normalized(&loaded_model, &normalized)
        .expect("normalized audio should convert directly to model input");
    let wrapped = preprocess::prepare_model_input(&loaded_model, &decoded)
        .expect("wrapper should preserve existing behavior");

    assert_eq!(prepared.shape, wrapped.shape);
    assert_eq!(prepared.samples, wrapped.samples);
}
