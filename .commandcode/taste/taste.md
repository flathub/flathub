# Taste (Continuously Learned by [CommandCode][cmd])

[cmd]: https://commandcode.ai/

# Architecture

- For macOS native features, use AppKit (Objective-C) not SwiftUI. Confidence: 0.90
- When modifying or refactoring, do not add fallback/compatibility shim code — clean break with the new implementation only. Confidence: 0.85

# Workflow

- Apply first-principles reasoning: start from the original need and problem, do not assume the user knows exactly what they want or how to get it. If motivation or goals are unclear, pause and discuss before proceeding. Confidence: 0.85

# Code Style

- Remove dead/orphaned code when replacing functionality — do not leave old paths or compatibility branches. Confidence: 0.80
