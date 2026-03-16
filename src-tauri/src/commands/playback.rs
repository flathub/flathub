use crate::{
    audio::{
        decode, output,
        playback::{
            monotonic_now_ms, playback_position_event, LoadedStems, PlaybackController,
            PlaybackStateSnapshot, StemName, StemSet, PLAYBACK_POSITION_EVENT,
        },
    },
    cache,
    commands::error::{
        database_error, internal_error, playback_error, state_lock_error, CommandResult,
    },
    library_root::LibraryRoot,
    AppState,
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use std::sync::{
    atomic::{AtomicU64, Ordering},
    Arc, Mutex,
};
use tauri::{AppHandle, Emitter, State};

#[tauri::command]
pub fn play(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    song_id: String,
) -> CommandResult<PlaybackStateSnapshot> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    let request_id = state.playback_request_id.fetch_add(1, Ordering::SeqCst) + 1;
    let song = cache::get_song_by_hash(&connection, &song_id)
        .map_err(database_error)?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))
        .map_err(playback_error)?;
    let absolute_path = library_root.resolve(&song.file_path);
    let snapshot = decode_then_start_track_if_latest(
        &state.playback,
        &state.playback_request_id,
        request_id,
        song.hash,
        || {
            decode::decode_file(&absolute_path)
                .with_context(|| format!("failed to decode audio for {}", song.file_path))
        },
    )
    .map_err(playback_error)?;

    output::ensure_output_thread(
        &state.audio_output_started,
        &state.audio_output_start_lock,
        state.playback.clone(),
    )
    .map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn resume(
    state: State<'_, AppState>,
    app_handle: AppHandle,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot = playback.play(monotonic_now_ms()).map_err(playback_error)?;
    drop(playback);

    output::ensure_output_thread(
        &state.audio_output_started,
        &state.audio_output_start_lock,
        state.playback.clone(),
    )
    .map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn pause(
    state: State<'_, AppState>,
    app_handle: AppHandle,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot = playback.pause(monotonic_now_ms()).map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn seek(
    state: State<'_, AppState>,
    app_handle: AppHandle,
    ms: u64,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;
    let snapshot = playback
        .seek(ms, monotonic_now_ms())
        .map_err(playback_error)?;

    emit_playback_position(&app_handle, &snapshot)
        .map_err(|error| internal_error(error.to_string()))?;

    Ok(snapshot)
}

#[tauri::command]
pub fn set_volume(state: State<'_, AppState>, level: f32) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    playback.set_volume(level).map_err(playback_error)
}

#[tauri::command]
pub fn set_stem_volume(
    state: State<'_, AppState>,
    stem: StemName,
    level: f32,
) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    playback
        .set_stem_volume(stem, level)
        .map_err(playback_error)
}

#[tauri::command]
pub fn load_stems(state: State<'_, AppState>) -> CommandResult<PlaybackStateSnapshot> {
    let library_root = state.library_root()?;
    let connection = cache::open_database(&library_root.database_path()).map_err(database_error)?;
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    let song_id = playback
        .current_song_id()
        .context("no track is loaded")
        .map_err(playback_error)?
        .to_owned();

    if playback.has_stems() {
        return Ok(playback.snapshot(monotonic_now_ms()));
    }

    drop(playback);

    let cached = cache::stems::get_cached_stem_entry(&connection, &song_id)
        .context("failed to load cached stems")
        .and_then(|entry| entry.with_context(|| format!("no cached stems for song {song_id}")))
        .map_err(playback_error)?;

    let load_stem = |path: &str| -> Result<decode::DecodedAudio> {
        let abs = library_root.resolve(path);
        decode::decode_file(&abs).with_context(|| format!("failed to decode stem {}", path))
    };

    let snapshot = decode_then_attach_stems_if_current_song(&state.playback, &song_id, || {
        if cached.has_individual_stems() {
            Ok(LoadedStems::FourStem(StemSet {
                vocals: load_stem(&cached.vocals_path)?,
                drums: load_stem(cached.drums_path.as_ref().unwrap())?,
                bass: load_stem(cached.bass_path.as_ref().unwrap())?,
                other: load_stem(cached.other_path.as_ref().unwrap())?,
            }))
        } else {
            Ok(LoadedStems::TwoStem {
                vocals: load_stem(&cached.vocals_path)?,
                accompaniment: load_stem(&cached.accomp_path)?,
            })
        }
    })
    .map_err(playback_error)?;

    Ok(snapshot)
}

fn decode_then_start_track_if_latest<F>(
    playback: &Arc<Mutex<PlaybackController>>,
    latest_request_id: &AtomicU64,
    request_id: u64,
    song_id: String,
    decode_audio: F,
) -> Result<PlaybackStateSnapshot>
where
    F: FnOnce() -> Result<decode::DecodedAudio>,
{
    let decoded_audio = decode_audio()?;
    let mut playback = playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;

    if latest_request_id.load(Ordering::SeqCst) != request_id {
        return Ok(playback.snapshot(monotonic_now_ms()));
    }

    Ok(playback.start_track(song_id, decoded_audio, monotonic_now_ms()))
}
fn decode_then_attach_stems_if_current_song<F>(
    playback: &Arc<Mutex<PlaybackController>>,
    song_id: &str,
    decode_stems: F,
) -> Result<PlaybackStateSnapshot>
where
    F: FnOnce() -> Result<LoadedStems>,
{
    let loaded_stems = decode_stems()?;
    let mut playback = playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;

    if playback.current_song_id() != Some(song_id) {
        return Ok(playback.snapshot(monotonic_now_ms()));
    }

    playback.attach_stems(song_id, loaded_stems)?;

    Ok(playback.snapshot(monotonic_now_ms()))
}

#[tauri::command]
pub fn get_playback_state(state: State<'_, AppState>) -> CommandResult<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| state_lock_error("playback controller lock was poisoned"))?;

    Ok(playback.snapshot(monotonic_now_ms()))
}

pub fn play_song_from_library(
    connection: &Connection,
    library_root: &LibraryRoot,
    controller: &mut PlaybackController,
    song_id: &str,
    now_ms: u64,
) -> Result<PlaybackStateSnapshot> {
    let song = cache::get_song_by_hash(connection, song_id)
        .context("failed to load song from library")?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;
    let absolute_path = library_root.resolve(&song.file_path);
    let decoded_audio = decode::decode_file(&absolute_path)
        .with_context(|| format!("failed to decode audio for {}", song.file_path))?;

    Ok(controller.start_track(song.hash, decoded_audio, now_ms))
}

pub fn load_stems_for_current_track(
    connection: &Connection,
    library_root: &LibraryRoot,
    controller: &mut PlaybackController,
) -> Result<()> {
    let song_id = controller
        .current_song_id()
        .context("no track is loaded")?
        .to_owned();

    if controller.has_stems() {
        return Ok(());
    }

    let cached = cache::stems::get_cached_stem_entry(connection, &song_id)
        .context("failed to load cached stems")?
        .with_context(|| format!("no cached stems for song {song_id}"))?;

    let load_stem = |path: &str| -> Result<decode::DecodedAudio> {
        let abs = library_root.resolve(path);
        decode::decode_file(&abs).with_context(|| format!("failed to decode stem {}", path))
    };

    // Load as 4-stem if individual stems are available, otherwise 2-stem
    let loaded = if cached.has_individual_stems() {
        LoadedStems::FourStem(StemSet {
            vocals: load_stem(&cached.vocals_path)?,
            drums: load_stem(cached.drums_path.as_ref().unwrap())?,
            bass: load_stem(cached.bass_path.as_ref().unwrap())?,
            other: load_stem(cached.other_path.as_ref().unwrap())?,
        })
    } else {
        LoadedStems::TwoStem {
            vocals: load_stem(&cached.vocals_path)?,
            accompaniment: load_stem(&cached.accomp_path)?,
        }
    };

    controller.attach_stems(&song_id, loaded)?;

    Ok(())
}

pub fn emit_playback_position(
    app_handle: &AppHandle,
    snapshot: &PlaybackStateSnapshot,
) -> tauri::Result<()> {
    if snapshot.song_id.is_none() {
        return Ok(());
    }

    app_handle.emit(
        PLAYBACK_POSITION_EVENT,
        playback_position_event(snapshot).map_err(|error| tauri::Error::Anyhow(error.into()))?,
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::{
        atomic::{AtomicU64, Ordering},
        mpsc, Arc, Mutex,
    };
    use std::time::Duration;

    fn dummy_audio() -> decode::DecodedAudio {
        decode::DecodedAudio {
            sample_rate: 44_100,
            channels: 2,
            duration_ms: 1_000,
            samples: vec![0.0; 44_100 * 2],
        }
    }

    fn dummy_stems() -> LoadedStems {
        LoadedStems::TwoStem {
            vocals: dummy_audio(),
            accompaniment: dummy_audio(),
        }
    }

    #[test]
    fn decodes_track_before_locking_playback_controller() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        let latest_request = AtomicU64::new(1);
        let (entered_tx, entered_rx) = mpsc::sync_channel(1);
        let (resume_tx, resume_rx) = mpsc::sync_channel(1);

        let worker_playback = Arc::clone(&playback);
        let handle = std::thread::spawn(move || {
            decode_then_start_track_if_latest(
                &worker_playback,
                &latest_request,
                1,
                "song-a".to_owned(),
                || {
                    entered_tx.send(()).unwrap();
                    resume_rx.recv().unwrap();
                    Ok(dummy_audio())
                },
            )
        });

        entered_rx.recv_timeout(Duration::from_secs(1)).unwrap();
        assert!(playback.try_lock().is_ok());

        resume_tx.send(()).unwrap();

        let snapshot = handle.join().unwrap().unwrap();
        assert_eq!(snapshot.song_id.as_deref(), Some("song-a"));
    }

    #[test]
    fn stale_play_request_does_not_replace_the_newer_track() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        let latest_request = AtomicU64::new(2);

        playback
            .lock()
            .unwrap()
            .start_track("song-b".to_owned(), dummy_audio(), 0);

        let snapshot = decode_then_start_track_if_latest(
            &playback,
            &latest_request,
            1,
            "song-a".to_owned(),
            || Ok(dummy_audio()),
        )
        .unwrap();

        assert_eq!(snapshot.song_id.as_deref(), Some("song-b"));
        assert_eq!(latest_request.load(Ordering::SeqCst), 2);
        assert_eq!(playback.lock().unwrap().current_song_id(), Some("song-b"));
    }

    #[test]
    fn track_start_time_is_set_after_decode_finishes() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        let latest_request = AtomicU64::new(1);

        decode_then_start_track_if_latest(
            &playback,
            &latest_request,
            1,
            "song-a".to_owned(),
            || {
                std::thread::sleep(Duration::from_millis(40));
                Ok(dummy_audio())
            },
        )
        .unwrap();

        let position_ms = playback
            .lock()
            .unwrap()
            .snapshot(monotonic_now_ms())
            .position_ms;
        assert!(
            position_ms < 20,
            "expected a fresh start time after decode, got {position_ms}ms"
        );
    }

    #[test]
    fn decodes_stems_before_locking_playback_controller() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        playback
            .lock()
            .unwrap()
            .start_track("song-a".to_owned(), dummy_audio(), 0);

        let (entered_tx, entered_rx) = mpsc::sync_channel(1);
        let (resume_tx, resume_rx) = mpsc::sync_channel(1);

        let worker_playback = Arc::clone(&playback);
        let handle = std::thread::spawn(move || {
            decode_then_attach_stems_if_current_song(&worker_playback, "song-a", || {
                entered_tx.send(()).unwrap();
                resume_rx.recv().unwrap();
                Ok(dummy_stems())
            })
        });

        entered_rx.recv_timeout(Duration::from_secs(1)).unwrap();
        assert!(playback.try_lock().is_ok());

        resume_tx.send(()).unwrap();

        let snapshot = handle.join().unwrap().unwrap();
        assert!(snapshot.has_stems);
    }

    #[test]
    fn stale_stem_decode_is_ignored_if_the_track_changed() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        playback
            .lock()
            .unwrap()
            .start_track("song-b".to_owned(), dummy_audio(), 0);

        let snapshot =
            decode_then_attach_stems_if_current_song(&playback, "song-a", || Ok(dummy_stems()))
                .unwrap();

        assert_eq!(snapshot.song_id.as_deref(), Some("song-b"));
        assert!(!snapshot.has_stems);
    }
}
