---
layout: page
title: FAQ
eyebrow: FAQ
intro: A quick overview of the questions people usually ask before they try OpenKara.
description: Answers to common questions about OpenKara.
body_class: faq-markdown
permalink: faq.html
---

## What is OpenKara?

OpenKara is an open-source desktop karaoke app that turns your own music library into a local-first karaoke setup with on-device stem separation and synced lyrics.

## Does it upload my music?

No. The main workflow is designed around local files and on-device processing. Lyrics lookup can use online sources, but your audio library does not need to be uploaded.

## Which platforms are supported?

OpenKara targets macOS, Windows, and Linux desktop environments.

## Where do lyrics come from?

OpenKara can fetch synced lyrics from LRCLIB, read embedded tags, or use sidecar `.lrc` files stored next to the track.

## Can I move my library between machines?

Yes. The karaoke library is designed to be self-contained, so it can live on an external drive, NAS, or another location you carry between machines.

## How large is the AI model?

The first launch downloads an ONNX build of the Demucs model, roughly 80 MB, when it is needed for separation.

## Can I choose between simpler and more detailed separation?

Yes. OpenKara supports both 2-stem and 4-stem separation modes, and individual songs can be upgraded to 4 stems later when you want more control.

## Is the app meant for technical users?

No. The goal is to keep setup, import, and playback approachable for casual singers, even though the app is backed by a fairly advanced local audio pipeline.

## Where can I read more detail?

- [Project README](https://github.com/thedavidweng/OpenKara/blob/main/README.md)
- [README 中文](https://github.com/thedavidweng/OpenKara/blob/main/README_CN.md)
- [Architecture](https://github.com/thedavidweng/OpenKara/blob/main/docs/internal/architecture.md)
- [Release workflow](https://github.com/thedavidweng/OpenKara/blob/main/docs/internal/releasing.md)
