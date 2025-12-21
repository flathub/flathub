use crate::listen::Listen;
use crate::meta::{Meta, TrackInfo};
use crate::metainfo::metainfo_description;
use crate::station::Station;

use adw::glib;
use adw::gtk::{
    self,
    gdk::{gdk_pixbuf::InterpType::Bilinear, gdk_pixbuf::Pixbuf, Display, Texture},
    gio::{Cancellable, MemoryInputStream, Menu, SimpleAction},
    ApplicationWindow, Button, GestureClick, HeaderBar, MenuButton, Orientation, Picture, Popover,
};
use adw::prelude::*;
use adw::{Application, StyleManager, WindowTitle};
use gettextrs::gettext;
#[cfg(all(target_os = "linux", feature = "controls"))]
use souvlaki::{MediaControlEvent, MediaControls, MediaMetadata, MediaPlayback, PlatformConfig};
#[cfg(all(target_os = "linux", feature = "controls"))]
use std::cell::RefCell;
use std::error::Error;
use std::rc::Rc;
use std::sync::mpsc;
use std::thread;
use std::time::Duration;

const COVER_MAX_SIZE: i32 = 250;
#[cfg(debug_assertions)]
const APP_ID: &str = "io.github.noobping.listenmoe_develop";
#[cfg(not(debug_assertions))]
const APP_ID: &str = "io.github.noobping.listenmoe";

fn make_action<F>(name: &str, f: F) -> SimpleAction
where
    F: Fn() + 'static,
{
    let action = SimpleAction::new(name, None);
    action.connect_activate(move |_, _| f());
    action
}

fn create_station_action(
    station: Station,
    play_button: &Button,
    window: &ApplicationWindow,
    radio: &Rc<Listen>,
    meta: &Rc<Meta>,
) -> SimpleAction {
    let radio = radio.clone();
    let meta = meta.clone();
    let win_clone = window.clone();
    let play = play_button.clone();

    make_action(station.name(), move || {
        radio.set_station(station);
        meta.set_station(station);
        if play.is_visible() {
            let _ = adw::prelude::WidgetExt::activate_action(
                &win_clone,
                "win.play",
                None::<&glib::Variant>,
            );
        }
    })
}

fn other_station(s: Station) -> Station {
    match s {
        Station::Jpop => Station::Kpop,
        Station::Kpop => Station::Jpop,
    }
}

