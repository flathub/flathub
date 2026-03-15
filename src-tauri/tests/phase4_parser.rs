use openkara_lib::lyrics::parser::{parse_lrc, LyricLine};

#[test]
fn parses_lrc_lines_with_multiple_timestamps_and_ignores_metadata_tags() {
    let parsed = parse_lrc(
        "[ar:Coldplay]\n[ti:Yellow]\n[00:35.66] Look at the stars\n[00:40.00][00:41.50] Look how they shine for you\n",
    )
    .expect("LRC should parse");

    assert_eq!(
        parsed,
        vec![
            LyricLine {
                time_ms: 35_660,
                text: "Look at the stars".to_owned(),
                words: None,
            },
            LyricLine {
                time_ms: 40_000,
                text: "Look how they shine for you".to_owned(),
                words: None,
            },
            LyricLine {
                time_ms: 41_500,
                text: "Look how they shine for you".to_owned(),
                words: None,
            },
        ]
    );
}
