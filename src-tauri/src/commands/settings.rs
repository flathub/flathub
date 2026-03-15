use crate::commands::error::{internal_error, CommandResult};
use crate::config::{self, StemMode};
use serde::Serialize;
use tauri::{AppHandle, Manager};

#[derive(Debug, Serialize)]
pub struct AppSettings {
    pub stem_mode: String,
    pub language: Option<String>,
}

#[tauri::command]
pub fn get_settings(app_handle: AppHandle) -> CommandResult<AppSettings> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|e| internal_error(format!("failed to get app data dir: {e}")))?;
    let config = config::load_config(&app_data_dir)
        .map_err(|e| internal_error(format!("failed to load config: {e}")))?
        .unwrap_or_default();
    let mode = config.effective_stem_mode();
    Ok(AppSettings {
        stem_mode: match mode {
            StemMode::TwoStem => "two_stem".to_owned(),
            StemMode::FourStem => "four_stem".to_owned(),
        },
        language: config.language.clone(),
    })
}

#[tauri::command]
pub fn set_stem_mode(app_handle: AppHandle, mode: String) -> CommandResult<AppSettings> {
    let stem_mode = match mode.as_str() {
        "two_stem" => StemMode::TwoStem,
        "four_stem" => StemMode::FourStem,
        _ => return Err(internal_error(format!("invalid stem mode: {mode}"))),
    };
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|e| internal_error(format!("failed to get app data dir: {e}")))?;
    let mut config = config::load_config(&app_data_dir)
        .map_err(|e| internal_error(format!("failed to load config: {e}")))?
        .unwrap_or_default();
    config.stem_mode = Some(stem_mode);
    config::save_config(&app_data_dir, &config)
        .map_err(|e| internal_error(format!("failed to save config: {e}")))?;
    Ok(AppSettings {
        stem_mode: mode,
        language: config.language.clone(),
    })
}

#[tauri::command]
pub fn set_language(app_handle: AppHandle, language: String) -> CommandResult<AppSettings> {
    let app_data_dir = app_handle
        .path()
        .app_data_dir()
        .map_err(|e| internal_error(format!("failed to get app data dir: {e}")))?;
    let mut config = config::load_config(&app_data_dir)
        .map_err(|e| internal_error(format!("failed to load config: {e}")))?
        .unwrap_or_default();
    config.language = Some(language.clone());
    config::save_config(&app_data_dir, &config)
        .map_err(|e| internal_error(format!("failed to save config: {e}")))?;
    let mode = config.effective_stem_mode();
    Ok(AppSettings {
        stem_mode: match mode {
            StemMode::TwoStem => "two_stem".to_owned(),
            StemMode::FourStem => "four_stem".to_owned(),
        },
        language: config.language,
    })
}