/// Build the user interface.  This function is called once when the application
/// is activated.  It constructs the window, header bar, actions and spawns
/// background tasks for streaming audio and metadata.
pub fn build_ui(app: &Application) {
    let station = Station::Jpop;
    let radio = Listen::new(station);
    let (tx, rx) = mpsc::channel::<TrackInfo>();
    let meta = Meta::new(station, tx, radio.lag_ms());
    let (cover_tx, cover_rx) = mpsc::channel::<Result<Vec<u8>, String>>();
    let win_title = WindowTitle::new("LISTEN.moe", "JPOP/KPOP Radio");

    let play_button = Button::from_icon_name("media-playback-start-symbolic");
    play_button.set_action_name(Some("win.play"));
    let pause_button = Button::from_icon_name("media-playback-pause-symbolic");
    pause_button.set_action_name(Some("win.pause"));
    pause_button.set_visible(false);

    let window = ApplicationWindow::builder()
        .application(app)
        .title(&gettext("Listen.moe Radio"))
        .icon_name(APP_ID)
        .default_width(300)
        .default_height(40)
        .resizable(false)
        .build();

    window.add_css_class("cover-tint");
    let style_manager = StyleManager::default();
    style_manager.set_color_scheme(adw::ColorScheme::Default);
    let css_provider = install_css_provider();

    #[cfg(all(target_os = "linux", feature = "controls"))]
    let platform_config = PlatformConfig {
        display_name: "LISTEN.moe",
        dbus_name: APP_ID,
        #[cfg(target_os = "windows")]
        hwnd: window
            .surface()
            .and_then(|s| s.downcast::<gdk4_win32::Win32Surface>().ok())
            .map(|s| s.handle()),
        #[cfg(not(target_os = "windows"))]
        hwnd: None,
    };
    #[cfg(all(target_os = "linux", feature = "controls"))]
    let controls = MediaControls::new(platform_config).expect("Failed to init media controls");
    #[cfg(all(target_os = "linux", feature = "controls"))]
    let controls = Rc::new(RefCell::new(controls));
    #[cfg(all(target_os = "linux", feature = "controls"))]
    let (ctrl_tx, ctrl_rx) = mpsc::channel::<MediaControlEvent>();
    #[cfg(all(target_os = "linux", feature = "controls"))]
    {
        let tx = ctrl_tx.clone();
        controls
            .borrow_mut()
            .attach(move |event| {
                let _ = tx.send(event);
            })
            .expect("Failed to attach media control events");
    }
    window.add_action(&{
        let radio = radio.clone();
        let meta = meta.clone();
        let win = win_title.clone();
        let play = play_button.clone();
        let pause = pause_button.clone();
        #[cfg(all(target_os = "linux", feature = "controls"))]
        let controls = controls.clone();
        make_action("play", move || {
            win.set_title("LISTEN.moe");
            win.set_subtitle("Connecting...");
            meta.start();
            radio.start();
            play.set_visible(false);
            pause.set_visible(true);
            #[cfg(all(target_os = "linux", feature = "controls"))]
            let _ = controls
                .borrow_mut()
                .set_playback(MediaPlayback::Playing { progress: None });
        })
    });
    window.add_action(&{
        let radio = radio.clone();
        let meta = meta.clone();
        let win = win_title.clone();
        let play = play_button.clone();
        let pause = pause_button.clone();
        #[cfg(all(target_os = "linux", feature = "controls"))]
        let controls = controls.clone();
        make_action("pause", move || {
            meta.pause();
            radio.pause();
            pause.set_visible(false);
            play.set_visible(true);
            win.set_title("LISTEN.moe");
            win.set_subtitle(&gettext("JPOP/KPOP Radio"));
            #[cfg(all(target_os = "linux", feature = "controls"))]
            let _ = controls
                .borrow_mut()
                .set_playback(MediaPlayback::Paused { progress: None });
        })
    });
    window.add_action(&{
        let radio = radio.clone();
        let meta = meta.clone();
        let win = win_title.clone();
        let play = play_button.clone();
        let stop = pause_button.clone();
        #[cfg(all(target_os = "linux", feature = "controls"))]
        let controls = controls.clone();
        make_action("stop", move || {
            meta.stop();
            radio.stop();
            stop.set_visible(false);
            play.set_visible(true);
            win.set_title("LISTEN.moe");
            win.set_subtitle(&gettext("JPOP/KPOP Radio"));
            #[cfg(all(target_os = "linux", feature = "controls"))]
            let _ = controls
                .borrow_mut()
                .set_playback(MediaPlayback::Paused { progress: None });
        })
    });
    window.add_action(&{
        let win = window.clone();
        make_action("quit", move || win.close())
    });
    window.add_action(&{
        let win_clone = window.clone();
        make_action("about", move || {
            let authors: Vec<_> = env!("CARGO_PKG_AUTHORS").split(':').collect();
            let homepage = option_env!("CARGO_PKG_HOMEPAGE").unwrap_or("");
            let issues = format!("{}/issues", env!("CARGO_PKG_REPOSITORY"));
            let comments = metainfo_description().unwrap_or_else(|| gettext(env!("CARGO_PKG_DESCRIPTION")));
            let about = adw::AboutDialog::builder()
                .application_name("LISTEN.moe")
                .application_icon(APP_ID)
                .version(env!("CARGO_PKG_VERSION"))
                .developers(&authors[..])
                .translator_credits("AI translation (GPT-5.2); reviewed by Nick and Kana")
                .website(homepage)
                .issue_url(issues)
                .support_url(format!("{}discord", homepage))
                .license_type(gtk::License::MitX11)
                .comments(comments)
                .build();
            about.present(Some(&win_clone));
        })
    });
    window.add_action(&{
        let play = play_button.clone();
        let pause = pause_button.clone();
        let win_clone = window.clone();
        make_action("toggle", move || {
            if play.is_visible() {
                let _ = adw::prelude::WidgetExt::activate_action(
                    &win_clone,
                    "win.play",
                    None::<&glib::Variant>,
                );
            } else if pause.is_visible() {
                let _ = adw::prelude::WidgetExt::activate_action(
                    &win_clone,
                    "win.pause",
                    None::<&glib::Variant>,
                );
            }
        })
    });
    window.add_action(&{
        let win = win_title.clone();
        make_action("copy", move || {
            let artist = win.title();
            let title = win.subtitle();
            if artist.is_empty() && title.is_empty() {
                return;
            }
            let text = if artist.is_empty() {
                title.to_string()
            } else if title.is_empty() {
                artist.to_string()
            } else {
                format!("{artist}, {title}")
            };
            if let Some(display) = Display::default() {
                let clipboard = display.clipboard();
                clipboard.set_text(&text);
            }
        })
    });
    window.add_action(&{
        let radio = radio.clone();
        let meta = meta.clone();
        let win_clone = window.clone();
        let play = play_button.clone();
        make_action("next_station", move || {
            if play.is_visible() {
                let _ = adw::prelude::WidgetExt::activate_action(
                    &win_clone,
                    "win.play",
                    None::<&glib::Variant>,
                );
                return;
            }
            let current = radio.get_station();
            let next = other_station(current);
            radio.set_station(next);
            meta.set_station(next);
        })
    });
    window.add_action(&{
        let radio = radio.clone();
        let meta = meta.clone();
        let play = play_button.clone();
        make_action("prev_station", move || {
            if play.is_visible() {
                return; // paused -> do nothing
            }
            let current = radio.get_station();
            let prev = other_station(current);
            radio.set_station(prev);
            meta.set_station(prev);
        })
    });

    // Build UI
    let menu = Menu::new();
    menu.append(Some(&gettext("Copy title & artist")), Some("win.copy"));
    let more_button = MenuButton::builder()
        .icon_name("view-more-symbolic")
        .tooltip_text("Main Menu")
        .menu_model(&menu)
        .build();
    let buttons = gtk::Box::new(Orientation::Horizontal, 0);
    buttons.append(&more_button);
    buttons.append(&play_button);
    buttons.append(&pause_button);
    let header = HeaderBar::new();
    header.pack_start(&buttons);
    header.set_title_widget(Some(&win_title));
    header.set_show_title_buttons(false);
    header.add_css_class("cover-tint");

    let art_picture = Picture::builder()
        .can_shrink(true)
        .focusable(false)
        .sensitive(false)
        .build();
    let art_popover = Popover::builder()
        .has_arrow(true)
        .position(gtk::PositionType::Bottom)
        .autohide(true)
        .child(&art_picture)
        .build();
    art_popover.set_parent(&header);
    art_popover.add_css_class("cover-tint");
    let title_click = GestureClick::new();
    {
        let picture = art_picture.clone();
        let art = art_popover.clone();
        title_click.connect_released(move |_, _, _, _| {
            if art.is_visible() {
                art.popdown();
            } else if picture.paintable().is_some() {
                art.popup();
            }
        });
    }
    win_title.add_controller(title_click);
    let close_any_click = GestureClick::new();
    {
        let art = art_popover.clone();
        close_any_click.connect_released(move |_, _, _, _| {
            art.popdown();
        });
    }
    art_popover.add_controller(close_any_click);

    // Tiny dummy content so GTK can shrink the window
    let dummy = gtk::Box::new(Orientation::Vertical, 0);
    dummy.set_height_request(0);
    dummy.set_vexpand(false);

    let close_btn = Button::from_icon_name("window-close-symbolic");
    close_btn.set_action_name(Some("win.quit"));
    header.pack_end(&close_btn);

    window.set_titlebar(Some(&header));
    window.set_child(Some(&dummy));

    for station in [Station::Jpop, Station::Kpop] {
        let action = create_station_action(station, &play_button, &window, &radio, &meta);
        window.add_action(&action);
        menu.append(
            Some(
                gettext("Play %s")
                    .replace("%s", station.display_name())
                    .as_str(),
            ),
            Some(&format!("win.{}", station.name())),
        );
    }
    menu.append(Some(&gettext("About")), Some("win.about"));
    menu.append(Some(&gettext("Quit")), Some("win.quit"));

    app.set_accels_for_action("win.about", &["F1"]);
    app.set_accels_for_action("win.copy", &["<primary>c"]);
    app.set_accels_for_action("win.jpop", &["<primary>j"]);
    app.set_accels_for_action("win.kpop", &["<primary>k"]);
    app.set_accels_for_action("win.quit", &["<primary>q", "Escape"]);
    app.set_accels_for_action("win.prev_station", &["<primary>z", "XF86AudioPrev"]);
    app.set_accels_for_action(
        "win.next_station",
        &["<primary>y", "<primary><shift>z", "XF86AudioNext"],
    );
    app.set_accels_for_action(
        "win.toggle",
        &["<primary>p", "space", "Return", "<primary>s"],
    );
    app.set_accels_for_action("win.play", &["XF86AudioPlay"]);
    app.set_accels_for_action("win.stop", &["XF86AudioStop"]);
    app.set_accels_for_action("win.pause", &["XF86AudioPause"]);

    // Poll the channels on the GTK main thread and update the UI.
    {
        let win = win_title.clone();
        let art_popover = art_popover.clone();
        let art_picture = art_picture.clone();
        let cover_rx = cover_rx;
        let cover_tx = cover_tx.clone();
        let window = window.clone();
        let menu = more_button.clone();
        #[cfg(all(target_os = "linux", feature = "controls"))]
        let media_controls = controls.clone();
        #[cfg(all(target_os = "linux", feature = "controls"))]
        let ctrl_rx = ctrl_rx;
        glib::timeout_add_local(Duration::from_millis(100), move || {
            #[cfg(all(target_os = "linux", feature = "controls"))]
            for event in ctrl_rx.try_iter() {
                let _ = match event {
                    MediaControlEvent::Play => adw::prelude::WidgetExt::activate_action(&window, "win.play", None::<&glib::Variant>),
                    MediaControlEvent::Pause => adw::prelude::WidgetExt::activate_action(&win, "win.pause", None::<&glib::Variant>),
                    MediaControlEvent::Stop => adw::prelude::WidgetExt::activate_action(&win, "win.stop", None::<&glib::Variant>),
                    MediaControlEvent::Toggle => adw::prelude::WidgetExt::activate_action(&window, "win.toggle", None::<&glib::Variant>),
                    MediaControlEvent::Next => adw::prelude::WidgetExt::activate_action(&window, "win.next_station", None::<&glib::Variant>),
                    MediaControlEvent::Previous => adw::prelude::WidgetExt::activate_action(&window, "win.prev_station", None::<&glib::Variant>),
                    _ => Ok(())
                };
            }

            for info in rx.try_iter() {
                win.set_title(&info.artist);
                win.set_subtitle(&info.title);

                #[cfg(all(target_os = "linux", feature = "controls"))]
                let cover = info
                    .album_cover
                    .as_ref()
                    .or(info.artist_image.as_ref())
                    .map(|s| s.as_str());
                #[cfg(all(target_os = "linux", feature = "controls"))]
                let mut controls = media_controls.borrow_mut();
                #[cfg(all(target_os = "linux", feature = "controls"))]
                let _ = controls.set_metadata(MediaMetadata {
                    title: Some(&info.title),
                    artist: Some(&info.artist),
                    album: Some("LISTEN.moe"),
                    cover_url: cover,
                    ..Default::default()
                });

                if let Some(url) = info.album_cover.as_ref().or(info.artist_image.as_ref()) {
                    let tx = cover_tx.clone();
                    let url = url.to_string();
                    thread::spawn(move || {
                        let result = fetch_cover_bytes_blocking(&url).map_err(|e| e.to_string());
                        let _ = tx.send(result);
                    });
                } else {
                    art_popover.popdown();
                    style_manager.set_color_scheme(adw::ColorScheme::Default);
                    apply_cover_tint_css_clear(&css_provider);
                }
            }

            for result in cover_rx.try_iter() {
                match result {
                    Ok(bytes_vec) => {
                        let bytes = glib::Bytes::from_owned(bytes_vec);
                        let stream = MemoryInputStream::from_bytes(&bytes);
                        match Pixbuf::from_stream_at_scale(
                            &stream,
                            COVER_MAX_SIZE,
                            COVER_MAX_SIZE,
                            true,
                            None::<&Cancellable>,
                        ) {
                            Ok(pixbuf) => {
                                let texture = Texture::for_pixbuf(&pixbuf);
                                art_picture.set_paintable(Some(&texture));

                                // derive a strong, saturated color from the cover
                                let (r, g, b) = avg_rgb_from_pixbuf(&pixbuf);
                                let (r, g, b) = boost_saturation(r, g, b, 1.15);
                                let tint = (r, g, b);

                                // decide if that color is light or dark
                                let cover_is_light = is_light_color(r, g, b);

                                // force the app theme based on the cover (not system)
                                style_manager.set_color_scheme(if cover_is_light {
                                    adw::ColorScheme::ForceLight
                                } else {
                                    adw::ColorScheme::ForceDark
                                });

                                // paint header + popover using the exact tint color
                                apply_color(&css_provider, tint, cover_is_light);

                                let menu_popover = menu.popover().and_then(|w| w.downcast::<gtk::Popover>().ok());
                                let menu_open = menu_popover.as_ref().map(|p| p.is_visible()).unwrap_or(false);
                                if window.is_active() && !menu.has_focus() && !menu_open {
                                    art_popover.popup();
                                }
                            }
                            Err(err) => {
                                eprintln!("Failed to decode cover pixbuf: {err}");
                                art_popover.popdown();
                                style_manager.set_color_scheme(adw::ColorScheme::Default);
                                apply_cover_tint_css_clear(&css_provider);
                            }
                        }
                    }
                    Err(err) => {
                        eprintln!("Failed to load cover bytes: {err}");
                        art_popover.popdown();
                        style_manager.set_color_scheme(adw::ColorScheme::Default);
                        apply_cover_tint_css_clear(&css_provider);
                    }
                }
            }

            glib::ControlFlow::Continue
        });
    }

    window.present();
}

