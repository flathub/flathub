pub mod lyrics;
pub mod stems;

use crate::library::Song;
use anyhow::Context;
use rusqlite::{params, Connection, Row};
use std::{
    fs,
    path::{Path, PathBuf},
};
use tauri::Manager;

const DATABASE_FILENAME: &str = "openkara.sqlite3";
// Keep the SQL in the migrations directory so tests and runtime initialization
// execute the exact same schema definition.
const MIGRATIONS: [&str; 4] = [
    include_str!("../../migrations/001_init.sql"),
    include_str!("../../migrations/002_stems.sql"),
    include_str!("../../migrations/003_lyrics.sql"),
    include_str!("../../migrations/004_portable_paths.sql"),
];

fn database_path(base_dir: &Path) -> PathBuf {
    base_dir.join(DATABASE_FILENAME)
}

pub fn initialize_database(app_handle: &tauri::AppHandle) -> anyhow::Result<PathBuf> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .context("failed to resolve application data directory")?;

    fs::create_dir_all(&app_data_dir).with_context(|| {
        format!(
            "failed to create application data directory at {}",
            app_data_dir.display()
        )
    })?;

    let database_path = database_path(&app_data_dir);
    let connection = open_database(&database_path)?;

    apply_migrations(&connection).context("failed to apply SQLite migrations")?;

    Ok(database_path)
}

pub fn open_database(database_path: &Path) -> anyhow::Result<Connection> {
    Connection::open(database_path).with_context(|| {
        format!(
            "failed to open SQLite database at {}",
            database_path.display()
        )
    })
}

pub fn apply_migrations(connection: &Connection) -> rusqlite::Result<()> {
    for migration in MIGRATIONS {
        connection.execute_batch(migration)?;
    }

    // ALTER TABLE lacks IF NOT EXISTS in SQLite, so we check manually.
    if !column_exists(connection, "songs", "original_ext")? {
        connection.execute_batch("ALTER TABLE songs ADD COLUMN original_ext TEXT;")?;
    }
    if !column_exists(connection, "songs", "cdg_path")? {
        connection.execute_batch("ALTER TABLE songs ADD COLUMN cdg_path TEXT;")?;
    }
    if !column_exists(connection, "songs", "media_g_container")? {
        connection.execute_batch("ALTER TABLE songs ADD COLUMN media_g_container TEXT;")?;
    }
    if !column_exists(connection, "songs", "instrumental")? {
        connection.execute_batch(
            "ALTER TABLE songs ADD COLUMN instrumental INTEGER NOT NULL DEFAULT 0;",
        )?;
    }

    // 005_individual_stem_paths – add per-instrument columns to stems table.
    if !column_exists(connection, "stems", "drums_path")? {
        connection.execute_batch("ALTER TABLE stems ADD COLUMN drums_path TEXT;")?;
    }
    if !column_exists(connection, "stems", "bass_path")? {
        connection.execute_batch("ALTER TABLE stems ADD COLUMN bass_path TEXT;")?;
    }
    if !column_exists(connection, "stems", "other_path")? {
        connection.execute_batch("ALTER TABLE stems ADD COLUMN other_path TEXT;")?;
    }

    // 006_stem_model_variant – track which model produced each song's stems.
    if !column_exists(connection, "stems", "model_variant")? {
        connection
            .execute_batch("ALTER TABLE stems ADD COLUMN model_variant TEXT DEFAULT 'htdemucs';")?;
    }

    Ok(())
}

fn column_exists(connection: &Connection, table: &str, column: &str) -> rusqlite::Result<bool> {
    let sql = format!("PRAGMA table_info({})", table);
    let mut stmt = connection.prepare(&sql)?;
    let names: Vec<String> = stmt
        .query_map([], |row| row.get::<_, String>(1))?
        .collect::<rusqlite::Result<Vec<_>>>()?;
    Ok(names.iter().any(|name| name == column))
}

/// Initialize a database at an explicit path (for use inside a LibraryRoot).
pub fn initialize_library_database(database_path: &Path) -> anyhow::Result<()> {
    let connection = open_database(database_path)?;
    apply_migrations(&connection).context("failed to apply SQLite migrations")?;
    Ok(())
}

pub fn upsert_song(connection: &Connection, song: &Song) -> rusqlite::Result<()> {
    connection.execute(
        "INSERT INTO songs (
            hash,
            file_path,
            cdg_path,
            media_g_container,
            instrumental,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(hash) DO UPDATE SET
            file_path = excluded.file_path,
            cdg_path = excluded.cdg_path,
            media_g_container = excluded.media_g_container,
            instrumental = excluded.instrumental,
            title = excluded.title,
            artist = excluded.artist,
            album = excluded.album,
            duration_ms = excluded.duration_ms,
            cover_art = excluded.cover_art,
            imported_at = excluded.imported_at,
            original_ext = excluded.original_ext",
        params![
            song.hash,
            song.file_path,
            song.cdg_path,
            song.media_g_container,
            song.instrumental,
            song.title,
            song.artist,
            song.album,
            song.duration_ms,
            song.cover_art,
            song.imported_at,
            song.original_ext,
        ],
    )?;

    Ok(())
}

