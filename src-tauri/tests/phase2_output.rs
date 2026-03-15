use std::path::PathBuf;

use openkara_lib::audio::{decode, output::render_output_buffer, playback::PlaybackController};

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

// Use a common device sample rate and channel count for tests
const TEST_SAMPLE_RATE: u32 = 44_100;
const TEST_CHANNELS: usize = 2;

#[test]
fn render_output_buffer_returns_silence_without_an_active_track() {
    let mut controller = PlaybackController::default();
    let mut output = vec![1.0; 128];

    let rendered_samples =
        render_output_buffer(&mut controller, 0, &mut output, TEST_SAMPLE_RATE, TEST_CHANNELS);

    assert_eq!(rendered_samples, 0);
    assert!(output.iter().all(|sample| *sample == 0.0));
}

#[test]
fn render_output_buffer_writes_audio_when_playing_and_silence_when_paused() {
    let decoded = decode::decode_file(&fixture_path("audio", "fixture.wav")).unwrap();
    let mut controller = PlaybackController::default();
    controller.start_track("song-a".into(), decoded, 0);

    let mut playing_output = vec![0.0; 256];
    let rendered_samples = render_output_buffer(
        &mut controller,
        0,
        &mut playing_output,
        TEST_SAMPLE_RATE,
        TEST_CHANNELS,
    );
    assert!(rendered_samples > 0);
    assert!(playing_output.iter().any(|sample| *sample != 0.0));

    controller.pause(100).expect("pause should succeed");

    let mut paused_output = vec![1.0; 256];
    let rendered_after_pause = render_output_buffer(
        &mut controller,
        100,
        &mut paused_output,
        TEST_SAMPLE_RATE,
        TEST_CHANNELS,
    );
    assert_eq!(rendered_after_pause, 0);
    assert!(paused_output.iter().all(|sample| *sample == 0.0));
}
