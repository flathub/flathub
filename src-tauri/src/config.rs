use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::{
    fs,
    path::{Path, PathBuf},
};

const CONFIG_FILENAME: &str = "config.json";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, Default)]
#[serde(rename_all = "snake_case")]
pub enum StemMode {
    #[default]
    TwoStem,
    FourStem,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, Default)]
#[serde(rename_all = "snake_case")]
pub enum ModelVariant {
    #[default]
    Htdemucs,
    HtdemucsFt,
}

impl ModelVariant {
    pub fn as_str(self) -> &'static str {
        match self {
            ModelVariant::Htdemucs => "htdemucs",
            ModelVariant::HtdemucsFt => "htdemucs_ft",
        }
    }

    pub fn from_str(s: &str) -> Option<ModelVariant> {
        match s {
            "htdemucs" => Some(ModelVariant::Htdemucs),
            "htdemucs_ft" => Some(ModelVariant::HtdemucsFt),
            _ => None,
        }
    }
}

/// Per-machine configuration stored in `{app_data_dir}/config.json`.
///
/// This is the only file that stays outside the portable library directory.
/// It tells the app where the user's karaoke library lives.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct AppConfig {
    /// Absolute path to the library root directory.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub library_path: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub stem_mode: Option<StemMode>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub language: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hide_batch_separate: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub model_variant: Option<ModelVariant>,
    // Lyrics font size is a per-machine display preference, so it belongs in
    // config.json with the rest of app settings rather than in the lyrics table.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub lyrics_font_step: Option<i8>,
}

impl AppConfig {
    pub fn effective_stem_mode(&self) -> StemMode {
        self.stem_mode.unwrap_or_default()
    }

    pub fn effective_model_variant(&self) -> ModelVariant {
        self.model_variant.unwrap_or_default()
    }

    pub fn effective_lyrics_font_step(&self) -> i8 {
        self.lyrics_font_step.unwrap_or(0)
    }
}

/// Load the per-machine config. Returns `Ok(None)` if the file does not exist.
pub fn load_config(app_data_dir: &Path) -> Result<Option<AppConfig>> {
    let config_path = config_path(app_data_dir);
    if !config_path.exists() {
        return Ok(None);
    }

    let contents = fs::read_to_string(&config_path)
        .with_context(|| format!("failed to read config at {}", config_path.display()))?;
    let config: AppConfig = serde_json::from_str(&contents)
        .with_context(|| format!("failed to parse config at {}", config_path.display()))?;

    Ok(Some(config))
}

/// Persist the per-machine config to disk.
pub fn save_config(app_data_dir: &Path, config: &AppConfig) -> Result<()> {
    fs::create_dir_all(app_data_dir)
        .with_context(|| format!("failed to create app data dir {}", app_data_dir.display()))?;

    let config_path = config_path(app_data_dir);
    let json = serde_json::to_string_pretty(config).context("failed to serialize config")?;
    fs::write(&config_path, json)
        .with_context(|| format!("failed to write config to {}", config_path.display()))?;

    Ok(())
}

fn config_path(app_data_dir: &Path) -> PathBuf {
    app_data_dir.join(CONFIG_FILENAME)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn load_returns_none_when_missing() {
        let tmp = tempfile::tempdir().unwrap();
        let config = load_config(tmp.path()).unwrap();
        assert!(config.is_none());
    }

    #[test]
    fn save_and_load_round_trip() {
        let tmp = tempfile::tempdir().unwrap();
        let config = AppConfig {
            library_path: Some("/Users/test/Music/MyLibrary".to_owned()),
            stem_mode: Some(StemMode::FourStem),
            language: None,
            hide_batch_separate: None,
            model_variant: None,
            lyrics_font_step: Some(1),
        };

        save_config(tmp.path(), &config).unwrap();
        let loaded = load_config(tmp.path()).unwrap().unwrap();
        assert_eq!(loaded.library_path, config.library_path);
        assert_eq!(loaded.stem_mode, Some(StemMode::FourStem));
        assert_eq!(loaded.lyrics_font_step, Some(1));
    }

    #[test]
    fn effective_stem_mode_defaults_to_two_stem() {
        let config = AppConfig::default();
        assert_eq!(config.effective_stem_mode(), StemMode::TwoStem);
    }

    #[test]
    fn stem_mode_none_is_omitted_from_json() {
        let config = AppConfig {
            library_path: None,
            stem_mode: None,
            language: None,
            hide_batch_separate: None,
            model_variant: None,
            lyrics_font_step: None,
        };
        let json = serde_json::to_string(&config).unwrap();
        assert!(!json.contains("stem_mode"));
    }

    #[test]
    fn effective_lyrics_font_step_defaults_to_zero() {
        let config = AppConfig::default();
        assert_eq!(config.effective_lyrics_font_step(), 0);
    }

    #[test]
    fn lyrics_font_step_none_is_omitted_from_json() {
        let config = AppConfig {
            library_path: None,
            stem_mode: None,
            language: None,
            hide_batch_separate: None,
            model_variant: None,
            lyrics_font_step: None,
        };
        let json = serde_json::to_string(&config).unwrap();
        assert!(!json.contains("lyrics_font_step"));
    }
}
