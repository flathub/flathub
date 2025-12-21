use std::time::{SystemTime, UNIX_EPOCH};

pub fn now_string() -> String {
    let d = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default();

    let total_secs = d.as_secs();
    let millis = d.subsec_millis();

    let secs = total_secs % 60;
    let mins = (total_secs / 60) % 60;
    let hours = (total_secs / 3600) % 24;

    format!("{:02}:{:02}:{:02}.{:03}", hours, mins, secs, millis)
}
