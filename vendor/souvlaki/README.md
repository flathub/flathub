

# Souvlaki [![Crates.io](https://img.shields.io/crates/v/souvlaki.svg)](https://crates.io/crates/souvlaki) [![Docs](https://docs.rs/souvlaki/badge.svg)](https://docs.rs/souvlaki) [![CI](https://github.com/Sinono3/souvlaki/actions/workflows/build.yml/badge.svg)](https://github.com/Sinono3/souvlaki/actions/workflows/build.yml)

A cross-platform library for handling OS media controls and metadata. One abstraction for Linux, MacOS/iOS, Windows.

## Supported platforms

- Linux (via MPRIS)
- MacOS/iOS
- Windows

## Windows

- Update metadata:\
![image](https://user-images.githubusercontent.com/8389938/106080661-4a515e80-60f6-11eb-81e0-81ab0eda5188.png)
- Play and pause polling.\
![play_pause](https://user-images.githubusercontent.com/8389938/106080917-bdf36b80-60f6-11eb-98b5-f3071ae3eab6.gif)

## Linux

- GNOME: \
![GNOME](https://user-images.githubusercontent.com/59307989/150836249-3270c4fc-78b9-4b8d-8d50-dd030b72b631.png)

- playerctl:
```shell
# In one shell
$ cd souvlaki 
$ cargo run --example window

# In another shell
$ playerctl metadata
my_player xesam:artist              Slowdive
my_player xesam:album               Souvlaki
my_player mpris:artUrl              https://c.pxhere.com/photos/34/c1/souvlaki_authentic_greek_greek_food_mezes-497780.jpg!d
my_player mpris:trackid             '/'
my_player mpris:length              290000000
my_player xesam:title               When The Sun Hits
```

## MacOS

- Control Center:\
![Control Center](https://user-images.githubusercontent.com/434125/171526539-ecb07a74-5dc5-4f4b-8305-4a99d4d5c31c.png)
- Now Playing:\
![Now Playing](https://user-images.githubusercontent.com/434125/171526759-9232be58-63ed-4eea-ac15-aa50258d8254.png)

## Requirements

Minimum supported Rust version is 1.67.

## Usage

The main struct is `MediaControls`. In order to create this struct you need a `PlatformConfig`. This struct contains all of the platform-specific requirements for spawning media controls. Here are the differences between the platforms:

- MacOS: No config needed, but requires an AppDelegate/winit event loop (an open window is not required.) ([#23](https://github.com/Sinono3/souvlaki/issues/23))
- iOS: No config needed.
- Linux: 
	- `dbus_name`: The way your player will appear on D-Bus. It should follow [the D-Bus specification](https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names-bus). 
	- `display_name`: This could be however you want. It's the name that will be shown to the users.
- Windows: 
	- `hwnd`: In this platform, a window needs to be opened to create media controls. The argument required is an `HWND`, a value of type `*mut c_void`. This value can be extracted when you open a window in your program, for example using the `raw_window_handle` in winit.

### Linux backends: D-Bus and `zbus`

When using the library on Linux, the default backend is `dbus-crossroads`. This backend has some issues with consistency in general, but is more stable and uses the native D-Bus library behind the scenes. The zbus backend however, is more modern and is written in pure Rust. It spawns another thread and stars an async `pollster` runtime, handling the incoming MPRIS messages. 

To enable the zbus backend, in your Cargo.toml, set `default-features` to false and enable the `use_zbus` feature:

```toml
souvlaki = { version = "<version>", default-features = false, features = ["use_zbus"] }
```


**Note:** If you think there's a better way of using the zbus library regarding the async runtime in another thread, feel free to leave a PR or issue.

## Example

```rust
use souvlaki::{MediaControlEvent, MediaControls, MediaMetadata, PlatformConfig};

fn main() {
    #[cfg(not(target_os = "windows"))]
    let hwnd = None;

    #[cfg(target_os = "windows")]
    let hwnd = {
        use raw_window_handle::windows::WindowsHandle;

        let handle: WindowsHandle = unimplemented!();
        Some(handle.hwnd)
    };

    let config = PlatformConfig {
        dbus_name: "my_player",
        display_name: "My Player",
        hwnd,
    };

    let mut controls = MediaControls::new(config).unwrap();

    // The closure must be Send and have a static lifetime.
    controls
        .attach(|event: MediaControlEvent| println!("Event received: {:?}", event))
        .unwrap();

    // Update the media metadata.
    controls
        .set_metadata(MediaMetadata {
            title: Some("Souvlaki Space Station"),
            artist: Some("Slowdive"),
            album: Some("Souvlaki"),
            ..Default::default()
        })
        .unwrap();

    // Your actual logic goes here.
    loop {
        std::thread::sleep(std::time::Duration::from_secs(1));
    }

    // The controls automatically detach on drop.
}
```

[Check out this example here.](https://github.com/Sinono3/souvlaki/blob/master/examples/print_events.rs)

## Thanks 💗

- To [jpochyla](https://github.com/jpochyla) for being a contributor to library architecture and the sole developer of MacOS support.
