use openkara_lib::airplay_stream::{AirPlayAudioTap, AirPlayHttpServer};

#[test]
fn audio_tap_bumps_epoch_and_drops_stale_chunks() {
    let tap = AirPlayAudioTap::new(2);

    tap.push_interleaved(44_100, 2, &[0.1, 0.2, 0.3, 0.4]);
    let first_epoch = tap.bump_epoch();
    tap.push_interleaved(44_100, 2, &[0.5, 0.6, 0.7, 0.8]);

    let drained = tap.drain_newer_than(first_epoch.saturating_sub(1));

    assert_eq!(first_epoch, 2);
    assert_eq!(drained.len(), 1);
    assert_eq!(drained[0].epoch, 2);
    assert_eq!(drained[0].samples, vec![0.5, 0.6, 0.7, 0.8]);
}

#[test]
fn audio_tap_respects_bounded_capacity() {
    let tap = AirPlayAudioTap::new(2);

    tap.push_interleaved(44_100, 2, &[0.1, 0.2]);
    tap.push_interleaved(44_100, 2, &[0.3, 0.4]);
    tap.push_interleaved(44_100, 2, &[0.5, 0.6]);

    let drained = tap.drain_newer_than(0);

    assert_eq!(drained.len(), 2);
    assert_eq!(drained[0].samples, vec![0.3, 0.4]);
    assert_eq!(drained[1].samples, vec![0.5, 0.6]);
}

#[test]
fn http_server_uses_loopback_urls() {
    let dir = tempfile::tempdir().expect("temp dir should be created");
    let server = AirPlayHttpServer::bind(dir.path()).expect("server should start");

    assert!(server.base_url().starts_with("http://127.0.0.1:"));
    assert_eq!(server.root_dir(), dir.path());
}
