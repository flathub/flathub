CREATE TABLE IF NOT EXISTS songs (
    hash        TEXT PRIMARY KEY,
    file_path   TEXT,
    title       TEXT,
    artist      TEXT,
    album       TEXT,
    duration_ms INTEGER,
    cover_art   BLOB,
    imported_at INTEGER NOT NULL
);
