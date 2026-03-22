use crate::lyrics::fetch::LyricsSource;
use anyhow::{anyhow, Context, Result};
use rusqlite::{params, Connection};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LyricsCacheEntry {
    pub song_hash: String,
    pub lrc: String,
    pub source: LyricsSource,
    pub offset_ms: i64,
    pub fetched_at: i64,
}

pub fn upsert_lyrics_cache_entry(
    connection: &Connection,
    entry: &LyricsCacheEntry,
) -> rusqlite::Result<()> {
    connection.execute(
        "INSERT INTO lyrics (
            song_hash,
            lrc,
            source,
            offset_ms,
            fetched_at
        ) VALUES (?1, ?2, ?3, ?4, ?5)
        ON CONFLICT(song_hash) DO UPDATE SET
            lrc = excluded.lrc,
            source = excluded.source,
            offset_ms = excluded.offset_ms,
            fetched_at = excluded.fetched_at",
        params![
            entry.song_hash,
            entry.lrc,
            serialize_source(&entry.source),
            entry.offset_ms,
            entry.fetched_at,
        ],
    )?;

    Ok(())
}

pub fn get_lyrics_cache_entry(
    connection: &Connection,
    song_hash: &str,
) -> Result<Option<LyricsCacheEntry>> {
    let mut statement = connection.prepare(
        "SELECT song_hash, lrc, source, offset_ms, fetched_at
        FROM lyrics
        WHERE song_hash = ?1
        LIMIT 1",
    )?;

    let mut rows = statement.query([song_hash])?;
    match rows.next()? {
        Some(row) => Ok(Some(LyricsCacheEntry {
            song_hash: row.get(0)?,
            lrc: row.get(1)?,
            source: deserialize_source(row.get::<_, String>(2)?.as_str())?,
            offset_ms: row.get(3)?,
            fetched_at: row.get(4)?,
        })),
        None => Ok(None),
    }
}

pub fn set_lyrics_offset(
    connection: &Connection,
    song_hash: &str,
    offset_ms: i64,
) -> rusqlite::Result<()> {
    connection.execute(
        "UPDATE lyrics
        SET offset_ms = ?2
        WHERE song_hash = ?1",
        params![song_hash, offset_ms],
    )?;

    Ok(())
}

/// Delete all lyrics cache entries from the database.
/// Returns the number of deleted entries.
pub fn delete_all_lyrics_cache_entries(connection: &Connection) -> rusqlite::Result<usize> {
    connection.execute("DELETE FROM lyrics", [])
}

fn serialize_source(source: &LyricsSource) -> &'static str {
    match source {
        LyricsSource::LrcLib => "lrclib",
        LyricsSource::LrcApi => "lrc_api",
        LyricsSource::Embedded => "embedded",
        LyricsSource::Sidecar => "sidecar",
        LyricsSource::Manual => "manual",
    }
}

fn deserialize_source(source: &str) -> Result<LyricsSource> {
    match source {
        "lrclib" => Ok(LyricsSource::LrcLib),
        "lrc_api" => Ok(LyricsSource::LrcApi),
        "embedded" => Ok(LyricsSource::Embedded),
        "sidecar" => Ok(LyricsSource::Sidecar),
        "manual" => Ok(LyricsSource::Manual),
        other => Err(anyhow!("unknown lyrics source {other}"))
            .with_context(|| format!("failed to deserialize lyrics source {source}")),
    }
}
