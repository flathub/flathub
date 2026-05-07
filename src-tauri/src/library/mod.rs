use crate::commands::error::CommandError;
use serde::Serialize;

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct Song {
    pub hash: String,
    pub file_path: Option<String>,
    pub cdg_path: Option<String>,
    pub media_g_container: Option<String>,
    pub instrumental: bool,
    pub language: Option<String>,
    pub audio_source_kind: String,
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

    pub fn is_instrumental(&self) -> bool {
        self.instrumental
    }

    pub fn is_separable(&self) -> bool {
        !self.is_media_g() && !self.is_instrumental()
    }

    pub fn is_media_g_zip(&self) -> bool {
        self.media_g_container.as_deref() == Some("zip")
    }

    pub fn is_remote(&self) -> bool {
        self.audio_source_kind != "original"
    }

    pub fn is_remote_stems(&self) -> bool {
        self.audio_source_kind == "stems_remote"
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

#[cfg(test)]
mod tests {
    use super::Song;

    fn sample_song() -> Song {
        Song {
            hash: "song-1".to_owned(),
            file_path: Some("media/song-1.mp3".to_owned()),
            cdg_path: None,
            media_g_container: None,
            instrumental: false,
            language: None,
            audio_source_kind: "original".to_owned(),
            title: Some("Song".to_owned()),
            artist: None,
            album: None,
            duration_ms: 1_000,
            cover_art: None,
            imported_at: 1,
            original_ext: Some("mp3".to_owned()),
        }
    }

    #[test]
    fn separable_songs_must_be_plain_audio_and_not_instrumental() {
        let plain_audio = sample_song();
        assert!(plain_audio.is_separable());

        let mut instrumental = sample_song();
        instrumental.instrumental = true;
        assert!(!instrumental.is_separable());

        let mut media_g = sample_song();
        media_g.cdg_path = Some("media-g/song-1.cdg".to_owned());
        assert!(!media_g.is_separable());
    }
}
