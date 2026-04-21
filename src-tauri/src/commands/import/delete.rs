use crate::{
    cache,
    library_root::LibraryRoot,
    media_g::{MEDIA_G_PAIRED, MEDIA_G_ZIP},
};
use anyhow::{Context, Result};
use rusqlite::{params, Connection};
use std::fs;

pub(crate) fn delete_song_from_library(
    connection: &Connection,
    library: &LibraryRoot,
    song_id: &str,
) -> Result<()> {
    let song = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;

    if let Some(container) = song.media_g_container.as_deref() {
        match container {
            MEDIA_G_PAIRED => {
                if let Some(relative_path) = song.file_path.as_deref() {
                    delete_relative_file(library, relative_path)?;
                }
                if let Some(cdg_path) = song.cdg_path.as_deref() {
                    delete_relative_file(library, cdg_path)?;
                }
            }
            MEDIA_G_ZIP => {
                if let Some(relative_path) = song.file_path.as_deref() {
                    delete_relative_file(library, relative_path)?;
                }
            }
            _ => {}
        }
    } else {
        if let Some(relative_path) = song.file_path.as_deref() {
            delete_relative_file(library, relative_path)?;
        }
    }

    cache::stems::delete_stem_cache_entry(connection, library, song_id).ok();
    connection
        .execute("DELETE FROM lyrics WHERE song_hash = ?1", params![song_id])
        .context("failed to delete cached lyrics for song")?;
    if table_exists(connection, "play_history")? {
        connection
            .execute(
                "DELETE FROM play_history WHERE song_hash = ?1",
                params![song_id],
            )
            .context("failed to delete play history for song")?;
    }
    connection
        .execute("DELETE FROM songs WHERE hash = ?1", params![song_id])
        .context("failed to delete song row from database")?;

    Ok(())
}

fn delete_relative_file(library: &LibraryRoot, relative_path: &str) -> Result<()> {
    let absolute = library.resolve(relative_path);
    if absolute.exists() {
        fs::remove_file(&absolute)
            .with_context(|| format!("failed to remove {}", absolute.display()))?;
    }
    Ok(())
}

fn table_exists(connection: &Connection, table: &str) -> Result<bool> {
    let count: i64 = connection
        .query_row(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = ?1",
            [table],
            |row| row.get(0),
        )
        .context("failed to inspect sqlite tables")?;
    Ok(count > 0)
}
