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
    if !column_exists(connection, "songs", "language")? {
        connection.execute_batch("ALTER TABLE songs ADD COLUMN language TEXT;")?;
    }
    if !column_exists(connection, "songs", "audio_source_kind")? {
        connection.execute_batch(include_str!("../../migrations/005_audio_source_kind.sql"))?;
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

    migrate_legacy_song_schema(connection)?;

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

fn column_is_not_null(
    connection: &Connection,
    table: &str,
    column: &str,
) -> rusqlite::Result<bool> {
    let sql = format!("PRAGMA table_info({})", table);
    let mut stmt = connection.prepare(&sql)?;
    let columns = stmt
        .query_map([], |row| {
            Ok((row.get::<_, String>(1)?, row.get::<_, i64>(3)?))
        })?
        .collect::<rusqlite::Result<Vec<_>>>()?;

    Ok(columns
        .into_iter()
        .any(|(name, not_null)| name == column && not_null != 0))
}

fn migrate_legacy_song_schema(connection: &Connection) -> rusqlite::Result<()> {
    let file_path_is_not_null = column_is_not_null(connection, "songs", "file_path")?;
    let has_audio_source_kind = column_exists(connection, "songs", "audio_source_kind")?;

    if !file_path_is_not_null && has_audio_source_kind {
        return Ok(());
    }

    connection.execute_batch(
        "
        PRAGMA foreign_keys = OFF;
        BEGIN;
        DROP TABLE IF EXISTS songs_new;
        CREATE TABLE songs_new (
            hash               TEXT PRIMARY KEY,
            file_path          TEXT,
            title              TEXT,
            artist             TEXT,
            album              TEXT,
            duration_ms        INTEGER,
            cover_art          BLOB,
            imported_at        INTEGER NOT NULL,
            original_ext       TEXT,
            cdg_path           TEXT,
            media_g_container  TEXT,
            instrumental       INTEGER NOT NULL DEFAULT 0,
            audio_source_kind  TEXT NOT NULL DEFAULT 'original'
        );
        INSERT INTO songs_new (
            hash,
            file_path,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext,
            cdg_path,
            media_g_container,
            instrumental,
            audio_source_kind
        )
        SELECT
            hash,
            file_path,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext,
            cdg_path,
            media_g_container,
            instrumental,
            COALESCE(audio_source_kind, 'original')
        FROM songs;
        DROP TABLE songs;
        ALTER TABLE songs_new RENAME TO songs;
        COMMIT;
        PRAGMA foreign_keys = ON;
        ",
    )?;

    Ok(())
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
            language,
            audio_source_kind,
            title,
            artist,
            album,
            duration_ms,
            cover_art,
            imported_at,
            original_ext
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(hash) DO UPDATE SET
            file_path = excluded.file_path,
            cdg_path = excluded.cdg_path,
            media_g_container = excluded.media_g_container,
            instrumental = excluded.instrumental,
            language = excluded.language,
            audio_source_kind = excluded.audio_source_kind,
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
            song.language,
            song.audio_source_kind,
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
            language,
            audio_source_kind,
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
            language,
            audio_source_kind,
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
           OR lower(coalesce(file_path, '')) LIKE ?1
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
            language,
            audio_source_kind,
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

pub fn update_song_language(
    connection: &Connection,
    hash: &str,
    language: Option<&str>,
) -> rusqlite::Result<usize> {
    connection.execute(
        "UPDATE songs SET language = ?1 WHERE hash = ?2",
        params![language, hash],
    )
}

fn map_song_row(row: &Row<'_>) -> rusqlite::Result<Song> {
    Ok(Song {
        hash: row.get(0)?,
        file_path: row.get(1)?,
        cdg_path: row.get(2)?,
        media_g_container: row.get(3)?,
        instrumental: row.get(4)?,
        language: row.get(5)?,
        audio_source_kind: row.get(6)?,
        title: row.get(7)?,
        artist: row.get(8)?,
        album: row.get(9)?,
        duration_ms: row.get(10)?,
        cover_art: row.get(11)?,
        imported_at: row.get(12)?,
        original_ext: row.get(13)?,
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

    #[test]
    fn migrates_legacy_song_schema_to_nullable_file_path_and_audio_source_kind() {
        let connection = Connection::open_in_memory().expect("in-memory database should open");

        connection
            .execute_batch(
                "
                CREATE TABLE songs (
                    hash        TEXT PRIMARY KEY,
                    file_path   TEXT NOT NULL,
                    title       TEXT,
                    artist      TEXT,
                    album       TEXT,
                    duration_ms INTEGER,
                    cover_art   BLOB,
                    imported_at INTEGER NOT NULL
                );
                INSERT INTO songs (
                    hash, file_path, title, artist, album, duration_ms, cover_art, imported_at
                ) VALUES (
                    'song-1',
                    'media/song-1.mp3',
                    'Song',
                    'Artist',
                    'Album',
                    1234,
                    X'',
                    1
                );
                ",
            )
            .expect("legacy schema should create");

        apply_migrations(&connection).expect("legacy schema migration should succeed");

        let file_path_nullable: i64 = connection
            .query_row(
                "SELECT COUNT(*) FROM pragma_table_info('songs') WHERE name = 'file_path' AND \"notnull\" = 0",
                [],
                |row| row.get(0),
            )
            .expect("file_path nullability lookup should succeed");
        assert_eq!(file_path_nullable, 1);

        let audio_source_kind_present: i64 = connection
            .query_row(
                "SELECT COUNT(*) FROM pragma_table_info('songs') WHERE name = 'audio_source_kind'",
                [],
                |row| row.get(0),
            )
            .expect("audio_source_kind lookup should succeed");
        assert_eq!(audio_source_kind_present, 1);

        let (file_path, audio_source_kind): (Option<String>, String) = connection
            .query_row(
                "SELECT file_path, audio_source_kind FROM songs WHERE hash = 'song-1'",
                [],
                |row| Ok((row.get(0)?, row.get(1)?)),
            )
            .expect("migrated song row should load");

        assert_eq!(file_path.as_deref(), Some("media/song-1.mp3"));
        assert_eq!(audio_source_kind, "original");
    }
}
