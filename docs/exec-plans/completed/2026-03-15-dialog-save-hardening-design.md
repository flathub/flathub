# Dialog Save Hardening Design

## Goal

Harden metadata and lyrics editing flows so failed saves do not silently close dialogs, discard user input, or allow accidental double-submission.

## Scope

- `src/components/Library/SongEditDialog.tsx`
- `src/components/Lyrics/LyricsEditDialog.tsx`
- `src/stores/library-store.ts`
- `src/stores/lyrics-store.ts`

## Problems Observed

1. Store save actions catch errors internally and do not report success or failure to callers.
2. Both dialogs close immediately after awaiting the store action, even if persistence failed.
3. Failed saves rely on global toasts only, which is easy to miss while the dialog closes.
4. Repeated Enter presses or button clicks can trigger duplicate save attempts while a request is already in flight.

## Desired Behavior

1. Dialogs close only after a confirmed successful save.
2. On failure, the dialog stays open and preserves the current input.
3. The user sees inline error feedback near the action area.
4. While saving, submit actions are disabled and Enter should not trigger another save.
5. Existing global error toasts may remain, but inline feedback becomes the primary recovery cue.

## Design

### Store Contract

Change save-oriented store methods to return a boolean success value:

- `updateSongMetadata(...) => Promise<boolean>`
- `saveManualLyrics(...) => Promise<boolean>`

`true` means the update succeeded and store state was updated.
`false` means an error was handled and the caller should stay in the current UI state.

This keeps the current store architecture intact while giving dialogs enough information to avoid false-success behavior.

### Dialog Behavior

Both dialogs should:

- track `saving` state locally
- track a local `saveError` message
- clear any stale error before a new save attempt
- guard against concurrent submits
- close only when the store reports success

Inline error messaging should be short and non-technical. If the thrown value is not useful, fall back to the existing translated generic error string.

### Error Presentation

Show inline save errors above the footer actions, styled consistently with the existing UI. The message should wrap instead of truncating so long filesystem or backend messages do not overflow the dialog.

### Accessibility

- Mark the inline error with `role="alert"`
- Keep focus in the dialog after a failed save
- Preserve current keyboard behavior except for suppressing repeat submit during save

## Testing Strategy

Use TDD with focused unit coverage for the new store return contract:

1. `library-store` returns `true` on successful metadata save
2. `library-store` returns `false` on failed metadata save
3. `lyrics-store` returns `true` on successful manual lyrics save
4. `lyrics-store` returns `false` on failed manual lyrics save

The dialogs then consume this explicit contract with minimal implementation changes.

## Out of Scope

- Global notification redesign
- Broader i18n or RTL work
- General dialog layout hardening beyond save error resilience
