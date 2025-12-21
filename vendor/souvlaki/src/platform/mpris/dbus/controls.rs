use dbus::arg::{RefArg, Variant};
use dbus::blocking::Connection;
use dbus::channel::{MatchingReceiver, Sender};
use dbus::ffidisp::stdintf::org_freedesktop_dbus::PropertiesPropertiesChanged;
use dbus::message::SignalArgs;
use dbus::Path;
use std::collections::HashMap;
use std::convert::From;
use std::convert::TryInto;
use std::sync::{mpsc, Arc, Mutex};
use std::thread::{self, JoinHandle};
use std::time::Duration;

use super::super::Error;
use crate::{MediaControlEvent, MediaMetadata, MediaPlayback, PlatformConfig};

/// A handle to OS media controls.
pub struct MediaControls {
    thread: Option<ServiceThreadHandle>,
    dbus_name: String,
    friendly_name: String,
}

struct ServiceThreadHandle {
    event_channel: mpsc::Sender<InternalEvent>,
    thread: JoinHandle<Result<(), Error>>,
}

#[derive(Clone, PartialEq, Debug)]
enum InternalEvent {
    ChangeMetadata(OwnedMetadata),
    ChangePlayback(MediaPlayback),
    ChangeVolume(f64),
    Kill,
}

#[derive(Debug)]
pub struct ServiceState {
    pub metadata: OwnedMetadata,
    pub metadata_dict: HashMap<String, Variant<Box<dyn RefArg>>>,
    pub playback_status: MediaPlayback,
    pub volume: f64,
}

impl ServiceState {
    pub fn set_metadata(&mut self, metadata: OwnedMetadata) {
        self.metadata_dict = create_metadata_dict(&metadata);
        self.metadata = metadata;
    }

    pub fn get_playback_status(&self) -> &'static str {
        match self.playback_status {
            MediaPlayback::Playing { .. } => "Playing",
            MediaPlayback::Paused { .. } => "Paused",
            MediaPlayback::Stopped => "Stopped",
        }
    }
}

pub fn create_metadata_dict(metadata: &OwnedMetadata) -> HashMap<String, Variant<Box<dyn RefArg>>> {
    let mut dict = HashMap::<String, Variant<Box<dyn RefArg>>>::new();

    let mut insert = |k: &str, v| dict.insert(k.to_string(), Variant(v));

    let OwnedMetadata {
        ref title,
        ref album,
        ref artist,
        ref cover_url,
        ref duration,
    } = metadata;

    // TODO: this is just a workaround to enable SetPosition.
    let path = Path::new("/").unwrap();

    // MPRIS
    insert("mpris:trackid", Box::new(path));

    if let Some(length) = duration {
        insert("mpris:length", Box::new(*length));
    }
    if let Some(cover_url) = cover_url {
        insert("mpris:artUrl", Box::new(cover_url.clone()));
    }

    // Xesam
    if let Some(title) = title {
        insert("xesam:title", Box::new(title.clone()));
    }
    if let Some(artist) = artist {
        insert("xesam:artist", Box::new(vec![artist.clone()]));
    }
    if let Some(album) = album {
        insert("xesam:album", Box::new(album.clone()));
    }

    dict
}

#[derive(Clone, PartialEq, Eq, Debug, Default)]
pub struct OwnedMetadata {
    pub title: Option<String>,
    pub album: Option<String>,
    pub artist: Option<String>,
    pub cover_url: Option<String>,
    pub duration: Option<i64>,
}

impl From<MediaMetadata<'_>> for OwnedMetadata {
    fn from(other: MediaMetadata) -> Self {
        OwnedMetadata {
            title: other.title.map(|s| s.to_string()),
            artist: other.artist.map(|s| s.to_string()),
            album: other.album.map(|s| s.to_string()),
            cover_url: other.cover_url.map(|s| s.to_string()),
            // TODO: This should probably not have an unwrap
            duration: other.duration.map(|d| d.as_micros().try_into().unwrap()),
        }
    }
}

impl MediaControls {
    /// Create media controls with the specified config.
    pub fn new(config: PlatformConfig) -> Result<Self, Error> {
        let PlatformConfig {
            dbus_name,
            display_name,
            ..
        } = config;

        Ok(Self {
            thread: None,
            dbus_name: dbus_name.to_string(),
            friendly_name: display_name.to_string(),
        })
    }

