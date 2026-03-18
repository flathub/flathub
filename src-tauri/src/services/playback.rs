use crate::{
    audio::{
        decode, output,
        playback::{
            monotonic_now_ms, playback_position_event, LoadedStems, PlaybackController,
            PlaybackStateSnapshot, StemName, StemSet, PLAYBACK_POSITION_EVENT,
        },
    },
    cache,
    library_root::LibraryRoot,
    media_g::{self, MEDIA_G_ZIP},
    services::cdg::{load_cdg_state_for_song, mark_cdg_reset_for_seek},
    AppState,
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use std::sync::{
    atomic::{AtomicU64, Ordering},
    Arc, Mutex,
};
use tauri::{AppHandle, Emitter, Runtime};

pub fn play<R: Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: &str,
) -> Result<PlaybackStateSnapshot> {
    let library_root = state
        .library_root()
        .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    let connection = cache::open_database(&library_root.database_path())?;
    let request_id = state.playback_request_id.fetch_add(1, Ordering::SeqCst) + 1;
    let song = cache::get_song_by_hash(&connection, song_id)?
        .with_context(|| format!("song with hash {song_id} was not found in the library"))?;
    let active_song_id = song.hash.clone();
    let snapshot = decode_then_start_track_if_latest(
        &state.playback,
        &state.playback_request_id,
        request_id,
        active_song_id.clone(),
        || load_song_audio(&library_root, &song),
    )?;

    if snapshot.song_id.as_deref() == Some(active_song_id.as_str()) {
        // Only attach CDG state if this play request still won. Slow decode or
        // sidecar work from an older request must not clobber the current song.
        let next_cdg_state = load_cdg_state_for_song(&library_root, &song);
        let mut cdg_state = state
            .cdg_state
            .lock()
            .map_err(|_| anyhow::anyhow!("CDG state lock was poisoned"))?;
        *cdg_state = next_cdg_state;
    }

    ensure_output_thread(state)?;
    emit_playback_position(app_handle, &snapshot)?;

    Ok(snapshot)
}

pub fn resume<R: Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
) -> Result<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;
    let snapshot = playback.play(monotonic_now_ms())?;
    drop(playback);

    ensure_output_thread(state)?;
    emit_playback_position(app_handle, &snapshot)?;

    Ok(snapshot)
}

pub fn pause<R: Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
) -> Result<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;
    let snapshot = playback.pause(monotonic_now_ms())?;
    emit_playback_position(app_handle, &snapshot)?;
    Ok(snapshot)
}

pub fn seek<R: Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    ms: u64,
) -> Result<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;
    let previous_position_ms = playback.snapshot(monotonic_now_ms()).position_ms;
    let snapshot = playback.seek(ms, monotonic_now_ms())?;
    drop(playback);

    let mut cdg_state = state
        .cdg_state
        .lock()
        .map_err(|_| anyhow::anyhow!("CDG state lock was poisoned"))?;
    mark_cdg_reset_for_seek(&mut cdg_state, previous_position_ms, snapshot.position_ms);
    drop(cdg_state);

    emit_playback_position(app_handle, &snapshot)?;
    Ok(snapshot)
}

pub fn set_volume(state: &AppState, level: f32) -> Result<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;
    Ok(playback.set_volume(level)?)
}

pub fn set_stem_volume(
    state: &AppState,
    stem: StemName,
    level: f32,
) -> Result<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;
    Ok(playback.set_stem_volume(stem, level)?)
}

