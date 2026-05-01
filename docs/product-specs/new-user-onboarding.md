# New User Onboarding

## Goal

A new user should be able to install OpenKara, point it at a music library, import songs, and start playback without reading internal engineering docs.

## First-Run Flow

1. Launch the app.
2. Choose a language.
3. Pick one of the first-run library paths:
   - create a new local library
   - open an existing local library
   - connect a remote repository
4. If the user chooses a remote repository, guide them through provider-specific setup.
5. See the active library open with songs and metadata available from the selected source.
6. Start playback from the library.
7. Optionally fetch or edit lyrics.
8. Optionally download a separation model when the user needs karaoke stems.

## Expectations

- The first-run flow should not require a hosted account.
- Library setup should explain what directory or remote path is being used.
- Import should work with common local audio formats.
- The app should remain usable even before a model download finishes.
- Lyrics should degrade gracefully: cached, online, embedded, sidecar, or manual.
- Remote setup should clearly distinguish Google Drive, Dropbox, and WebDAV provider requirements.

## Remote Repository Expectations

- A user connecting by Google Drive should be guided through browser-based OAuth and brought back into OpenKara without having to understand the Drive API model.
- A user connecting by WebDAV should be able to enter the server URL, credentials, and target repository path without reading engineering docs.
- If remote setup fails, the UI should explain whether the problem is authentication, server reachability, or remote repository initialization.
- Google Drive, Dropbox, and WebDAV should all be presented as working provider flows, with provider-specific setup and recovery guidance.
- Refreshing a remote repository means updating the local working copy from the remote database, not publishing local edits.
- Reauthorizing a remote repository renews provider access. If the repository location changed, OpenKara should ask before replacing the saved location and should reject empty locations that are not already OpenKara repositories.
- Disconnecting a remote repository removes only the local OpenKara registration and credentials; deleting one removes the provider-hosted repository contents and local working copy.
