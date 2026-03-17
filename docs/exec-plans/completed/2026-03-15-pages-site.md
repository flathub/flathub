# GitHub Pages Microsite Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Reorganize `docs/` for a clean public/private split and add a lightweight GitHub Pages microsite for OpenKara, with Markdown-driven public FAQ content.

**Architecture:** Public pages live in `docs/site/` as a small Jekyll site with custom layouts, while contributor-facing docs move to `docs/internal/`. A standalone GitHub Pages workflow builds `docs/site/` with Jekyll and uploads the generated `_site` artifact so the Tauri application build remains unchanged.

**Tech Stack:** Jekyll, HTML, CSS, Markdown, GitHub Actions

---

### Task 1: Move contributor docs into `docs/internal/`

**Files:**

- Modify: `docs/README.md`
- Create: `docs/internal/README.md`
- Move: `docs/architecture.md`
- Move: `docs/project-structure.md`
- Move: `docs/development-phases.md`
- Move: `docs/roadmap.md`
- Move: `docs/milestones.md`
- Move: `docs/RELEASING.md`
- Move: `docs/phase-5-performance-baseline.md`

**Step 1:** Create the `docs/internal/` and `docs/internal/performance/` folders.

**Step 2:** Move the existing engineering docs into those folders with stable, readable names.

**Step 3:** Rewrite `docs/README.md` as a top-level index covering the public site, internal docs, contracts, and plans.

**Step 4:** Add `docs/internal/README.md` as a dedicated internal-doc index.

**Step 5:** Verify all moved-file links resolve with a repo-wide path search.

### Task 2: Add the public GitHub Pages site

**Files:**

- Create: `docs/site/_config.yml`
- Create: `docs/site/_layouts/default.html`
- Create: `docs/site/_layouts/landing.html`
- Create: `docs/site/_layouts/page.html`
- Create: `docs/site/index.md`
- Create: `docs/site/faq.md`
- Create: `docs/site/assets/css/site.css`

**Step 1:** Create a strong landing page that explains what OpenKara does, reuses the app icon, and folds install guidance into the homepage.

**Step 2:** Add a Markdown FAQ page that Jekyll renders into the public site.

**Step 3:** Implement shared layouts so homepage design and Markdown content pages can coexist cleanly.

**Step 4:** Implement one shared stylesheet with a lightweight but product-facing visual system.

**Step 5:** Keep all links and assets compatible with the GitHub project Pages base path.

### Task 3: Wire deployment and repo references

**Files:**

- Create: `.github/workflows/pages.yml`
- Modify: `README.md`
- Modify: `README_CN.md`
- Modify: `docs/plans/2026-03-13-code-agent-plan.md`
- Modify: `docs/plans/2026-03-13-handoff-master-plan.md`
- Modify: `docs/plans/2026-03-13-ui-agent-plan.md`

**Step 1:** Add a GitHub Pages workflow that builds the Jekyll site and uploads `docs/site/_site` as the deploy artifact.

**Step 2:** Update the README documentation links to point at `docs/internal/`.

**Step 3:** Update old plan docs that still point at the former root-level docs paths.

**Step 4:** Run verification commands for formatting and build output.

**Step 5:** Review the final diff, stage only the intended files, and commit with a docs/site-focused message.