/// Download an image synchronously. This helper runs in a worker thread and
/// therefore does not block the GTK main loop. It returns the raw bytes
/// representing the image or an error if the request fails.
pub fn fetch_cover_bytes_blocking(url: &str) -> Result<Vec<u8>, Box<dyn Error + Send + Sync>> {
    let resp = reqwest::blocking::get(url)?;
    if !resp.status().is_success() {
        return Err(format!("Non-success status: {}", resp.status()).into());
    }
    let body = resp.bytes()?;
    Ok(body.to_vec())
}

fn install_css_provider() -> gtk::CssProvider {
    let provider = gtk::CssProvider::new();
    if let Some(display) = Display::default() {
        gtk::style_context_add_provider_for_display(
            &display,
            &provider,
            gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
        );
    }
    provider
}

fn avg_rgb_from_pixbuf(pixbuf: &Pixbuf) -> (u8, u8, u8) {
    let small = pixbuf
        .scale_simple(32, 32, Bilinear)
        .unwrap_or_else(|| pixbuf.clone());

    let w = small.width() as usize;
    let h = small.height() as usize;
    let n_channels = small.n_channels() as usize;
    let rowstride = small.rowstride() as usize;
    let has_alpha = small.has_alpha();
    let pixels = unsafe { small.pixels() };

    let mut r_sum: u64 = 0;
    let mut g_sum: u64 = 0;
    let mut b_sum: u64 = 0;
    let mut count: u64 = 0;

    for y in 0..h {
        let row = &pixels[y * rowstride..(y * rowstride + w * n_channels)];
        for x in 0..w {
            let i = x * n_channels;
            let r = row[i] as u64;
            let g = row[i + 1] as u64;
            let b = row[i + 2] as u64;

            if has_alpha {
                let a = row[i + 3] as u64;
                if a < 20 {
                    continue; // ignore near-transparent
                }
            }

            r_sum += r;
            g_sum += g;
            b_sum += b;
            count += 1;
        }
    }
    if count == 0 {
        return (128, 128, 128);
    }

    (
        (r_sum / count) as u8,
        (g_sum / count) as u8,
        (b_sum / count) as u8,
    )
}

