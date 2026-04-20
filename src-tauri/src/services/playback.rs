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
    atomic::{AtomicBool, AtomicU64, Ordering},
    Arc, Mutex,
};
use std::{
    thread,
    time::{Duration, Instant},
};
use tauri::{AppHandle, Emitter, Runtime};

fn bump_airplay_stream_generation(state: &AppState) {
    state
        .airplay_stream_generation
        .fetch_add(1, Ordering::SeqCst);
}

pub(crate) fn spawn_airplay_control_refresh_worker(
    airplay_audience_active: Arc<AtomicBool>,
    airplay_control_refresh_token: Arc<AtomicU64>,
    airplay_audio_tap: Arc<crate::airplay_stream::AirPlayAudioTap>,
    airplay_stream_generation: Arc<AtomicU64>,
) {
    spawn_airplay_control_refresh_worker_with_timing(
        airplay_audience_active,
        airplay_control_refresh_token,
        airplay_audio_tap,
        airplay_stream_generation,
        Duration::from_millis(180),
        Duration::from_millis(25),
    );
}

fn spawn_airplay_control_refresh_worker_with_timing(
    airplay_audience_active: Arc<AtomicBool>,
    airplay_control_refresh_token: Arc<AtomicU64>,
    airplay_audio_tap: Arc<crate::airplay_stream::AirPlayAudioTap>,
    airplay_stream_generation: Arc<AtomicU64>,
    debounce_window: Duration,
    poll_interval: Duration,
) {
    thread::spawn(move || {
        let mut flushed_token = 0u64;
        let mut pending_token: Option<u64> = None;
        let mut pending_since: Option<Instant> = None;

        loop {
            let current_token = airplay_control_refresh_token.load(Ordering::SeqCst);
            if current_token != flushed_token && pending_token != Some(current_token) {
                pending_token = Some(current_token);
                pending_since = Some(Instant::now());
            }

            if let (Some(token), Some(since)) = (pending_token, pending_since) {
                if since.elapsed() >= debounce_window {
                    flushed_token = token;
                    pending_token = None;
                    pending_since = None;
                    if airplay_audience_active.load(Ordering::SeqCst) {
                        airplay_audio_tap.bump_epoch();
                        crate::airplay_stream::notify_audio_epoch(
                            airplay_audio_tap.current_epoch(),
                        );
                        airplay_stream_generation.fetch_add(1, Ordering::SeqCst);
                    }
                }
            }

            thread::sleep(poll_interval);
        }
    });
}

