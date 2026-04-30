# Tech Debt Tracker

## Open Items

| Area                    | Debt                                                                                                                            | Why it matters                                                   | Suggested next step                                                              |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| Documentation ownership | Product specs are only partially backfilled. Most user-facing behavior still lives across `README`, `website`, and design docs. | Product behavior can drift when no single spec owns it.          | Gradually add feature-level specs under `docs/product-specs/`.                   |
| Generated docs          | `docs/generated/db-schema.md` is refreshed manually today.                                                                      | Generated summaries become stale if no refresh workflow exists.  | Add a small script that regenerates the schema doc from `src-tauri/migrations/`. |
| Plan archival policy    | Active vs archived classification is currently hand-maintained.                                                                 | Useful plans can be lost or remain active too long.              | Keep only still-actionable plans in `docs/exec-plans/active/`.                   |
| Dead-code review        | This pass confirmed `src/lib/cover-art.ts` is still used, but there is no recurring dead-code sweep.                            | Small unused utilities accumulate and add noise for future work. | Run periodic repo-wide usage checks during cleanup-oriented tasks.               |
