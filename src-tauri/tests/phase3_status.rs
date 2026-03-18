use openkara_lib::commands::error::{separation_error, ErrorCode, FallbackAction};
use openkara_lib::commands::separation::{
    completed_status, failed_status, idle_status, running_status, SeparationState,
};

#[test]
fn separation_status_helpers_cover_idle_running_completed_and_failed_states() {
    let idle = idle_status("song-a");
    assert_eq!(idle.song_id, "song-a");
    assert_eq!(idle.state, SeparationState::Idle);
    assert_eq!(idle.percent, 0);
    assert!(!idle.cache_hit);

    let running = running_status("song-a", 45);
    assert_eq!(running.state, SeparationState::Running);
    assert_eq!(running.percent, 45);

    let completed = completed_status(
        "song-a",
        "/tmp/vocals.ogg",
        "/tmp/accompaniment.ogg",
        true,
        None,
        None,
        None,
    );
    assert_eq!(completed.state, SeparationState::Completed);
    assert_eq!(completed.percent, 100);
    assert!(completed.cache_hit);
    assert_eq!(completed.vocals_path.as_deref(), Some("/tmp/vocals.ogg"));
    assert_eq!(
        completed.accomp_path.as_deref(),
        Some("/tmp/accompaniment.ogg")
    );

    let failed = failed_status("song-a", separation_error("boom"));
    assert_eq!(failed.state, SeparationState::Failed);
    let failed_error = failed
        .error
        .expect("failed status should carry a structured error");
    assert_eq!(failed_error.code, ErrorCode::SeparationFailed);
    assert_eq!(failed_error.fallback, FallbackAction::Retry);
    assert!(failed_error.retryable);
}
