---
layout: landing
title: OpenKara
description: Turn your music library into a karaoke stage with on-device AI stem separation and synced lyrics.
hero:
  eyebrow: Open-source desktop karaoke, shaped for local music libraries.
  title: Turn your own songs into a cleaner, calmer karaoke setup.
  body: OpenKara keeps the heavy work on your machine so you can import tracks you already own, fetch synced lyrics, separate stems, and start singing without wrestling with a streaming workflow.
  note: A local-first desktop app built for casual singers who want clear setup, fast playback, and less friction.
  primary:
    label: Download releases
    href: https://github.com/thedavidweng/OpenKara/releases
  secondary:
    label: Read the FAQ
    href: faq.html
  meta:
    - macOS, Windows, and Linux
    - On-device stem separation
    - Synced lyrics from LRCLIB, tags, or sidecar files
  signals:
    - label: Local audio import
      title: Bring your own files.
      body: Use music you already own. No subscription wall, no forced catalog, no cloud-first detour.
    - label: Portable library
      title: Keep the library moveable.
      body: Songs, cache, and metadata live in one self-contained directory that works across machines.
    - label: Karaoke playback
      title: Stay focused on singing.
      body: Playback, lyrics timing, and stem controls stay compact enough to feel like a desktop tool, not a dashboard.
features:
  - kicker: AI stem separation
    title: Separate vocals and accompaniment on-device.
    body: Start with a simple two-stem split or move into four stems when you want separate control over drums, bass, and other instruments.
    style: feature-large
  - kicker: Synced lyrics
    title: Use online lyrics, embedded tags, or nearby `.lrc` files.
    body: The lyrics system is designed to meet your files where they are instead of making you reformat everything first.
  - kicker: 4-stem mixer
    title: Pull the mix toward the way you actually want to sing.
    body: OpenKara gives vocals, drums, bass, and other instruments their own volume control while keeping the interface light.
    style: accent-block
  - kicker: Resumable separation
    title: Pick back up after interruptions.
    body: Chunk-level checkpoints help longer separation jobs continue from where they stopped instead of starting over.
  - kicker: Cross-platform desktop
    title: Keep one workflow across your machines.
    body: The app is built for macOS, Windows, and Linux with the same local-first approach on each platform.
  - kicker: Efficient stem storage
    title: Cache the heavy work without bloating the library.
    body: Compressed stem storage keeps repeat playback practical while still giving you fast reuse of finished separations.
install:
  release_formats:
    - platform: macOS Apple Silicon
      format: .dmg
    - platform: macOS Intel
      format: .dmg
    - platform: Windows
      format: .exe installer
    - platform: Linux
      format: .AppImage
  release_note: On first launch, OpenKara prompts you to create a karaoke library and download the AI model only when it is needed.
  release_cta:
    label: Open releases
    href: https://github.com/thedavidweng/OpenKara/releases
  prerequisites:
    - Node.js 20+
    - pnpm 10+
    - Rust stable toolchain
    - Tauri 2 platform prerequisites
  commands:
    - git clone https://github.com/thedavidweng/OpenKara.git
    - cd OpenKara
    - pnpm install
    - ./scripts/setup.sh
    - pnpm tauri dev
details:
  - title: Lyrics that adapt to your files
    body: OpenKara can fetch synced lyrics from LRCLIB, read embedded tags, or pick up sidecar `.lrc` files next to the track.
  - title: Demucs model download on first launch
    body: The ONNX build of Demucs is downloaded automatically, so the initial install stays lighter and the model arrives when the app needs it.
  - title: A self-contained karaoke library
    body: Relative paths and a single library directory make it practical to move your setup to a NAS, USB drive, or another machine later.
---
