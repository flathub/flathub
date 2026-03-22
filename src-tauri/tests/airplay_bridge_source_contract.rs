#[test]
fn airplay_bridge_snapshots_state_instead_of_reading_runtime_properties_on_encoder_queue() {
    let source = include_str!("../src/macos/airplay_bridge.m");

    assert!(
        source.contains("- (NSDictionary *)stateSnapshot"),
        "airplay bridge should expose a state snapshot helper"
    );
    assert!(
        !source.contains("NSDictionary *config = self.latestSceneConfig ?: @{};"),
        "encoder-side scene rendering should not read latestSceneConfig directly"
    );
    assert!(
        !source.contains("NSDictionary *runtime = self.latestRuntimeState ?: @{};"),
        "encoder-side scene rendering should not read latestRuntimeState directly"
    );
}

#[test]
fn airplay_bridge_keeps_pending_audio_mutations_on_audio_queue() {
    let source = include_str!("../src/macos/airplay_bridge.m");

    assert!(
        !source.contains("[self.pendingAudienceAudioData setLength:0];\n    self.pendingAudienceAudioOffset = 0;\n    [self resetOutputClockTracking];"),
        "resetMediaPipelineForGeneration should not mutate pending audio state directly from encoder queue"
    );
}
