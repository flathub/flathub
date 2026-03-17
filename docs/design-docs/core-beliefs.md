# Core Beliefs

## Local First

OpenKara should work primarily with a singer's own library on their own machine.
Critical flows such as playback, lyrics editing, cache reuse, and model management should not depend on a hosted backend.

## Portable Libraries Matter

Library data should survive machine moves and folder moves when possible.
That is why the backend stores library paths in a normalized, relative form instead of hard-coding one machine's absolute paths.

## Expensive Work Must Be Reusable

Stem separation and lyrics lookup are slow or rate-limited operations.
Generated artifacts should be cached and reused so normal playback stays fast after the first preparation pass.

## Stable Contracts Beat Tribal Knowledge

Frontend work should consume explicit contracts instead of reverse-engineering Rust modules.
When behavior freezes for a phase, the contract docs should become the source of truth for shared command and payload semantics.

## Docs Live With The Code

Design notes, plans, generated summaries, and product specs should stay in the repository so architectural drift is visible in code review.
When implementation changes invalidate a document, updating the document is part of finishing the work.
