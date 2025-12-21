use adw::glib::language_names;
use quick_xml::escape::unescape;
use quick_xml::events::Event;
use quick_xml::name::QName;
use quick_xml::Reader;
use std::borrow::Cow;
use std::collections::HashMap;

const METAINFO_XML: &str = include_str!(concat!(
    env!("CARGO_MANIFEST_DIR"),
    "/data/",
    "io.github.noobping.listenmoe",
    ".metainfo.xml"
));

fn collapse_ws(s: &str) -> String {
    s.split_whitespace().collect::<Vec<_>>().join(" ")
}

fn decode_unescape(reader: &Reader<&[u8]>, bytes: &[u8]) -> Option<String> {
    let decoded: Cow<str> = reader.decoder().decode(bytes).ok()?;
    let unescaped: Cow<str> = unescape(decoded.as_ref()).ok()?;
    Some({
        let this = &unescaped;
        this.to_string()
    })
}

fn normalize_lang(tag: &str) -> String {
    tag.replace('_', "-")
}

fn lang_candidates() -> Vec<String> {
    language_names()
        .into_iter()
        .map(|s| normalize_lang(&s))
        .collect()
}

#[derive(Debug, Clone)]
enum DescPart {
    Para(String),
    Bullet(Vec<String>),
}

fn description_to_plain(parts: &[DescPart]) -> String {
    let mut out = String::new();
    let mut first = true;

    for part in parts {
        match part {
            DescPart::Para(s) => {
                let s = s.trim();
                if !s.is_empty() {
                    if !first {
                        out.push_str("\n\n");
                    }
                    out.push_str(s);
                    first = false;
                }
            }
            DescPart::Bullet(items) => {
                let items: Vec<_> = items
                    .iter()
                    .map(|i| i.trim())
                    .filter(|i| !i.is_empty())
                    .collect();
                if !items.is_empty() {
                    if !first {
                        out.push_str("\n\n");
                    }
                    for i in items {
                        out.push_str("• ");
                        out.push_str(i);
                        out.push('\n');
                    }
                    out.pop(); // trailing '\n'
                    first = false;
                }
            }
        }
    }

    out
}

fn elem_lang(e: &quick_xml::events::BytesStart<'_>) -> String {
    for a in e.attributes().flatten() {
        if a.key == QName(b"xml:lang") {
            if let Ok(v) = a.unescape_value() {
                return normalize_lang(&v);
            }
        }
    }
    String::new()
}

fn push_part(map: &mut HashMap<String, Vec<DescPart>>, lang: String, part: DescPart) {
    map.entry(lang).or_default().push(part);
}

