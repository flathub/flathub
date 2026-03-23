use tauri::{State, Webview};

#[tauri::command]
pub fn get_window_shell_state(
    state: State<'_, crate::window_shell::WindowShellState>,
) -> crate::window_shell::WindowShellState {
    state.inner().clone()
}

#[tauri::command]
pub fn set_native_sidebar_visibility(webview: Webview, visible: bool) -> Result<(), String> {
    crate::window_shell::set_native_sidebar_visibility(&webview, visible)
        .map_err(|error| error.to_string())
}
