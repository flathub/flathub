use openkara_lib::audio::{
    decode::DecodedAudio,
    output::render_output_buffer,
    playback::{LoadedStems, PlaybackController, StemName, StemSet},
};

const TEST_SAMPLE_RATE: u32 = 44_100;
const TEST_CHANNELS: usize = 2;

fn decoded_audio(samples: Vec<f32>) -> DecodedAudio {
    DecodedAudio {
        sample_rate: 44_100,
        channels: 2,
        duration_ms: 10_000, // 10 seconds — long enough that the track won't auto-stop during the test
        samples,
    }
}

#[test]
fn render_output_mixes_stems_with_individual_volumes() {
    let mut controller = PlaybackController::default();
    controller.start_track(
        "song-a".into(),
        decoded_audio(vec![0.8, 0.7, -0.8, -0.7]),
        0,
    );

    let stems = LoadedStems::FourStem(StemSet {
        vocals: decoded_audio(vec![0.4, 0.3, -0.4, -0.3]),
        drums: decoded_audio(vec![0.2, 0.1, -0.2, -0.1]),
        bass: decoded_audio(vec![0.1, 0.1, -0.1, -0.1]),
        other: decoded_audio(vec![0.1, 0.1, -0.1, -0.1]),
    });
    controller
        .attach_stems("song-a", stems)
        .expect("stems should attach to the current song");

    assert!(controller.has_stems());

    // Mute vocals, keep everything else at 1.0
    controller
        .set_stem_volume(StemName::Vocals, 0.0)
        .expect("setting stem volume should succeed");

    let mut output = vec![0.0; 4];
    let rendered = render_output_buffer(
        &mut controller,
        0,
        &mut output,
        TEST_SAMPLE_RATE,
        TEST_CHANNELS,
    );

    assert_eq!(rendered, 4);
    // With vocals muted, output should be drums + bass + other
    // drums: [0.2, 0.1, -0.2, -0.1], bass: [0.1, 0.1, -0.1, -0.1], other: [0.1, 0.1, -0.1, -0.1]
    // sum:   [0.4, 0.3, -0.4, -0.3]
    for (actual, expected) in output.iter().zip([0.4_f32, 0.3, -0.4, -0.3].iter()) {
        assert!(
            (actual - expected).abs() < 1e-5,
            "expected {expected}, got {actual}"
        );
    }
}

#[test]
fn render_output_falls_back_to_original_when_no_stems() {
    let mut controller = PlaybackController::default();
    controller.start_track(
        "song-b".into(),
        decoded_audio(vec![0.5, 0.4, -0.5, -0.4]),
        0,
    );

    assert!(!controller.has_stems());

    let mut output = vec![0.0; 4];
    let rendered = render_output_buffer(
        &mut controller,
        0,
        &mut output,
        TEST_SAMPLE_RATE,
        TEST_CHANNELS,
    );

    assert_eq!(rendered, 4);
    for (actual, expected) in output.iter().zip([0.5_f32, 0.4, -0.5, -0.4].iter()) {
        assert!(
            (actual - expected).abs() < 1e-5,
            "expected {expected}, got {actual}"
        );
    }
}
