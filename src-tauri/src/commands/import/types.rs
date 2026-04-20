use crate::{commands::error::CommandError, library::Song};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize)]
pub struct SongProperties {
    pub format: String,
    pub sample_rate: Option<u32>,
    pub channels: Option<u16>,
    pub bit_rate: Option<u32>,
    pub file_size: u64,
    pub duration_ms: i64,
    pub hash: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct DeleteSongsFailure {
    pub song_id: String,
    pub error: CommandError,
}

#[derive(Debug, Clone, Serialize)]
pub struct DeleteSongsResult {
    pub deleted_song_ids: Vec<String>,
    pub failed: Vec<DeleteSongsFailure>,
}

#[derive(Debug, Clone, Serialize)]
pub struct ExtractEmbeddedCoverArtFailure {
    pub song_id: String,
    pub error: CommandError,
}

#[derive(Debug, Clone, Serialize)]
pub struct ExtractEmbeddedCoverArtResult {
    pub updated_songs: Vec<Song>,
    pub failed: Vec<ExtractEmbeddedCoverArtFailure>,
}

#[derive(Debug, Clone, Serialize)]
pub struct ImportCandidateDetails {
    pub path: String,
    pub format: String,
    pub bit_rate: Option<u32>,
    pub file_size: u64,
    pub duration_ms: Option<i64>,
}

#[derive(Debug, Clone, Serialize)]
pub struct ExpandedImportPaths {
    pub paths: Vec<String>,
    pub song_count: usize,
}

#[derive(Debug, Clone, Default, Deserialize, Serialize)]
pub struct ImportSongsOptions {
    #[serde(default)]
    pub explicit_cdg_by_audio_path: HashMap<String, String>,
    #[serde(default)]
    pub skip_cdg_for_audio_paths: Vec<String>,
}
