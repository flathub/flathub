//! temp-dir
//! ========
//! [![crates.io version](https://img.shields.io/crates/v/temp-dir.svg)](https://crates.io/crates/temp-dir)
//! [![license: Apache 2.0](https://gitlab.com/leonhard-llc/ops/-/raw/main/license-apache-2.0.svg)](https://gitlab.com/leonhard-llc/ops/-/raw/main/temp-dir/LICENSE)
//! [![unsafe forbidden](https://gitlab.com/leonhard-llc/ops/-/raw/main/unsafe-forbidden.svg)](https://github.com/rust-secure-code/safety-dance/)
//! [![pipeline status](https://gitlab.com/leonhard-llc/ops/badges/main/pipeline.svg)](https://gitlab.com/leonhard-llc/ops/-/pipelines)
//!
//! Provides a `TempDir` struct.
//!
//! # Features
//! - Makes a directory in a system temporary directory
//! - Recursively deletes the directory and its contents on drop
//! - Deletes symbolic links and does not follow them
//! - Optional name prefix
//! - Depends only on `std`
//! - `forbid(unsafe_code)`
//! - 100% test coverage
//!
//! # Limitations
//! - Not security-hardened.
//!   For example, directory and file names are predictable.
//! - This crate uses
//!   [`std::fs::remove_dir_all`](https://doc.rust-lang.org/stable/std/fs/fn.remove_dir_all.html)
//!   which may be unreliable on Windows.
//!   See [rust#29497](https://github.com/rust-lang/rust/issues/29497) and
//!   [`remove_dir_all`](https://crates.io/crates/remove_dir_all) crate.
//!
//! # Alternatives
//! - [`tempdir`](https://crates.io/crates/tempdir)
//!   - Unmaintained
//!   - Popular and mature
//!   - Heavy dependencies (rand, winapi)
//! - [`tempfile`](https://crates.io/crates/tempfile)
//!   - Popular and mature
//!   - Contains `unsafe`, dependencies full of `unsafe`
//!   - Heavy dependencies (libc, winapi, rand, etc.)
//! - [`test_dir`](https://crates.io/crates/test_dir)
//!   - Has a handy `TestDir` struct
//!   - Incomplete documentation
//! - [`temp_testdir`](https://crates.io/crates/temp_testdir)
//!   - Incomplete documentation
//! - [`mktemp`](https://crates.io/crates/mktemp)
//!   - Sets directory mode 0700 on unix
//!   - Contains `unsafe`
//!   - No readme or online docs
//!
//! # Related Crates
//! - [`temp-file`](https://crates.io/crates/temp-file)
//!
//! # Example
//! ```rust
//! use temp_dir::TempDir;
//! let d = TempDir::new().unwrap();
//! // Prints "/tmp/t1a9b-0".
//! println!("{:?}", d.path());
//! let f = d.child("file1");
//! // Prints "/tmp/t1a9b-0/file1".
//! println!("{:?}", f);
//! std::fs::write(&f, b"abc").unwrap();
//! assert_eq!(
//!     "abc",
//!     std::fs::read_to_string(&f).unwrap(),
//! );
//! // Prints "/tmp/t1a9b-1".
//! println!(
//!     "{:?}", TempDir::new().unwrap().path());
//! ```
//!
//! # Cargo Geiger Safety Report
//! # Changelog
//! - v0.1.16 - `dont_delete_on_drop()`.  Thanks to [A L Manning](https://gitlab.com/A-Manning) for [discussion](https://gitlab.com/leonhard-llc/ops/-/merge_requests/5).
//! - v0.1.15 - Remove a dev dependency.
//! - v0.1.14 - `AsRef<Path>`
//! - v0.1.13 - Update docs.
//! - v0.1.12 - Work when the directory already exists.
//! - v0.1.11
//!   - Return `std::io::Error` instead of `String`.
//!   - Add
//!     [`cleanup`](https://docs.rs/temp-file/latest/temp_file/struct.TempFile.html#method.cleanup).
//! - v0.1.10 - Implement `Eq`, `Ord`, `Hash`
//! - v0.1.9 - Increase test coverage
//! - v0.1.8 - Add [`leak`](https://docs.rs/temp-dir/latest/temp_dir/struct.TempDir.html#method.leak).
//! - v0.1.7 - Update docs:
//!   Warn about `std::fs::remove_dir_all` being unreliable on Windows.
//!   Warn about predictable directory and file names.
//!   Thanks to Reddit user
//!   [burntsushi](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/gruo5iu/).
//! - v0.1.6 - Add
//!   [`TempDir::panic_on_cleanup_error`](https://docs.rs/temp-dir/latest/temp_dir/struct.TempDir.html#method.panic_on_cleanup_error).
//!   Thanks to Reddit users
//!   [`KhorneLordOfChaos`](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/grsb5s3/)
//!   and
//!   [`dpc_pw`](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/gru26df/)
//!   for their comments.
//! - v0.1.5 - Explain how it handles symbolic links.
//!   Thanks to Reddit user Mai4eeze for this
//!   [idea](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/grsoz2g/).
//! - v0.1.4 - Update docs
//! - v0.1.3 - Minor code cleanup, update docs
//! - v0.1.2 - Update docs
//! - v0.1.1 - Fix license
//! - v0.1.0 - Initial version
#![forbid(unsafe_code)]
use core::sync::atomic::{AtomicU32, Ordering};
use std::io::ErrorKind;
use std::path::{Path, PathBuf};
use std::sync::atomic::AtomicBool;

#[doc(hidden)]
pub static INTERNAL_COUNTER: AtomicU32 = AtomicU32::new(0);
#[doc(hidden)]
pub static INTERNAL_RETRY: AtomicBool = AtomicBool::new(true);

