use std::{
    convert::{TryFrom, TryInto},
    sync::{Arc, Mutex},
    time::Duration,
};

use dbus::Path;
use dbus_crossroads::{Crossroads, IfaceBuilder};

use crate::{MediaControlEvent, MediaPlayback, MediaPosition, SeekDirection};

use super::controls::{create_metadata_dict, ServiceState};

// TODO: This type is super messed up, but it's the only way to get seeking working properly
// on graphical media controls using dbus-crossroads.
pub type SeekedSignal =
    Arc<Mutex<Option<Box<dyn Fn(&Path<'_>, &(String,)) -> dbus::Message + Send + Sync>>>>;

pub fn register_methods<F>(
    state: &Arc<Mutex<ServiceState>>,
    event_handler: &Arc<Mutex<F>>,
    friendly_name: String,
    seeked_signal: SeekedSignal,
) -> Crossroads
where
    F: Fn(MediaControlEvent) + Send + 'static,
{
    let mut cr = Crossroads::new();
    let app_interface = cr.register("org.mpris.MediaPlayer2", {
        let event_handler = event_handler.clone();

        move |b| {
            b.property("Identity")
                .get(move |_, _| Ok(friendly_name.clone()));

            register_method(b, &event_handler, "Raise", MediaControlEvent::Raise);
            register_method(b, &event_handler, "Quit", MediaControlEvent::Quit);

            // TODO: allow user to set these properties
            b.property("CanQuit")
                .get(|_, _| Ok(true))
                .emits_changed_true();
            b.property("CanRaise")
                .get(|_, _| Ok(true))
                .emits_changed_true();
            b.property("HasTracklist")
                .get(|_, _| Ok(false))
                .emits_changed_true();
            b.property("SupportedUriSchemes")
                .get(move |_, _| Ok(&[] as &[String]))
                .emits_changed_true();
            b.property("SupportedMimeTypes")
                .get(move |_, _| Ok(&[] as &[String]))
                .emits_changed_true();
        }
    });

    let player_interface = cr.register("org.mpris.MediaPlayer2.Player", |b| {
        register_method(b, event_handler, "Next", MediaControlEvent::Next);
        register_method(b, event_handler, "Previous", MediaControlEvent::Previous);
        register_method(b, event_handler, "Pause", MediaControlEvent::Pause);
        register_method(b, event_handler, "PlayPause", MediaControlEvent::Toggle);
        register_method(b, event_handler, "Stop", MediaControlEvent::Stop);
        register_method(b, event_handler, "Play", MediaControlEvent::Play);

        b.method("Seek", ("Offset",), (), {
            let event_handler = event_handler.clone();

            move |ctx, _, (offset,): (i64,)| {
                let abs_offset = offset.unsigned_abs();
                let direction = if offset > 0 {
                    SeekDirection::Forward
                } else {
                    SeekDirection::Backward
                };

                (event_handler.lock().unwrap())(MediaControlEvent::SeekBy(
                    direction,
                    Duration::from_micros(abs_offset),
                ));
                ctx.push_msg(ctx.make_signal("Seeked", ()));
                Ok(())
            }
        });

        b.method("SetPosition", ("TrackId", "Position"), (), {
            let state = state.clone();
            let event_handler = event_handler.clone();

            move |_, _, (_trackid, position): (Path, i64)| {
                let state = state.lock().unwrap();

                // According to the MPRIS specification:

                // TODO: If the TrackId argument is not the same as the current
                // trackid, the call is ignored as stale.
                // (Maybe it should be optional?)

                if let Some(duration) = state.metadata.duration {
                    // If the Position argument is greater than the track length, do nothing.
                    if position > duration {
                        return Ok(());
                    }
                }

                // If the Position argument is less than 0, do nothing.
                if let Ok(position) = u64::try_from(position) {
                    let position = Duration::from_micros(position);

                    (event_handler.lock().unwrap())(MediaControlEvent::SetPosition(MediaPosition(
                        position,
                    )));
                }
                Ok(())
            }
        });

        b.method("OpenUri", ("Uri",), (), {
            let event_handler = event_handler.clone();

            move |_, _, (uri,): (String,)| {
                (event_handler.lock().unwrap())(MediaControlEvent::OpenUri(uri));
                Ok(())
            }
        });

        *seeked_signal.lock().unwrap() = Some(b.signal::<(String,), _>("Seeked", ("x",)).msg_fn());

        b.property("PlaybackStatus")
            .get({
                let state = state.clone();
                move |_, _| {
                    let state = state.lock().unwrap();
                    Ok(state.get_playback_status().to_string())
                }
            })
            .emits_changed_true();

        b.property("Rate").get(|_, _| Ok(1.0)).emits_changed_true();

        b.property("Metadata")
            .get({
                let state = state.clone();
                move |_, _| Ok(create_metadata_dict(&state.lock().unwrap().metadata))
            })
            .emits_changed_true();

        b.property("Volume")
            .get({
                let state = state.clone();
                move |_, _| {
                    let state = state.lock().unwrap();
                    Ok(state.volume)
                }
            })
            .set({
                let event_handler = event_handler.clone();
                move |_, _, volume: f64| {
                    (event_handler.lock().unwrap())(MediaControlEvent::SetVolume(volume));
                    Ok(Some(volume))
                }
            })
            .emits_changed_true();

        b.property("Position").get({
            let state = state.clone();
            move |_, _| {
                let state = state.lock().unwrap();
                let progress: i64 = match state.playback_status {
                    MediaPlayback::Playing {
                        progress: Some(progress),
                    }
                    | MediaPlayback::Paused {
                        progress: Some(progress),
                    } => progress.0.as_micros(),
                    _ => 0,
                }
                .try_into()
                .unwrap();
                Ok(progress)
            }
        });

        b.property("MinimumRate")
            .get(|_, _| Ok(1.0))
            .emits_changed_true();
        b.property("MaximumRate")
            .get(|_, _| Ok(1.0))
            .emits_changed_true();

        b.property("CanGoNext")
            .get(|_, _| Ok(true))
            .emits_changed_true();
        b.property("CanGoPrevious")
            .get(|_, _| Ok(true))
            .emits_changed_true();
        b.property("CanPlay")
            .get(|_, _| Ok(true))
            .emits_changed_true();
        b.property("CanPause")
            .get(|_, _| Ok(true))
            .emits_changed_true();
        b.property("CanSeek")
            .get(|_, _| Ok(true))
            .emits_changed_true();
        b.property("CanControl")
            .get(|_, _| Ok(true))
            .emits_changed_true();
    });

    cr.insert(
        "/org/mpris/MediaPlayer2",
        &[app_interface, player_interface],
        (),
    );

    seeked_signal.lock().ok();

    cr
}

fn register_method<F>(
    b: &mut IfaceBuilder<()>,
    event_handler: &Arc<Mutex<F>>,
    name: &'static str,
    event: MediaControlEvent,
) where
    F: Fn(MediaControlEvent) + Send + 'static,
{
    let event_handler = event_handler.clone();

    b.method(name, (), (), move |_, _, _: ()| {
        (event_handler.lock().unwrap())(event.clone());
        Ok(())
    });
}
