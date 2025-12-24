# Product Guidelines - QKMJ Refactoring

## Documentation & Prose Style
- **Technical Precision:** All documentation, commit messages, and code comments must be technically accurate, objective, and precise.
- **Minimalism:** Prioritize clarity and brevity. Avoid unnecessary verbosity; focus on essential architectural details, "why" decisions, and usage instructions.
- **Language:** Code comments should primarily use Traditional Chinese (Taiwan), while technical documentation and commit messages should be in English for broader accessibility.

## Visual Design & UX ("Modernized Classic")
- **Aesthetic:** A "Modernized Classic" approach that preserves the original game's functional layout and core loop while employing modern (2025+) design principles.
- **Color Palette:** Leverage modern terminal capabilities (Extended 256/TrueColor) for a sophisticated, high-contrast, and professional theme. Avoid dated or muddy color combinations.
- **Readability:** Prioritize maximum legibility and accessibility. Use clear visual hierarchies and appropriate spacing to ensure the game state is understandable at a glance.

## AI Interaction & Presence
- **Discreet Integration:** In its current "assist" mode, the AI should operate transparently and seamlessly. Its presence should be subtle, avoiding disruption to the classic gameplay flow.
- **Future-Proofing:** Design internal structures to support the transition from an "assist" tool to a fully autonomous "summonable" player that can participate alongside humans without intervention.
- **Feedback:** Keep AI-related feedback minimal and non-intrusive, focusing on a fast-paced and immersive experience.

## Technical Standards
- **Refactoring Philosophy:** Systematically replace legacy C patterns (e.g., unsafe string handling, global state where avoidable) with modern, secure, and maintainable C11/C17 equivalents.
- **Stability & Performance:** Improvements should never come at the cost of stability. Maintain the lightweight and responsive nature of a CLI-based game.
