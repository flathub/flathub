use crate::{
    library::Song,
    lyrics::{
        lrcapi::LrcApiClient,
        lrclib::{LrcLibClient, LyricsLookupQuery},
        parser,
    },
    metadata,
};
use anyhow::{Context, Result};
use lofty::{file::TaggedFileExt, tag::ItemKey};
use serde::Serialize;
use std::{fs, path::Path};

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum LyricsSource {
    LrcLib,
    LrcApi,
    Embedded,
    Sidecar,
    Manual,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LyricsFetchResult {
    pub source: LyricsSource,
    pub raw_lrc: String,
}

#[derive(Debug, Clone, Copy)]
pub enum TimedLyricsProvider<'a> {
    LrcLib(&'a LrcLibClient),
    LrcApi(&'a LrcApiClient),
}

impl TimedLyricsProvider<'_> {
    fn source(self) -> LyricsSource {
        match self {
            Self::LrcLib(_) => LyricsSource::LrcLib,
            Self::LrcApi(_) => LyricsSource::LrcApi,
        }
    }

    fn fetch_timed_lrc(self, query: &LyricsLookupQuery) -> Result<Option<String>> {
        match self {
            Self::LrcLib(client) => client.fetch_by_track(query).map(|result| {
                result.and_then(|lyrics| {
                    lyrics
                        .synced_lyrics
                        .filter(|lyrics| !lyrics.trim().is_empty())
                })
            }),
            Self::LrcApi(client) => client.fetch_by_track(query).map(|result| {
                result.and_then(|lyrics| {
                    let lrc = lyrics.lrc.trim();
                    if lrc.is_empty() {
                        None
                    } else {
                        Some(lyrics.lrc)
                    }
                })
            }),
        }
    }
}

pub fn fetch_lyrics_for_song(
    providers: &[TimedLyricsProvider<'_>],
    song: &Song,
    resolved_audio_path: &Path,
) -> Result<Option<LyricsFetchResult>> {
    if let Some(query) = lookup_query_from_song(song) {
        if let Ok(Some(lyrics)) = fetch_online_timed_lyrics(providers, &query) {
            return Ok(Some(lyrics));
        }
    }

    if song.is_media_g_zip() {
        return Ok(None);
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

pub fn fetch_online_timed_lyrics(
    providers: &[TimedLyricsProvider<'_>],
    query: &LyricsLookupQuery,
) -> Result<Option<LyricsFetchResult>> {
    let mut last_error: Option<anyhow::Error> = None;

    for provider in providers {
        match (*provider).fetch_timed_lrc(query) {
            Ok(Some(raw_lrc)) => {
                if has_timed_lines(&raw_lrc) {
                    return Ok(Some(LyricsFetchResult {
                        source: (*provider).source(),
                        raw_lrc,
                    }));
                }
            }
            Ok(None) => {}
            Err(error) => {
                last_error = Some(error);
            }
        }
    }

    if let Some(error) = last_error {
        Err(error)
    } else {
        Ok(None)
    }
}

pub fn read_embedded_lyrics(path: &Path) -> Result<Option<String>> {
    let tagged_file = metadata::read_tagged_file_from_path(path).with_context(|| {
        format!(
            "failed to read embedded lyrics tags from {}",
            path.display()
        )
    })?;

    for tag in tagged_file.tags() {
        if let Some(lyrics) = tag.get_string(ItemKey::Lyrics) {
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

fn has_timed_lines(raw_lrc: &str) -> bool {
    parser::parse_lrc(raw_lrc)
        .map(|lines| !lines.is_empty())
        .unwrap_or(false)
}
