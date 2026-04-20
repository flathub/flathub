use std::{fs, path::PathBuf};

mod support;

use openkara_lib::{
    cache,
    commands::import::import_songs_from_paths,
    library_root::LibraryRoot,
    perf::{
        build_backend_performance_report, write_report_json, LYRICS_JITTER_THRESHOLD_MS,
        PLAYBACK_DB_LOOKUP_LATENCY_THRESHOLD_MS, PLAYBACK_FULL_DECODE_LATENCY_THRESHOLD_MS,
        PLAYBACK_METADATA_PROBE_LATENCY_THRESHOLD_MS,
        SEEK_LATENCY_THRESHOLD_MS,
    },
};
use rusqlite::Connection;
use serde_json;

fn fixture_path(directory: &str, filename: &str) -> String {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
        .display()
        .to_string()
}

fn unique_report_path() -> PathBuf {
    support::unique_temp_path("phase5-perf").with_extension("json")
}

#[test]
fn backend_performance_report_stays_within_phase5_thresholds() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let tmp = tempfile::tempdir().expect("temp dir should create");
    let library =
        LibraryRoot::create(tmp.path().join("lib").as_path()).expect("library should create");

    let import_result = import_songs_from_paths(
        &connection,
        &library,
        &[fixture_path("metadata", "fixture.mp3")],
    );
    assert_eq!(import_result.imported.len(), 1);
    let song_id = import_result.imported[0].hash.clone();

    let report = build_backend_performance_report(&connection, &library, &song_id, 128)
        .expect("performance report should generate");
    println!(
        "{}",
        serde_json::to_string_pretty(&report).expect("perf report should serialize")
    );

    assert!(
        report.playback.track_db_lookup_latency_ms < PLAYBACK_DB_LOOKUP_LATENCY_THRESHOLD_MS
    );
    assert!(
        report.playback.track_metadata_probe_latency_ms
            < PLAYBACK_METADATA_PROBE_LATENCY_THRESHOLD_MS
    );
    assert!(
        report.playback.track_full_decode_latency_ms < PLAYBACK_FULL_DECODE_LATENCY_THRESHOLD_MS
    );
    assert!(report.playback.seek_latency_max_ms < SEEK_LATENCY_THRESHOLD_MS);
    assert!(report.lyrics_sync.jitter_budget_ms < LYRICS_JITTER_THRESHOLD_MS);

    let report_path = unique_report_path();
    write_report_json(&report, &report_path).expect("report should write to disk");
    let contents = fs::read_to_string(&report_path).expect("report should be readable");
    assert!(contents.contains("\"lyrics_sync\""));
    assert!(contents.contains("\"playback\""));
    fs::remove_file(report_path).expect("temporary report should be removable");
}
