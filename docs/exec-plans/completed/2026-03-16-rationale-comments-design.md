# Rationale Comments Design

**Goal:** Preserve product and architecture decisions in code so future agents do not "simplify" behavior in ways that break OpenKara's actual goals.

## Scope

This pass adds comments only to high-risk decision points that are easy to misread as arbitrary implementation detail.

The target is not broad inline documentation. The target is compact "why this must stay this way" notes for places where a reasonable refactor could violate project intent.

## Comment Rules

- Explain `why`, not `what`
- Prefer 1-3 lines directly above the risky branch, constant, or helper
- Mention the product or operational tradeoff being protected
- Avoid narrating obvious control flow
- Keep comments stable enough to outlive small refactors

## Initial High-Risk Areas

### Audio and Stem Storage

- `src-tauri/src/audio/encode.rs`
- `src-tauri/src/cache/stems.rs`

These files need rationale comments explaining that OGG/Vorbis is a deliberate library-size tradeoff, not an incidental codec choice. Agents should understand that switching cached stems back to WAV may help a local debugging scenario while directly harming OpenKara's storage goals.

### Library Path Portability

- `src-tauri/src/library_root.rs`

These comments should explain that the database stores relative, forward-slash-normalized paths so a library can be moved between machines and operating systems without rewriting the database.

### Model Bootstrap and Variant Resolution

- `src-tauri/src/lib.rs`
- `src-tauri/src/commands/bootstrap.rs`
- `src-tauri/src/separator/model.rs`

These files need comments that protect the distinction between "file exists" and "active model is verified and ready". They also need to preserve variant-aware resolution so future agents do not collapse everything back to the default standard model.

### Playback and CDG Synchronization

- `src-tauri/src/commands/playback.rs`
- `src-tauri/src/commands/cdg.rs`
- `src/hooks/use-cdg-sync.ts`

These comments should explain the non-obvious choices around stale-request protection, non-fatal sidecar parsing, first-frame CDG rendering, backward-seek resets, and drawing directly to canvas instead of driving high-frequency frame updates through React state.

## Agent Guidance Update

`AGENTS.md` should explicitly require agents to preserve and add rationale comments whenever code reflects a product tradeoff, compatibility constraint, or performance/storage decision that is not obvious from the implementation alone.
