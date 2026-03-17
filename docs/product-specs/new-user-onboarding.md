# New User Onboarding

## Goal

A new user should be able to install OpenKara, point it at a music library, import songs, and start playback without reading internal engineering docs.

## First-Run Flow

1. Launch the app.
2. Pick or confirm a library directory.
3. Import one or more supported audio files.
4. See imported songs appear in the library list with basic metadata.
5. Start playback from the library.
6. Optionally fetch or edit lyrics.
7. Optionally download a separation model when the user needs karaoke stems.

## Expectations

- The first-run flow should not require a hosted account.
- Library setup should explain what directory is being used.
- Import should work with common local audio formats.
- The app should remain usable even before a model download finishes.
- Lyrics should degrade gracefully: cached, online, embedded, sidecar, or manual.

## Out Of Scope

- Cloud sync
- Multi-user accounts
- Hosted media libraries
