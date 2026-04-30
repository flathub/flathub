# Remote Library Hardening Plan

## Archived Status

Google Drive, Dropbox, and WebDAV are implemented as real remote-library
providers. This plan was completed and archived on 2026-04-30.

Completion notes:

- Remote database uploads now compare provider revision metadata before upload.
- User-visible remote-library mutations sync the latest remote database before
  applying the local edit, so the edit is replayed on the current working copy.
- Upload races that occur after the local edit are blocked with a Settings
  recovery message instead of overwriting the newer remote database.
- Settings exposes force resync, reconnect provider, and update credentials
  controls for registered remote libraries.
- Deterministic frontend and backend coverage was added for the revision guard
  and Settings recovery controls.
- Real-account OAuth/browser smoke coverage remains a release smoke-test item;
  it is no longer tracked as active implementation work in this plan.

## Completed Baseline

- Remote libraries can be registered, selected, synced, published to, and played
  from through the shared library registry.
- Google Drive and Dropbox use OAuth + PKCE and store user tokens in the system
  credential store.
- WebDAV validates server reachability and credentials before registration.
- Remote libraries use a cached local working copy instead of coupling the UI
  directly to each provider API.
- Remote songs and stems can be downloaded on demand for playback.

## Completed Goals

### 1. Remote database conflict handling

**Goal:** Prevent two OpenKara instances from silently overwriting each other's
`openkara.db` changes.

**Implementation direction:**

- Check provider-level revision metadata before uploading a changed database.
- When the remote revision changed, download the latest database, replay the
  current local single-song mutation, then retry upload.
- Apply the same writeback path to lyrics, title, artist, offset, and other
  user-visible metadata edits.

**Acceptance criteria:**

- Alternating edits from two devices do not blindly replace the newer remote
  database.
- The user-facing error explains whether OpenKara resolved the conflict or needs
  manual action.

### 2. Settings recovery UX

**Goal:** Make remote-library failure recovery possible from Settings without
requiring internal docs.

**Implementation direction:**

- Add explicit actions for reconnecting a provider, updating credentials, and
  forcing a resync.
- Keep remove/unregister behavior distinct from deleting remote data.
- Surface provider-specific recovery copy for authentication failure, server
  reachability failure, and remote-library initialization failure.

**Acceptance criteria:**

- A user can recover from expired OAuth credentials or changed WebDAV
  credentials from Settings.
- Failed sync and upload flows point to the next concrete user action.

### 3. Verification coverage

**Goal:** Cover the remote-library flows that currently depend on manual
provider checks.

**Implementation direction:**

- Keep backend provider tests for URL construction, credential resolution, and
  sync-path behavior.
- Add UI-level coverage for reconnect, update-credentials, and forced-resync
  states once those controls exist.
- Track real-account OAuth/browser smoke coverage separately from deterministic
  automated tests.

**Acceptance criteria:**

- Deterministic tests cover the UI state transitions and backend command paths.
- Real-account OAuth smoke coverage is documented when it is run, including
  provider, platform, and result.