pub fn load_stems(state: &AppState) -> Result<PlaybackStateSnapshot> {
    let library_root = state
        .library_root()
        .map_err(|error| anyhow::anyhow!(error.message.clone()))?;
    let connection = cache::open_database(&library_root.database_path())?;
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;

    let song_id = playback
        .current_song_id()
        .context("no track is loaded")?
        .to_owned();

    if playback.has_stems() {
        return Ok(playback.snapshot(monotonic_now_ms()));
    }

    drop(playback);

    let cached = cache::stems::get_cached_stem_entry(&connection, &song_id)
        .context("failed to load cached stems")?
        .with_context(|| format!("no cached stems for song {song_id}"))?;

    let load_stem = |path: &str| -> Result<decode::DecodedAudio> {
        let abs = library_root.resolve(path);
        decode::decode_file(&abs).with_context(|| format!("failed to decode stem {}", path))
    };

    decode_then_attach_stems_if_current_song(&state.playback, &song_id, || {
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
}

pub fn get_state(state: &AppState) -> Result<PlaybackStateSnapshot> {
    let mut playback = state
        .playback
        .lock()
        .map_err(|_| anyhow::anyhow!("playback controller lock was poisoned"))?;
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
    let decoded_audio = load_song_audio(library_root, &song)?;
    Ok(controller.start_track(song.hash, decoded_audio, now_ms))
}

fn ensure_output_thread(state: &AppState) -> Result<()> {
    output::ensure_output_thread(
        &state.audio_output_started,
        &state.audio_output_start_lock,
        state.playback.clone(),
    )?;
    Ok(())
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
    // Decode before taking the playback lock so expensive file IO does not stall
    // output/control paths, then apply a latest-request-wins guard before swap-in.
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

fn load_song_audio(
    library_root: &LibraryRoot,
    song: &crate::library::Song,
) -> Result<decode::DecodedAudio> {
    let absolute_path = library_root.resolve(&song.file_path);
    if song.media_g_container.as_deref() == Some(MEDIA_G_ZIP) {
        let asset = media_g::inspect_zip_for_media_g(&absolute_path)?;
        return decode::decode_bytes(asset.audio_bytes, &asset.audio_extension)
            .with_context(|| format!("failed to decode audio for {}", song.file_path));
    }

    decode::decode_file(&absolute_path)
        .with_context(|| format!("failed to decode audio for {}", song.file_path))
}

pub fn emit_playback_position<R: Runtime>(
    app_handle: &AppHandle<R>,
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
    use std::{
        sync::mpsc,
        time::Duration,
    };

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
                    entered_tx.send(()).expect("decode hook should notify test");
                    resume_rx.recv().expect("test should resume decode");
                    Ok(dummy_audio())
                },
            )
        });

        entered_rx
            .recv_timeout(Duration::from_secs(1))
            .expect("decode hook should enter");
        assert!(playback.try_lock().is_ok());

        resume_tx.send(()).expect("decode should be released");

        let snapshot = handle
            .join()
            .expect("worker thread should join")
            .expect("playback decode should succeed");
        assert_eq!(snapshot.song_id.as_deref(), Some("song-a"));
    }

    #[test]
    fn stale_play_request_does_not_replace_the_newer_track() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        let latest_request = AtomicU64::new(2);

        playback
            .lock()
            .expect("playback lock should succeed")
            .start_track("song-b".to_owned(), dummy_audio(), 0);

        let snapshot = decode_then_start_track_if_latest(
            &playback,
            &latest_request,
            1,
            "song-a".to_owned(),
            || Ok(dummy_audio()),
        )
        .expect("stale play request should still return a snapshot");

        assert_eq!(snapshot.song_id.as_deref(), Some("song-b"));
        assert_eq!(latest_request.load(Ordering::SeqCst), 2);
        assert_eq!(
            playback
                .lock()
                .expect("playback lock should succeed")
                .current_song_id(),
            Some("song-b")
        );
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
        .expect("playback decode should succeed");

        let position_ms = playback
            .lock()
            .expect("playback lock should succeed")
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
            .expect("playback lock should succeed")
            .start_track("song-a".to_owned(), dummy_audio(), 0);

        let (entered_tx, entered_rx) = mpsc::sync_channel(1);
        let (resume_tx, resume_rx) = mpsc::sync_channel(1);

        let worker_playback = Arc::clone(&playback);
        let handle = std::thread::spawn(move || {
            decode_then_attach_stems_if_current_song(&worker_playback, "song-a", || {
                entered_tx.send(()).expect("stem decode hook should notify test");
                resume_rx.recv().expect("test should resume stem decode");
                Ok(dummy_stems())
            })
        });

        entered_rx
            .recv_timeout(Duration::from_secs(1))
            .expect("stem decode hook should enter");
        assert!(playback.try_lock().is_ok());

        resume_tx.send(()).expect("stem decode should be released");

        let snapshot = handle
            .join()
            .expect("worker thread should join")
            .expect("stem decode should succeed");
        assert!(snapshot.has_stems);
    }

    #[test]
    fn stale_stem_decode_is_ignored_if_the_track_changed() {
        let playback = Arc::new(Mutex::new(PlaybackController::default()));
        playback
            .lock()
            .expect("playback lock should succeed")
            .start_track("song-b".to_owned(), dummy_audio(), 0);

        let snapshot =
            decode_then_attach_stems_if_current_song(&playback, "song-a", || Ok(dummy_stems()))
                .expect("stale stem decode should still return a snapshot");

        assert_eq!(snapshot.song_id.as_deref(), Some("song-b"));
        assert!(!snapshot.has_stems);
    }
}
