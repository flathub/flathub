use crate::{
    commands::import::import_songs_from_paths,
    config::StemMode,
    perf::build_backend_performance_report,
    separator::{bootstrap, job, model, model_cache::ModelCache},
};
use anyhow::{Context, Result};
use rusqlite::Connection;
use serde::Serialize;
use std::{
    collections::HashMap,
    fs,
    path::{Path, PathBuf},
    sync::{Arc, Mutex},
    time::{SystemTime, UNIX_EPOCH},
};

const JSON_REPORT_FILENAME: &str = "local-audio-smoke-report.json";
const MARKDOWN_REPORT_FILENAME: &str = "local-audio-smoke-report.md";

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SeparationSmokeMode {
    Auto,
    Disabled,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LocalAudioSmokeConfig {
    pub input_dir: PathBuf,
    pub output_dir: PathBuf,
    pub separation_mode: SeparationSmokeMode,
    pub seek_iterations: usize,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum SmokeStepStatus {
    Passed,
    Skipped,
    Failed,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct LocalAudioSmokeSummary {
    pub discovered_files: usize,
    pub imported: usize,
    pub import_failed: usize,
    pub playback_passed: usize,
    pub playback_failed: usize,
    pub separation_passed: usize,
    pub separation_failed: usize,
    pub separation_skipped: usize,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct SmokeModelStatus {
    pub status: SmokeStepStatus,
    pub path: Option<String>,
    pub message: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct LocalAudioSmokeSongReport {
    pub source_path: String,
    pub song_id: Option<String>,
    pub import_status: SmokeStepStatus,
    pub import_message: Option<String>,
    pub playback_status: SmokeStepStatus,
    pub playback_message: Option<String>,
    pub separation_status: SmokeStepStatus,
    pub separation_message: Option<String>,
    pub performance: Option<crate::perf::PlaybackPerformanceReport>,
    pub accompaniment_path: Option<String>,
    pub vocals_path: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Serialize)]
pub struct LocalAudioSmokeReport {
    pub generated_at: i64,
    pub input_dir: String,
    pub output_dir: String,
    pub report_json_path: PathBuf,
    pub report_markdown_path: PathBuf,
    pub model: SmokeModelStatus,
    pub summary: LocalAudioSmokeSummary,
    pub songs: Vec<LocalAudioSmokeSongReport>,
}

pub fn run_local_audio_smoke(config: LocalAudioSmokeConfig) -> Result<LocalAudioSmokeReport> {
    fs::create_dir_all(&config.output_dir).with_context(|| {
        format!(
            "failed to create smoke output directory {}",
            config.output_dir.display()
        )
    })?;

    let audio_files = discover_audio_files(&config.input_dir)?;
    if audio_files.is_empty() {
        anyhow::bail!(
            "no supported audio files were found under {}",
            config.input_dir.display()
        );
    }

    let database_path = config.output_dir.join("local-audio-smoke.sqlite3");
    if database_path.exists() {
        fs::remove_file(&database_path).with_context(|| {
            format!(
                "failed to reset previous smoke database {}",
                database_path.display()
            )
        })?;
    }

    let connection = Connection::open(&database_path).with_context(|| {
        format!(
            "failed to open smoke database at {}",
            database_path.display()
        )
    })?;
    crate::cache::apply_migrations(&connection)
        .context("failed to apply smoke database migrations")?;

    let import_paths: Vec<String> = audio_files
        .iter()
        .map(|path| path.display().to_string())
        .collect();
    let library =
        crate::library_root::LibraryRoot::create(&config.output_dir.join("smoke-library"))
            .or_else(|_| {
                crate::library_root::LibraryRoot::open(&config.output_dir.join("smoke-library"))
            })
            .context("failed to set up smoke library root")?;
    let import_result = import_songs_from_paths(&connection, &library, &import_paths);
    // `song.file_path` is now a library-relative path (`media/{hash}.{ext}`),
    // so we reconstruct a source-path-to-Song map.  `import_songs_from_paths`
    // processes paths in order: each goes into either `imported` or `failed`,
    // preserving the original order within each bucket.
    let failed_paths: std::collections::HashSet<&str> = import_result
        .failed
        .iter()
        .map(|f| f.path.as_str())
        .collect();
    let imported_by_path: HashMap<String, crate::library::Song> = import_paths
        .iter()
        .filter(|p| !failed_paths.contains(p.as_str()))
        .zip(import_result.imported.iter())
        .map(|(path, song)| (path.clone(), song.clone()))
        .collect();
    let failed_by_path = import_result
        .failed
        .iter()
        .map(|failure| (failure.path.clone(), failure.error.message.clone()))
        .collect::<HashMap<_, _>>();

    let model = resolve_model_status(&config)?;
    let separator_model_cache = Arc::new(Mutex::new(ModelCache::<model::LoadedModel>::default()));
    let mut songs = Vec::with_capacity(audio_files.len());

    for path in &audio_files {
        let source_path = path.display().to_string();

        if let Some(song) = imported_by_path.get(&source_path) {
            let playback = build_backend_performance_report(
                &connection,
                &library,
                &song.hash,
                config.seek_iterations.max(1),
            );
            let (playback_status, playback_message, performance) = match playback {
                Ok(report) => (SmokeStepStatus::Passed, None, Some(report.playback)),
                Err(error) => (SmokeStepStatus::Failed, Some(error.to_string()), None),
            };

            let (separation_status, separation_message, accompaniment_path, vocals_path) =
                match (&config.separation_mode, &model) {
                    (SeparationSmokeMode::Disabled, _) => (
                        SmokeStepStatus::Skipped,
                        Some("separation was disabled for this smoke run".to_string()),
                        None,
                        None,
                    ),
                    (
                        SeparationSmokeMode::Auto,
                        SmokeModelStatus {
                            status: SmokeStepStatus::Passed,
                            path: Some(model_path),
                            ..
                        },
                    ) => {
                        match job::separate_song_into_cache(
                            &connection,
                            &library,
                            &separator_model_cache,
                            Path::new(model_path),
                            &song.hash,
                            StemMode::default(),
                            "htdemucs",
                            crate::config::ExecutionProviderPreference::default(),
                            |_| {},
                        ) {
                            Ok(artifacts) => (
                                SmokeStepStatus::Passed,
                                if artifacts.cache_hit {
                                    Some("reused cached stems".to_string())
                                } else {
                                    Some("generated fresh stems".to_string())
                                },
                                Some(artifacts.accomp_path),
                                Some(artifacts.vocals_path),
                            ),
                            Err(error) => {
                                (SmokeStepStatus::Failed, Some(error.to_string()), None, None)
                            }
                        }
                    }
                    (SeparationSmokeMode::Auto, _) => {
                        (SmokeStepStatus::Skipped, model.message.clone(), None, None)
                    }
                };

            songs.push(LocalAudioSmokeSongReport {
                source_path,
                song_id: Some(song.hash.clone()),
                import_status: SmokeStepStatus::Passed,
                import_message: None,
                playback_status,
                playback_message,
                separation_status,
                separation_message,
                performance,
                accompaniment_path,
                vocals_path,
            });
        } else {
            songs.push(LocalAudioSmokeSongReport {
                source_path: source_path.clone(),
                song_id: None,
                import_status: SmokeStepStatus::Failed,
                import_message: failed_by_path.get(&source_path).cloned(),
                playback_status: SmokeStepStatus::Skipped,
                playback_message: Some("playback skipped because import failed".to_string()),
                separation_status: SmokeStepStatus::Skipped,
                separation_message: Some("separation skipped because import failed".to_string()),
                performance: None,
                accompaniment_path: None,
                vocals_path: None,
            });
        }
    }

    let report_json_path = config.output_dir.join(JSON_REPORT_FILENAME);
    let report_markdown_path = config.output_dir.join(MARKDOWN_REPORT_FILENAME);
    let report = LocalAudioSmokeReport {
        generated_at: unix_timestamp(),
        input_dir: config.input_dir.display().to_string(),
        output_dir: config.output_dir.display().to_string(),
        report_json_path: report_json_path.clone(),
        report_markdown_path: report_markdown_path.clone(),
        model,
        summary: summarize(&songs),
        songs,
    };

    write_report_json(&report, &report_json_path)?;
    write_report_markdown(&report, &report_markdown_path)?;

    Ok(report)
}

pub fn discover_audio_files(root: &Path) -> Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    collect_audio_files(root, &mut files)
        .with_context(|| format!("failed to scan audio fixtures under {}", root.display()))?;
    files.sort();
    Ok(files)
}

fn collect_audio_files(root: &Path, files: &mut Vec<PathBuf>) -> Result<()> {
    for entry in fs::read_dir(root)
        .with_context(|| format!("failed to read directory {}", root.display()))?
    {
        let entry = entry.with_context(|| format!("failed to read entry in {}", root.display()))?;
        let path = entry.path();
        if path.is_dir() {
            collect_audio_files(&path, files)?;
            continue;
        }

        if is_supported_audio_file(&path) {
            files.push(path.canonicalize().with_context(|| {
                format!(
                    "failed to canonicalize discovered audio file {}",
                    path.display()
                )
            })?);
        }
    }

    Ok(())
}

fn is_supported_audio_file(path: &Path) -> bool {
    matches!(
        path.extension()
            .and_then(|extension| extension.to_str())
            .map(|extension| extension.to_ascii_lowercase()),
        Some(extension)
            if matches!(
                extension.as_str(),
                "aac" | "flac" | "m4a" | "mp3" | "ogg" | "wav"
            )
    )
}

fn resolve_model_status(config: &LocalAudioSmokeConfig) -> Result<SmokeModelStatus> {
    match config.separation_mode {
        SeparationSmokeMode::Disabled => Ok(SmokeModelStatus {
            status: SmokeStepStatus::Skipped,
            path: None,
            message: Some("separation was disabled for this smoke run".to_string()),
        }),
        SeparationSmokeMode::Auto => {
            let dev_model_path = model::default_model_path();
            let placeholder_managed_path =
                config.output_dir.join(".smoke-managed-model-placeholder");
            let resolved = bootstrap::resolve_existing_model_path(
                &placeholder_managed_path,
                &dev_model_path,
                bootstrap::MODEL_SHA256,
            )?;

            match resolved {
                Some(path) => Ok(SmokeModelStatus {
                    status: SmokeStepStatus::Passed,
                    path: Some(path.path.display().to_string()),
                    message: Some(format!("using verified model from {}", path.path.display())),
                }),
                None => Ok(SmokeModelStatus {
                    status: SmokeStepStatus::Skipped,
                    path: None,
                    message: Some(format!(
                        "verified model not found; run ./scripts/setup.sh to enable separation (expected {})",
                        dev_model_path.display()
                    )),
                }),
            }
        }
    }
}

fn summarize(songs: &[LocalAudioSmokeSongReport]) -> LocalAudioSmokeSummary {
    let mut summary = LocalAudioSmokeSummary {
        discovered_files: songs.len(),
        imported: 0,
        import_failed: 0,
        playback_passed: 0,
        playback_failed: 0,
        separation_passed: 0,
        separation_failed: 0,
        separation_skipped: 0,
    };

    for song in songs {
        tally_status(
            &song.import_status,
            &mut summary.imported,
            &mut summary.import_failed,
        );
        tally_status(
            &song.playback_status,
            &mut summary.playback_passed,
            &mut summary.playback_failed,
        );
        match song.separation_status {
            SmokeStepStatus::Passed => summary.separation_passed += 1,
            SmokeStepStatus::Failed => summary.separation_failed += 1,
            SmokeStepStatus::Skipped => summary.separation_skipped += 1,
        }
    }

    summary
}

fn tally_status(status: &SmokeStepStatus, passed: &mut usize, failed: &mut usize) {
    match status {
        SmokeStepStatus::Passed => *passed += 1,
        SmokeStepStatus::Skipped => {}
        SmokeStepStatus::Failed => *failed += 1,
    }
}

fn write_report_json(report: &LocalAudioSmokeReport, path: &Path) -> Result<()> {
    let json = serde_json::to_string_pretty(report).context("failed to serialize smoke report")?;
    fs::write(path, json)
        .with_context(|| format!("failed to write smoke json report to {}", path.display()))?;
    Ok(())
}

fn write_report_markdown(report: &LocalAudioSmokeReport, path: &Path) -> Result<()> {
    let mut markdown = String::from("# Local Audio Smoke Report\n\n");
    markdown.push_str(&format!("- Generated at: `{}`\n", report.generated_at));
    markdown.push_str(&format!("- Input dir: `{}`\n", report.input_dir));
    markdown.push_str(&format!("- Output dir: `{}`\n", report.output_dir));
    markdown.push_str(&format!(
        "- Model: `{}`{}\n\n",
        status_label(&report.model.status),
        report
            .model
            .message
            .as_ref()
            .map(|message| format!(" — {}", message))
            .unwrap_or_default()
    ));

    markdown.push_str("## Summary\n\n");
    markdown.push_str(&format!(
        "- discovered: `{}`\n- imported: `{}`\n- import failed: `{}`\n- playback passed: `{}`\n- playback failed: `{}`\n- separation passed: `{}`\n- separation failed: `{}`\n- separation skipped: `{}`\n\n",
        report.summary.discovered_files,
        report.summary.imported,
        report.summary.import_failed,
        report.summary.playback_passed,
        report.summary.playback_failed,
        report.summary.separation_passed,
        report.summary.separation_failed,
        report.summary.separation_skipped,
    ));

    markdown.push_str("## Songs\n\n");
    markdown.push_str("| File | Import | Playback | Separation | Notes |\n");
    markdown.push_str("| --- | --- | --- | --- | --- |\n");

    for song in &report.songs {
        let notes = [
            song.import_message.as_deref(),
            song.playback_message.as_deref(),
            song.separation_message.as_deref(),
        ]
        .into_iter()
        .flatten()
        .collect::<Vec<_>>()
        .join("; ");

        markdown.push_str(&format!(
            "| `{}` | `{}` | `{}` | `{}` | {} |\n",
            song.source_path,
            status_label(&song.import_status),
            status_label(&song.playback_status),
            status_label(&song.separation_status),
            if notes.is_empty() {
                "-"
            } else {
                notes.as_str()
            }
        ));
    }

    fs::write(path, markdown).with_context(|| {
        format!(
            "failed to write smoke markdown report to {}",
            path.display()
        )
    })?;
    Ok(())
}

fn status_label(status: &SmokeStepStatus) -> &'static str {
    match status {
        SmokeStepStatus::Passed => "passed",
        SmokeStepStatus::Skipped => "skipped",
        SmokeStepStatus::Failed => "failed",
    }
}

fn unix_timestamp() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_secs() as i64
}
