use openkara_lib::commands::airplay::{
    AirPlayAudienceMessages, AirPlayAudienceMode, AirPlayAudienceStatePayload, AirPlayViewport,
};

#[test]
fn audience_payload_deserializes_viewport() {
    let payload = serde_json::from_value::<AirPlayAudienceStatePayload>(serde_json::json!({
        "mode": "lyrics",
        "songId": "song-1",
        "isPlaying": true,
        "positionMs": 1200,
        "lines": [],
        "activeLineIndex": 0,
        "offsetMs": 100,
        "isLoading": false,
        "lyricsFontStep": 1,
        "messages": {
            "selectSong": "Select a song to start",
            "loadingLyrics": "Loading lyrics...",
            "noLyrics": "No lyrics available for this track",
            "addLyrics": "Add Lyrics"
        },
        "viewport": {
            "widthPx": 1280,
            "heightPx": 720,
            "bottomInsetPx": 0
        }
    }))
    .expect("payload should deserialize");

    assert_eq!(payload.mode, AirPlayAudienceMode::Lyrics);
    assert_eq!(payload.song_id.as_deref(), Some("song-1"));
    assert_eq!(
        payload.viewport,
        AirPlayViewport {
            width_px: 1280,
            height_px: 720,
            bottom_inset_px: 0,
        }
    );
    assert_eq!(payload.is_loading, false);
    assert_eq!(
        payload.messages,
        AirPlayAudienceMessages {
            select_song: "Select a song to start".to_owned(),
            loading_lyrics: "Loading lyrics...".to_owned(),
            no_lyrics: "No lyrics available for this track".to_owned(),
            add_lyrics: "Add Lyrics".to_owned(),
        }
    );
}
