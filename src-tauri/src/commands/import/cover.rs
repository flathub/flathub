use crate::{
    cache,
    commands::error::{database_error, CommandError, ErrorCode, FallbackAction},
    library::Song,
    library_root::LibraryRoot,
    media_g::{self, MEDIA_G_ZIP},
    metadata,
};
use anyhow::{Context, Result};
use rusqlite::Connection;

use super::types::{ExtractEmbeddedCoverArtFailure, ExtractEmbeddedCoverArtResult};

pub(super) fn extract_embedded_cover_art_for_song(
    connection: &Connection,
    library: &LibraryRoot,
    song_id: &str,
) -> Result<Song, CommandError> {
    let song = cache::get_song_by_hash(connection, song_id)
        .map_err(|e| database_error(e.to_string()))?
        .ok_or_else(|| {
            CommandError::new(
                ErrorCode::SongNotFound,
                format!("song {song_id} not found"),
                false,
                FallbackAction::RefreshLibrary,
            )
        })?;

    let cover_art = read_embedded_cover_art(library, &song).map_err(|error| {
        let message = error.to_string();
        if message.contains("does not contain embedded cover art") {
            return CommandError::new(
                ErrorCode::MediaReadFailed,
                message,
                false,
                FallbackAction::KeepCurrentState,
            );
        }

        crate::commands::error::library_error(message)
    })?;

    cache::update_song_cover_art(connection, &song.hash, Some(&cover_art))
        .map_err(|e| database_error(e.to_string()))?;

    cache::get_song_by_hash(connection, &song.hash)
        .map_err(|e| database_error(e.to_string()))?
        .ok_or_else(|| {
            CommandError::new(
                ErrorCode::SongNotFound,
                format!("song {} not found after updating cover art", song.hash),
                false,
                FallbackAction::RefreshLibrary,
            )
        })
}

pub(super) fn read_embedded_cover_art(library: &LibraryRoot, song: &Song) -> Result<Vec<u8>> {
    let resolved_path = library.resolve(&song.file_path);

    let metadata = match song.media_g_container.as_deref() {
        Some(MEDIA_G_ZIP) => {
            let asset = media_g::inspect_zip_for_media_g(&resolved_path)?;
            metadata::read_from_bytes(&asset.audio_bytes, &asset.audio_extension)?
        }
        _ => metadata::read_from_path(&resolved_path)?,
    };

    metadata
        .cover_art
        .with_context(|| format!("song {} does not contain embedded cover art", song.hash))
}

pub fn extract_embedded_cover_art_from_connection(
    connection: &Connection,
    library: &LibraryRoot,
    song_ids: &[String],
) -> ExtractEmbeddedCoverArtResult {
    let mut updated_songs = Vec::new();
    let mut failed = Vec::new();

    for song_id in song_ids {
        match extract_embedded_cover_art_for_song(connection, library, song_id) {
            Ok(song) => updated_songs.push(song),
            Err(error) => failed.push(ExtractEmbeddedCoverArtFailure {
                song_id: song_id.clone(),
                error,
            }),
        }
    }

    ExtractEmbeddedCoverArtResult {
        updated_songs,
        failed,
    }
}
