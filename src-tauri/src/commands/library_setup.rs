use crate::{
    cache,
    commands::error::{library_error, state_lock_error, CommandResult},
    config::{self, AppConfig},
    library_root::LibraryRoot,
    AppState,
};
use std::path::PathBuf;
use tauri::State;

fn updated_library_config(mut config: AppConfig, path: String) -> AppConfig {
    config.library_path = Some(path);
    config
}

fn persist_library_path(app_data_dir: &std::path::Path, path: String) -> CommandResult<()> {
    let existing = config::load_config(app_data_dir)
        .map_err(library_error)?
        .unwrap_or_default();
    let next_config = updated_library_config(existing, path);

    config::save_config(app_data_dir, &next_config).map_err(library_error)
}

#[tauri::command]
pub fn create_library(state: State<'_, AppState>, path: String) -> CommandResult<()> {
    let lib_path = PathBuf::from(&path);

    let lib = LibraryRoot::create(&lib_path).map_err(library_error)?;

    let db_path = lib.database_path();
    cache::initialize_library_database(&db_path).map_err(library_error)?;

    persist_library_path(&state.app_data_dir, path)?;

    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(lib);

    Ok(())
}

#[tauri::command]
pub fn open_library(state: State<'_, AppState>, path: String) -> CommandResult<()> {
    let lib_path = PathBuf::from(&path);

    let lib = LibraryRoot::open(&lib_path).map_err(library_error)?;

    let db_path = lib.database_path();
    cache::initialize_library_database(&db_path).map_err(library_error)?;

    persist_library_path(&state.app_data_dir, path)?;

    let mut guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;
    *guard = Some(lib);

    Ok(())
}

#[tauri::command]
pub fn get_library_path(state: State<'_, AppState>) -> CommandResult<Option<String>> {
    let guard = state
        .library
        .lock()
        .map_err(|_| state_lock_error("library lock was poisoned"))?;

    Ok(guard.as_ref().map(|lib| lib.root().display().to_string()))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn updating_library_path_preserves_existing_machine_settings() {
        let config = AppConfig {
            library_path: Some("/old".to_owned()),
            stem_mode: Some(config::StemMode::FourStem),
            language: Some("zh-CN".to_owned()),
            hide_batch_separate: Some(true),
            model_variant: Some(config::ModelVariant::HtdemucsFt),
            lyrics_font_step: Some(2),
            macos_shell_mode: Some(config::MacOsShellMode::Native),
        };

        let updated = updated_library_config(config, "/new".to_owned());

        assert_eq!(updated.library_path.as_deref(), Some("/new"));
        assert_eq!(updated.stem_mode, Some(config::StemMode::FourStem));
        assert_eq!(updated.language.as_deref(), Some("zh-CN"));
        assert_eq!(updated.hide_batch_separate, Some(true));
        assert_eq!(
            updated.model_variant,
            Some(config::ModelVariant::HtdemucsFt)
        );
        assert_eq!(updated.lyrics_font_step, Some(2));
        assert_eq!(
            updated.macos_shell_mode,
            Some(config::MacOsShellMode::Native)
        );
    }

    #[test]
    fn updating_library_path_from_default_config_only_sets_the_path() {
        let updated = updated_library_config(AppConfig::default(), "/library".to_owned());

        assert_eq!(updated.library_path.as_deref(), Some("/library"));
        assert_eq!(updated.macos_shell_mode, None);
        assert_eq!(updated.language, None);
    }
}