/// The path of an existing writable directory in a system temporary directory.
///
/// Drop the struct to delete the directory and everything under it.
/// Deletes symbolic links and does not follow them.
///
/// Ignores any error while deleting.
/// See [`TempDir::panic_on_cleanup_error`](struct.TempDir.html#method.panic_on_cleanup_error).
///
/// # Example
/// ```rust
/// use temp_dir::TempDir;
/// let d = TempDir::new().unwrap();
/// // Prints "/tmp/t1a9b-0".
/// println!("{:?}", d.path());
/// let f = d.child("file1");
/// // Prints "/tmp/t1a9b-0/file1".
/// println!("{:?}", f);
/// std::fs::write(&f, b"abc").unwrap();
/// assert_eq!(
///     "abc",
///     std::fs::read_to_string(&f).unwrap(),
/// );
/// // Prints "/tmp/t1a9b-1".
/// println!("{:?}", TempDir::new().unwrap().path());
/// ```
#[derive(Clone, PartialOrd, Ord, PartialEq, Eq, Hash, Debug)]
pub struct TempDir {
    delete_on_drop: bool,
    panic_on_delete_err: bool,
    path_buf: PathBuf,
}
impl TempDir {
    fn remove_dir(path: &Path) -> Result<(), std::io::Error> {
        match std::fs::remove_dir_all(path) {
            Ok(()) => Ok(()),
            Err(e) if e.kind() == ErrorKind::NotFound => Ok(()),
            Err(e) => Err(std::io::Error::new(
                e.kind(),
                format!("error removing directory and contents {path:?}: {e}"),
            )),
        }
    }

    /// Create a new empty directory in a system temporary directory.
    ///
    /// Drop the struct to delete the directory and everything under it.
    /// Deletes symbolic links and does not follow them.
    ///
    /// Ignores any error while deleting.
    /// See [`TempDir::panic_on_cleanup_error`](struct.TempDir.html#method.panic_on_cleanup_error).
    ///
    /// # Errors
    /// Returns `Err` when it fails to create the directory.
    ///
    /// # Example
    /// ```rust
    /// // Prints "/tmp/t1a9b-0".
    /// println!("{:?}", temp_dir::TempDir::new().unwrap().path());
    /// ```
    pub fn new() -> Result<Self, std::io::Error> {
        // Prefix with 't' to avoid name collisions with `temp-file` crate.
        Self::with_prefix("t")
    }

    /// Create a new empty directory in a system temporary directory.
    /// Use `prefix` as the first part of the directory's name.
    ///
    /// Drop the struct to delete the directory and everything under it.
    /// Deletes symbolic links and does not follow them.
    ///
    /// Ignores any error while deleting.
    /// See [`TempDir::panic_on_cleanup_error`](struct.TempDir.html#method.panic_on_cleanup_error).
    ///
    /// # Errors
    /// Returns `Err` when it fails to create the directory.
    ///
    /// # Example
    /// ```rust
    /// // Prints "/tmp/ok1a9b-0".
    /// println!("{:?}", temp_dir::TempDir::with_prefix("ok").unwrap().path());
    /// ```
    pub fn with_prefix(prefix: impl AsRef<str>) -> Result<Self, std::io::Error> {
        loop {
            let path_buf = std::env::temp_dir().join(format!(
                "{}{:x}-{:x}",
                prefix.as_ref(),
                std::process::id(),
                INTERNAL_COUNTER.fetch_add(1, Ordering::AcqRel),
            ));
            match std::fs::create_dir(&path_buf) {
                Err(e)
                    if e.kind() == ErrorKind::AlreadyExists
                        && INTERNAL_RETRY.load(Ordering::Acquire) => {}
                Err(e) => {
                    return Err(std::io::Error::new(
                        e.kind(),
                        format!("error creating directory {path_buf:?}: {e}"),
                    ))
                }
                Ok(()) => {
                    return Ok(Self {
                        delete_on_drop: true,
                        panic_on_delete_err: false,
                        path_buf,
                    })
                }
            }
        }
    }

    /// Remove the directory and its contents now.
    ///
    /// # Errors
    /// Returns an error if the directory exists and we fail to remove it and its contents.
    #[allow(clippy::missing_panics_doc)]
    pub fn cleanup(self) -> Result<(), std::io::Error> {
        Self::remove_dir(&self.path_buf)
    }

    /// Make the struct panic on drop if it hits an error while
    /// removing the directory or its contents.
    #[must_use]
    pub fn panic_on_cleanup_error(mut self) -> Self {
        self.panic_on_delete_err = true;
        self
    }

    /// Do not delete the directory or its contents.
    ///
    /// This is useful when debugging a test.
    pub fn leak(mut self) {
        self.delete_on_drop = false;
    }

    /// Do not delete the directory or its contents on Drop.
    #[must_use]
    pub fn dont_delete_on_drop(mut self) -> Self {
        self.delete_on_drop = false;
        self
    }

    /// The path to the directory.
    #[must_use]
    #[allow(clippy::missing_panics_doc)]
    pub fn path(&self) -> &Path {
        &self.path_buf
    }

    /// The path to `name` under the directory.
    #[must_use]
    #[allow(clippy::missing_panics_doc)]
    pub fn child(&self, name: impl AsRef<str>) -> PathBuf {
        let mut result = self.path_buf.clone();
        result.push(name.as_ref());
        result
    }
}
impl Drop for TempDir {
    fn drop(&mut self) {
        if self.delete_on_drop {
            let result = Self::remove_dir(&self.path_buf);
            if self.panic_on_delete_err {
                if let Err(e) = result {
                    panic!("{}", e);
                }
            }
        }
    }
}
impl AsRef<Path> for TempDir {
    fn as_ref(&self) -> &Path {
        self.path()
    }
}
