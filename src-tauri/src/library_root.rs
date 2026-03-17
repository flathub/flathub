use anyhow::{bail, Context, Result};
use serde::{Deserialize, Serialize};
use std::{
    fs,
    path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
};

const MARKER_FILENAME: &str = ".openkara-library";
const DATABASE_FILENAME: &str = "openkara.db";
const MEDIA_DIRECTORY: &str = "media";
const STEMS_DIRECTORY: &str = "stems";

#[derive(Debug, Clone, Serialize, Deserialize)]
struct LibraryMarker {
    version: u32,
    created_at: i64,
    identifier: String,
}

/// A self-contained karaoke library directory.
///
/// All paths stored in the database are relative to the library root and use
/// forward slashes regardless of the host OS. The `resolve` method converts
/// them back to absolute platform paths at runtime. This is a portability rule,
/// not just formatting preference: it keeps a library movable across machines,
/// drives, and operating systems without rewriting database rows.
#[derive(Debug, Clone)]
pub struct LibraryRoot {
    root: PathBuf,
}

impl LibraryRoot {
    /// Create a new library at `path`, writing the marker file and creating
    /// the `media/` and `stems/` subdirectories.
    pub fn create(path: &Path) -> Result<Self> {
        if path.join(MARKER_FILENAME).exists() {
            bail!("a library already exists at {}", path.display());
        }

        fs::create_dir_all(path)
            .with_context(|| format!("failed to create library directory at {}", path.display()))?;

        let marker = LibraryMarker {
            version: 1,
            created_at: unix_timestamp(),
            identifier: "com.openkara.library".to_owned(),
        };
        let marker_json =
            serde_json::to_string_pretty(&marker).context("failed to serialize library marker")?;
        fs::write(path.join(MARKER_FILENAME), marker_json)
            .with_context(|| format!("failed to write library marker at {}", path.display()))?;

        fs::create_dir_all(path.join(MEDIA_DIRECTORY))
            .context("failed to create media directory")?;
        fs::create_dir_all(path.join(STEMS_DIRECTORY))
            .context("failed to create stems directory")?;

        Ok(Self {
            root: path.to_owned(),
        })
    }

    /// Open an existing library, validating that the marker file exists.
    pub fn open(path: &Path) -> Result<Self> {
        let marker_path = path.join(MARKER_FILENAME);
        if !marker_path.exists() {
            bail!(
                "{} is not a valid OpenKara library (missing {})",
                path.display(),
                MARKER_FILENAME
            );
        }

        // Ensure subdirectories exist (may have been created by an older version).
        fs::create_dir_all(path.join(MEDIA_DIRECTORY)).ok();
        fs::create_dir_all(path.join(STEMS_DIRECTORY)).ok();

        Ok(Self {
            root: path.to_owned(),
        })
    }

    /// The root directory of this library.
    pub fn root(&self) -> &Path {
        &self.root
    }

    /// Path to the SQLite database inside this library.
    pub fn database_path(&self) -> PathBuf {
        self.root.join(DATABASE_FILENAME)
    }

    /// Path to the `media/` directory that holds copied audio originals.
    pub fn media_dir(&self) -> PathBuf {
        self.root.join(MEDIA_DIRECTORY)
    }

    /// Path to the `stems/` directory that holds separation output.
    pub fn stems_dir(&self) -> PathBuf {
        self.root.join(STEMS_DIRECTORY)
    }

    /// Build the absolute path for a media file given its hash and extension.
    ///
    /// Example: `media_path("a1b2c3", "mp3")` → `<root>/media/a1b2c3.mp3`
    pub fn media_path(&self, hash: &str, ext: &str) -> PathBuf {
        self.root
            .join(MEDIA_DIRECTORY)
            .join(format!("{}.{}", hash, ext))
    }

    /// Resolve a database-relative path (forward slashes) to an absolute
    /// platform path.
    ///
    /// Example: `resolve("media/a1b2c3.mp3")` → `/Users/x/MyLib/media/a1b2c3.mp3`
    pub fn resolve(&self, relative: &str) -> PathBuf {
        // Database paths always use forward slashes.  On Windows we need to
        // convert them to the native separator before joining.
        let native = if cfg!(windows) {
            relative.replace('/', "\\")
        } else {
            relative.to_owned()
        };
        self.root.join(native)
    }

    /// Convert an absolute path that lives inside this library to a
    /// database-relative path with forward slashes.
    pub fn to_relative(&self, absolute: &Path) -> Result<String> {
        let relative = absolute.strip_prefix(&self.root).with_context(|| {
            format!(
                "{} is not inside library root {}",
                absolute.display(),
                self.root.display()
            )
        })?;

        // Keep DB paths OS-agnostic so a library created on one machine can be
        // copied to another without path migrations.
        let normalised = relative
            .components()
            .map(|c| c.as_os_str().to_string_lossy().into_owned())
            .collect::<Vec<_>>()
            .join("/");

        Ok(normalised)
    }
}

fn unix_timestamp() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system time should be after unix epoch")
        .as_secs() as i64
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn create_and_open_library() {
        let tmp = tempfile::tempdir().unwrap();
        let lib_path = tmp.path().join("TestLibrary");

        let lib = LibraryRoot::create(&lib_path).unwrap();
        assert!(lib_path.join(MARKER_FILENAME).exists());
        assert!(lib_path.join(MEDIA_DIRECTORY).is_dir());
        assert!(lib_path.join(STEMS_DIRECTORY).is_dir());
        assert_eq!(lib.database_path(), lib_path.join(DATABASE_FILENAME));

        let reopened = LibraryRoot::open(&lib_path).unwrap();
        assert_eq!(reopened.root(), lib.root());
    }

    #[test]
    fn create_rejects_existing_library() {
        let tmp = tempfile::tempdir().unwrap();
        let lib_path = tmp.path().join("Existing");
        LibraryRoot::create(&lib_path).unwrap();
        assert!(LibraryRoot::create(&lib_path).is_err());
    }

    #[test]
    fn open_rejects_non_library_directory() {
        let tmp = tempfile::tempdir().unwrap();
        assert!(LibraryRoot::open(tmp.path()).is_err());
    }

    #[test]
    fn resolve_and_to_relative_round_trip() {
        let tmp = tempfile::tempdir().unwrap();
        let lib = LibraryRoot::create(tmp.path().join("Lib").as_path()).unwrap();

        let relative = "media/abc123.mp3";
        let absolute = lib.resolve(relative);
        assert!(absolute.is_absolute());
        assert_eq!(lib.to_relative(&absolute).unwrap(), relative);
    }

    #[test]
    fn to_relative_rejects_outside_path() {
        let tmp = tempfile::tempdir().unwrap();
        let lib = LibraryRoot::create(tmp.path().join("Lib").as_path()).unwrap();
        assert!(lib.to_relative(Path::new("/some/other/path")).is_err());
    }

    #[test]
    fn media_path_builds_correct_path() {
        let tmp = tempfile::tempdir().unwrap();
        let lib = LibraryRoot::create(tmp.path().join("Lib").as_path()).unwrap();
        let p = lib.media_path("deadbeef", "flac");
        assert_eq!(p, tmp.path().join("Lib/media/deadbeef.flac"));
    }
}