pub fn list_songs(connection: &Connection) -> rusqlite::Result<Vec<Song>> {
    let mut statement = connection.prepare(
        "SELECT
            hash,
            file_path,
            cdg_path,
            media_g_container,
            instrumental,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext
        FROM songs
        ORDER BY imported_at DESC, title COLLATE NOCASE ASC, hash ASC",
    )?;

    let songs = statement
        .query_map([], map_song_row)?
        .collect::<rusqlite::Result<Vec<_>>>()?;

    Ok(songs)
}

pub fn search_songs(connection: &Connection, query: &str) -> rusqlite::Result<Vec<Song>> {
    let pattern = format!("%{}%", query.to_lowercase());
    let mut statement = connection.prepare(
        "SELECT
            hash,
            file_path,
            cdg_path,
            media_g_container,
            instrumental,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext
        FROM songs
        WHERE lower(coalesce(title, '')) LIKE ?1
           OR lower(coalesce(artist, '')) LIKE ?1
           OR lower(coalesce(album, '')) LIKE ?1
           OR lower(file_path) LIKE ?1
        ORDER BY imported_at DESC, title COLLATE NOCASE ASC, hash ASC",
    )?;

    let songs = statement
        .query_map([pattern], map_song_row)?
        .collect::<rusqlite::Result<Vec<_>>>()?;

    Ok(songs)
}

pub fn get_song_by_hash(connection: &Connection, hash: &str) -> rusqlite::Result<Option<Song>> {
    let mut statement = connection.prepare(
        "SELECT
            hash,
            file_path,
            cdg_path,
            media_g_container,
            instrumental,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext
        FROM songs
        WHERE hash = ?1
        LIMIT 1",
    )?;

    let mut rows = statement.query([hash])?;
    match rows.next()? {
        Some(row) => Ok(Some(map_song_row(row)?)),
        None => Ok(None),
    }
}

pub fn update_song_title_artist(
    connection: &Connection,
    hash: &str,
    title: Option<&str>,
    artist: Option<&str>,
) -> rusqlite::Result<()> {
    connection.execute(
        "UPDATE songs SET title = ?, artist = ? WHERE hash = ?",
        params![title, artist, hash],
    )?;
    Ok(())
}

pub fn update_song_cover_art(
    connection: &Connection,
    hash: &str,
    cover_art: Option<&[u8]>,
) -> rusqlite::Result<()> {
    connection.execute(
        "UPDATE songs SET cover_art = ? WHERE hash = ?",
        params![cover_art, hash],
    )?;
    Ok(())
}

pub fn update_song_instrumental(
    connection: &Connection,
    hash: &str,
    instrumental: bool,
) -> rusqlite::Result<usize> {
    connection.execute(
        "UPDATE songs SET instrumental = ?1 WHERE hash = ?2",
        params![instrumental, hash],
    )
}

fn map_song_row(row: &Row<'_>) -> rusqlite::Result<Song> {
    Ok(Song {
        hash: row.get(0)?,
        file_path: row.get(1)?,
        cdg_path: row.get(2)?,
        media_g_container: row.get(3)?,
        instrumental: row.get(4)?,
        title: row.get(5)?,
        artist: row.get(6)?,
        album: row.get(7)?,
        duration_ms: row.get(8)?,
        cover_art: row.get(9)?,
        imported_at: row.get(10)?,
        original_ext: row.get(11)?,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn applies_migrations_and_creates_songs_table() {
        let connection = Connection::open_in_memory().expect("in-memory database should open");

        apply_migrations(&connection).expect("migrations should succeed");

        let songs_table_count: i64 = connection
            .query_row(
                "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'songs'",
                [],
                |row| row.get(0),
            )
            .expect("songs table lookup should succeed");

        assert_eq!(songs_table_count, 1);

        let stems_table_count: i64 = connection
            .query_row(
                "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'stems'",
                [],
                |row| row.get(0),
            )
            .expect("stems table lookup should succeed");

        assert_eq!(stems_table_count, 1);

        let instrumental_column_count: i64 = connection
            .query_row(
                "SELECT COUNT(*) FROM pragma_table_info('songs') WHERE name = 'instrumental'",
                [],
                |row| row.get(0),
            )
            .expect("instrumental column lookup should succeed");

        assert_eq!(instrumental_column_count, 1);

        let lyrics_table_count: i64 = connection
            .query_row(
                "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'lyrics'",
                [],
                |row| row.get(0),
            )
            .expect("lyrics table lookup should succeed");

        assert_eq!(lyrics_table_count, 1);
    }

    #[test]
    fn applies_migrations_idempotently() {
        let connection = Connection::open_in_memory().expect("in-memory database should open");

        apply_migrations(&connection).expect("first migration pass should succeed");
        apply_migrations(&connection).expect("second migration pass should also succeed");
    }
}