    /// Attach the media control events to a handler.
    pub fn attach<F>(&mut self, event_handler: F) -> Result<(), Error>
    where
        F: Fn(MediaControlEvent) + Send + 'static,
    {
        self.detach()?;

        let dbus_name = self.dbus_name.clone();
        let friendly_name = self.friendly_name.clone();
        let (event_channel, rx) = mpsc::channel();

        // Check if the connection can be created BEFORE spawning the new thread
        let conn = Connection::new_session()?;
        let name = format!("org.mpris.MediaPlayer2.{}", dbus_name);
        conn.request_name(name, false, true, false)?;

        self.thread = Some(ServiceThreadHandle {
            event_channel,
            thread: thread::spawn(move || run_service(conn, friendly_name, event_handler, rx)),
        });
        Ok(())
    }

    /// Detach the event handler.
    pub fn detach(&mut self) -> Result<(), Error> {
        if let Some(ServiceThreadHandle {
            event_channel,
            thread,
        }) = self.thread.take()
        {
            // We don't care about the result of this event, since we immedieately
            // check if the thread has panicked on the next line.
            event_channel.send(InternalEvent::Kill).ok();
            // One error in case the thread panics, and the other one in case the
            // thread has returned an error.
            thread.join().map_err(|_| Error::ThreadPanicked)??;
        }
        Ok(())
    }

    /// Set the current playback status.
    pub fn set_playback(&mut self, playback: MediaPlayback) -> Result<(), Error> {
        self.send_internal_event(InternalEvent::ChangePlayback(playback))
    }

    /// Set the metadata of the currently playing media item.
    pub fn set_metadata(&mut self, metadata: MediaMetadata) -> Result<(), Error> {
        self.send_internal_event(InternalEvent::ChangeMetadata(metadata.into()))
    }

    /// Set the volume level (0.0-1.0) (Only available on MPRIS)
    pub fn set_volume(&mut self, volume: f64) -> Result<(), Error> {
        self.send_internal_event(InternalEvent::ChangeVolume(volume))
    }

    fn send_internal_event(&mut self, event: InternalEvent) -> Result<(), Error> {
        let thread = &self.thread.as_ref().ok_or(Error::ThreadNotRunning)?;
        thread
            .event_channel
            .send(event)
            .map_err(|_| Error::ThreadPanicked)
    }
}

fn run_service<F>(
    conn: Connection,
    friendly_name: String,
    event_handler: F,
    event_channel: mpsc::Receiver<InternalEvent>,
) -> Result<(), Error>
where
    F: Fn(MediaControlEvent) + Send + 'static,
{
    let state = Arc::new(Mutex::new(ServiceState {
        metadata: Default::default(),
        metadata_dict: create_metadata_dict(&Default::default()),
        playback_status: MediaPlayback::Stopped,
        volume: 1.0,
    }));
    let event_handler = Arc::new(Mutex::new(event_handler));
    let seeked_signal = Arc::new(Mutex::new(None));

    let mut cr =
        super::interfaces::register_methods(&state, &event_handler, friendly_name, seeked_signal);

    conn.start_receive(
        dbus::message::MatchRule::new_method_call(),
        Box::new(move |msg, conn| {
            cr.handle_message(msg, conn).unwrap();
            true
        }),
    );

    loop {
        if let Ok(event) = event_channel.recv_timeout(Duration::from_millis(10)) {
            if event == InternalEvent::Kill {
                break;
            }

            let mut changed_properties = HashMap::new();

            match event {
                InternalEvent::ChangeMetadata(metadata) => {
                    let mut state = state.lock().unwrap();
                    state.set_metadata(metadata);
                    changed_properties.insert(
                        "Metadata".to_owned(),
                        Variant(state.metadata_dict.box_clone()),
                    );
                }
                InternalEvent::ChangePlayback(playback) => {
                    let mut state = state.lock().unwrap();
                    state.playback_status = playback;
                    changed_properties.insert(
                        "PlaybackStatus".to_owned(),
                        Variant(Box::new(state.get_playback_status().to_string())),
                    );
                }
                InternalEvent::ChangeVolume(volume) => {
                    let mut state = state.lock().unwrap();
                    state.volume = volume;
                    changed_properties.insert("Volume".to_owned(), Variant(Box::new(volume)));
                }
                _ => (),
            }

            let properties_changed = PropertiesPropertiesChanged {
                interface_name: "org.mpris.MediaPlayer2.Player".to_owned(),
                changed_properties,
                invalidated_properties: Vec::new(),
            };

            conn.send(
                properties_changed.to_emit_message(&Path::new("/org/mpris/MediaPlayer2").unwrap()),
            )
            .ok();
        }
        conn.process(Duration::from_millis(1000))?;
    }

    Ok(())
}
