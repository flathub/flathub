# Rationale Comments Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add high-value rationale comments to OpenKara's most easily misread design decisions, and require future agents to preserve this practice in `AGENTS.md`.

**Architecture:** Keep this pass narrow and intentional. Add short comments only where the code embodies a product goal or non-obvious constraint that a future agent might otherwise undo. Update the repo agent rules so later work treats rationale comments as part of the implementation contract, not optional decoration.

**Tech Stack:** Rust, TypeScript, Markdown

---

### Task 1: Document the rationale-comment policy

**Files:**

- Modify: `AGENTS.md`
- Create: `docs/plans/2026-03-16-rationale-comments-design.md`

**Step 1: Write the policy update**

Add a new section to `AGENTS.md` explaining that agents must preserve and add rationale comments around product tradeoffs, portability rules, and performance/storage constraints.

**Step 2: Keep examples concrete**

Reference examples like OGG stem caching, relative library paths, and model bootstrap decisions so future agents understand the intent.

### Task 2: Annotate high-risk backend decisions

**Files:**

- Modify: `src-tauri/src/audio/encode.rs`
- Modify: `src-tauri/src/cache/stems.rs`
- Modify: `src-tauri/src/library_root.rs`
- Modify: `src-tauri/src/lib.rs`
- Modify: `src-tauri/src/commands/bootstrap.rs`
- Modify: `src-tauri/src/separator/model.rs`
- Modify: `src-tauri/src/commands/playback.rs`
- Modify: `src-tauri/src/commands/cdg.rs`

**Step 1: Add storage rationale comments**

Protect the OGG/Vorbis choice and any data-layout assumptions that affect library size.

**Step 2: Add portability and bootstrap rationale comments**

Protect relative-path storage and active-variant bootstrap semantics.

**Step 3: Add playback/CDG rationale comments**

Protect stale-request handling, non-fatal sidecar parsing, and first-frame/reset behavior.

### Task 3: Annotate high-risk frontend runtime decisions

**Files:**

- Modify: `src/hooks/use-cdg-sync.ts`

**Step 1: Add render-loop rationale comments**

Explain why CDG frames are painted directly to canvas and throttled, rather than pushed through React state.

### Task 4: Verify formatting

**Files:**

- Modify: none

**Step 1: Run required formatting gate**

Run: `pnpm format`
Expected: PASS
