use std::{
    fs,
    path::{Path, PathBuf},
};

mod support;

use openkara_lib::smoke::{
    run_local_audio_smoke, LocalAudioSmokeConfig, SeparationSmokeMode, SmokeStepStatus,
};

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

fn unique_temp_dir(prefix: &str) -> PathBuf {
    support::unique_temp_path(prefix)
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary directory should be removable");
    }
}

#[test]
fn local_audio_smoke_scans_audio_files_and_writes_reports() {
    let input_dir = unique_temp_dir("phase6-local-smoke-input");
    let output_dir = unique_temp_dir("phase6-local-smoke-output");
    cleanup_dir(&input_dir);
    cleanup_dir(&output_dir);
    fs::create_dir_all(&input_dir).expect("input directory should create");

    fs::copy(
        fixture_path("metadata", "fixture.mp3"),
        input_dir.join("fixture.mp3"),
    )
    .expect("audio fixture should copy");
    fs::write(input_dir.join("notes.txt"), "skip me").expect("text fixture should write");

    let report = run_local_audio_smoke(LocalAudioSmokeConfig {
        input_dir: input_dir.clone(),
        output_dir: output_dir.clone(),
        separation_mode: SeparationSmokeMode::Disabled,
        seek_iterations: 8,
    })
    .expect("local audio smoke should succeed without separation");

    assert_eq!(report.summary.discovered_files, 1);
    assert_eq!(report.summary.imported, 1);
    assert_eq!(report.summary.playback_passed, 1);
    assert_eq!(report.summary.separation_skipped, 1);
    assert_eq!(report.songs.len(), 1);

    let song = &report.songs[0];
    assert_eq!(song.import_status, SmokeStepStatus::Passed);
    assert_eq!(song.playback_status, SmokeStepStatus::Passed);
    assert_eq!(song.separation_status, SmokeStepStatus::Skipped);
    assert!(song.performance.is_some());

    assert!(report.report_json_path.exists());
    assert!(report.report_markdown_path.exists());

    let json =
        fs::read_to_string(&report.report_json_path).expect("json report should be readable");
    assert!(json.contains("\"discovered_files\": 1"));
    let markdown = fs::read_to_string(&report.report_markdown_path)
        .expect("markdown report should be readable");
    assert!(markdown.contains("Local Audio Smoke Report"));
    assert!(markdown.contains("fixture.mp3"));

    cleanup_dir(&input_dir);
    cleanup_dir(&output_dir);
}

#[test]
fn local_audio_smoke_fails_when_no_supported_audio_files_exist() {
    let input_dir = unique_temp_dir("phase6-local-smoke-empty-input");
    let output_dir = unique_temp_dir("phase6-local-smoke-empty-output");
    cleanup_dir(&input_dir);
    cleanup_dir(&output_dir);
    fs::create_dir_all(&input_dir).expect("input directory should create");
    fs::write(input_dir.join("notes.txt"), "skip me").expect("text fixture should write");

    let error = run_local_audio_smoke(LocalAudioSmokeConfig {
        input_dir: input_dir.clone(),
        output_dir: output_dir.clone(),
        separation_mode: SeparationSmokeMode::Disabled,
        seek_iterations: 8,
    })
    .expect_err("smoke run should fail without supported audio files");

    assert!(error.to_string().contains("no supported audio files"));

    cleanup_dir(&input_dir);
    cleanup_dir(&output_dir);
}