fn parse_descriptions(xml: &str) -> HashMap<String, Vec<DescPart>> {
    let mut reader = Reader::from_str(xml);
    reader.config_mut().trim_text(true);

    let mut buf = Vec::new();
    let mut map: HashMap<String, Vec<DescPart>> = HashMap::new();

    // Store owned element names so we don't borrow from `buf`
    let mut stack: Vec<Vec<u8>> = Vec::new();
    let mut in_description = false;

    // <p>
    let mut in_p = false;
    let mut p_lang = String::new();
    let mut p_text = String::new();

    // <ul>/<li>
    let mut in_ul = false;
    let mut in_li = false;
    let mut li_lang = String::new();
    let mut li_text = String::new();

    // Current bullet run inside one <ul> for a given language
    let mut bullets_lang = String::new();
    let mut bullets: Vec<String> = Vec::new();

    loop {
        match reader.read_event_into(&mut buf) {
            Ok(Event::Start(e)) => {
                let name = e.name();

                // Detect: <component> ... <description> where description is a DIRECT child of component
                if name == QName(b"description")
                    && !in_description
                    && stack.len() == 1
                    && stack[0].as_slice() == b"component"
                {
                    in_description = true;

                    // reset state
                    in_p = false;
                    in_ul = false;
                    in_li = false;

                    p_lang.clear();
                    p_text.clear();

                    li_lang.clear();
                    li_text.clear();

                    bullets_lang.clear();
                    bullets.clear();
                }

                // push owned name onto stack
                stack.push(name.as_ref().to_vec());

                if in_description && name == QName(b"p") {
                    in_p = true;
                    p_lang = elem_lang(&e);
                    p_text.clear();
                } else if in_description && name == QName(b"ul") {
                    in_ul = true;
                    bullets_lang.clear();
                    bullets.clear();
                } else if in_description && in_ul && name == QName(b"li") {
                    in_li = true;
                    li_lang = elem_lang(&e);
                    li_text.clear();

                    // If language changes inside this <ul>, flush the previous bullet run
                    if !bullets.is_empty() && li_lang != bullets_lang {
                        push_part(
                            &mut map,
                            std::mem::take(&mut bullets_lang),
                            DescPart::Bullet(std::mem::take(&mut bullets)),
                        );
                    }

                    bullets_lang = li_lang.clone();
                }
            }

            Ok(Event::Text(t)) => {
                if in_description {
                    if let Some(s) = decode_unescape(&reader, t.as_ref()) {
                        let s = collapse_ws(&s);
                        if s.is_empty() {
                            // ignore
                        } else if in_p {
                            if !p_text.is_empty() {
                                p_text.push(' ');
                            }
                            p_text.push_str(&s);
                        } else if in_li {
                            if !li_text.is_empty() {
                                li_text.push(' ');
                            }
                            li_text.push_str(&s);
                        }
                    }
                }
            }

            Ok(Event::End(e)) => {
                let name = e.name();

                if in_description && name == QName(b"p") && in_p {
                    in_p = false;
                    if !p_text.trim().is_empty() {
                        push_part(
                            &mut map,
                            std::mem::take(&mut p_lang),
                            DescPart::Para(std::mem::take(&mut p_text)),
                        );
                    }
                } else if in_description && name == QName(b"li") && in_li {
                    in_li = false;
                    if !li_text.trim().is_empty() {
                        bullets.push(std::mem::take(&mut li_text));
                    }
                    li_lang.clear();
                } else if in_description && name == QName(b"ul") && in_ul {
                    in_ul = false;

                    // flush remaining bullets for the last language in this <ul>
                    if !bullets.is_empty() {
                        push_part(
                            &mut map,
                            std::mem::take(&mut bullets_lang),
                            DescPart::Bullet(std::mem::take(&mut bullets)),
                        );
                    }

                    bullets_lang.clear();
                } else if in_description && name == QName(b"description") {
                    // leaving the top-level description
                    in_description = false;

                    // safety flush
                    if !bullets.is_empty() {
                        push_part(
                            &mut map,
                            std::mem::take(&mut bullets_lang),
                            DescPart::Bullet(std::mem::take(&mut bullets)),
                        );
                    }
                }

                // pop stack
                if let Some(last) = stack.last() {
                    if last.as_slice() == name.as_ref() {
                        stack.pop();
                    }
                }
            }

            Ok(Event::Eof) => break,
            Err(_) => break,
            _ => {}
        }

        buf.clear();
    }

    map
}

pub fn metainfo_description() -> Option<String> {
    let descriptions = parse_descriptions(METAINFO_XML);

    for cand in lang_candidates() {
        if let Some(parts) = descriptions.get(&cand) {
            let s = description_to_plain(parts);
            if !s.trim().is_empty() {
                return Some(s);
            }
        }

        if let Some((base, _)) = cand.split_once('-') {
            if let Some(parts) = descriptions.get(base) {
                let s = description_to_plain(parts);
                if !s.trim().is_empty() {
                    return Some(s);
                }
            }
        }
    }

    // default: <description> without xml:lang
    descriptions
        .get("")
        .map(|p| description_to_plain(p))
        .filter(|s| !s.trim().is_empty())
}
