use crate::commands::error::CommandError;
use serde::Serialize;

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct Song {
    pub hash: String,
    pub file_path: String,
    pub cdg_path: Option<String>,
    pub media_g_container: Option<String>,
    pub title: Option<String>,
    pub artist: Option<String>,
    pub album: Option<String>,
    pub duration_ms: i64,
    pub cover_art: Option<Vec<u8>>,
    pub imported_at: i64,
    pub original_ext: Option<String>,
}

impl Song {
    pub fn is_media_g(&self) -> bool {
        self.media_g_container.is_some() || self.cdg_path.is_some()
    }

    pub fn is_media_g_zip(&self) -> bool {
        self.media_g_container.as_deref() == Some("zip")
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ImportFailure {
    pub path: String,
    pub error: CommandError,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct ImportSongsResult {
    pub imported: Vec<Song>,
    pub failed: Vec<ImportFailure>,
}
