#![doc = include_str!("../README.md")]

mod config;
mod platform;

use std::{fmt::Debug, time::Duration};

pub use config::*;
pub use platform::{Error, MediaControls};

/// The status of media playback.
#[derive(Clone, PartialEq, Eq, Debug)]
pub enum MediaPlayback {
    Stopped,
    Paused { progress: Option<MediaPosition> },
    Playing { progress: Option<MediaPosition> },
}

/// The metadata of a media item.
#[derive(Clone, PartialEq, Eq, Debug, Default)]
pub struct MediaMetadata<'a> {
    pub title: Option<&'a str>,
    pub album: Option<&'a str>,
    pub artist: Option<&'a str>,
    /// Very platform specific. As of now, Souvlaki leaves it up to the user to change the URL depending on the platform.
    ///
    /// For Linux, we follow the MPRIS specification, which actually doesn't say much cover art apart from what's in [here](https://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata/#mpris:arturl). It only says that local files should start with `file://` and that it should be an UTF-8 string, which is enforced by Rust. Maybe you can look in the source code of desktop managers such as GNOME or KDE, since these read the field to display it on their media player controls.
    ///
    /// For Windows, we use the _SystemMediaTransportControlsDisplayUpdater_, which has [a thumbnail property](https://learn.microsoft.com/en-us/uwp/api/windows.media.systemmediatransportcontrolsdisplayupdater.thumbnail?view=winrt-22621#windows-media-systemmediatransportcontrolsdisplayupdater-thumbnail). It accepts multiple formats, but we choose to create it using an URI. If setting an URL starting with `file://`, the file is automatically loaded by souvlaki.
    ///
    /// For MacOS, you can look into [these lines](https://github.com/Sinono3/souvlaki/blob/384539fe83e8bf5c966192ba28e9405e3253619b/src/platform/macos/mod.rs#L131-L137) of the implementation. These lines refer to creating an [MPMediaItemArtwork](https://developer.apple.com/documentation/mediaplayer/mpmediaitemartwork) object.
    pub cover_url: Option<&'a str>,
    pub duration: Option<Duration>,
}

/// Events sent by the OS media controls.
#[derive(Clone, PartialEq, Debug)]
pub enum MediaControlEvent {
    Play,
    Pause,
    Toggle,
    Next,
    Previous,
    Stop,

    /// Seek forward or backward by an undetermined amount.
    Seek(SeekDirection),
    /// Seek forward or backward by a certain amount.
    SeekBy(SeekDirection, Duration),
    /// Set the position/progress of the currently playing media item.
    SetPosition(MediaPosition),
    /// Sets the volume. The value is intended to be from 0.0 to 1.0.
    /// But other values are also accepted. **It is up to the user to
    /// set constraints on this value.**
    /// **NOTE**: If the volume event was received and correctly handled,
    /// the user must call [`MediaControls::set_volume`]. Note that
    /// this must be done only with the MPRIS backend.
    SetVolume(f64),
    /// Open the URI in the media player.
    OpenUri(String),

    /// Bring the media player's user interface to the front using any appropriate mechanism available.
    Raise,
    /// Shut down the media player.
    Quit,
}

/// An instant in a media item.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct MediaPosition(pub Duration);

/// The direction to seek in.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum SeekDirection {
    Forward,
    Backward,
}

impl Drop for MediaControls {
    fn drop(&mut self) {
        // Ignores errors if there are any.
        self.detach().ok();
    }
}

impl Debug for MediaControls {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("MediaControls")?;
        Ok(())
    }
}
