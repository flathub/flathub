# Tech Debt Tracker

## Open Items

| Area                    | Debt                                                                                                                              | Why it matters                                                   | Suggested next step                                                              |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| Documentation ownership | Product specs are only partially backfilled. Most user-facing behavior still lives across `README`, `docs/site`, and design docs. | Product behavior can drift when no single spec owns it.          | Gradually add feature-level specs under `docs/product-specs/`.                   |
| Generated docs          | `docs/generated/db-schema.md` is refreshed manually today.                                                                        | Generated summaries become stale if no refresh workflow exists.  | Add a small script that regenerates the schema doc from `src-tauri/migrations/`. |
| Plan archival policy    | Active vs completed classification is currently hand-maintained.                                                                  | Useful plans can be lost or remain active too long.              | Define an explicit archival rule in `docs/exec-plans/active/index.md`.           |
| Dead-code review        | This pass removes one unused helper, but there is no recurring dead-code sweep.                                                   | Small unused utilities accumulate and add noise for future work. | Run periodic repo-wide usage checks during cleanup-oriented tasks.               |
