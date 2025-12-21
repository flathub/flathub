use std::{sync::mpsc, thread::sleep, time::Duration};

use souvlaki::{MediaControlEvent, MediaControls, MediaMetadata, MediaPlayback, PlatformConfig};
use winit::{
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

struct TestApp {
    playing: bool,
    song_index: u8,
}

fn main() {
    let event_loop = EventLoop::new();
    #[allow(unused_variables)]
    let window = WindowBuilder::new().build(&event_loop).unwrap();

    #[cfg(not(target_os = "windows"))]
    let hwnd = None;

    #[cfg(target_os = "windows")]
    let hwnd = {
        use raw_window_handle::{HasRawWindowHandle, RawWindowHandle};

        let handle = match window.raw_window_handle() {
            RawWindowHandle::Win32(h) => h,
            _ => unreachable!(),
        };
        Some(handle.hwnd)
    };

    let config = PlatformConfig {
        dbus_name: "my_player",
        display_name: "My Player",
        hwnd,
    };

    let mut controls = MediaControls::new(config).unwrap();

    let (tx, rx) = mpsc::sync_channel(32);
    let mut app = TestApp {
        playing: true,
        song_index: 0,
    };

    controls.attach(move |e| tx.send(e).unwrap()).unwrap();
    controls
        .set_playback(MediaPlayback::Playing { progress: None })
        .unwrap();
    controls
        .set_metadata(MediaMetadata {
            title: Some("When The Sun Hits"),
            album: Some("Souvlaki"),
            artist: Some("Slowdive"),
            duration: Some(Duration::from_secs_f64(4.0 * 60.0 + 50.0)),
            cover_url: Some("https://c.pxhere.com/photos/34/c1/souvlaki_authentic_greek_greek_food_mezes-497780.jpg!d"),
        })
        .unwrap();

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => *control_flow = ControlFlow::Exit,
            Event::MainEventsCleared => {
                let mut change = false;

                for event in rx.try_iter() {
                    match event {
                        MediaControlEvent::Toggle => app.playing = !app.playing,
                        MediaControlEvent::Play => app.playing = true,
                        MediaControlEvent::Pause => app.playing = false,
                        MediaControlEvent::Next => app.song_index = app.song_index.wrapping_add(1),
                        MediaControlEvent::Previous => {
                            app.song_index = app.song_index.wrapping_sub(1)
                        }
                        MediaControlEvent::Stop => app.playing = false,
                        _ => (),
                    }
                    change = true;
                }
                sleep(Duration::from_millis(50));

                if change {
                    controls
                        .set_playback(if app.playing {
                            MediaPlayback::Playing { progress: None }
                        } else {
                            MediaPlayback::Paused { progress: None }
                        })
                        .unwrap();

                    eprintln!(
                        "{} (song {})",
                        if app.playing { "Playing" } else { "Paused" },
                        app.song_index
                    );
                }
            }
            _ => (),
        }
    });
}
