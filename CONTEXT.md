# OpenKara Context

This glossary captures domain language for OpenKara so product docs, contracts,
and implementation discussions use the same terms.

## Language

**Remote Repository (远程资料库)**:
A registered OpenKara library whose database and media artifacts are stored in a remote provider and opened through a local working copy.
_Avoid_: Remote library, remote song library, cloud folder

**Remote Provider**:
The storage service that hosts a **Remote Repository**, such as Google Drive, Dropbox, or WebDAV.
_Avoid_: Cloud account, backend

**Remote Repository Location**:
The provider-specific folder, path, or URL where a **Remote Repository** is stored.
_Avoid_: Locator, root, cloud folder

**Local Working Copy**:
The local cached copy of a **Remote Repository** that OpenKara opens and edits.
_Avoid_: Local mirror, cache

**Repository Credentials**:
The OAuth tokens or WebDAV username and password that let OpenKara access a **Remote Repository**.
_Avoid_: Login, account

**Reauthorize Repository (重新授权)**:
The recovery action that renews OpenKara's permission to access an existing **Remote Repository** without changing its repository location.
_Avoid_: Reconnect provider, update credentials, login again

**Relocate Repository**:
The confirmed recovery action that replaces a **Remote Repository Location** after the user moved the same repository in the remote provider.
_Avoid_: Reauthorize, overwrite old repository, connect new repository

**Remote Revision**:
The provider revision marker used to detect whether the remote database changed outside the current local working copy.
_Avoid_: Version, timestamp

**Refresh Repository**:
The action that updates a **Local Working Copy** from the current **Remote Repository** state.
_Avoid_: Sync, force resync

**Publish Changes**:
The action that writes local database or media changes from a **Local Working Copy** to a **Remote Repository**.
_Avoid_: Sync, upload database

**Publish Song**:
The action that makes one song and its required karaoke artifacts available in a **Remote Repository**.
_Avoid_: Sync song

**Mirror Local Library**:
The one-time action that initializes a **Remote Repository** from an existing local library.
_Avoid_: Sync local library

**Disconnect Repository**:
The action that removes a repository from OpenKara on the current device without deleting the repository contents.
_Avoid_: Delete library, remove data

**Delete Repository**:
The destructive action that deletes repository contents from their storage location; for a **Remote Repository**, this deletes the provider-hosted repository contents.
_Avoid_: Disconnect, remove registration

**Pre-Mutation Refresh**:
The automatic refresh OpenKara performs before a local edit when the remote revision is newer than the local working copy.
_Avoid_: Conflict merge, background sync

**Pre-Publish Conflict**:
A safety stop that occurs when the remote revision changes after the local edit but before OpenKara publishes the result.
_Avoid_: Sync failure, upload error

## Relationships

- A **Remote Repository** belongs to exactly one **Remote Provider** account and one **Remote Repository Location**.
- A **Remote Repository** has one **Local Working Copy** on each device that opens it.
- A **Local Working Copy** records the last known **Remote Revision** for conflict prevention.
- **Repository Credentials** grant access to a **Remote Repository**, but they are not the repository itself.
- **Reauthorize Repository** updates **Repository Credentials** for the same **Remote Repository** and must not change the **Remote Repository Location**.
- **Relocate Repository** updates the registered **Remote Repository Location** after explicit confirmation; it does not delete or overwrite contents at the old location.
- After **Relocate Repository**, OpenKara keeps the existing **Local Working Copy** directory but immediately performs **Refresh Repository** from the new location and records the new **Remote Revision**.
- **Relocate Repository** only accepts a location that already contains a valid OpenKara repository; an empty location belongs to new repository creation or mirroring, not relocation.
- **Refresh Repository** reads from a **Remote Repository** into a **Local Working Copy**.
- **Publish Changes** writes from a **Local Working Copy** into a **Remote Repository**.
- **Mirror Local Library** creates initial **Remote Repository** contents from a local library.
- **Disconnect Repository** removes OpenKara's local registration and credentials but leaves repository contents in place.
- **Delete Repository** removes repository contents and then disconnects the repository from OpenKara.
- A **Pre-Mutation Refresh** can proceed automatically because the user edit has not been applied yet.
- A **Pre-Publish Conflict** stops publication because applying the finished local edit over a newer remote database could overwrite another device.

## Example dialogue

> **Dev:** "If sync fails, should we reconnect the remote repository?"
> **Domain expert:** "Only if access expired. If the remote revision changed, refresh the local working copy first; if credentials expired, reauthorize the repository."

## Flagged ambiguities

- "Remote library" was used to mean both the user's karaoke library and the provider-hosted database/media container. Resolved: use **Remote Repository (远程资料库)** because the database also lives remotely.
- "Sync" was used for both refreshing from remote and publishing to remote. Resolved: use **Refresh Repository** for remote-to-local and **Publish Changes** for local-to-remote.
- "Remove" and "delete" were treated as similar Settings actions. Resolved: **Disconnect Repository** preserves repository contents, while **Delete Repository** deletes them from storage.
- "Reconnect provider" and "update credentials" were treated as separate user recovery actions. Resolved: use **Reauthorize Repository (重新授权)** for both OAuth renewal and WebDAV credential renewal, as long as the remote repository location does not change.
- "Overwrite the old one" during reauthorization means replacing OpenKara's registered remote location, not deleting or writing over data at the old remote location. Resolved: call this **Relocate Repository** and require explicit confirmation with a cancel path.
- Remote revision conflicts were used as one broad failure class. Resolved: **Pre-Mutation Refresh** is automatic, while **Pre-Publish Conflict** is a user-visible safety stop.
