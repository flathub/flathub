use super::error::{internal_error, CommandResult};
use tauri::{State, Webview};

#[tauri::command]
pub fn get_window_shell_state(
    state: State<'_, crate::window_shell::WindowShellState>,
) -> crate::window_shell::WindowShellState {
    state.inner().clone()
}

#[tauri::command]
pub fn set_native_sidebar_visibility(webview: Webview, visible: bool) -> CommandResult<()> {
    crate::window_shell::set_native_sidebar_visibility(&webview, visible)
        .map_err(|error| internal_error(error.to_string()))
}
