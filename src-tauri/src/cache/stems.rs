use crate::config::StemMode;
use crate::library_root::LibraryRoot;
use crate::separator::{
    inference::{self, SeparationResult},
    mix,
};
use anyhow::{Context, Result};
use rusqlite::{params, Connection};
use std::{
    fs,
    path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
};

const STEMS_CACHE_DIRECTORY: &str = "stems";
const ACCOMPANIMENT_FILENAME: &str = "accompaniment.ogg";
const VOCALS_FILENAME: &str = "vocals.ogg";
const DRUMS_FILENAME: &str = "drums.ogg";
const BASS_FILENAME: &str = "bass.ogg";
const OTHER_FILENAME: &str = "other.ogg";

#[derive(Debug, Clone, PartialEq)]
pub struct StemCacheEntry {
    pub song_hash: String,
    pub vocals_path: String,
    pub accomp_path: String,
    pub separated_at: i64,
    pub drums_path: Option<String>,
    pub bass_path: Option<String>,
    pub other_path: Option<String>,
    pub model_variant: String,
}

impl StemCacheEntry {
    pub fn has_individual_stems(&self) -> bool {
        self.drums_path.is_some() && self.bass_path.is_some() && self.other_path.is_some()
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct StemCacheResult {
    pub entry: StemCacheEntry,
    pub cache_hit: bool,
    pub stem_directory: PathBuf,
}

pub fn stem_cache_root(stems_base: &Path) -> PathBuf {
    stems_base.to_path_buf()
}

pub fn stem_directory(stems_base: &Path, song_hash: &str) -> PathBuf {
    stems_base.join(song_hash)
}

pub fn get_or_create_stem_cache<F>(
    connection: &Connection,
    stems_base: &Path,
    library_root: &LibraryRoot,
    song_hash: &str,
    stem_mode: StemMode,
    model_variant: &str,
    generate: F,
) -> Result<StemCacheResult>
where
    F: FnOnce() -> Result<SeparationResult>,
{
    ensure_song_exists(connection, song_hash)?;

    if let Some(existing) = get_valid_cached_stem_entry(connection, library_root, song_hash)? {
        return Ok(existing);
    }

    let separation = generate().context("failed to generate stems for cache population")?;
    store_generated_stem_cache(
        connection,
        stems_base,
        song_hash,
        &separation,
        stem_mode,
        model_variant,
    )
}

pub fn list_all_stem_entries(connection: &Connection) -> rusqlite::Result<Vec<StemCacheEntry>> {
    let mut statement = connection.prepare(
        "SELECT song_hash, vocals_path, accomp_path, separated_at,
                drums_path, bass_path, other_path, model_variant
        FROM stems",
    )?;

    let entries = statement
        .query_map([], map_stem_row)?
        .collect::<rusqlite::Result<Vec<_>>>()?;

    Ok(entries)
}

pub fn get_cached_stem_entry(
    connection: &Connection,
    song_hash: &str,
) -> rusqlite::Result<Option<StemCacheEntry>> {
    let mut statement = connection.prepare(
        "SELECT song_hash, vocals_path, accomp_path, separated_at,
                drums_path, bass_path, other_path, model_variant
        FROM stems
        WHERE song_hash = ?1
        LIMIT 1",
    )?;

    let mut rows = statement.query([song_hash])?;
    match rows.next()? {
        Some(row) => Ok(Some(map_stem_row(row)?)),
        None => Ok(None),
    }
}

pub fn get_valid_cached_stem_entry(
    connection: &Connection,
    library_root: &LibraryRoot,
    song_hash: &str,
) -> Result<Option<StemCacheResult>> {
    let stem_directory = stem_directory(&library_root.stems_dir(), song_hash);

    if let Some(entry) = get_cached_stem_entry(connection, song_hash)? {
        if cache_entry_files_exist(library_root, &entry) {
            return Ok(Some(StemCacheResult {
                entry,
                cache_hit: true,
                stem_directory,
            }));
        }
    }

    Ok(None)
}

pub fn store_generated_stem_cache(
    connection: &Connection,
    stems_base: &Path,
    song_hash: &str,
    separation: &SeparationResult,
    stem_mode: StemMode,
    model_variant: &str,
) -> Result<StemCacheResult> {
    ensure_song_exists(connection, song_hash)?;
    let stem_directory = stem_directory(stems_base, song_hash);

    if stem_directory.exists() {
        fs::remove_dir_all(&stem_directory).with_context(|| {
            format!(
                "failed to clear stale stem cache directory at {}",
                stem_directory.display()
            )
        })?;
    }
    fs::create_dir_all(&stem_directory).with_context(|| {
        format!(
            "failed to create stem cache directory at {}",
            stem_directory.display()
        )
    })?;

    let entry = match stem_mode {
        StemMode::TwoStem => {
            // Cached stems intentionally stay in OGG/Vorbis so large libraries do
            // not balloon in size after separation jobs. The original imported
            // song remains in `media/`, so this cache favors compact playback.
            let vocals_stem = separation
                .stems
                .iter()
                .find(|s| s.name == "vocals")
                .context("separation result missing vocals stem")?;
            let vocals_path = stem_directory.join(VOCALS_FILENAME);
            crate::audio::encode::write_ogg_file(&vocals_path, &vocals_stem.audio)
                .context("failed to write vocals ogg into cache")?;

            let accompaniment = mix::mix_accompaniment(separation)
                .context("failed to mix accompaniment for stem cache")?;
            let accompaniment_path = stem_directory.join(ACCOMPANIMENT_FILENAME);
            mix::write_accompaniment_ogg(&accompaniment, &accompaniment_path)
                .context("failed to write accompaniment ogg into cache")?;

            StemCacheEntry {
                song_hash: song_hash.to_owned(),
                vocals_path: format!("{STEMS_CACHE_DIRECTORY}/{song_hash}/{VOCALS_FILENAME}"),
                accomp_path: format!(
                    "{STEMS_CACHE_DIRECTORY}/{song_hash}/{ACCOMPANIMENT_FILENAME}"
                ),
                separated_at: unix_timestamp(),
                drums_path: None,
                bass_path: None,
                other_path: None,
                model_variant: model_variant.to_owned(),
            }
        }
        StemMode::FourStem => {
            // Keep the four-stem cache in the same compact format for the same
            // reason: these files are reusable playback artifacts, not master exports.
            inference::write_stems_to_directory(separation, &stem_directory)
                .context("failed to write stem ogg files into cache")?;

            StemCacheEntry {
                song_hash: song_hash.to_owned(),
                vocals_path: format!("{STEMS_CACHE_DIRECTORY}/{song_hash}/{VOCALS_FILENAME}"),
                // `accomp_path` stays non-null because older cache/schema callers
                // assume the column is always present even when FourStem mode uses
                // the individual drums/bass/other files instead.
                accomp_path: String::new(),
                separated_at: unix_timestamp(),
                drums_path: Some(format!(
                    "{STEMS_CACHE_DIRECTORY}/{song_hash}/{DRUMS_FILENAME}"
                )),
                bass_path: Some(format!(
                    "{STEMS_CACHE_DIRECTORY}/{song_hash}/{BASS_FILENAME}"
                )),
                other_path: Some(format!(
                    "{STEMS_CACHE_DIRECTORY}/{song_hash}/{OTHER_FILENAME}"
                )),
                model_variant: model_variant.to_owned(),
            }
        }
    };

    upsert_stem_cache_entry(connection, &entry).context("failed to persist stem cache entry")?;

    Ok(StemCacheResult {
        entry,
        cache_hit: false,
        stem_directory,
    })
}

fn upsert_stem_cache_entry(
    connection: &Connection,
    entry: &StemCacheEntry,
) -> rusqlite::Result<()> {
    connection.execute(
        "INSERT INTO stems (
            song_hash,
            vocals_path,
            accomp_path,
            separated_at,
            drums_path,
            bass_path,
            other_path,
            model_variant
        ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)
        ON CONFLICT(song_hash) DO UPDATE SET
            vocals_path = excluded.vocals_path,
            accomp_path = excluded.accomp_path,
            separated_at = excluded.separated_at,
            drums_path = excluded.drums_path,
            bass_path = excluded.bass_path,
            other_path = excluded.other_path,
            model_variant = excluded.model_variant",
        params![
            entry.song_hash,
            entry.vocals_path,
            entry.accomp_path,
            entry.separated_at,
            entry.drums_path,
            entry.bass_path,
            entry.other_path,
            entry.model_variant,
        ],
    )?;

    Ok(())
}

pub fn cache_entry_files_valid(library_root: &LibraryRoot, entry: &StemCacheEntry) -> bool {
    cache_entry_files_exist(library_root, entry)
}

fn cache_entry_files_exist(library_root: &LibraryRoot, entry: &StemCacheEntry) -> bool {
    if !library_root.resolve(&entry.vocals_path).exists() {
        return false;
    }

    // An empty accomp_path means FourStem mode where no accompaniment file is written.
    if !entry.accomp_path.is_empty() && !library_root.resolve(&entry.accomp_path).exists() {
        return false;
    }

    // When individual stem paths are recorded, verify those files exist too.
    for path in [&entry.drums_path, &entry.bass_path, &entry.other_path]
        .into_iter()
        .flatten()
    {
        if !library_root.resolve(path).exists() {
            return false;
        }
    }

    true
}

pub fn delete_stem_cache_entry(
    connection: &Connection,
    library_root: &LibraryRoot,
    song_hash: &str,
) -> Result<()> {
    // Delete the row from the database.
    connection
        .execute("DELETE FROM stems WHERE song_hash = ?1", params![song_hash])
        .context("failed to delete stem cache entry from database")?;

    // Remove the stem files from disk.
    let dir = stem_directory(&library_root.stems_dir(), song_hash);
    if dir.exists() {
        fs::remove_dir_all(&dir).with_context(|| {
            format!("failed to remove stem cache directory at {}", dir.display())
        })?;
    }

    Ok(())
}

/// Delete all stem cache entries from the database and remove all stem files from disk.
/// Returns the number of deleted entries.
pub fn delete_all_stem_cache_entries(
    connection: &Connection,
    library_root: &LibraryRoot,
) -> Result<usize> {
    let count: usize = connection
        .query_row("SELECT COUNT(*) FROM stems", [], |row| row.get(0))
        .context("failed to count stem cache entries")?;

    connection
        .execute("DELETE FROM stems", [])
        .context("failed to delete all stem cache entries from database")?;

    // Remove the entire stems directory and recreate it empty.
    let stems_dir = library_root.stems_dir();
    if stems_dir.exists() {
        fs::remove_dir_all(&stems_dir).with_context(|| {
            format!(
                "failed to remove stems directory at {}",
                stems_dir.display()
            )
        })?;
    }
    fs::create_dir_all(&stems_dir).with_context(|| {
        format!(
            "failed to recreate stems directory at {}",
            stems_dir.display()
        )
    })?;

    Ok(count)
}

/// Estimate total disk usage of cached stem files in bytes.
pub fn estimate_stems_disk_usage(library_root: &LibraryRoot) -> Result<u64> {
    let stems_dir = library_root.stems_dir();
    if !stems_dir.exists() {
        return Ok(0);
    }
    dir_size(&stems_dir)
}

fn dir_size(path: &Path) -> Result<u64> {
    let mut total: u64 = 0;
    if path.is_dir() {
        for entry in fs::read_dir(path)
            .with_context(|| format!("failed to read directory {}", path.display()))?
        {
            let entry = entry
                .with_context(|| format!("failed to read directory entry in {}", path.display()))?;
            let entry_path = entry.path();
            if entry_path.is_dir() {
                total += dir_size(&entry_path)?;
            } else {
                total += entry
                    .metadata()
                    .with_context(|| {
                        format!("failed to read metadata for {}", entry_path.display())
                    })?
                    .len();
            }
        }
    }
    Ok(total)
}

fn ensure_song_exists(connection: &Connection, song_hash: &str) -> Result<()> {
    let exists: bool = connection
        .query_row(
            "SELECT EXISTS(SELECT 1 FROM songs WHERE hash = ?1)",
            [song_hash],
            |row| row.get(0),
        )
        .with_context(|| format!("failed to look up song {song_hash} before caching stems"))?;

    if !exists {
        anyhow::bail!("cannot cache stems for missing song {song_hash}");
    }

    Ok(())
}

/// Downgrade a single 4-stem entry to 2-stem by mixing drums+bass+other into an
/// accompaniment file and deleting the individual stem files.
/// Returns the updated `StemCacheEntry` and the number of bytes freed.
pub fn downgrade_to_two_stem(
    connection: &Connection,
    library_root: &LibraryRoot,
    song_hash: &str,
) -> Result<(StemCacheEntry, u64)> {
    let entry = get_cached_stem_entry(connection, song_hash)?
        .with_context(|| format!("no stem cache entry found for song {song_hash}"))?;

    anyhow::ensure!(
        entry.has_individual_stems(),
        "song {song_hash} does not have individual stems to downgrade"
    );

    let drums_rel = entry.drums_path.as_ref().unwrap();
    let bass_rel = entry.bass_path.as_ref().unwrap();
    let other_rel = entry.other_path.as_ref().unwrap();

    let drums_abs = library_root.resolve(drums_rel);
    let bass_abs = library_root.resolve(bass_rel);
    let other_abs = library_root.resolve(other_rel);

    // Decode each stem file.
    let drums_audio =
        crate::audio::decode::decode_file(&drums_abs).context("failed to decode drums.ogg")?;
    let bass_audio =
        crate::audio::decode::decode_file(&bass_abs).context("failed to decode bass.ogg")?;
    let other_audio =
        crate::audio::decode::decode_file(&other_abs).context("failed to decode other.ogg")?;

    // Mix by summing samples element-wise.
    let len = drums_audio.samples.len();
    let mut mixed_samples = Vec::with_capacity(len);
    for i in 0..len {
        let d = drums_audio.samples.get(i).copied().unwrap_or(0.0);
        let b = bass_audio.samples.get(i).copied().unwrap_or(0.0);
        let o = other_audio.samples.get(i).copied().unwrap_or(0.0);
        mixed_samples.push(d + b + o);
    }

    let mixed_audio = crate::audio::decode::DecodedAudio {
        sample_rate: drums_audio.sample_rate,
        channels: drums_audio.channels,
        duration_ms: drums_audio.duration_ms,
        samples: mixed_samples,
    };

    // Write accompaniment file.
    let accomp_rel = format!("{STEMS_CACHE_DIRECTORY}/{song_hash}/{ACCOMPANIMENT_FILENAME}");
    let accomp_abs = library_root.resolve(&accomp_rel);
    crate::audio::encode::write_ogg_file(&accomp_abs, &mixed_audio)
        .context("failed to write accompaniment.ogg")?;

    // Calculate freed bytes before deleting.
    let freed_bytes = file_size_or_zero(&drums_abs)
        + file_size_or_zero(&bass_abs)
        + file_size_or_zero(&other_abs);

    // Delete individual stem files.
    fs::remove_file(&drums_abs)
        .with_context(|| format!("failed to remove {}", drums_abs.display()))?;
    fs::remove_file(&bass_abs)
        .with_context(|| format!("failed to remove {}", bass_abs.display()))?;
    fs::remove_file(&other_abs)
        .with_context(|| format!("failed to remove {}", other_abs.display()))?;

    // Update the database row.
    connection
        .execute(
            "UPDATE stems SET accomp_path = ?2, drums_path = NULL, bass_path = NULL, other_path = NULL WHERE song_hash = ?1",
            params![song_hash, accomp_rel],
        )
        .context("failed to update stem cache entry for downgrade")?;

    let updated_entry = StemCacheEntry {
        song_hash: entry.song_hash,
        vocals_path: entry.vocals_path,
        accomp_path: accomp_rel,
        separated_at: entry.separated_at,
        drums_path: None,
        bass_path: None,
        other_path: None,
        model_variant: entry.model_variant,
    };

    Ok((updated_entry, freed_bytes))
}

/// Downgrade all 4-stem entries to 2-stem.
/// Returns (downgraded_count, total_freed_bytes).
pub fn batch_downgrade_to_two_stem(
    connection: &Connection,
    library_root: &LibraryRoot,
) -> Result<(usize, u64)> {
    let entries = list_all_stem_entries(connection)
        .context("failed to list stem entries for batch downgrade")?;

    let four_stem_hashes: Vec<String> = entries
        .into_iter()
        .filter(|e| e.has_individual_stems())
        .map(|e| e.song_hash)
        .collect();

    let mut count = 0usize;
    let mut total_freed = 0u64;

    for hash in &four_stem_hashes {
        let (_, freed) = downgrade_to_two_stem(connection, library_root, hash)?;
        count += 1;
        total_freed += freed;
    }

    Ok((count, total_freed))
}

/// Estimate the disk space that would be freed by downgrading all 4-stem entries to 2-stem.
pub fn estimate_downgrade_savings(
    connection: &Connection,
    library_root: &LibraryRoot,
) -> Result<u64> {
    let entries = list_all_stem_entries(connection)
        .context("failed to list stem entries for downgrade estimate")?;

    let mut total = 0u64;
    for entry in entries.iter().filter(|e| e.has_individual_stems()) {
        for rel_path in [&entry.drums_path, &entry.bass_path, &entry.other_path]
            .into_iter()
            .flatten()
        {
            total += file_size_or_zero(&library_root.resolve(rel_path));
        }
    }

    Ok(total)
}

fn file_size_or_zero(path: &Path) -> u64 {
    fs::metadata(path).map(|m| m.len()).unwrap_or(0)
}

fn map_stem_row(row: &rusqlite::Row<'_>) -> rusqlite::Result<StemCacheEntry> {
    Ok(StemCacheEntry {
        song_hash: row.get(0)?,
        vocals_path: row.get(1)?,
        accomp_path: row.get(2)?,
        separated_at: row.get(3)?,
        drums_path: row.get(4)?,
        bass_path: row.get(5)?,
        other_path: row.get(6)?,
        model_variant: row
            .get::<_, Option<String>>(7)?
            .unwrap_or_else(|| "htdemucs".to_owned()),
    })
}

fn unix_timestamp() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_secs() as i64
}
