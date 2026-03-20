use openkara_lib::commands::airplay::{
    normalize_host_y, AirPlayAudienceMode, AIRPLAY_OUTPUT_STATE_EVENT,
};

#[test]
fn airplay_output_event_name_is_stable() {
    assert_eq!(AIRPLAY_OUTPUT_STATE_EVENT, "openkara://airplay-output-state");
}

#[test]
fn host_y_normalization_flips_dom_coordinates_into_appkit_space() {
    assert_eq!(normalize_host_y(360.0, 24.0, 32.0), 304.0);
}

#[test]
fn airplay_audience_mode_serializes_expected_variants() {
    assert_eq!(serde_json::to_string(&AirPlayAudienceMode::Idle).unwrap(), "\"idle\"");
    assert_eq!(
        serde_json::to_string(&AirPlayAudienceMode::Lyrics).unwrap(),
        "\"lyrics\""
    );
    assert_eq!(serde_json::to_string(&AirPlayAudienceMode::Cdg).unwrap(), "\"cdg\"");
}
