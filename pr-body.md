<!-- ⚠️⚠️  Submission pull request MUST be made against the `new-pr` **base branch** ⚠️⚠️  -->

### Please confirm your submission meets all the criteria

- [x] Please describe the application briefly.

**QuixiCore** is a native Rust desktop workspace for AI. It runs models locally or over the internet,
keeps its memory in a folder of markdown you own, and works on your own files. Upstream QuixiDesk is
excellent on macOS and does nothing on Linux; this build carries the Linux platform layer that fixes
that: screen capture through the KDE `ScreenShot2` portal in-process (~29 ms for a full desktop versus
~921 ms for `spectacle`), Wayland clipboard read/write, synthetic input through `ydotool`, file and
folder pickers, display listing, cursor position, and application launching — all wired to real Linux
backends rather than the silent no-ops upstream ships.

It is MIT licensed. Upstream: https://github.com/QuixiAI/QuixiDesk.

- [x] Please attach a video showcasing the application on Linux using the Flatpak.

https://github.com/volkdude85/QuixiDesk/releases/download/v3.6556.12-linux/quixidesk-flatpak.mp4

Recorded from this exact manifest building the app inside the sandbox, launched with
`flatpak run io.github.volkdude85.QuixiDesk`, running on X11.

- [x] The Flatpak ID follows all the rules listed in the [Application ID requirements][appid].

`io.github.volkdude85.QuixiDesk` — the id maps to https://github.com/volkdude85/QuixiDesk, which exists
and is where the source tarball for this manifest is published.

- [x] I have read and followed all the [Submission requirements][reqs] and the [Submission guide][reqs2] and I agree to them.
  - [x] The application has a meaningful development history, evidence of real-world use, and a clear commitment to ongoing maintenance, as required by the [development history requirements][history].

The packaging lives in https://github.com/volkdude85/quixidesk-volkdude85-published, which carries the
full patch series (ninety-seven patches against upstream v3.6556.12, each described in `PATCHES.md`),
a PKGBUILD that has shipped tagged releases, and CI that builds both the Arch package and this Flatpak
on every push. The Flatpak has been building from this manifest since before this submission; the
source tarball referenced below is a tagged release of the fork.

  - [x] I have disclosed any AI-generated material included in the application or its Flathub packaging, as required by the [Generative AI policy][ai]. **Affected parts and approximate extent:**
None.
  - [x] I have not used AI tools or agents to generate or automate this submission pull request or its review interactions.
- [x] I am an author/developer/upstream contributor to the project. If not, I contacted upstream developers about this submission. **Link:**
This is a fork of QuixiAI/QuixiDesk under a distinct name and id, adding the Linux platform layer. The
upstream author (Eric Hartford, @ehartford) has write access to the packaging repository and has been
given a full write-up of what this edition changes, including where this Flatpak lives:
https://github.com/volkdude85/quixidesk-arch/issues/1

### Notes for reviewers

- **Source.** `quixidesk-linux-3.6556.12.tar.gz` is upstream v3.6556.12 with the Linux series already
  applied, published as one tagged release asset rather than ninety-seven patch sources. sha256
  `fefdbbc7cf9ff761aab17304da68961602cf4be2a9f23df8c95fa43879cfc00f`.
- **Crates.** `cargo-sources.json` was generated from the tree's `Cargo.lock`; 564 crate archives, all
  hashed against `static.crates.io`. The build runs `cargo build --release --offline` with
  `CARGO_NET_OFFLINE=true`, so nothing is fetched inside the sandbox.
- **Verified offline.** Built with `flatpak-builder` from a clean state directory with the tarball
  fetched from the URL above and no local sources: 26,978,240-byte binary, appstream composed, desktop
  entry and metainfo exported. That is the run the video above was recorded from.
- **`--filesystem=home`.** The application is a workspace: it opens projects from the user's own
  directories, and its memory is a folder of markdown the user owns and can read. Narrowing this to
  specific subpaths would put the user's own workspace outside the sandbox and break the feature that
  makes it a workspace rather than a demo. If you would rather see a narrower set, say which and I will
  change it.
- **`--talk-name=org.kde.KWin*`.** Screen capture and window control go through the KDE portal and its
  scripting interface. Without the talk-names the sandbox blocks every screen read and the feature
  looks broken rather than blocked. On non-KDE desktops it falls back to the portal alone.

<!-- Please mention below the GitHub usernames of any additional maintainers needed (if any) -->

<!-- ⚠️⚠️  Please DO NOT modify anything below this line ⚠️⚠️  -->

[appid]: https://docs.flathub.org/docs/for-app-authors/requirements#application-id
[reqs]: https://docs.flathub.org/docs/for-app-authors/requirements
[reqs2]: https://docs.flathub.org/docs/for-app-authors/submission
[history]: https://docs.flathub.org/docs/for-app-authors/requirements#insufficient-development-history
[ai]: https://docs.flathub.org/docs/for-app-authors/requirements#generative-ai-policy
