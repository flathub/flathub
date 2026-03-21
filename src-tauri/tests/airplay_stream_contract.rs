use openkara_lib::commands::airplay::{
    AirPlayAudienceMessages, AirPlayAudienceMode, AirPlayAudienceStatePayload,
    AudiencePresentationSpec, AirPlayViewport,
};

#[test]
fn audience_payload_deserializes_viewport() {
    let payload = serde_json::from_value::<AirPlayAudienceStatePayload>(serde_json::json!({
        "mode": "lyrics",
        "songId": "song-1",
        "lines": [],
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
        },
        "presentationSpec": {
            "contentWidthRatio": 0.92,
            "contentMaxWidthPx": 1600,
            "horizontalPaddingPx": 64,
            "verticalPaddingPx": 56,
            "lineGapPx": 40,
            "fontSizePx": 96,
            "lineHeightMultiple": 1.08,
            "activeScale": 1.05,
            "statusFontSizePx": 18,
            "activeGlowBlurPx": 12,
            "activeTextColor": { "red": 1.0, "green": 1.0, "blue": 1.0, "alpha": 1.0 },
            "pastTextColor": { "red": 0.2823529411764706, "green": 0.2823529411764706, "blue": 0.2901960784313726, "alpha": 1.0 },
            "futureTextColor": { "red": 0.22745098039215686, "green": 0.22745098039215686, "blue": 0.23529411764705882, "alpha": 1.0 },
            "plainTextColor": { "red": 1.0, "green": 1.0, "blue": 1.0, "alpha": 1.0 },
            "statusTextColor": { "red": 0.5568627450980392, "green": 0.5568627450980392, "blue": 0.5764705882352941, "alpha": 1.0 },
            "activeGlowColor": { "red": 1.0, "green": 1.0, "blue": 1.0, "alpha": 0.8 }
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
        payload.presentation_spec,
        AudiencePresentationSpec {
            font_size_px: 96,
            ..AudiencePresentationSpec::default()
        }
    );
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
