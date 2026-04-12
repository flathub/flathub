use super::types::ExpandedImportPaths;
use std::{
    collections::{HashMap, HashSet},
    fs,
    path::{Path, PathBuf},
};

pub(super) const MAX_IMPORT_SCAN_DEPTH: usize = 3;

// Three levels of recursive folder scanning cover the common "artist/album/song"
// layout without letting a single import action walk an arbitrarily deep tree and
// stall the UI on large libraries or network mounts.
pub(super) const SUPPORTED_IMPORT_EXTENSIONS: &[&str] = &[
    "mp3", "flac", "wav", "ogg", "m4a", "aac", "wma", "opus", "aiff", "aif", "cdg", "zip", "lrc",
];

#[derive(Default)]
pub(super) struct ClassifiedImportPaths {
    pub audio_paths: Vec<PathBuf>,
    pub cdg_paths: Vec<PathBuf>,
    pub zip_paths: Vec<PathBuf>,
}

pub(super) fn classify_import_paths(paths: &[String]) -> ClassifiedImportPaths {
    let mut classified = ClassifiedImportPaths::default();

    for raw_path in paths {
        let path = PathBuf::from(raw_path);
        match path
            .extension()
            .and_then(|value| value.to_str())
            .map(|value| value.to_ascii_lowercase())
            .as_deref()
        {
            Some("cdg") => classified.cdg_paths.push(path),
            Some("zip") => classified.zip_paths.push(path),
            _ => classified.audio_paths.push(path),
        }
    }

    classified
}

pub(super) fn collect_expandable_import_paths(raw_paths: &[String]) -> ExpandedImportPaths {
    let mut paths = Vec::new();
    let mut seen = HashSet::new();
    let mut song_count = 0;

    for raw_path in raw_paths {
        let path = PathBuf::from(raw_path);
        collect_expandable_import_paths_from_path(&path, 0, &mut seen, &mut paths, &mut song_count);
    }

    paths.sort();

    ExpandedImportPaths {
        paths: paths
            .into_iter()
            .map(|path| path.display().to_string())
            .collect(),
        song_count,
    }
}

fn collect_expandable_import_paths_from_path(
    path: &Path,
    depth: usize,
    seen: &mut HashSet<String>,
    paths: &mut Vec<PathBuf>,
    song_count: &mut usize,
) {
    let key = path.display().to_string();
    if !seen.insert(key) {
        return;
    }

    let Ok(metadata) = fs::metadata(path) else {
        return;
    };

    if metadata.is_dir() {
        if depth > MAX_IMPORT_SCAN_DEPTH {
            return;
        }

        let Ok(entries) = fs::read_dir(path) else {
            return;
        };

        for entry in entries.flatten() {
            let child_path = entry.path();
            if let Ok(file_type) = entry.file_type() {
                if file_type.is_dir() {
                    if depth < MAX_IMPORT_SCAN_DEPTH {
                        collect_expandable_import_paths_from_path(
                            &child_path,
                            depth + 1,
                            seen,
                            paths,
                            song_count,
                        );
                    }
                } else if file_type.is_file() {
                    collect_expandable_file_path(&child_path, paths, song_count);
                }
            }
        }

        return;
    }

    if metadata.is_file() {
        collect_expandable_file_path(path, paths, song_count);
    }
}

fn collect_expandable_file_path(path: &Path, paths: &mut Vec<PathBuf>, song_count: &mut usize) {
    let Some(extension) = path.extension().and_then(|value| value.to_str()) else {
        return;
    };

    let extension = extension.to_ascii_lowercase();
    if !SUPPORTED_IMPORT_EXTENSIONS.contains(&extension.as_str()) {
        return;
    }

    if extension != "cdg" && extension != "lrc" {
        *song_count += 1;
    }

    paths.push(path.to_path_buf());
}

pub(super) fn build_selected_cdg_lookup(paths: &[PathBuf]) -> HashMap<String, Vec<PathBuf>> {
    let mut by_stem = HashMap::new();
    for path in paths {
        let Some(stem) = path.file_stem().and_then(|value| value.to_str()) else {
            continue;
        };
        by_stem
            .entry(stem.to_ascii_lowercase())
            .or_insert_with(Vec::new)
            .push(path.clone());
    }
    by_stem
}

pub(super) fn match_cdg_source(
    audio_path: &Path,
    selected_cdg_by_stem: &HashMap<String, Vec<PathBuf>>,
    explicit_cdg_by_audio_path: &HashMap<String, String>,
    skip_cdg_for_audio_paths: &[String],
) -> Option<PathBuf> {
    let audio_key = audio_path.display().to_string();
    if skip_cdg_for_audio_paths
        .iter()
        .any(|path| path == &audio_key)
    {
        return None;
    }

    if let Some(cdg_path) = explicit_cdg_by_audio_path.get(&audio_key) {
        return Some(PathBuf::from(cdg_path));
    }

    let stem = audio_path
        .file_stem()
        .and_then(|value| value.to_str())
        .map(|value| value.to_ascii_lowercase())?;

    if let Some(candidates) = selected_cdg_by_stem.get(&stem) {
        if candidates.len() == 1 {
            return Some(candidates[0].clone());
        }

        return None;
    }

    let sibling_cdg = audio_path.with_extension("cdg");
    if sibling_cdg.is_file() {
        return Some(sibling_cdg);
    }

    None
}
