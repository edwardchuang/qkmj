# Product Guide - QKMJ Refactoring

## Initial Concept
Refactor and modernize a 30-year-old Mahjong game (QKMJ) from C89/Big-5 to C11/C17/UTF-8, improving security, maintainability, and portability.

## Target Users
- **Legacy Enthusiasts:** Fans of the original QKMJ seeking a stable, modernized version that runs natively on current operating systems while preserving the classic feel.
- **Modern CLI Gamers:** Users who enjoy terminal-based games and are interested in a robust Mahjong experience enhanced by modern AI capabilities.

## Product Goals
- **Faithful Preservation:** Maintain the original game logic and user experience while upgrading the underlying technology stack.
- **Functional Enhancement:** Introduce modern features such as advanced AI opponents, persistent server-side logging via MongoDB, and improved networking protocols.
- **Technical Excellence:** Systematically refactor the legacy codebase to adhere to C11/C17 standards, replacing unsafe patterns with secure, modern alternatives.

## Key Features
- **AI-Powered Gameplay:** Integration with a specialized AI backend to provide intelligent decision-making for non-human players.
- **Modern Infrastructure:** Full containerization support for easy deployment and a robust logging system using MongoDB for game analysis and debugging.

## Quality Standards
- **Security First:** Elimination of legacy vulnerabilities such as buffer overflows by strictly using safe string and memory handling functions.
- **Maintainability:** Adoption of a clean, documented, and modular code structure (following Google C++ Style Guide adapted for C) to facilitate long-term maintenance.