pub fn play<R: Runtime>(
    state: &AppState,
    app_handle: &AppHandle<R>,
    song_id: &str,
) -> Result<PlaybackStateSnapshot> {
    state.airplay_audio_tap.bump_epoch();
    crate::airplay_stream::notify_audio_epoch(state.airplay_audio_tap.current_epoch());
    bump_airplay_stream_generation(state);
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
    state.airplay_audio_tap.bump_epoch();
    crate::airplay_stream::notify_audio_epoch(state.airplay_audio_tap.current_epoch());
    bump_airplay_stream_generation(state);
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
    state.airplay_audio_tap.bump_epoch();
    crate::airplay_stream::notify_audio_epoch(state.airplay_audio_tap.current_epoch());
    bump_airplay_stream_generation(state);
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
    state.airplay_audio_tap.bump_epoch();
    crate::airplay_stream::notify_audio_epoch(state.airplay_audio_tap.current_epoch());
    bump_airplay_stream_generation(state);
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
    let snapshot = playback.set_volume(level)?;
    drop(playback);
    if state.airplay_audience_active.load(Ordering::SeqCst) {
        state
            .airplay_control_refresh_token
            .fetch_add(1, Ordering::SeqCst);
    }
    Ok(snapshot)
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
    let snapshot = playback.set_stem_volume(stem, level)?;
    drop(playback);
    if state.airplay_audience_active.load(Ordering::SeqCst) {
        state
            .airplay_control_refresh_token
            .fetch_add(1, Ordering::SeqCst);
    }
    Ok(snapshot)
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
        state.airplay_audio_tap.clone(),
        state.airplay_local_output_suppressed.clone(),
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

pub(crate) fn probe_song_audio(
    library_root: &LibraryRoot,
    song: &crate::library::Song,
) -> Result<()> {
    let absolute_path = library_root.resolve(&song.file_path);
    if song.media_g_container.as_deref() == Some(MEDIA_G_ZIP) {
        let asset = media_g::inspect_zip_for_media_g(&absolute_path)?;
        return decode::probe_bytes(asset.audio_bytes, &asset.audio_extension)
            .with_context(|| format!("failed to probe audio for {}", song.file_path));
    }

    decode::probe_file(&absolute_path)
        .with_context(|| format!("failed to probe audio for {}", song.file_path))
}

pub(crate) fn load_song_audio(
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
    use crate::{
        airplay_stream::AirPlayAudioTap, commands::bootstrap, separator::model_cache::ModelCache,
        AppState,
    };
    use std::{
        collections::HashMap,
        path::PathBuf,
        sync::{
            atomic::{AtomicBool, AtomicU64, Ordering},
            mpsc, Arc, Mutex,
        },
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

    fn fixture_path(directory: &str, filename: &str) -> PathBuf {
        PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("tests")
            .join("fixtures")
            .join(directory)
            .join(filename)
    }

    fn airplay_state() -> AppState {
        let decoded = decode::decode_file(&fixture_path("audio", "fixture.wav"))
            .expect("fixture audio should decode");
        let mut playback = PlaybackController::default();
        playback.start_track("song-a".to_owned(), decoded, 0);

        AppState {
            library: Arc::new(Mutex::new(None)),
            app_data_dir: PathBuf::from(env!("CARGO_MANIFEST_DIR"))
                .join("tests")
                .join("tmp"),
            model_path: PathBuf::from("model.bin"),
            playback: Arc::new(Mutex::new(playback)),
            cdg_state: Arc::new(Mutex::new(None)),
            airplay_audio_tap: Arc::new(AirPlayAudioTap::new(4)),
            airplay_stream_generation: Arc::new(AtomicU64::new(7)),
            airplay_audience_active: Arc::new(AtomicBool::new(false)),
            airplay_control_refresh_token: Arc::new(AtomicU64::new(0)),
            airplay_http_server: Arc::new(Mutex::new(None)),
            airplay_local_output_suppressed: Arc::new(AtomicBool::new(false)),
            playback_request_id: AtomicU64::new(0),
            audio_output_started: Arc::new(AtomicBool::new(true)),
            audio_output_start_lock: Arc::new(Mutex::new(())),
            model_bootstrap_status: Arc::new(Mutex::new(bootstrap::pending_status("model.bin"))),
            separation_statuses: Arc::new(Mutex::new(HashMap::new())),
            separator_model_cache: Arc::new(Mutex::new(ModelCache::default())),
            batch_running: Arc::new(AtomicBool::new(false)),
            batch_cancel: Arc::new(AtomicBool::new(false)),
        }
    }

    fn wait_for_generation(generation: &AtomicU64, expected: u64, timeout: Duration) -> bool {
        let deadline = std::time::Instant::now() + timeout;
        while std::time::Instant::now() < deadline {
            if generation.load(Ordering::SeqCst) == expected {
                return true;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        generation.load(Ordering::SeqCst) == expected
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
                entered_tx
                    .send(())
                    .expect("stem decode hook should notify test");
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

    #[test]
    fn pause_and_resume_refresh_airplay_stream_generation() {
        let app = tauri::test::mock_app();
        let app_handle = app.handle().clone();
        let state = airplay_state();

        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);

        let paused = pause(&state, &app_handle).expect("pause should succeed");
        assert!(!paused.is_playing);
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation + 1
        );

        let resumed = resume(&state, &app_handle).expect("resume should succeed");
        assert!(resumed.is_playing);
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation + 2
        );
    }

    #[test]
    fn seek_refreshes_airplay_stream_generation() {
        let app = tauri::test::mock_app();
        let app_handle = app.handle().clone();
        let state = airplay_state();

        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);
        let snapshot = seek(&state, &app_handle, 500).expect("seek should succeed");

        assert_eq!(snapshot.position_ms, 500);
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation + 1
        );
    }

    #[test]
    fn airplay_control_refresh_debounces_multiple_stem_updates() {
        let state = airplay_state();
        state.airplay_audience_active.store(true, Ordering::SeqCst);

        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);
        let initial_epoch = state.airplay_audio_tap.current_epoch();

        spawn_airplay_control_refresh_worker_with_timing(
            Arc::clone(&state.airplay_audience_active),
            Arc::clone(&state.airplay_control_refresh_token),
            Arc::clone(&state.airplay_audio_tap),
            Arc::clone(&state.airplay_stream_generation),
            Duration::from_millis(300),
            Duration::from_millis(5),
        );

        set_stem_volume(&state, StemName::Vocals, 0.9).expect("stem update should succeed");
        set_stem_volume(&state, StemName::Drums, 0.8).expect("stem update should succeed");
        set_stem_volume(&state, StemName::Bass, 0.7).expect("stem update should succeed");
        assert_eq!(
            state.airplay_control_refresh_token.load(Ordering::SeqCst),
            3
        );

        std::thread::sleep(Duration::from_millis(100));
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation
        );

        assert!(wait_for_generation(
            &state.airplay_stream_generation,
            initial_generation + 1,
            Duration::from_millis(1_500),
        ));
        assert_eq!(state.airplay_audio_tap.current_epoch(), initial_epoch + 1);
    }

    #[test]
    fn airplay_control_refresh_debounces_volume_updates_until_user_stops_dragging() {
        let state = airplay_state();
        state.airplay_audience_active.store(true, Ordering::SeqCst);

        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);
        let initial_epoch = state.airplay_audio_tap.current_epoch();

        spawn_airplay_control_refresh_worker_with_timing(
            Arc::clone(&state.airplay_audience_active),
            Arc::clone(&state.airplay_control_refresh_token),
            Arc::clone(&state.airplay_audio_tap),
            Arc::clone(&state.airplay_stream_generation),
            Duration::from_millis(300),
            Duration::from_millis(5),
        );

        set_volume(&state, 0.9).expect("volume update should succeed");
        set_volume(&state, 0.7).expect("volume update should succeed");
        assert_eq!(
            state.airplay_control_refresh_token.load(Ordering::SeqCst),
            2
        );

        std::thread::sleep(Duration::from_millis(100));
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation
        );

        assert!(wait_for_generation(
            &state.airplay_stream_generation,
            initial_generation + 1,
            Duration::from_millis(1_500),
        ));
        assert_eq!(state.airplay_audio_tap.current_epoch(), initial_epoch + 1);
    }

    #[test]
    fn airplay_control_refresh_does_not_fire_while_idle() {
        let state = airplay_state();
        state.airplay_audience_active.store(false, Ordering::SeqCst);

        let initial_generation = state.airplay_stream_generation.load(Ordering::SeqCst);
        let initial_epoch = state.airplay_audio_tap.current_epoch();

        spawn_airplay_control_refresh_worker_with_timing(
            Arc::clone(&state.airplay_audience_active),
            Arc::clone(&state.airplay_control_refresh_token),
            Arc::clone(&state.airplay_audio_tap),
            Arc::clone(&state.airplay_stream_generation),
            Duration::from_millis(300),
            Duration::from_millis(5),
        );

        set_volume(&state, 0.6).expect("volume update should succeed");

        std::thread::sleep(Duration::from_millis(250));
        assert_eq!(
            state.airplay_stream_generation.load(Ordering::SeqCst),
            initial_generation
        );
        assert_eq!(state.airplay_audio_tap.current_epoch(), initial_epoch);
    }
}
