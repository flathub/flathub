use openkara_lib::airplay_stream::{
    select_forwardable_audio_chunks, AirPlayAudioChunk, AirPlayAudioTap, AirPlayHttpServer,
};
use reqwest::{
    blocking::Client,
    header::{ACCEPT_RANGES, CONTENT_LENGTH, CONTENT_RANGE, RANGE},
    StatusCode,
};
use std::fs;
use std::net::Ipv4Addr;

#[test]
fn audio_tap_keeps_streaming_chunks_from_the_same_epoch() {
    let tap = AirPlayAudioTap::new(2);

    tap.push_interleaved(44_100, 2, &[0.1, 0.2, 0.3, 0.4]);
    tap.push_interleaved(44_100, 2, &[0.5, 0.6, 0.7, 0.8]);

    let drained = tap.drain_pending();

    assert_eq!(drained.len(), 2);
    assert_eq!(drained[0].epoch, 1);
    assert_eq!(drained[0].samples, vec![0.1, 0.2, 0.3, 0.4]);
    assert_eq!(drained[1].epoch, 1);
    assert_eq!(drained[1].samples, vec![0.5, 0.6, 0.7, 0.8]);
}

#[test]
fn audio_tap_respects_bounded_capacity() {
    let tap = AirPlayAudioTap::new(2);

    tap.push_interleaved(44_100, 2, &[0.1, 0.2]);
    tap.push_interleaved(44_100, 2, &[0.3, 0.4]);
    tap.push_interleaved(44_100, 2, &[0.5, 0.6]);

    let drained = tap.drain_pending();

    assert_eq!(drained.len(), 2);
    assert_eq!(drained[0].samples, vec![0.3, 0.4]);
    assert_eq!(drained[1].samples, vec![0.5, 0.6]);
}

#[test]
fn audio_forwarder_prefers_latest_epoch_and_drops_stale_chunks() {
    let (next_epoch, forwardable) = select_forwardable_audio_chunks(
        1,
        vec![
            AirPlayAudioChunk {
                epoch: 1,
                sample_rate: 44_100,
                channels: 2,
                samples: vec![0.1, 0.2],
            },
            AirPlayAudioChunk {
                epoch: 2,
                sample_rate: 44_100,
                channels: 2,
                samples: vec![0.3, 0.4],
            },
            AirPlayAudioChunk {
                epoch: 2,
                sample_rate: 44_100,
                channels: 2,
                samples: vec![0.5, 0.6],
            },
        ],
    );

    assert_eq!(next_epoch, 2);
    assert_eq!(forwardable.len(), 2);
    assert!(forwardable.iter().all(|chunk| chunk.epoch == 2));
    assert_eq!(forwardable[0].samples, vec![0.3, 0.4]);
    assert_eq!(forwardable[1].samples, vec![0.5, 0.6]);
}

#[test]
fn http_server_publishes_a_lan_reachable_host() {
    let dir = tempfile::tempdir().expect("temp dir should be created");
    let server = AirPlayHttpServer::bind_with_publish_ip(dir.path(), Ipv4Addr::new(192, 168, 50, 10))
        .expect("server should start");

    assert!(server.base_url().starts_with("http://192.168.50.10:"));
    assert_eq!(server.root_dir(), dir.path());
}

#[test]
fn http_server_supports_head_and_byte_ranges_for_media_clients() {
    let dir = tempfile::tempdir().expect("temp dir should be created");
    let file_path = dir.path().join("segment-1.m4s");
    fs::write(&file_path, (0u8..=63).collect::<Vec<_>>()).expect("fixture file should be written");

    let server = AirPlayHttpServer::bind_with_publish_ip(dir.path(), Ipv4Addr::new(127, 0, 0, 1))
        .expect("server should start");
    let client = Client::new();
    let url = format!("{}/segment-1.m4s", server.base_url());

    let head = client.head(&url).send().expect("head request should succeed");
    assert_eq!(head.status(), StatusCode::OK);
    assert_eq!(head.headers()[CONTENT_LENGTH], "64");
    assert_eq!(head.headers()[ACCEPT_RANGES], "bytes");

    let response = client
        .get(&url)
        .header(RANGE, "bytes=10-19")
        .send()
        .expect("range request should succeed");

    assert_eq!(response.status(), StatusCode::PARTIAL_CONTENT);
    assert_eq!(response.headers()[CONTENT_RANGE], "bytes 10-19/64");
    assert_eq!(response.headers()[CONTENT_LENGTH], "10");
    assert_eq!(response.bytes().unwrap().as_ref(), &(10u8..=19).collect::<Vec<_>>());
}
