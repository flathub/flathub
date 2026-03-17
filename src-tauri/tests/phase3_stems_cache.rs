use std::{
    cell::Cell,
    fs,
    path::Path,
};

mod support;

use openkara_lib::{
    audio::decode::DecodedAudio,
    cache::{self, stems},
    config::StemMode,
    library::Song,
    library_root::LibraryRoot,
    separator::inference::{SeparatedStem, SeparationResult},
};
use rusqlite::Connection;

fn unique_library_root() -> LibraryRoot {
    let path = support::unique_temp_path("phase3-cache");
    LibraryRoot::create(&path)
        .or_else(|_| LibraryRoot::open(&path))
        .expect("library root should be creatable")
}

fn cleanup_dir(path: &Path) {
    if path.exists() {
        fs::remove_dir_all(path).expect("temporary cache directory should be removable");
    }
}

fn sample_separation() -> SeparationResult {
    let make_stem = |name: &str, sample: f32| SeparatedStem {
        name: name.to_owned(),
        audio: DecodedAudio {
            sample_rate: 44_100,
            channels: 2,
            duration_ms: 1,
            samples: vec![sample, sample, -sample, -sample],
        },
    };

    SeparationResult {
        stems: vec![
            make_stem("drums", 0.2),
            make_stem("bass", 0.3),
            make_stem("other", 0.1),
            make_stem("vocals", 0.4),
        ],
    }
}

fn sample_song(hash: &str) -> Song {
    Song {
        hash: hash.to_owned(),
        file_path: format!("/music/{hash}.wav"),
        cdg_path: None,
        media_g_container: None,
        title: Some("Fixture Song".to_owned()),
        artist: Some("Fixture Artist".to_owned()),
        album: Some("Fixture Album".to_owned()),
        duration_ms: 1,
        cover_art: None,
        imported_at: 1,
        original_ext: None,
    }
}

#[test]
fn caches_stems_under_hash_directory_and_hits_cache_on_second_request() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    cache::upsert_song(&connection, &sample_song("song-hash")).expect("song insert should succeed");

    let library = unique_library_root();
    let library_root_path = library.root().to_owned();
    let generation_count = Cell::new(0_usize);

    let first = stems::get_or_create_stem_cache(
        &connection,
        &library.stems_dir(),
        &library,
        "song-hash",
        StemMode::TwoStem,
        "htdemucs",
        || {
            generation_count.set(generation_count.get() + 1);
            Ok(sample_separation())
        },
    )
    .expect("first separation should populate cache");

    assert!(!first.cache_hit);
    assert_eq!(generation_count.get(), 1);
    assert!(library
        .stems_dir()
        .join("song-hash")
        .join("vocals.ogg")
        .exists());
    assert!(library
        .stems_dir()
        .join("song-hash")
        .join("accompaniment.ogg")
        .exists());

    let second = stems::get_or_create_stem_cache(
        &connection,
        &library.stems_dir(),
        &library,
        "song-hash",
        StemMode::TwoStem,
        "htdemucs",
        || {
            generation_count.set(generation_count.get() + 1);
            Ok(sample_separation())
        },
    )
    .expect("second separation should hit cache");

    assert!(second.cache_hit);
    assert_eq!(generation_count.get(), 1);

    let cached_entry = stems::get_cached_stem_entry(&connection, "song-hash")
        .expect("cache lookup should succeed")
        .expect("cache entry should exist");
    assert!(library.resolve(&cached_entry.vocals_path).exists());
    assert!(library.resolve(&cached_entry.accomp_path).exists());

    cleanup_dir(&library_root_path);
}
