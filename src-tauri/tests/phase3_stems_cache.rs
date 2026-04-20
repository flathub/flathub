use std::{
    cell::Cell,
    fs,
    path::{Path, PathBuf},
};

mod support;

use lofty::{
    config::WriteOptions,
    file::{AudioFile, TaggedFileExt},
    picture::{MimeType, Picture, PictureType},
    prelude::Accessor,
    probe::Probe,
    tag::{Tag, TagType},
};
use openkara_lib::{
    audio::decode::DecodedAudio,
    cache::{self, stems},
    config::StemMode,
    library::Song,
    library_root::LibraryRoot,
    separator::inference::{SeparatedStem, SeparationResult},
};
use rusqlite::Connection;

fn metadata_fixture_path(filename: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("fixtures")
        .join("metadata")
        .join(filename)
}

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

fn sample_song(hash: &str, extension: &str) -> Song {
    Song {
        hash: hash.to_owned(),
        file_path: Some(format!("media/{hash}.{extension}")),
        cdg_path: None,
        media_g_container: None,
        instrumental: false,
        audio_source_kind: "original".to_owned(),
        title: Some("Fixture Song MP3".to_owned()),
        artist: Some("Fixture Artist".to_owned()),
        album: Some("Fixture Album".to_owned()),
        duration_ms: 1,
        cover_art: None,
        imported_at: 1,
        original_ext: Some(extension.to_owned()),
    }
}

fn copy_mp3_with_embedded_cover(destination: &Path) {
    fs::copy(metadata_fixture_path("fixture.mp3"), destination).expect("fixture audio should copy");

    let mut tagged_file = Probe::open(destination)
        .expect("fixture should open")
        .read()
        .expect("fixture tags should read");
    let mut tag = Tag::new(TagType::Id3v2);
    tag.set_title("Fixture Song MP3".to_owned());
    tag.set_artist("Fixture Artist".to_owned());
    tag.set_album("Fixture Album".to_owned());
    tag.push_picture(
        Picture::unchecked(vec![0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a])
            .pic_type(PictureType::CoverFront)
            .mime_type(MimeType::Png)
            .build(),
    );
    tagged_file.insert_tag(tag);
    tagged_file
        .save_to_path(destination, WriteOptions::default())
        .expect("fixture cover art should save");
}

fn tagged_song_in_library(library: &LibraryRoot, hash: &str) -> Song {
    let destination = library.media_path(hash, "mp3");
    copy_mp3_with_embedded_cover(&destination);
    sample_song(hash, "mp3")
}

fn read_tagged_title(path: &Path) -> String {
    let tagged_file = Probe::open(path)
        .expect("output should open")
        .read()
        .expect("output tags should read");

    tagged_file
        .primary_tag()
        .or_else(|| tagged_file.first_tag())
        .and_then(|tag| tag.title().map(|value| value.into_owned()))
        .expect("output title should exist")
}

fn assert_preserved_artist_album_and_cover(path: &Path) {
    let tagged_file = Probe::open(path)
        .expect("output should open")
        .read()
        .expect("output tags should read");
    let tag = tagged_file
        .primary_tag()
        .or_else(|| tagged_file.first_tag())
        .expect("output should contain a tag");

    assert_eq!(tag.artist().as_deref(), Some("Fixture Artist"));
    assert_eq!(tag.album().as_deref(), Some("Fixture Album"));
    assert_eq!(tag.pictures().len(), 1);
}

