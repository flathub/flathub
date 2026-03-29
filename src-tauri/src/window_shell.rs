use serde::Serialize;
use tauri::{Manager, Runtime};

const DESKTOP_TOOLBAR_HEIGHT: u16 = 48;
const MAC_LEGACY_TOOLBAR_HEIGHT: u16 = 52;
const MAC_NATIVE_TOOLBAR_HEIGHT: u16 = 56;
const DESKTOP_TRAFFIC_LIGHT_INSET_LEADING: u16 = 0;
const MAC_LEGACY_TRAFFIC_LIGHT_INSET_LEADING: u16 = 64;
const MAC_NATIVE_TRAFFIC_LIGHT_INSET_LEADING: u16 = 78;
const DESKTOP_SIDEBAR_HEADER_HEIGHT: u16 = 0;
const MAC_LEGACY_SIDEBAR_HEADER_HEIGHT: u16 = 0;
const MAC_NATIVE_SIDEBAR_HEADER_HEIGHT: u16 = 40;
const DEFAULT_SIDEBAR_WIDTH: u16 = 260;
const MAC_NATIVE_SIDEBAR_WIDTH: u16 = 420;
pub const SIDEBAR_CHILD_WEBVIEW_LABEL: &str = "main-sidebar";
pub const MAIN_WEBVIEW_LABEL: &str = "main";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum WindowChromeVariant {
    Desktop,
    Mac,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum WindowShellTier {
    Desktop,
    MacLegacy,
    MacNative,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct WindowShellState {
    pub chrome_variant: WindowChromeVariant,
    pub tier: WindowShellTier,
    pub toolbar_height: u16,
    pub traffic_light_inset_leading: u16,
    pub sidebar_header_height: u16,
    pub sidebar_width: u16,
    pub sidebar_webview_label: Option<String>,
    pub main_content_webview_label: Option<String>,
}

impl WindowShellState {
    pub fn desktop() -> Self {
        Self {
            chrome_variant: WindowChromeVariant::Desktop,
            tier: WindowShellTier::Desktop,
            toolbar_height: DESKTOP_TOOLBAR_HEIGHT,
            traffic_light_inset_leading: DESKTOP_TRAFFIC_LIGHT_INSET_LEADING,
            sidebar_header_height: DESKTOP_SIDEBAR_HEADER_HEIGHT,
            sidebar_width: DEFAULT_SIDEBAR_WIDTH,
            sidebar_webview_label: None,
            main_content_webview_label: None,
        }
    }

    pub fn mac_legacy() -> Self {
        Self {
            chrome_variant: WindowChromeVariant::Mac,
            tier: WindowShellTier::MacLegacy,
            toolbar_height: MAC_LEGACY_TOOLBAR_HEIGHT,
            traffic_light_inset_leading: MAC_LEGACY_TRAFFIC_LIGHT_INSET_LEADING,
            sidebar_header_height: MAC_LEGACY_SIDEBAR_HEADER_HEIGHT,
            sidebar_width: DEFAULT_SIDEBAR_WIDTH,
            sidebar_webview_label: None,
            main_content_webview_label: None,
        }
    }

    pub fn mac_native() -> Self {
        Self {
            chrome_variant: WindowChromeVariant::Mac,
            tier: WindowShellTier::MacNative,
            toolbar_height: MAC_NATIVE_TOOLBAR_HEIGHT,
            traffic_light_inset_leading: MAC_NATIVE_TRAFFIC_LIGHT_INSET_LEADING,
            sidebar_header_height: MAC_NATIVE_SIDEBAR_HEADER_HEIGHT,
            sidebar_width: MAC_NATIVE_SIDEBAR_WIDTH,
            sidebar_webview_label: Some(SIDEBAR_CHILD_WEBVIEW_LABEL.to_owned()),
            main_content_webview_label: Some(MAIN_WEBVIEW_LABEL.to_owned()),
        }
    }

    pub fn for_macos_major_version(major_version: u64) -> Self {
        if major_version >= 26 {
            Self::mac_native()
        } else {
            Self::mac_legacy()
        }
    }

    pub fn macos_major_version(&self) -> u64 {
        match self.tier {
            WindowShellTier::MacNative => 26,
            WindowShellTier::MacLegacy => 15,
            WindowShellTier::Desktop => 0,
        }
    }

    fn with_native_metrics(
        mut self,
        toolbar_height: u16,
        traffic_light_inset_leading: u16,
        sidebar_header_height: u16,
    ) -> Self {
        self.toolbar_height = toolbar_height;
        self.traffic_light_inset_leading = traffic_light_inset_leading;
        self.sidebar_header_height = sidebar_header_height;
        self
    }
}

#[cfg(target_os = "macos")]
fn split_shell_enabled(config: Option<&crate::config::AppConfig>) -> bool {
    config
        .map(|config| config.effective_macos_shell_mode() == crate::config::MacOsShellMode::Native)
        .unwrap_or(false)
}

#[cfg(target_os = "macos")]
fn resolve_window_shell_state_for_mode(
    major_version: u64,
    native_shell_enabled: bool,
) -> WindowShellState {
    let _ = major_version;

    if native_shell_enabled {
        WindowShellState::mac_native()
    } else {
        WindowShellState::mac_legacy()
    }
}

pub fn initialize_main_window<R: Runtime>(
    app: &tauri::App<R>,
    app_config: Option<&crate::config::AppConfig>,
) -> WindowShellState {
    #[cfg(target_os = "macos")]
    {
        let native_shell_enabled = split_shell_enabled(app_config);
        let detected = native::detect_window_shell_state()
            .map(|state| {
                resolve_window_shell_state_for_mode(
                    state.macos_major_version(),
                    native_shell_enabled,
                )
            })
            .unwrap_or_else(WindowShellState::mac_legacy);

        let Some(window) = app.get_webview_window("main") else {
            return WindowShellState::mac_legacy();
        };

        // RATIONALE: A missing or failed AppKit pass should keep the app on the
        // old mac toolbar treatment instead of exposing half-applied Tahoe chrome.
        match native::apply_main_window_shell(&window, &detected) {
            Ok(Some(applied)) => {
                if applied.tier == WindowShellTier::MacNative
                    && native::configure_native_split_shell(app, &applied).is_err()
                {
                    let _ =
                        native::apply_main_window_shell(&window, &WindowShellState::mac_legacy());
                    return WindowShellState::mac_legacy();
                }

                applied
            }
            Ok(None) => WindowShellState::mac_legacy(),
            Err(_) => WindowShellState::mac_legacy(),
        }
    }

    #[cfg(not(target_os = "macos"))]
    {
        let _ = (app, app_config);
        WindowShellState::desktop()
    }
}

#[cfg(target_os = "macos")]
mod native {
    use super::{
        WindowShellState, WindowShellTier, MAIN_WEBVIEW_LABEL, SIDEBAR_CHILD_WEBVIEW_LABEL,
    };
    use std::{ffi::c_void, sync::mpsc::channel};
    use tauri::{
        webview::WebviewBuilder, LogicalPosition, LogicalSize, Manager, Runtime, Webview,
        WebviewUrl, WebviewWindow,
    };

    #[repr(C)]
    #[derive(Debug, Clone, Copy)]
    struct NativeWindowShellProfile {
        macos_major_version: isize,
        tier_tag: isize,
        toolbar_height: isize,
        traffic_light_inset_leading: isize,
        sidebar_header_height: isize,
    }

    unsafe extern "C" {
        fn ok_window_shell_detect_profile(profile_out: *mut NativeWindowShellProfile);
        fn ok_window_shell_configure_main_window(
            ns_view_ptr: *mut c_void,
            tier_tag: isize,
            toolbar_height: f64,
            traffic_light_inset_leading: f64,
            sidebar_header_height: f64,
            profile_out: *mut NativeWindowShellProfile,
        ) -> bool;
        fn ok_window_shell_mount_container_views(
            ns_window_ptr: *mut c_void,
            sidebar_webview_ptr: *mut c_void,
            main_content_webview_ptr: *mut c_void,
            sidebar_width: f64,
            sidebar_visible: bool,
        ) -> bool;
        fn ok_window_shell_set_split_sidebar_visibility(
            ns_window_ptr: *mut c_void,
            sidebar_width: f64,
            sidebar_visible: bool,
        ) -> bool;
    }

    struct NativeViewHandles {
        ns_window_ptr: *mut c_void,
        webview_ptr: *mut c_void,
    }

    fn url_for_shell_mode<R: Runtime>(
        webview: &Webview<R>,
        shell_mode: &str,
    ) -> anyhow::Result<tauri::Url> {
        let mut shell_url = webview.url()?;
        shell_url.set_query(Some(&format!("shell={shell_mode}")));

        Ok(shell_url)
    }

    fn shell_url_for_mode<R: Runtime>(
        webview: &Webview<R>,
        shell_mode: &str,
    ) -> anyhow::Result<WebviewUrl> {
        let shell_url = url_for_shell_mode(webview, shell_mode)?;

        Ok(match shell_url.scheme() {
            "http" | "https" => WebviewUrl::External(shell_url),
            _ => WebviewUrl::CustomProtocol(shell_url),
        })
    }

    fn rollback_split_shell_setup<R: Runtime>(
        main_content_webview: &Webview<R>,
        sidebar_webview: &Webview<R>,
        created_sidebar_webview: bool,
    ) {
        let _ = main_content_webview.set_auto_resize(true);

        if let Ok(full_app_url) = url_for_shell_mode(main_content_webview, "full-app") {
            let _ = main_content_webview.navigate(full_app_url);
        }

        if created_sidebar_webview {
            let _ = sidebar_webview.close();
        } else {
            let _ = sidebar_webview.hide();
        }
    }

    fn native_view_handles<R: Runtime>(webview: &Webview<R>) -> anyhow::Result<NativeViewHandles> {
        let (tx, rx) = channel();

        webview.with_webview(move |platform_webview| {
            let _ = tx.send((
                platform_webview.ns_window() as usize,
                platform_webview.inner() as usize,
            ));
        })?;

        let (ns_window_ptr, webview_ptr) = rx.recv().map_err(|error| {
            anyhow::anyhow!("failed to receive native webview handles: {error}")
        })?;

        Ok(NativeViewHandles {
            ns_window_ptr: ns_window_ptr as *mut c_void,
            webview_ptr: webview_ptr as *mut c_void,
        })
    }

    fn window_shell_state_from_profile(
        profile: NativeWindowShellProfile,
    ) -> Option<WindowShellState> {
        let toolbar_height = u16::try_from(profile.toolbar_height).ok()?;
        let traffic_light_inset_leading =
            u16::try_from(profile.traffic_light_inset_leading).ok()?;
        let sidebar_header_height = u16::try_from(profile.sidebar_header_height).ok()?;
        let base_state = match profile.tier_tag {
            0 => WindowShellState::desktop(),
            1 => WindowShellState::mac_legacy(),
            2 => WindowShellState::mac_native(),
            _ => return None,
        };

        Some(base_state.with_native_metrics(
            toolbar_height,
            traffic_light_inset_leading,
            sidebar_header_height,
        ))
    }

    pub(super) fn detect_window_shell_state() -> Option<WindowShellState> {
        let mut profile = NativeWindowShellProfile {
            macos_major_version: 0,
            tier_tag: 0,
            toolbar_height: 0,
            traffic_light_inset_leading: 0,
            sidebar_header_height: 0,
        };

        // SAFETY: The Objective-C bridge writes a small POD struct into the out pointer.
        unsafe {
            ok_window_shell_detect_profile(&mut profile);
        }

        window_shell_state_from_profile(profile)
    }

    pub(super) fn apply_main_window_shell<R: Runtime>(
        window: &WebviewWindow<R>,
        state: &WindowShellState,
    ) -> anyhow::Result<Option<WindowShellState>> {
        let ns_view = window
            .ns_view()
            .map_err(|error| anyhow::anyhow!("failed to get ns_view: {error}"))?;

        let tier_tag = match state.tier {
            WindowShellTier::Desktop => 0,
            WindowShellTier::MacLegacy => 1,
            WindowShellTier::MacNative => 2,
        };

        let mut resolved_profile = NativeWindowShellProfile {
            macos_major_version: 0,
            tier_tag,
            toolbar_height: state.toolbar_height as isize,
            traffic_light_inset_leading: state.traffic_light_inset_leading as isize,
            sidebar_header_height: state.sidebar_header_height as isize,
        };

        // SAFETY: Tauri exposes the live NSView pointer for the current webview window.
        let configured = unsafe {
            ok_window_shell_configure_main_window(
                ns_view,
                tier_tag,
                f64::from(state.toolbar_height),
                f64::from(state.traffic_light_inset_leading),
                f64::from(state.sidebar_header_height),
                &mut resolved_profile,
            )
        };

        if !configured {
            return Ok(None);
        }

        Ok(window_shell_state_from_profile(resolved_profile))
    }
    pub(super) fn set_native_sidebar_visibility<R: Runtime>(
        webview: &Webview<R>,
        visible: bool,
    ) -> anyhow::Result<()> {
        let handles = native_view_handles(webview)?;

        let updated = unsafe {
            ok_window_shell_set_split_sidebar_visibility(
                handles.ns_window_ptr,
                f64::from(super::MAC_NATIVE_SIDEBAR_WIDTH),
                visible,
            )
        };

        if !updated {
            return Err(anyhow::anyhow!("failed to sync native sidebar visibility"));
        }

        Ok(())
    }

    pub(super) fn configure_native_split_shell<R: Runtime>(
        app: &tauri::App<R>,
        state: &WindowShellState,
    ) -> anyhow::Result<()> {
        let sidebar_shell_url = {
            let main_webview = app
                .get_webview(MAIN_WEBVIEW_LABEL)
                .ok_or_else(|| anyhow::anyhow!("missing main content webview"))?;
            shell_url_for_mode(&main_webview, "sidebar-webview")?
        };
        let main_content_webview = app
            .get_webview(MAIN_WEBVIEW_LABEL)
            .ok_or_else(|| anyhow::anyhow!("missing main content webview"))?;
        let sidebar_label = state
            .sidebar_webview_label
            .as_deref()
            .unwrap_or(SIDEBAR_CHILD_WEBVIEW_LABEL);
        let main_content_shell_url =
            url_for_shell_mode(&main_content_webview, "main-content-webview")?;

        let (sidebar_webview, created_sidebar_webview) =
            if let Some(existing) = app.get_webview(sidebar_label) {
                (existing, false)
            } else {
                let created = main_content_webview.window().add_child(
                    WebviewBuilder::new(sidebar_label, sidebar_shell_url),
                    LogicalPosition::new(0.0, 0.0),
                    LogicalSize::new(f64::from(state.sidebar_width), 100.0),
                )?;
                (created, true)
            };

        let sidebar_handles = match native_view_handles(&sidebar_webview) {
            Ok(handles) => handles,
            Err(error) => {
                rollback_split_shell_setup(
                    &main_content_webview,
                    &sidebar_webview,
                    created_sidebar_webview,
                );
                return Err(error);
            }
        };
        let main_content_handles = match native_view_handles(&main_content_webview) {
            Ok(handles) => handles,
            Err(error) => {
                rollback_split_shell_setup(
                    &main_content_webview,
                    &sidebar_webview,
                    created_sidebar_webview,
                );
                return Err(error);
            }
        };

        let mounted = unsafe {
            ok_window_shell_mount_container_views(
                main_content_handles.ns_window_ptr,
                sidebar_handles.webview_ptr,
                main_content_handles.webview_ptr,
                f64::from(state.sidebar_width),
                true,
            )
        };

        if !mounted {
            rollback_split_shell_setup(
                &main_content_webview,
                &sidebar_webview,
                created_sidebar_webview,
            );
            return Err(anyhow::anyhow!("failed to mount native split shell"));
        }

        if let Err(error) = sidebar_webview.set_auto_resize(false) {
            rollback_split_shell_setup(
                &main_content_webview,
                &sidebar_webview,
                created_sidebar_webview,
            );
            return Err(anyhow::anyhow!(
                "failed to configure sidebar auto resize: {error}"
            ));
        }

        if let Err(error) = main_content_webview.navigate(main_content_shell_url) {
            rollback_split_shell_setup(
                &main_content_webview,
                &sidebar_webview,
                created_sidebar_webview,
            );
            return Err(anyhow::anyhow!(
                "failed to navigate main content shell: {error}"
            ));
        }

        if let Err(error) = main_content_webview.set_auto_resize(false) {
            rollback_split_shell_setup(
                &main_content_webview,
                &sidebar_webview,
                created_sidebar_webview,
            );
            return Err(anyhow::anyhow!(
                "failed to configure main content auto resize: {error}"
            ));
        }

        Ok(())
    }
}

pub fn set_native_sidebar_visibility<R: Runtime>(
    webview: &tauri::Webview<R>,
    visible: bool,
) -> anyhow::Result<()> {
    #[cfg(target_os = "macos")]
    {
        native::set_native_sidebar_visibility(webview, visible)
    }

    #[cfg(not(target_os = "macos"))]
    {
        let _ = (webview, visible);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn desktop_shell_state_preserves_existing_non_mac_behavior() {
        let state = WindowShellState::desktop();

        assert_eq!(state.chrome_variant, WindowChromeVariant::Desktop);
        assert_eq!(state.tier, WindowShellTier::Desktop);
        assert_eq!(state.toolbar_height, 48);
        assert_eq!(state.traffic_light_inset_leading, 0);
    }

    #[test]
    fn macos_26_uses_the_native_shell_profile() {
        let state = WindowShellState::for_macos_major_version(26);

        assert_eq!(state.chrome_variant, WindowChromeVariant::Mac);
        assert_eq!(state.tier, WindowShellTier::MacNative);
        assert_eq!(state.toolbar_height, 56);
        assert_eq!(state.traffic_light_inset_leading, 78);
        assert_eq!(state.sidebar_header_height, 40);
        assert_eq!(state.sidebar_width, 420);
    }

    #[test]
    fn older_macos_versions_fall_back_to_the_legacy_shell_profile() {
        let state = WindowShellState::for_macos_major_version(15);

        assert_eq!(state.chrome_variant, WindowChromeVariant::Mac);
        assert_eq!(state.tier, WindowShellTier::MacLegacy);
        assert_eq!(state.toolbar_height, 52);
        assert_eq!(state.traffic_light_inset_leading, 64);
    }

    #[test]
    fn native_mode_uses_the_native_host_tier_even_on_older_macos_versions() {
        let state = resolve_window_shell_state_for_mode(15, true);

        assert_eq!(state.tier, WindowShellTier::MacNative);
        assert_eq!(state.sidebar_webview_label.as_deref(), Some("main-sidebar"));
        assert_eq!(state.main_content_webview_label.as_deref(), Some("main"));
    }

    #[test]
    fn macos_native_shell_tracks_split_webview_labels() {
        let state = WindowShellState::for_macos_major_version(26);

        assert_eq!(state.sidebar_webview_label.as_deref(), Some("main-sidebar"));
        assert_eq!(state.main_content_webview_label.as_deref(), Some("main"));
    }

    #[test]
    fn desktop_shell_does_not_expose_child_webview_labels() {
        let state = WindowShellState::desktop();

        assert_eq!(state.sidebar_webview_label, None);
        assert_eq!(state.main_content_webview_label, None);
    }

    #[test]
    fn stable_mode_keeps_macos_26_on_the_legacy_shell_path() {
        let state = resolve_window_shell_state_for_mode(26, false);

        assert_eq!(state.tier, WindowShellTier::MacLegacy);
        assert_eq!(state.sidebar_webview_label, None);
        assert_eq!(state.main_content_webview_label, None);
    }

    #[test]
    fn macos_native_shell_does_not_install_a_separate_toolbar_layer() {
        let state = resolve_window_shell_state_for_mode(26, true);

        assert_eq!(state.tier, WindowShellTier::MacNative);
        assert_eq!(state.toolbar_height, 56);
        assert_eq!(state.sidebar_header_height, 40);
    }
}
