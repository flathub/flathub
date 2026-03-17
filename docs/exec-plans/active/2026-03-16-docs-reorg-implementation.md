# Documentation Reorganization Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Reorganize OpenKara's documentation into the requested structure, repair stale documentation references, remove low-risk dead files and code, and fix naming drift without moving application source directories.

**Architecture:** Treat the work as a docs-first migration with a small cleanup tail. Move existing documents into one canonical tree under `docs/design-docs`, `docs/exec-plans`, `docs/references`, `docs/generated`, and `docs/product-specs`, then repair every known inbound link. Keep cleanup conservative by deleting only files or helpers that repository-wide search proves are unused.

**Tech Stack:** Markdown, HTML, TypeScript, Rust, pnpm, Vite

---

### Task 1: Establish the new documentation skeleton

**Files:**

- Create: `ARCHITECTURE.md`
- Create: `docs/design-docs/index.md`
- Create: `docs/design-docs/core-beliefs.md`
- Create: `docs/exec-plans/active/index.md`
- Create: `docs/exec-plans/completed/index.md`
- Create: `docs/exec-plans/tech-debt-tracker.md`
- Create: `docs/generated/db-schema.md`
- Create: `docs/product-specs/index.md`
- Create: `docs/product-specs/new-user-onboarding.md`
- Create: `docs/references/index.md`
- Modify: `docs/README.md`

**Step 1: Create the requested top-level doc entry points**

Add the missing root `ARCHITECTURE.md` and index files for each new documentation category.

**Step 2: Rewrite `docs/README.md` around the new layout**

Make `docs/README.md` describe the canonical new directories instead of the old `internal/`, `contracts/`, and `plans/` split.

### Task 2: Migrate internal documentation into `docs/design-docs`

**Files:**

- Move: `docs/internal/README.md`
- Move: `docs/internal/architecture.md`
- Move: `docs/internal/project-structure.md`
- Move: `docs/internal/development-phases.md`
- Move: `docs/internal/roadmap.md`
- Move: `docs/internal/milestones.md`
- Move: `docs/internal/releasing.md`
- Move: `docs/internal/performance/phase-5-baseline.md`
- Modify: `docs/design-docs/architecture.md`
- Modify: `docs/design-docs/project-structure.md`

**Step 1: Move the docs without duplicating ownership**

Use `docs/design-docs/` as the only canonical home for architecture and engineering-process documentation.

**Step 2: Refresh stale content while moving**

Update the architecture doc to remove the Musixmatch fallback claim and rewrite the project-structure doc so it reflects the current repository layout.

### Task 3: Re-home plans and references

**Files:**

- Move: `docs/plans/*.md`
- Move: `docs/contracts/*.md`
- Move: `docs/contracts/README.md`
- Modify: `docs/exec-plans/active/*.md`
- Modify: `docs/references/contracts/*.md`

**Step 1: Split execution plans by status**

Place still-relevant plans in `docs/exec-plans/active/` and archive finished work in `docs/exec-plans/completed/`.

**Step 2: Move frozen contracts under references**

Put backend contract docs under `docs/references/contracts/` and repair their cross-links to the active execution plans.

### Task 4: Repair inbound and internal links

**Files:**

- Modify: `README.md`
- Modify: `README_CN.md`
- Modify: `docs/README.md`
- Modify: `docs/site/index.html`
- Modify: `docs/site/faq.md`
- Modify: `src-tauri/src/separator/README.md`

**Step 1: Update repo entry points**

Rewrite the root README links and the public site links so they point at the new canonical doc locations.

**Step 2: Repair moved-doc cross references**

Update active plans, contract docs, and backend READMEs so no live link points at removed directories.

### Task 5: Add generated and product-spec docs

**Files:**

- Create: `docs/generated/db-schema.md`
- Create: `docs/product-specs/index.md`
- Create: `docs/product-specs/new-user-onboarding.md`
- Create: `docs/exec-plans/tech-debt-tracker.md`

**Step 1: Summarize the database schema from migrations**

Describe the current SQLite tables and migration history from `src-tauri/migrations/*.sql` in `docs/generated/db-schema.md`.

**Step 2: Seed the product-spec area**

Add an onboarding spec and an index that explain what product-facing docs belong under `docs/product-specs/`.

### Task 6: Remove low-risk dead files and dead code

**Files:**

- Delete: `docs/.DS_Store`
- Delete: `public/.DS_Store`
- Delete: `src-tauri/icons/.DS_Store`
- Delete: `src/lib/cover-art.ts`

**Step 1: Remove tracked OS junk files**

Delete tracked `.DS_Store` files so they stop polluting the repository.

**Step 2: Remove the unused cover-art helper**

Delete `src/lib/cover-art.ts` only if repository-wide search still shows no callers.

### Task 7: Verify and finish

**Files:**

- Modify: none

**Step 1: Run the required formatting gate**

Run: `pnpm format`
Expected: PASS

**Step 2: Run frontend verification**

Run: `pnpm lint`
Expected: PASS

**Step 3: Run build verification**

Run: `pnpm build`
Expected: PASS

**Step 4: Commit**

Only if explicitly requested by the user.
