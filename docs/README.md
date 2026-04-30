# Documentation

The public website source lives at [`../website/`](../website/) so this folder
can stay focused on engineering docs, product specs, references, generated
summaries, and historical records.

## Design Docs

| Document                                                                                     | Description                                                       |
| -------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| [design-docs/index.md](./design-docs/index.md)                                               | Entry point for architecture, roadmap, release, and delivery docs |
| [design-docs/architecture.md](./design-docs/architecture.md)                                 | System architecture, tech stack, data flow, and runtime design    |
| [design-docs/core-beliefs.md](./design-docs/core-beliefs.md)                                 | Core product and engineering principles                           |
| [design-docs/project-structure.md](./design-docs/project-structure.md)                       | Directory layout and module responsibilities                      |
| [design-docs/roadmap.md](./design-docs/roadmap.md)                                           | Technical roadmap, API contracts, and risk notes                  |
| [design-docs/releasing.md](./design-docs/releasing.md)                                       | Release workflow, Homebrew distribution, and future channels      |
| [design-docs/performance/phase-5-baseline.md](./design-docs/performance/phase-5-baseline.md) | Backend benchmark baseline for profiling work                     |

## Execution Plans

| Document                                                             | Description                                                |
| -------------------------------------------------------------------- | ---------------------------------------------------------- |
| [exec-plans/active/index.md](./exec-plans/active/index.md)           | In-progress or still-actionable implementation plans       |
| [exec-plans/tech-debt-tracker.md](./exec-plans/tech-debt-tracker.md) | Cross-cutting debt items that do not belong to one feature |

Point-in-time plans that no longer drive active work live under
[archive/](./archive/README.md). Active plans live under
`docs/exec-plans/active/`.

## Generated Docs

| Document                                           | Description                                           |
| -------------------------------------------------- | ----------------------------------------------------- |
| [generated/db-schema.md](./generated/db-schema.md) | Current SQLite schema summary derived from migrations |

## Product Specs

| Document                                                                       | Description                                     |
| ------------------------------------------------------------------------------ | ----------------------------------------------- |
| [product-specs/index.md](./product-specs/index.md)                             | Product-spec index and ownership guidance       |
| [product-specs/new-user-onboarding.md](./product-specs/new-user-onboarding.md) | First-run and first-import experience reference |

## References

| Document                                                                                                               | Description                                                    |
| ---------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| [references/index.md](./references/index.md)                                                                           | Reference-doc index                                            |
| [references/contracts/README.md](./references/contracts/README.md)                                                     | Frozen backend contract index                                  |
| [references/contracts/phase-6-model-bootstrap-contract.md](./references/contracts/phase-6-model-bootstrap-contract.md) | Runtime model bootstrap contract for current distribution work |

## Archive

| Document                                                                                 | Description                                                                |
| ---------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| [archive/README.md](./archive/README.md)                                                 | Historical plans and docs kept for traceability, not active implementation |
| [archive/plans/remote-library-hardening.md](./archive/plans/remote-library-hardening.md) | Completed remote-library conflict, recovery, and verification plan         |
