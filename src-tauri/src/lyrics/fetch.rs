use crate::{
    library::Song,
    lyrics::lrclib::{LrcLibClient, LyricsLookupQuery},
};
use anyhow::{Context, Result};
use lofty::{file::TaggedFileExt, read_from_path, tag::ItemKey};
use serde::Serialize;
use std::{fs, path::Path};

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum LyricsSource {
    LrcLib,
    Embedded,
    Sidecar,
    Manual,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LyricsFetchResult {
    pub source: LyricsSource,
    pub raw_lrc: String,
}

pub fn fetch_lyrics_for_song(
    client: &LrcLibClient,
    song: &Song,
    resolved_audio_path: &Path,
) -> Result<Option<LyricsFetchResult>> {
    if let Some(query) = lookup_query_from_song(song) {
        if let Some(lyrics) = client.fetch_by_track(&query)? {
            if let Some(synced_lyrics) = lyrics
                .synced_lyrics
                .filter(|lyrics| !lyrics.trim().is_empty())
            {
                return Ok(Some(LyricsFetchResult {
                    source: LyricsSource::LrcLib,
                    raw_lrc: synced_lyrics,
                }));
            }
        }
    }

    if let Some(embedded_lyrics) = read_embedded_lyrics(resolved_audio_path)? {
        return Ok(Some(LyricsFetchResult {
            source: LyricsSource::Embedded,
            raw_lrc: embedded_lyrics,
        }));
    }

    if let Some(sidecar_lyrics) = read_sidecar_lrc(resolved_audio_path)? {
        return Ok(Some(LyricsFetchResult {
            source: LyricsSource::Sidecar,
            raw_lrc: sidecar_lyrics,
        }));
    }

    Ok(None)
}

pub fn lookup_query_from_song(song: &Song) -> Option<LyricsLookupQuery> {
    Some(LyricsLookupQuery {
        track_name: song.title.clone()?,
        artist_name: song.artist.clone()?,
        album_name: song.album.clone(),
        duration_seconds: Some((song.duration_ms / 1_000).max(0) as u64),
    })
}

pub fn read_embedded_lyrics(path: &Path) -> Result<Option<String>> {
    let tagged_file = read_from_path(path).with_context(|| {
        format!(
            "failed to read embedded lyrics tags from {}",
            path.display()
        )
    })?;

    for tag in tagged_file.tags() {
        if let Some(lyrics) = tag.get_string(&ItemKey::Lyrics) {
            let lyrics = lyrics.trim();
            if !lyrics.is_empty() {
                return Ok(Some(lyrics.to_owned()));
            }
        }
    }

    Ok(None)
}

fn read_sidecar_lrc(path: &Path) -> Result<Option<String>> {
    let sidecar_path = path.with_extension("lrc");
    if !sidecar_path.exists() {
        return Ok(None);
    }

    let contents = fs::read_to_string(&sidecar_path).with_context(|| {
        format!(
            "failed to read sidecar lyrics from {}",
            sidecar_path.display()
        )
    })?;
    let contents = contents.trim().to_owned();
    if contents.is_empty() {
        return Ok(None);
    }

    Ok(Some(contents))
}
