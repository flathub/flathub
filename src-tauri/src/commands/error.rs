use serde::Serialize;

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum ErrorCode {
    DatabaseUnavailable,
    MediaReadFailed,
    SongNotFound,
    ModelUnavailable,
    AudioDecodeFailed,
    AudioOutputUnavailable,
    KaraokeNotReady,
    LyricsNotReady,
    NetworkUnavailable,
    InvalidPlaybackState,
    SeparationFailed,
    Internal,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum FallbackAction {
    Retry,
    RefreshLibrary,
    ReimportSong,
    CheckAudioOutputDevice,
    StayInOriginalMode,
    ShowEmptyState,
    KeepCurrentState,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct CommandError {
    pub code: ErrorCode,
    pub message: String,
    pub retryable: bool,
    pub fallback: FallbackAction,
}

pub type CommandResult<T> = std::result::Result<T, CommandError>;

impl CommandError {
    pub fn new(
        code: ErrorCode,
        message: impl Into<String>,
        retryable: bool,
        fallback: FallbackAction,
    ) -> Self {
        Self {
            code,
            message: message.into(),
            retryable,
            fallback,
        }
    }
}

pub fn database_error(message: impl ToString) -> CommandError {
    CommandError::new(
        ErrorCode::DatabaseUnavailable,
        message.to_string(),
        true,
        FallbackAction::Retry,
    )
}

pub fn library_error(message: impl ToString) -> CommandError {
    let message = message.to_string();

    if message.contains("failed to open audio file")
        || message.contains("failed to read audio metadata")
        || message.contains("failed to read audio file")
        || message.contains("failed to canonicalize path")
    {
        return CommandError::new(
            ErrorCode::MediaReadFailed,
            message,
            false,
            FallbackAction::ReimportSong,
        );
    }

    if message.contains("failed to open SQLite database") {
        return database_error(message);
    }

    internal_error(message)
}

pub fn state_lock_error(message: impl ToString) -> CommandError {
    CommandError::new(
        ErrorCode::Internal,
        message.to_string(),
        false,
        FallbackAction::KeepCurrentState,
    )
}

pub fn internal_error(message: impl ToString) -> CommandError {
    CommandError::new(
        ErrorCode::Internal,
        message.to_string(),
        true,
        FallbackAction::Retry,
    )
}

pub fn playback_error(message: impl ToString) -> CommandError {
    let message = message.to_string();

    if message.contains("was not found in the library") {
        return CommandError::new(
            ErrorCode::SongNotFound,
            message,
            false,
            FallbackAction::RefreshLibrary,
        );
    }

    if message.contains("does not have cached stems")
        || message.contains("karaoke audio is not loaded")
    {
        return CommandError::new(
            ErrorCode::KaraokeNotReady,
            message,
            true,
            FallbackAction::StayInOriginalMode,
        );
    }

    if message.contains("failed to decode audio")
        || message.contains("failed to open audio file")
        || message.contains("failed to probe audio format")
        || message.contains("audio container does not expose a default track")
        || message.contains("failed to create audio decoder")
        || message.contains("failed while reading audio packets")
    {
        return CommandError::new(
            ErrorCode::AudioDecodeFailed,
            message,
            false,
            FallbackAction::ReimportSong,
        );
    }

    if message.contains("no default output audio device is available")
        || message.contains("failed to read default audio output config")
        || message.contains("failed to start audio output stream")
    {
        return CommandError::new(
            ErrorCode::AudioOutputUnavailable,
            message,
            true,
            FallbackAction::CheckAudioOutputDevice,
        );
    }

    if message.contains("no track is loaded") {
        return CommandError::new(
            ErrorCode::InvalidPlaybackState,
            message,
            false,
            FallbackAction::KeepCurrentState,
        );
    }

    internal_error(message)
}

pub fn model_bootstrap_error(message: impl ToString) -> CommandError {
    CommandError::new(
        ErrorCode::ModelUnavailable,
        message.to_string(),
        true,
        FallbackAction::Retry,
    )
}

pub fn lyrics_error(message: impl ToString) -> CommandError {
    let message = message.to_string();

    if message.contains("was not found in the library") {
        return CommandError::new(
            ErrorCode::SongNotFound,
            message,
            false,
            FallbackAction::RefreshLibrary,
        );
    }

    if message.contains("does not have cached lyrics")
        || message.contains("failed to parse synced lyrics")
        || message.contains("failed to parse cached synced lyrics")
    {
        return CommandError::new(
            ErrorCode::LyricsNotReady,
            message,
            true,
            FallbackAction::ShowEmptyState,
        );
    }

    if message.contains("failed to request timed lyrics")
        || message.contains("timed lyrics provider returned a non-success response")
        || message.contains("failed to deserialize timed lyrics response")
        || message.contains("timed lyrics provider returned an unexpected response")
        || message.contains("failed to request lyrics from LRCLIB")
        || message.contains("LRCLIB returned a non-success response")
        || message.contains("failed to deserialize LRCLIB lyrics response")
    {
        return CommandError::new(
            ErrorCode::NetworkUnavailable,
            message,
            true,
            FallbackAction::Retry,
        );
    }

    if message.contains("failed to cache fetched lyrics")
        || message.contains("failed to persist lyrics offset")
    {
        return CommandError::new(
            ErrorCode::DatabaseUnavailable,
            message,
            true,
            FallbackAction::Retry,
        );
    }

    internal_error(message)
}

pub fn separation_error(message: impl ToString) -> CommandError {
    let message = message.to_string();

    if message.contains("was not found in the library") {
        return CommandError::new(
            ErrorCode::SongNotFound,
            message,
            false,
            FallbackAction::RefreshLibrary,
        );
    }

    if message.contains("failed to decode audio") {
        return CommandError::new(
            ErrorCode::AudioDecodeFailed,
            message,
            false,
            FallbackAction::ReimportSong,
        );
    }

    CommandError::new(
        ErrorCode::SeparationFailed,
        message,
        true,
        FallbackAction::Retry,
    )
}
