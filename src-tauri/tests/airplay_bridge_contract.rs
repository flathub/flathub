use openkara_lib::commands::airplay::{
    normalize_host_y, AirPlayAudienceMode, AirPlayOutputPhase, AirPlayOutputStateEvent,
    AirPlayPlainTextPageDirection, AIRPLAY_OUTPUT_STATE_EVENT,
};

#[test]
fn airplay_output_event_name_is_stable() {
    assert_eq!(
        AIRPLAY_OUTPUT_STATE_EVENT,
        "openkara://airplay-output-state"
    );
}

#[test]
fn host_y_normalization_flips_dom_coordinates_into_appkit_space() {
    assert_eq!(normalize_host_y(360.0, 24.0, 32.0), 304.0);
}

#[test]
fn airplay_audience_mode_serializes_expected_variants() {
    assert_eq!(
        serde_json::to_string(&AirPlayAudienceMode::Idle).unwrap(),
        "\"idle\""
    );
    assert_eq!(
        serde_json::to_string(&AirPlayAudienceMode::Lyrics).unwrap(),
        "\"lyrics\""
    );
    assert_eq!(
        serde_json::to_string(&AirPlayAudienceMode::Cdg).unwrap(),
        "\"cdg\""
    );
}

#[test]
fn airplay_output_phase_serializes_expected_variants() {
    assert_eq!(
        serde_json::to_string(&AirPlayOutputPhase::Idle).unwrap(),
        "\"idle\""
    );
    assert_eq!(
        serde_json::to_string(&AirPlayOutputPhase::RouteSelected).unwrap(),
        "\"route_selected\""
    );
    assert_eq!(
        serde_json::to_string(&AirPlayOutputPhase::Buffering).unwrap(),
        "\"buffering\""
    );
    assert_eq!(
        serde_json::to_string(&AirPlayOutputPhase::Playing).unwrap(),
        "\"playing\""
    );
    assert_eq!(
        serde_json::to_string(&AirPlayOutputPhase::Failed).unwrap(),
        "\"failed\""
    );
}

#[test]
fn airplay_output_event_serializes_phase_and_detail() {
    let json = serde_json::to_value(AirPlayOutputStateEvent {
        active: false,
        audio_active: true,
        route_name: Some("Bedroom TV".to_owned()),
        mode: AirPlayAudienceMode::Lyrics,
        phase: AirPlayOutputPhase::Buffering,
        detail: Some("waiting_for_audio".to_owned()),
        displayed_position_ms: Some(1_250),
        stream_generation: 7,
        latency_ms: Some(420),
    })
    .unwrap();

    assert_eq!(json["active"], false);
    assert_eq!(json["audioActive"], true);
    assert_eq!(json["routeName"], "Bedroom TV");
    assert_eq!(json["mode"], "lyrics");
    assert_eq!(json["phase"], "buffering");
    assert_eq!(json["detail"], "waiting_for_audio");
    assert_eq!(json["displayedPositionMs"], 1_250);
    assert_eq!(json["streamGeneration"], 7);
    assert_eq!(json["latencyMs"], 420);
}

#[test]
fn airplay_plain_text_page_direction_deserializes_expected_variant() {
    let direction =
        serde_json::from_value::<AirPlayPlainTextPageDirection>(serde_json::json!("next"))
            .unwrap();

    assert_eq!(direction, AirPlayPlainTextPageDirection::Next);
}
