# Dialog Save Hardening Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Prevent lyrics and song metadata dialogs from closing on failed saves, preserve user input, and show inline save errors while blocking duplicate submissions.

**Architecture:** Add a minimal success/failure return contract to the existing Zustand store save actions, then update the two dialogs to use that contract for local save state and inline error handling. Keep the current global toast path intact, but make dialog-local feedback the primary recovery affordance.

**Tech Stack:** React 19, TypeScript, Zustand, Vitest

---

### Task 1: Add failing store tests for save result contracts

**Files:**

- Create: `src/stores/library-store.test.ts`
- Modify: `src/stores/queue-store.test.ts` (none expected)
- Test: `src/stores/library-store.test.ts`

**Step 1: Write the failing test**

Add tests that mock `@/lib/tauri` and assert `updateSongMetadata()` resolves to `true` on success and `false` on failure.

**Step 2: Run test to verify it fails**

Run: `pnpm test src/stores/library-store.test.ts`
Expected: FAIL because the method currently resolves without a boolean contract.

**Step 3: Write minimal implementation**

Update `src/stores/library-store.ts` so `updateSongMetadata()` returns `true` after a successful API update and `false` after catching an error.

**Step 4: Run test to verify it passes**

Run: `pnpm test src/stores/library-store.test.ts`
Expected: PASS

### Task 2: Add failing lyrics store tests for save result contracts

**Files:**

- Create: `src/stores/lyrics-store.test.ts`
- Modify: `src/stores/lyrics-store.ts`
- Test: `src/stores/lyrics-store.test.ts`

**Step 1: Write the failing test**

Add tests that mock `@/lib/tauri` and assert `saveManualLyrics()` resolves to `true` on success and `false` on failure.

**Step 2: Run test to verify it fails**

Run: `pnpm test src/stores/lyrics-store.test.ts`
Expected: FAIL because the method currently resolves without a boolean contract.

**Step 3: Write minimal implementation**

Update `src/stores/lyrics-store.ts` so `saveManualLyrics()` returns `true` after a successful API update and `false` after catching an error.

**Step 4: Run test to verify it passes**

Run: `pnpm test src/stores/lyrics-store.test.ts`
Expected: PASS

### Task 3: Harden song metadata dialog save flow

**Files:**

- Modify: `src/components/Library/SongEditDialog.tsx`
- Test: existing store tests cover the success contract

**Step 1: Add local dialog save state**

Track `saving` and `saveError` locally. Clear `saveError` before a new save attempt.

**Step 2: Guard against duplicate submission**

Return early in `handleSave()` if `saving` is already true. Keep Enter handling gated on `saving`.

**Step 3: Use explicit success contract**

Await `updateSongMetadata(...)`. Close the dialog only if it returns `true`; otherwise keep it open and set an inline fallback error if needed.

**Step 4: Add inline error presentation**

Render an inline wrapped error block with `role="alert"` above the footer actions.

### Task 4: Harden lyrics dialog save flow

**Files:**

- Modify: `src/components/Lyrics/LyricsEditDialog.tsx`
- Test: existing store tests cover the success contract

**Step 1: Add local dialog save state**

Track `saving` and `saveError` locally. Reset stale error text when reopening the dialog.

**Step 2: Guard against duplicate submission**

Return early when already saving. Disable the save button while the request is active.

**Step 3: Use explicit success contract**

Await `saveManualLyrics(...)`. Close only on `true`; otherwise keep the dialog open and show inline feedback.

**Step 4: Keep text intact on failure**

Do not reset the textarea after failed persistence.

### Task 5: Verify integrated behavior

**Files:**

- Modify: none
- Test: `src/stores/library-store.test.ts`
- Test: `src/stores/lyrics-store.test.ts`

**Step 1: Run focused tests**

Run: `pnpm test src/stores/library-store.test.ts src/stores/lyrics-store.test.ts src/stores/queue-store.test.ts src/components/Player/queue-dnd.test.ts`
Expected: PASS

**Step 2: Run lint if affected files compile cleanly**

Run: `pnpm lint`
Expected: PASS

**Step 3: Review diff for scope control**

Confirm only the two stores, two dialogs, and new test files changed, plus plan docs.

**Step 4: Commit**

Only if explicitly requested by the user.
