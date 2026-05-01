# Dropbox Review Notes

OpenKara's Dropbox integration is a desktop OAuth flow for remote repository
storage. It stores OpenKara-created karaoke library files in a Dropbox folder
and refreshes or publishes the local portable library database, imported media, CD+G files, and
processed stem files across devices.

## Public Test Builds

Download the latest release from:

- https://github.com/thedavidweng/OpenKara/releases/latest

Current release assets include:

- macOS Apple Silicon: `OpenKara_<version>_aarch64.dmg`
- macOS Intel: `OpenKara_<version>_x64.dmg`
- Windows: `OpenKara_<version>_x64-setup.exe`
- Linux: `OpenKara_<version>_amd64.AppImage` or `OpenKara_<version>_amd64.deb`

No OpenKara account is required. Reviewers should use their own Dropbox account
for the OAuth consent flow.

On macOS, drag `OpenKara.app` from the DMG into `/Applications`. If Gatekeeper
blocks the unsigned build, run:

```bash
xattr -rd com.apple.quarantine /Applications/OpenKara.app
open /Applications/OpenKara.app
```

## Dropbox App Configuration

Recommended Dropbox app settings:

- Access type: App Folder
- Redirect URI: `http://localhost:53682/oauth2/callback`
- Required scopes:
  - `files.metadata.read`
  - `files.content.read`
  - `files.content.write`

The app does not need `account_info.read` because OpenKara uses the
`account_id` returned by the OAuth token response as the stable remote account
identifier. The app also does not need `files.metadata.write`; OpenKara does not
call Dropbox file properties endpoints.

## Why Each Scope Is Needed

`files.metadata.read` is used with `/2/files/get_metadata` to detect whether the
remote repository folder, `.openkara-library`, `openkara.db`, media, and stem files
already exist, and to read revision metadata for conflict checks.

`files.content.read` is used with `/2/files/download` to download the remote
repository database, original audio files, CD+G graphics, and processed stem files
into the local working copy when opening or playing a remote repository.

`files.content.write` is used with `/2/files/create_folder_v2`,
`/2/files/upload`, and `/2/files/delete_v2` to create the OpenKara app folder,
publish `openkara.db`, upload imported media and processed stems, and delete
repository files when the user deletes the remote repository from OpenKara.

## Review Smoke Test

1. Install and launch OpenKara.
2. On first launch, create a local karaoke library in any writable folder.
3. Open Settings, choose Remote Repository, select Dropbox, and click Connect
   Dropbox.
4. Complete the Dropbox authorization flow in the browser.
5. Return to OpenKara and create a new Dropbox remote repository.
6. Import a small local audio file.
7. Publish the library to the Dropbox-backed remote repository.
8. Close and reopen OpenKara, open the same Dropbox remote repository, and confirm
   the library metadata and imported file are visible.
