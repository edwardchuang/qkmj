# Tech Stack - QKMJ Refactoring

## Core Technologies
- **Programming Languages:** 
  - **C11:** The primary language for the game client (`qkmj`) and server (`mjgps`).
  - **Python 3:** Used for the AI agent backend (`agent/`).
- **User Interface:** **ncursesw** (Wide character support for UTF-8/Traditional Chinese).
- **Build System:** **CMake (>= 3.10)**.
- **Database:** **MongoDB** (via `libmongoc-1.0` and `libbson-1.0`).

## Libraries & Frameworks
- **JSON Processing:** **libcjson**.
- **Networking:** **libcurl** (for communication between the C client and Python AI backend).
- **Testing:** **GoogleTest (v1.17.0)** for C++ unit tests and mock-based testing.
- **System Libraries:** `libm` (math), `libcrypt`.

## Infrastructure & DevOps
- **Containerization:** **Docker** and **Docker Compose** (Multi-stage builds for client and server).
- **CI/CD:** **GitHub Actions** for automated build and test workflows.
- **Infrastructure as Code:** **Terraform** for cloud resource management.
- **Cloud Platform:** Configured for deployment via **Google Cloud Build**.

## Development Environment
- **Primary OS:** macOS (Darwin).
- **Target OS:** Portable across modern Unix-like environments (Linux, macOS).
- **Encoding:** **UTF-8** (Migrated from legacy Big-5).
