use anyhow::{bail, Context, Result};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct WordToken {
    pub time_ms: u64,
    pub text: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct LyricLine {
    pub time_ms: u64,
    pub text: String,
    pub words: Option<Vec<WordToken>>,
}

#[derive(Debug, Clone, Default)]
pub struct LrcMetadata {
    pub artist: Option<String>,
    pub title: Option<String>,
    pub album: Option<String>,
    pub offset_ms: Option<i64>,
}

/// Extract metadata tags from an LRC string: `[ar:Artist]`, `[ti:Title]`,
/// `[al:Album]`, `[offset:+/-ms]`.
pub fn parse_lrc_metadata(lrc: &str) -> LrcMetadata {
    let mut meta = LrcMetadata::default();
    for line in lrc.lines() {
        let line = line.trim();
        if let Some(stripped) = line.strip_prefix('[') {
            if let Some(close) = stripped.find(']') {
                let tag = &stripped[..close];
                if let Some(value) = tag.strip_prefix("ar:") {
                    meta.artist = Some(value.trim().to_owned());
                } else if let Some(value) = tag.strip_prefix("ti:") {
                    meta.title = Some(value.trim().to_owned());
                } else if let Some(value) = tag.strip_prefix("al:") {
                    meta.album = Some(value.trim().to_owned());
                } else if let Some(value) = tag.strip_prefix("offset:") {
                    if let Ok(offset) = value.trim().parse::<i64>() {
                        meta.offset_ms = Some(offset);
                    }
                }
            }
        }
    }
    meta
}

pub fn parse_lrc(lrc: &str) -> Result<Vec<LyricLine>> {
    let mut parsed_lines = Vec::new();

    for raw_line in lrc.lines() {
        let mut cursor = raw_line;
        let mut timestamps = Vec::new();

        while let Some(stripped) = cursor.strip_prefix('[') {
            let closing = stripped
                .find(']')
                .with_context(|| format!("missing closing ] in LRC line: {raw_line}"))?;
            let tag = &stripped[..closing];

            if let Some(timestamp_ms) = parse_timestamp_tag(tag)? {
                timestamps.push(timestamp_ms);
                cursor = &stripped[closing + 1..];
                continue;
            }

            timestamps.clear();
            break;
        }

        if timestamps.is_empty() {
            continue;
        }

        let trimmed = cursor.trim();
        let (lyric_text, words) = match parse_word_tokens(trimmed) {
            Some((plain, tokens)) => (plain, Some(tokens)),
            None => (trimmed.to_owned(), None),
        };

        for timestamp_ms in timestamps {
            parsed_lines.push(LyricLine {
                time_ms: timestamp_ms,
                text: lyric_text.clone(),
                words: words.clone(),
            });
        }
    }

    parsed_lines.sort_by_key(|line| line.time_ms);
    Ok(parsed_lines)
}

/// Scans `text` for `<mm:ss.xx>` inline word-timing tags.
/// Returns `None` if no valid word tokens are found (standard LRC line).
/// Returns `Some((plain_text, tokens))` where `plain_text` is the text with
/// all `<>` tags stripped, and `tokens` is the list of `WordToken`s.
pub fn parse_word_tokens(text: &str) -> Option<(String, Vec<WordToken>)> {
    // Quick pre-check: must contain at least one '<'
    if !text.contains('<') {
        return None;
    }

    let mut tokens: Vec<WordToken> = Vec::new();
    let mut plain = String::new();
    let mut remaining = text;

    while !remaining.is_empty() {
        if let Some(stripped) = remaining.strip_prefix('<') {
            // Try to parse an inline timestamp tag
            if let Some(close) = stripped.find('>') {
                let tag = &stripped[..close];
                match parse_timestamp_tag(tag) {
                    Ok(Some(time_ms)) => {
                        // Collect text until the next '<' or end
                        let rest = &stripped[close + 1..];
                        let word_end = rest.find('<').unwrap_or(rest.len());
                        let word_text = &rest[..word_end];
                        plain.push_str(word_text);
                        tokens.push(WordToken {
                            time_ms,
                            text: word_text.to_owned(),
                        });
                        remaining = &rest[word_end..];
                        continue;
                    }
                    _ => {
                        // Not a valid timestamp tag — treat the '<' as literal text
                        plain.push('<');
                        remaining = stripped;
                        continue;
                    }
                }
            } else {
                // No closing '>' — treat '<' as literal
                plain.push('<');
                remaining = stripped;
                continue;
            }
        } else {
            // Find the next '<' and consume everything before it as plain text
            let next = remaining.find('<').unwrap_or(remaining.len());
            plain.push_str(&remaining[..next]);
            remaining = &remaining[next..];
        }
    }

    if tokens.is_empty() {
        return None;
    }

    Some((plain, tokens))
}

fn parse_timestamp_tag(tag: &str) -> Result<Option<u64>> {
    let Some((minutes, remainder)) = tag.split_once(':') else {
        return Ok(None);
    };

    // Support both '.' and ':' as the seconds/fractional separator.
    // Also support bare "mm:ss" with no fractional part.
    let (seconds_str, fractional_opt) = if let Some((s, f)) = remainder.split_once('.') {
        (s, Some(f))
    } else if let Some((s, f)) = remainder.split_once(':') {
        (s, Some(f))
    } else {
        // No separator at all — bare mm:ss
        (remainder, None)
    };

    let Ok(minutes) = minutes.parse::<u64>() else {
        return Ok(None);
    };
    let Ok(seconds) = seconds_str.parse::<u64>() else {
        return Ok(None);
    };
    if seconds >= 60 {
        bail!("invalid LRC seconds value {seconds} in tag {tag}");
    }

    let fraction_ms = match fractional_opt {
        None => 0,
        Some(fractional) => {
            let Ok(frac_val) = fractional.parse::<u64>() else {
                return Ok(None);
            };
            match fractional.len() {
                1 => frac_val * 100,
                2 => frac_val * 10,
                3 => frac_val,
                _ => return Ok(None),
            }
        }
    };

    Ok(Some(minutes * 60_000 + seconds * 1_000 + fraction_ms))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_standard_lrc() {
        let lrc = "[ar:Test]\n[00:10.00]Hello world\n[00:20.50]Goodbye\n";
        let lines = parse_lrc(lrc).expect("should parse");
        assert_eq!(
            lines,
            vec![
                LyricLine {
                    time_ms: 10_000,
                    text: "Hello world".to_owned(),
                    words: None,
                },
                LyricLine {
                    time_ms: 20_500,
                    text: "Goodbye".to_owned(),
                    words: None,
                },
            ]
        );
    }

    #[test]
    fn parses_enhanced_lrc_with_word_tokens() {
        let lrc = "[00:12.00]<00:12.00>I <00:12.30>see <00:12.60>trees\n";
        let lines = parse_lrc(lrc).expect("should parse");
        assert_eq!(lines.len(), 1);
        let line = &lines[0];
        assert_eq!(line.time_ms, 12_000);
        assert_eq!(line.text, "I see trees");
        assert_eq!(
            line.words,
            Some(vec![
                WordToken {
                    time_ms: 12_000,
                    text: "I ".to_owned(),
                },
                WordToken {
                    time_ms: 12_300,
                    text: "see ".to_owned(),
                },
                WordToken {
                    time_ms: 12_600,
                    text: "trees".to_owned(),
                },
            ])
        );
    }

    #[test]
    fn parses_timestamp_without_fractional() {
        // mm:ss — no fractional part
        let ms = parse_timestamp_tag("01:05")
            .expect("should not error")
            .expect("should be Some");
        assert_eq!(ms, 65_000);
    }

    #[test]
    fn parses_timestamp_with_colon_separator() {
        // mm:ss:xx — colon used instead of dot
        let ms = parse_timestamp_tag("00:30:75")
            .expect("should not error")
            .expect("should be Some");
        assert_eq!(ms, 30_750);
    }

    #[test]
    fn mixed_standard_and_enhanced_lines() {
        let lrc = "[00:05.00]Plain line\n[00:10.00]<00:10.00>Word <00:10.50>two\n";
        let lines = parse_lrc(lrc).expect("should parse");
        assert_eq!(lines.len(), 2);

        assert_eq!(lines[0].text, "Plain line");
        assert!(lines[0].words.is_none());

        assert_eq!(lines[1].text, "Word two");
        assert!(lines[1].words.is_some());
        let words = lines[1].words.as_ref().unwrap();
        assert_eq!(words.len(), 2);
        assert_eq!(words[0].text, "Word ");
        assert_eq!(words[0].time_ms, 10_000);
        assert_eq!(words[1].text, "two");
        assert_eq!(words[1].time_ms, 10_500);
    }
}
