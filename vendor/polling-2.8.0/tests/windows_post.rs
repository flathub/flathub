//! Tests for the post() function on Windows.

#![cfg(windows)]

use polling::os::iocp::{CompletionPacket, PollerIocpExt};
use polling::{Event, Poller};

use std::sync::Arc;
use std::thread;
use std::time::Duration;

#[test]
fn post_smoke() {
    let poller = Poller::new().unwrap();
    let mut events = Vec::new();

    poller
        .post(CompletionPacket::new(Event::readable(1)))
        .unwrap();
    poller.wait(&mut events, None).unwrap();

    assert_eq!(events.len(), 1);
    assert_eq!(events[0], Event::readable(1));
}

#[test]
fn post_multithread() {
    let poller = Arc::new(Poller::new().unwrap());
    let mut events = Vec::new();

    thread::spawn({
        let poller = Arc::clone(&poller);
        move || {
            for i in 0..3 {
                poller
                    .post(CompletionPacket::new(Event::writable(i)))
                    .unwrap();

                thread::sleep(Duration::from_millis(100));
            }
        }
    });

    for i in 0..3 {
        poller
            .wait(&mut events, Some(Duration::from_secs(5)))
            .unwrap();

        assert_eq!(events.len(), 1);
        assert_eq!(events.pop(), Some(Event::writable(i)));
    }

    poller
        .wait(&mut events, Some(Duration::from_millis(10)))
        .unwrap();
    assert_eq!(events.len(), 0);
}
