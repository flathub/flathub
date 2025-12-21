//! Benchmarks for registering timers.

use async_io::Timer;
use criterion::{black_box, criterion_group, criterion_main, Criterion};
use futures_lite::future;
use std::time::Duration;

/// Create a new `Timer` and poll it once to register it into the timer wheel.
fn make_timer() -> Timer {
    let mut timer = Timer::after(Duration::from_secs(1));
    future::block_on(future::poll_once(&mut timer));
    timer
}

/// Benchmark the time it takes to register and deregister a timer.
fn register_timer(c: &mut Criterion) {
    let mut group = c.benchmark_group("register_timer");
    for prev_timer_count in [0, 1_000_000] {
        // Add timers to the timer wheel.
        let mut timers = Vec::new();
        for _ in 0..prev_timer_count {
            timers.push(make_timer());
        }

        // Benchmark registering a timer.
        group.bench_function(
            format!("register_timer.({} previous timers)", prev_timer_count),
            |b| {
                b.iter(|| {
                    let timer = make_timer();
                    black_box(timer);
                });
            },
        );
    }
}

criterion_group!(benches, register_timer);
criterion_main!(benches);
