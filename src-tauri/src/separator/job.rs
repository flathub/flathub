use crate::{
    audio::decode,
    cache,
    config::{ExecutionProviderPreference, StemMode},
    library_root::LibraryRoot,
    separator::{checkpoint, inference, model, model_cache::ModelCache},
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use std::{
    path::Path,
    sync::{Arc, Mutex},
};

const CACHE_HIT_PROGRESS: u8 = 100;
const LOOKUP_PROGRESS: u8 = 2;
const DECODE_PROGRESS: u8 = 5;
const MODEL_LOAD_PROGRESS: u8 = 10;
const CACHE_WRITE_PROGRESS: u8 = 95;
const COMPLETE_PROGRESS: u8 = 100;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SeparationArtifacts {
    pub vocals_path: String,
    pub accomp_path: String,
    pub cache_hit: bool,
    pub drums_path: Option<String>,
    pub bass_path: Option<String>,
    pub other_path: Option<String>,
}

pub fn separate_song_into_cache(
    connection: &Connection,
    library_root: &LibraryRoot,
    model_cache: &Arc<Mutex<ModelCache<model::LoadedModel>>>,
    model_path: &Path,
    song_hash: &str,
    stem_mode: StemMode,
    model_variant: &str,
    ep_preference: ExecutionProviderPreference,
    mut report_progress: impl FnMut(u8),
) -> Result<SeparationArtifacts> {
    if let Some(cached) =
        cache::stems::get_valid_cached_stem_entry(connection, library_root, song_hash)?
    {
        // Verify the cached entry matches the requested stem mode AND model variant.
        let variant_matches = cached.entry.model_variant == model_variant;
        let mode_matches = match stem_mode {
            StemMode::TwoStem => true,
            StemMode::FourStem => cached.entry.has_individual_stems(),
        };
        if mode_matches && variant_matches {
            report_progress(CACHE_HIT_PROGRESS);
            return Ok(artifacts_from_cache_entry(cached.entry, true));
        }
    }

    report_progress(LOOKUP_PROGRESS);
    let song = cache::get_song_by_hash(connection, song_hash)
        .context("failed to load song before stem separation")?
        .with_context(|| format!("song with hash {song_hash} was not found in the library"))?;

    report_progress(DECODE_PROGRESS);
    let absolute_path = library_root.resolve(&song.file_path);
    let decoded_audio = decode::decode_file(&absolute_path)
        .with_context(|| format!("failed to decode audio for {}", song.file_path))?;

    report_progress(MODEL_LOAD_PROGRESS);
    let mut model_cache = model_cache
        .lock()
        .map_err(|_| anyhow::anyhow!("separator model cache lock was poisoned"))?;
    let loaded_model = model_cache.get_or_load_with(model_path, |path| {
        model::load_from_path(path, ep_preference)
            .with_context(|| format!("failed to load Demucs model from {}", path.display()))
    })?;

    let checkpoint_dir = checkpoint::checkpoint_dir(&library_root.stems_dir(), song_hash);
    let inference_progress = |completed: usize, total: usize| {
        if total > 0 {
            let fraction = completed as f64 / total as f64;
            let percent = MODEL_LOAD_PROGRESS as f64
                + fraction * (CACHE_WRITE_PROGRESS as f64 - MODEL_LOAD_PROGRESS as f64);
            report_progress(percent.round() as u8);
        }
    };
    let separation = inference::separate_audio(
        loaded_model,
        &decoded_audio,
        inference_progress,
        Some(checkpoint_dir.as_path()),
    )
    .with_context(|| format!("failed to separate stems for song {song_hash}"))?;

    report_progress(CACHE_WRITE_PROGRESS);
    let stems_base = library_root.stems_dir();
    let cached = cache::stems::store_generated_stem_cache(
        connection,
        &stems_base,
        &song,
        &absolute_path,
        song_hash,
        &separation,
        stem_mode,
        model_variant,
    )
    .with_context(|| format!("failed to cache generated stems for song {song_hash}"))?;

    let _ = checkpoint::cleanup(&checkpoint_dir);

    report_progress(COMPLETE_PROGRESS);
    Ok(artifacts_from_cache_entry(cached.entry, false))
}

fn artifacts_from_cache_entry(
    entry: cache::stems::StemCacheEntry,
    cache_hit: bool,
) -> SeparationArtifacts {
    SeparationArtifacts {
        vocals_path: entry.vocals_path,
        accomp_path: entry.accomp_path,
        cache_hit,
        drums_path: entry.drums_path,
        bass_path: entry.bass_path,
        other_path: entry.other_path,
    }
}
