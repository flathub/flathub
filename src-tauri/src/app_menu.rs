use tauri::{
    menu::{AboutMetadata, Menu, MenuEvent, MenuItem, PredefinedMenuItem, Submenu},
    AppHandle, Emitter, Runtime,
};

#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
pub const MENU_ACTION_EVENT: &str = "openkara://menu-action";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
pub const MENU_ACTION_IMPORT_FILES: &str = "import-files";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
pub const MENU_ACTION_OPEN_SETTINGS: &str = "open-settings";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
pub const MENU_ACTION_TOGGLE_SIDEBAR: &str = "toggle-sidebar";

#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
const MENU_ITEM_IMPORT_FILES: &str = "file.import";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
const MENU_ITEM_OPEN_SETTINGS: &str = "app.settings";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
const MENU_ITEM_TOGGLE_SIDEBAR: &str = "view.toggle-sidebar";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
const ABOUT_AUTHOR_CREDIT: &str = "@David Weng";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
const ABOUT_REPOSITORY_URL: &str = "https://github.com/thedavidweng/OpenKara";
#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
const ABOUT_REPOSITORY_LABEL: &str = "Official Repository";

#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
fn build_about_metadata<R: Runtime>(app_handle: &AppHandle<R>) -> AboutMetadata<'static> {
    let pkg_info = app_handle.package_info();
    let config = app_handle.config();

    AboutMetadata {
        name: Some(pkg_info.name.clone()),
        version: Some(pkg_info.version.to_string()),
        copyright: config.bundle.copyright.clone(),
        authors: Some(vec!["@David Weng".to_owned()]),
        credits: Some(ABOUT_AUTHOR_CREDIT.to_owned()),
        website: Some(ABOUT_REPOSITORY_URL.to_owned()),
        website_label: Some(ABOUT_REPOSITORY_LABEL.to_owned()),
        ..Default::default()
    }
}

#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
pub fn build_app_menu<R: Runtime>(app_handle: &AppHandle<R>) -> tauri::Result<Menu<R>> {
    let _pkg_info = app_handle.package_info();
    let about_metadata = build_about_metadata(app_handle);

    let import_item = MenuItem::with_id(
        app_handle,
        MENU_ITEM_IMPORT_FILES,
        "Import...",
        true,
        Some("CmdOrCtrl+O"),
    )?;

    let window_menu = Submenu::with_items(
        app_handle,
        "Window",
        true,
        &[
            &PredefinedMenuItem::minimize(app_handle, None)?,
            &PredefinedMenuItem::maximize(app_handle, None)?,
            #[cfg(target_os = "macos")]
            &PredefinedMenuItem::separator(app_handle)?,
            &PredefinedMenuItem::close_window(app_handle, None)?,
        ],
    )?;

    let help_menu = Submenu::with_items(
        app_handle,
        "Help",
        true,
        &[
            #[cfg(not(target_os = "macos"))]
            &PredefinedMenuItem::about(app_handle, None, Some(about_metadata.clone()))?,
        ],
    )?;

    let menu = Menu::with_items(
        app_handle,
        &[
            #[cfg(target_os = "macos")]
            &Submenu::with_items(
                app_handle,
                _pkg_info.name.clone(),
                true,
                &[
                    &PredefinedMenuItem::about(app_handle, None, Some(about_metadata.clone()))?,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &MenuItem::with_id(
                        app_handle,
                        MENU_ITEM_OPEN_SETTINGS,
                        "Settings...",
                        true,
                        Some("CmdOrCtrl+,"),
                    )?,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &PredefinedMenuItem::services(app_handle, None)?,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &PredefinedMenuItem::hide(app_handle, None)?,
                    &PredefinedMenuItem::hide_others(app_handle, None)?,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &PredefinedMenuItem::quit(app_handle, None)?,
                ],
            )?,
            &Submenu::with_items(
                app_handle,
                "File",
                true,
                &[
                    &import_item,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &PredefinedMenuItem::close_window(app_handle, None)?,
                    #[cfg(not(target_os = "macos"))]
                    &PredefinedMenuItem::quit(app_handle, None)?,
                ],
            )?,
            &Submenu::with_items(
                app_handle,
                "Edit",
                true,
                &[
                    &PredefinedMenuItem::undo(app_handle, None)?,
                    &PredefinedMenuItem::redo(app_handle, None)?,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &PredefinedMenuItem::cut(app_handle, None)?,
                    &PredefinedMenuItem::copy(app_handle, None)?,
                    &PredefinedMenuItem::paste(app_handle, None)?,
                    &PredefinedMenuItem::select_all(app_handle, None)?,
                ],
            )?,
            #[cfg(target_os = "macos")]
            &Submenu::with_items(
                app_handle,
                "View",
                true,
                &[
                    &MenuItem::with_id(
                        app_handle,
                        MENU_ITEM_TOGGLE_SIDEBAR,
                        "Toggle Sidebar",
                        true,
                        Some("CmdOrCtrl+B"),
                    )?,
                    &PredefinedMenuItem::separator(app_handle)?,
                    &PredefinedMenuItem::fullscreen(app_handle, None)?,
                ],
            )?,
            &window_menu,
            &help_menu,
        ],
    )?;

    Ok(menu)
}

#[cfg_attr(not(target_os = "macos"), allow(dead_code))]
pub fn handle_menu_event<R: Runtime>(app_handle: &AppHandle<R>, event: MenuEvent) {
    match event.id().as_ref() {
        MENU_ITEM_IMPORT_FILES => {
            let _ = app_handle.emit_to("main", MENU_ACTION_EVENT, MENU_ACTION_IMPORT_FILES);
        }
        MENU_ITEM_OPEN_SETTINGS => {
            let _ = app_handle.emit_to("main", MENU_ACTION_EVENT, MENU_ACTION_OPEN_SETTINGS);
        }
        MENU_ITEM_TOGGLE_SIDEBAR => {
            let _ = app_handle.emit_to("main", MENU_ACTION_EVENT, MENU_ACTION_TOGGLE_SIDEBAR);
        }
        _ => {}
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn menu_action_event_name_is_stable() {
        assert_eq!(MENU_ACTION_EVENT, "openkara://menu-action");
    }

    #[test]
    fn menu_actions_match_frontend_runtime_contract() {
        assert_eq!(MENU_ACTION_IMPORT_FILES, "import-files");
        assert_eq!(MENU_ACTION_OPEN_SETTINGS, "open-settings");
        assert_eq!(MENU_ACTION_TOGGLE_SIDEBAR, "toggle-sidebar");
    }

    #[test]
    fn mac_about_credit_is_stable() {
        assert_eq!(ABOUT_AUTHOR_CREDIT, "@David Weng");
    }

    #[test]
    fn mac_about_repository_link_is_stable() {
        assert_eq!(
            ABOUT_REPOSITORY_URL,
            "https://github.com/thedavidweng/OpenKara"
        );
        assert_eq!(ABOUT_REPOSITORY_LABEL, "Official Repository");
    }
}