#[test]
fn caches_stems_under_hash_directory_and_hits_cache_on_second_request() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library_root();
    let library_root_path = library.root().to_owned();
    let song = tagged_song_in_library(&library, "song-hash");
    let source_audio_path = library.resolve(song.file_path.as_deref().unwrap());
    cache::upsert_song(&connection, &song).expect("song insert should succeed");
    let generation_count = Cell::new(0_usize);

    let first = stems::get_or_create_stem_cache(
        &connection,
        &library.stems_dir(),
        &library,
        &song,
        &source_audio_path,
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
        &song,
        &source_audio_path,
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

#[test]
fn two_stem_cache_preserves_metadata_and_overrides_titles() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library_root();
    let library_root_path = library.root().to_owned();
    let song = tagged_song_in_library(&library, "song-two-stem");
    let source_audio_path = library.resolve(song.file_path.as_deref().unwrap());
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let cached = stems::get_or_create_stem_cache(
        &connection,
        &library.stems_dir(),
        &library,
        &song,
        &source_audio_path,
        "song-two-stem",
        StemMode::TwoStem,
        "htdemucs",
        || Ok(sample_separation()),
    )
    .expect("two-stem cache should populate");

    let vocals_path = library.resolve(&cached.entry.vocals_path);
    let accompaniment_path = library.resolve(&cached.entry.accomp_path);

    assert_eq!(
        read_tagged_title(&vocals_path),
        "Fixture Song MP3 (Acapella)"
    );
    assert_eq!(
        read_tagged_title(&accompaniment_path),
        "Fixture Song MP3 (Instrumental)"
    );
    assert_preserved_artist_album_and_cover(&vocals_path);
    assert_preserved_artist_album_and_cover(&accompaniment_path);

    cleanup_dir(&library_root_path);
}

#[test]
fn four_stem_cache_writes_per_stem_titles() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library_root();
    let library_root_path = library.root().to_owned();
    let song = tagged_song_in_library(&library, "song-four-stem");
    let source_audio_path = library.resolve(song.file_path.as_deref().unwrap());
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    let cached = stems::get_or_create_stem_cache(
        &connection,
        &library.stems_dir(),
        &library,
        &song,
        &source_audio_path,
        "song-four-stem",
        StemMode::FourStem,
        "htdemucs",
        || Ok(sample_separation()),
    )
    .expect("four-stem cache should populate");

    assert_eq!(
        read_tagged_title(&library.resolve(&cached.entry.vocals_path)),
        "Fixture Song MP3 (Acapella)"
    );
    assert_eq!(
        read_tagged_title(&library.resolve(cached.entry.drums_path.as_ref().unwrap())),
        "Fixture Song MP3 (Drums)"
    );
    assert_eq!(
        read_tagged_title(&library.resolve(cached.entry.bass_path.as_ref().unwrap())),
        "Fixture Song MP3 (Bass)"
    );
    assert_eq!(
        read_tagged_title(&library.resolve(cached.entry.other_path.as_ref().unwrap())),
        "Fixture Song MP3 (Other)"
    );

    cleanup_dir(&library_root_path);
}

#[test]
fn downgrade_to_two_stem_rewrites_accompaniment_metadata() {
    let connection = Connection::open_in_memory().expect("in-memory database should open");
    cache::apply_migrations(&connection).expect("migrations should succeed");
    let library = unique_library_root();
    let library_root_path = library.root().to_owned();
    let song = tagged_song_in_library(&library, "song-downgrade");
    let source_audio_path = library.resolve(song.file_path.as_deref().unwrap());
    cache::upsert_song(&connection, &song).expect("song insert should succeed");

    stems::get_or_create_stem_cache(
        &connection,
        &library.stems_dir(),
        &library,
        &song,
        &source_audio_path,
        "song-downgrade",
        StemMode::FourStem,
        "htdemucs",
        || Ok(sample_separation()),
    )
    .expect("four-stem cache should populate");

    let (updated_entry, _) = stems::downgrade_to_two_stem(&connection, &library, "song-downgrade")
        .expect("downgrade should succeed");

    assert_eq!(
        read_tagged_title(&library.resolve(&updated_entry.accomp_path)),
        "Fixture Song MP3 (Instrumental)"
    );
    assert_preserved_artist_album_and_cover(&library.resolve(&updated_entry.accomp_path));

    cleanup_dir(&library_root_path);
}
