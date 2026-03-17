use rusqlite::{params, Connection};
use std::time::{SystemTime, UNIX_EPOCH};

pub fn record_play(connection: &Connection, song_hash: &str) -> rusqlite::Result<()> {
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs() as i64;

    connection.execute(
        "INSERT INTO play_history (song_hash, played_at) VALUES (?1, ?2)",
        params![song_hash, now],
    )?;
    Ok(())
}

pub fn get_recent_plays(connection: &Connection, limit: u32) -> rusqlite::Result<Vec<String>> {
    let mut stmt = connection.prepare(
        "SELECT song_hash FROM play_history
         GROUP BY song_hash
         ORDER BY MAX(played_at) DESC
         LIMIT ?1",
    )?;

    let hashes = stmt
        .query_map([limit], |row| row.get::<_, String>(0))?
        .collect::<rusqlite::Result<Vec<_>>>()?;

    Ok(hashes)
}
