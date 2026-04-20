use std::{
    fs,
    path::{Path, PathBuf},
};

mod support;

use openkara_lib::{
    cache,
    config::{ExecutionProviderPreference, StemMode},
    library::Song,
    library_root::LibraryRoot,
    separator::{job, model, model_cache::ModelCache},
};
use rusqlite::Connection;
use std::sync::{Arc, Mutex};

fn fixture_path(directory: &str, filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join(directory)
        .join(filename)
}

fn unique_library_root() -> LibraryRoot {
    let path = support::unique_temp_path("phase3-job");
    LibraryRoot::create(&path)
        .or_else(|_| LibraryRoot::open(&path))
        .expect("library root should be creatable")
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary cache directory should be removable");
    }
}

fn fixture_song(hash: &str, library: &LibraryRoot) -> Song {
    let src = fixture_path("audio", "fixture.wav");
    let relative = format!("media/{hash}.wav");
    let dest = library.resolve(&relative);
    fs::create_dir_all(dest.parent().unwrap()).ok();
    fs::copy(&src, &dest).expect("fixture audio should copy into library");
    Song {
        hash: hash.to_owned(),
        file_path: Some(relative),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        audio_source_kind: "original".to_owned(),
        title: Some("Fixture Song".to_owned()),
        artist: Some("Fixture Artist".to_owned()),
        album: Some("Fixture Album".to_owned()),
        duration_ms: 1_000,
        cover_art: None,
        imported_at: 1,
        original_ext: Some("wav".to_owned()),
    }
}

#[test]
fn separation_job_reports_monotonic_progress_and_hits_cache_on_second_run() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");

    let library = unique_library_root();
    let library_root_path = library.root().to_owned();
    cache::upsert_song(&connection, &fixture_song("fixture-song", &library))
        .expect("fixture song should insert");

    let model_path = model::default_model_path();
    let model_cache = Arc::new(Mutex::new(ModelCache::default()));

    let mut first_progress = Vec::new();
    let first = job::separate_song_into_cache(
        &connection,
        &library,
        &model_cache,
        &model_path,
        "fixture-song",
        StemMode::default(),
        "htdemucs",
        ExecutionProviderPreference::Cpu,
        |percent| first_progress.push(percent),
    )
    .expect("first separation should succeed");

    assert!(!first.cache_hit);
    assert_eq!(first_progress.last(), Some(&100));
    assert!(first_progress
        .windows(2)
        .all(|window| window[0] <= window[1]));
    assert!(library.resolve(&first.vocals_path).exists());
    assert!(library.resolve(&first.accomp_path).exists());

    let mut second_progress = Vec::new();
    let second = job::separate_song_into_cache(
        &connection,
        &library,
        &model_cache,
        &model_path,
        "fixture-song",
        StemMode::default(),
        "htdemucs",
        ExecutionProviderPreference::Cpu,
        |percent| second_progress.push(percent),
    )
    .expect("second separation should hit cache");

    assert!(second.cache_hit);
    assert_eq!(second_progress, vec![100]);
    assert_eq!(first.vocals_path, second.vocals_path);
    assert_eq!(first.accomp_path, second.accomp_path);

    cleanup_dir(&library_root_path);
}
