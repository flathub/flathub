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
    store_generated_stem_cache(connection, stems_base, song_hash, &separation, stem_mode)
}

pub fn list_all_stem_entries(connection: &Connection) -> rusqlite::Result<Vec<StemCacheEntry>> {
    let mut statement = connection.prepare(
        "SELECT song_hash, vocals_path, accomp_path, separated_at,
                drums_path, bass_path, other_path
        FROM stems",
    )?;

    let entries = statement
        .query_map([], |row| {
            Ok(StemCacheEntry {
                song_hash: row.get(0)?,
                vocals_path: row.get(1)?,
                accomp_path: row.get(2)?,
                separated_at: row.get(3)?,
                drums_path: row.get(4)?,
                bass_path: row.get(5)?,
                other_path: row.get(6)?,
            })
        })?
        .collect::<rusqlite::Result<Vec<_>>>()?;

    Ok(entries)
}

pub fn get_cached_stem_entry(
    connection: &Connection,
    song_hash: &str,
) -> rusqlite::Result<Option<StemCacheEntry>> {
    let mut statement = connection.prepare(
        "SELECT song_hash, vocals_path, accomp_path, separated_at,
                drums_path, bass_path, other_path
        FROM stems
        WHERE song_hash = ?1
        LIMIT 1",
    )?;

    let mut rows = statement.query([song_hash])?;
    match rows.next()? {
        Some(row) => Ok(Some(StemCacheEntry {
            song_hash: row.get(0)?,
            vocals_path: row.get(1)?,
            accomp_path: row.get(2)?,
            separated_at: row.get(3)?,
            drums_path: row.get(4)?,
            bass_path: row.get(5)?,
            other_path: row.get(6)?,
        })),
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
            // Write only vocals.ogg and accompaniment.ogg
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
            }
        }
        StemMode::FourStem => {
            // Write all 4 individual stems (vocals, drums, bass, other)
            inference::write_stems_to_directory(separation, &stem_directory)
                .context("failed to write stem ogg files into cache")?;

            StemCacheEntry {
                song_hash: song_hash.to_owned(),
                vocals_path: format!("{STEMS_CACHE_DIRECTORY}/{song_hash}/{VOCALS_FILENAME}"),
                // accomp_path is NOT NULL in the DB schema; store empty string for FourStem mode
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
            other_path
        ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)
        ON CONFLICT(song_hash) DO UPDATE SET
            vocals_path = excluded.vocals_path,
            accomp_path = excluded.accomp_path,
            separated_at = excluded.separated_at,
            drums_path = excluded.drums_path,
            bass_path = excluded.bass_path,
            other_path = excluded.other_path",
        params![
            entry.song_hash,
            entry.vocals_path,
            entry.accomp_path,
            entry.separated_at,
            entry.drums_path,
            entry.bass_path,
            entry.other_path,
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
    for path in [&entry.drums_path, &entry.bass_path, &entry.other_path].into_iter().flatten() {
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
            format!(
                "failed to remove stem cache directory at {}",
                dir.display()
            )
        })?;
    }

    Ok(())
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

fn unix_timestamp() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_secs() as i64
}
