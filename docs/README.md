# Documentation

## Public Site

| Document                                 | Description                                              |
| ---------------------------------------- | -------------------------------------------------------- |
| [site/index.html](./site/index.html)     | Handcrafted homepage source for the public landing page  |
| [site/faq.html](./site/faq.html)         | Handcrafted FAQ page for the public website              |
| [site/privacy.html](./site/privacy.html) | Public privacy policy for the website and desktop app    |
| [site/terms.html](./site/terms.html)     | Public terms of service for the website and desktop app  |
| [site/\_layouts/](./site/_layouts)       | Custom Jekyll layouts for the homepage and content pages |
| [site/\_config.yml](./site/_config.yml)  | Jekyll configuration for the GitHub Pages site           |

Install guidance lives inside `docs/site/index.html` instead of a standalone install page.

## Design Docs

| Document                                                                                     | Description                                                       |
| -------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| [design-docs/index.md](./design-docs/index.md)                                               | Entry point for architecture, roadmap, release, and delivery docs |
| [design-docs/architecture.md](./design-docs/architecture.md)                                 | System architecture, tech stack, data flow, and runtime design    |
| [design-docs/core-beliefs.md](./design-docs/core-beliefs.md)                                 | Core product and engineering principles                           |
| [design-docs/project-structure.md](./design-docs/project-structure.md)                       | Directory layout and module responsibilities                      |
| [design-docs/development-phases.md](./design-docs/development-phases.md)                     | Executable development phase checklist with verify steps          |
| [design-docs/roadmap.md](./design-docs/roadmap.md)                                           | Technical roadmap, API contracts, and risk notes                  |
| [design-docs/milestones.md](./design-docs/milestones.md)                                     | Milestone task table with exit criteria and ownership tracking    |
| [design-docs/releasing.md](./design-docs/releasing.md)                                       | Release workflow, Homebrew distribution, and future channels      |
| [design-docs/performance/phase-5-baseline.md](./design-docs/performance/phase-5-baseline.md) | Backend benchmark baseline for profiling work                     |

## Execution Plans

| Document                                                                                                                 | Description                                                |
| ------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------------- |
| [exec-plans/active/index.md](./exec-plans/active/index.md)                                                               | In-progress or still-actionable implementation plans       |
| [exec-plans/completed/index.md](./exec-plans/completed/index.md)                                                         | Archived design and implementation plans                   |
| [exec-plans/tech-debt-tracker.md](./exec-plans/tech-debt-tracker.md)                                                     | Cross-cutting debt items that do not belong to one feature |
| [exec-plans/active/2026-03-16-docs-reorg-implementation.md](./exec-plans/active/2026-03-16-docs-reorg-implementation.md) | Current documentation cleanup plan                         |
| [plans/2026-04-19 Remote Library.md](./plans/2026-04-19%20Remote%20Library.md)                                           | Current remote-library implementation and completion plan  |

Point-in-time plans that no longer drive active work now live under [archive/](./archive/README.md). The top-level `docs/plans/` folder only keeps plans that still guide current or next-step implementation.

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

| Document                                 | Description                                                                |
| ---------------------------------------- | -------------------------------------------------------------------------- |
| [archive/README.md](./archive/README.md) | Historical plans and docs kept for traceability, not active implementation |
