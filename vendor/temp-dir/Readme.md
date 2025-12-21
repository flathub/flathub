temp-dir
========
[![crates.io version](https://img.shields.io/crates/v/temp-dir.svg)](https://crates.io/crates/temp-dir)
[![license: Apache 2.0](https://gitlab.com/leonhard-llc/ops/-/raw/main/license-apache-2.0.svg)](https://gitlab.com/leonhard-llc/ops/-/raw/main/temp-dir/LICENSE)
[![unsafe forbidden](https://gitlab.com/leonhard-llc/ops/-/raw/main/unsafe-forbidden.svg)](https://github.com/rust-secure-code/safety-dance/)
[![pipeline status](https://gitlab.com/leonhard-llc/ops/badges/main/pipeline.svg)](https://gitlab.com/leonhard-llc/ops/-/pipelines)

Provides a `TempDir` struct.

# Features
- Makes a directory in a system temporary directory
- Recursively deletes the directory and its contents on drop
- Deletes symbolic links and does not follow them
- Optional name prefix
- Depends only on `std`
- `forbid(unsafe_code)`
- 100% test coverage

# Limitations
- Not security-hardened.
  For example, directory and file names are predictable.
- This crate uses
  [`std::fs::remove_dir_all`](https://doc.rust-lang.org/stable/std/fs/fn.remove_dir_all.html)
  which may be unreliable on Windows.
  See [rust#29497](https://github.com/rust-lang/rust/issues/29497) and
  [`remove_dir_all`](https://crates.io/crates/remove_dir_all) crate.

# Alternatives
- [`tempdir`](https://crates.io/crates/tempdir)
  - Unmaintained
  - Popular and mature
  - Heavy dependencies (rand, winapi)
- [`tempfile`](https://crates.io/crates/tempfile)
  - Popular and mature
  - Contains `unsafe`, dependencies full of `unsafe`
  - Heavy dependencies (libc, winapi, rand, etc.)
- [`test_dir`](https://crates.io/crates/test_dir)
  - Has a handy `TestDir` struct
  - Incomplete documentation
- [`temp_testdir`](https://crates.io/crates/temp_testdir)
  - Incomplete documentation
- [`mktemp`](https://crates.io/crates/mktemp)
  - Sets directory mode 0700 on unix
  - Contains `unsafe`
  - No readme or online docs

# Related Crates
- [`temp-file`](https://crates.io/crates/temp-file)

# Example
```rust
use temp_dir::TempDir;
let d = TempDir::new().unwrap();
// Prints "/tmp/t1a9b-0".
println!("{:?}", d.path());
let f = d.child("file1");
// Prints "/tmp/t1a9b-0/file1".
println!("{:?}", f);
std::fs::write(&f, b"abc").unwrap();
assert_eq!(
    "abc",
    std::fs::read_to_string(&f).unwrap(),
);
// Prints "/tmp/t1a9b-1".
println!(
    "{:?}", TempDir::new().unwrap().path());
```

# Cargo Geiger Safety Report
```

Metric output format: x/y
    x = unsafe code used by the build
    y = total unsafe code found in the crate

Symbols:
    🔒  = No `unsafe` usage found, declares #![forbid(unsafe_code)]
    ❓  = No `unsafe` usage found, missing #![forbid(unsafe_code)]
    ☢️  = `unsafe` usage found

Functions  Expressions  Impls  Traits  Methods  Dependency

0/0        0/0          0/0    0/0     0/0      🔒  temp-dir 0.1.16

0/0        0/0          0/0    0/0     0/0

```
# Changelog
- v0.1.16 - `dont_delete_on_drop()`.  Thanks to [A L Manning](https://gitlab.com/A-Manning) for [discussion](https://gitlab.com/leonhard-llc/ops/-/merge_requests/5).
- v0.1.15 - Remove a dev dependency.
- v0.1.14 - `AsRef<Path>`
- v0.1.13 - Update docs.
- v0.1.12 - Work when the directory already exists.
- v0.1.11
  - Return `std::io::Error` instead of `String`.
  - Add
    [`cleanup`](https://docs.rs/temp-file/latest/temp_file/struct.TempFile.html#method.cleanup).
- v0.1.10 - Implement `Eq`, `Ord`, `Hash`
- v0.1.9 - Increase test coverage
- v0.1.8 - Add [`leak`](https://docs.rs/temp-dir/latest/temp_dir/struct.TempDir.html#method.leak).
- v0.1.7 - Update docs:
  Warn about `std::fs::remove_dir_all` being unreliable on Windows.
  Warn about predictable directory and file names.
  Thanks to Reddit user
  [burntsushi](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/gruo5iu/).
- v0.1.6 - Add
  [`TempDir::panic_on_cleanup_error`](https://docs.rs/temp-dir/latest/temp_dir/struct.TempDir.html#method.panic_on_cleanup_error).
  Thanks to Reddit users
  [`KhorneLordOfChaos`](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/grsb5s3/)
  and
  [`dpc_pw`](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/gru26df/)
  for their comments.
- v0.1.5 - Explain how it handles symbolic links.
  Thanks to Reddit user Mai4eeze for this
  [idea](https://www.reddit.com/r/rust/comments/ma6y0x/tempdir_simple_temporary_directory_with_cleanup/grsoz2g/).
- v0.1.4 - Update docs
- v0.1.3 - Minor code cleanup, update docs
- v0.1.2 - Update docs
- v0.1.1 - Fix license
- v0.1.0 - Initial version

License: Apache-2.0