fn apply_color(provider: &gtk::CssProvider, tint: (u8, u8, u8), tint_is_light: bool) {
    let (r, g, b) = tint;
    let (fr, fg, fb) = if tint_is_light {
        (0, 0, 0)
    } else {
        (255, 255, 255)
    };

    // slightly dim for backdrop
    let (br, bg, bb) = (
        (r as f32 * 0.92) as u8,
        (g as f32 * 0.92) as u8,
        (b as f32 * 0.92) as u8,
    );

    let css = format!(
        r#"
        headerbar.cover-tint {{
            background: rgb({r} {g} {b});
            color: rgb({fr} {fg} {fb});
        }}
        headerbar.cover-tint:backdrop {{
            background: rgb({br} {bg} {bb});
            color: rgb({fr} {fg} {fb});
        }}
        headerbar.cover-tint button {{
            color: inherit;
            background: transparent;
        }}
        headerbar.cover-tint button:hover {{
            background: rgba({fr} {fg} {fb} / 0.12);
        }}
        headerbar.cover-tint button:active {{
            background: rgba({fr} {fg} {fb} / 0.18);
        }}

        popover.cover-tint > contents {{
            background: rgb({r} {g} {b});
            color: rgb({fr} {fg} {fb});
            border-radius: 12px;
        }}
        popover.cover-tint > arrow {{
            background: rgb({r} {g} {b});
        }}
        "#
    );

    provider.load_from_data(&css);
}

fn is_light_color(r: u8, g: u8, b: u8) -> bool {
    let luma = 0.2126 * r as f32 + 0.7152 * g as f32 + 0.0722 * b as f32; // Relative luminance (approximate, sRGB)
    luma > 160.0 // Threshold tuned for UI backgrounds
}

fn boost_saturation(r: u8, g: u8, b: u8, amount: f32) -> (u8, u8, u8) {
    let gray = (r as f32 + g as f32 + b as f32) / 3.0;
    let boost = |c| (gray + (c as f32 - gray) * amount).clamp(0.0, 255.0) as u8;
    (boost(r), boost(g), boost(b))
}

fn apply_cover_tint_css_clear(provider: &gtk::CssProvider) {
    provider.load_from_data("");
}
