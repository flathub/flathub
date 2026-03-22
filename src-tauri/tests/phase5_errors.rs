use openkara_lib::commands::error::{
    library_error, lyrics_error, playback_error, separation_error, ErrorCode, FallbackAction,
};

#[test]
fn playback_errors_map_decode_failures_to_reimport_song_fallback() {
    let error = playback_error("failed to decode audio for /tmp/corrupt.mp3");

    assert_eq!(error.code, ErrorCode::AudioDecodeFailed);
    assert_eq!(error.fallback, FallbackAction::ReimportSong);
    assert!(!error.retryable);
}

#[test]
fn playback_errors_map_missing_stems_to_original_mode_fallback() {
    let error = playback_error("song with hash song-a does not have cached stems");

    assert_eq!(error.code, ErrorCode::KaraokeNotReady);
    assert_eq!(error.fallback, FallbackAction::StayInOriginalMode);
    assert!(error.retryable);
}

#[test]
fn lyrics_errors_map_missing_cache_to_empty_state_fallback() {
    let error = lyrics_error("song with hash song-a does not have cached lyrics");

    assert_eq!(error.code, ErrorCode::LyricsNotReady);
    assert_eq!(error.fallback, FallbackAction::ShowEmptyState);
    assert!(error.retryable);
}

#[test]
fn lyrics_errors_map_network_failures_to_retry_fallback() {
    let error = lyrics_error("failed to request timed lyrics");

    assert_eq!(error.code, ErrorCode::NetworkUnavailable);
    assert_eq!(error.fallback, FallbackAction::Retry);
    assert!(error.retryable);
}

#[test]
fn separation_errors_map_worker_failures_to_retry_fallback() {
    let error = separation_error("failed to separate stems for song song-a");

    assert_eq!(error.code, ErrorCode::SeparationFailed);
    assert_eq!(error.fallback, FallbackAction::Retry);
    assert!(error.retryable);
}

#[test]
fn library_errors_map_missing_media_to_reimport_fallback() {
    let error = library_error("failed to open audio file at /tmp/missing.mp3");

    assert_eq!(error.code, ErrorCode::MediaReadFailed);
    assert_eq!(error.fallback, FallbackAction::ReimportSong);
    assert!(!error.retryable);
}
