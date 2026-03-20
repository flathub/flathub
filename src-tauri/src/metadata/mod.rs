use anyhow::{Context, Result};
use lofty::{
    file::FileType,
    config::{ParseOptions, WriteOptions},
    file::{AudioFile, TaggedFileExt},
    prelude::Accessor,
    probe::Probe,
    tag::{Tag, TagExt, TagType},
};
use std::{
    io::{BufReader, Cursor},
    path::Path,
};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SongMetadata {
    pub title: Option<String>,
    pub artist: Option<String>,
    pub album: Option<String>,
    pub duration_ms: i64,
    pub cover_art: Option<Vec<u8>>,
}

pub fn read_from_path(path: &Path) -> Result<SongMetadata> {
    let tagged_file = read_tagged_file_from_path(path)?;

    song_metadata_from_tagged_file(tagged_file)
}

pub fn read_from_bytes(bytes: &[u8], extension: &str) -> Result<SongMetadata> {
    let tagged_file = read_tagged_file_from_bytes(bytes, extension)?;

    song_metadata_from_tagged_file(tagged_file)
}

pub fn write_ogg_with_preserved_metadata(
    source_path: &Path,
    output_path: &Path,
    title: &str,
) -> Result<()> {
    let tagged_file = read_tagged_file_from_path(source_path)?;

    let mut output_tag = Tag::new(TagType::VorbisComments);
    if let Some(source_tag) = tagged_file
        .primary_tag()
        .or_else(|| tagged_file.first_tag())
    {
        for item in source_tag.items().cloned() {
            output_tag.push(item);
        }

        for picture in source_tag.pictures() {
            output_tag.push_picture(picture.clone());
        }
    }

    output_tag.set_title(title.to_owned());
    output_tag
        .save_to_path(output_path, WriteOptions::default())
        .with_context(|| format!("failed to write metadata to {}", output_path.display()))?;

    Ok(())
}

pub fn read_tagged_file_from_path(path: &Path) -> Result<lofty::file::TaggedFile> {
    Probe::open(path)
        .with_context(|| format!("failed to open audio file at {}", path.display()))?
        // Real-world libraries often contain MP4-family audio mislabeled as
        // .aac, so metadata reads must sniff the container instead of trusting
        // the extension.
        .guess_file_type()
        .with_context(|| format!("failed to inspect audio metadata from {}", path.display()))?
        .read()
        .with_context(|| format!("failed to read audio metadata from {}", path.display()))
}

pub fn read_tagged_file_from_bytes(
    bytes: &[u8],
    extension: &str,
) -> Result<lofty::file::TaggedFile> {
    let file_type = FileType::from_ext(extension)
        .with_context(|| format!("unsupported embedded audio extension {extension}"))?;

    Probe::new(BufReader::new(Cursor::new(bytes)))
        .options(ParseOptions::new())
        .guess_file_type()
        .map(|probe| {
            if probe.file_type().is_some() {
                probe
            } else {
                probe.set_file_type(file_type)
            }
        })
        .with_context(|| format!("failed to inspect audio metadata from in-memory {extension}"))?
        .read()
        .with_context(|| format!("failed to read audio metadata from in-memory {extension}"))
}

fn song_metadata_from_tagged_file(tagged_file: lofty::file::TaggedFile) -> Result<SongMetadata> {
    let properties = tagged_file.properties();
    let duration_ms = properties.duration().as_millis() as i64;
    let primary_tag = tagged_file
        .primary_tag()
        .or_else(|| tagged_file.first_tag());
    let cover_art = primary_tag
        .and_then(|tag| tag.pictures().first())
        .map(|picture| picture.data().to_vec());

    Ok(SongMetadata {
        title: primary_tag.and_then(|tag| tag.title().map(|value| value.into_owned())),
        artist: primary_tag.and_then(|tag| tag.artist().map(|value| value.into_owned())),
        album: primary_tag.and_then(|tag| tag.album().map(|value| value.into_owned())),
        duration_ms,
        cover_art,
    })
}
