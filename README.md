# QKMJ Refactoring Project

This is a refactored version of the QKMJ system, originally created by sywu (吳先祐).
This version is forked from 0.94 beta and has been modernized for portability, security, and maintainability.

## Refactoring Highlights

*   **Modern C Standards:** Refactored code to adhere to C11/C17 standards.
*   **UTF-8 Support:** Migrated from Big-5 to UTF-8 encoding, with `ncursesw` support.
*   **Security:**
    *   Replaced unsafe string functions (`strcpy`, `sprintf`, `strcat`, `gets`) with safer alternatives (`strncpy`, `snprintf`, `strncat`, `fgets`).
    *   Implemented constant-time string comparison for password checks to prevent timing attacks.
    *   Added checks for `malloc`, `accept`, and file operations.
    *   Replaced deprecated functions (`bzero`, `bcopy`) with standard ones (`memset`, `memmove`).
*   **Build System:** Switched to CMake for better portability and build management.
*   **Maintainability:** Refactored large functions and applied consistent code style.

## Build Instructions

### Prerequisites

*   CMake (>= 3.10)
*   C Compiler (GCC or Clang)
*   `ncursesw` library (for wide character support)
*   `pkg-config`

### Compiling

```bash
mkdir build
cd build
cmake ..
make
```

This will produce the following executables:
*   `qkmj`: The client application.
*   `mjgps`: The server application.
*   `mjrec`: The record management utility.

## Usage

### Running the Server

```bash
./mjgps [port]
```
Default port is 7001.

### Running the Client

```bash
./qkmj [server_ip] [port]
```
Default server is 0.0.0.0 and port 7001.

### Running Record Utility

```bash
./mjrec [record_file]
```

## Environment Variables

### Server (`mjgps`)
*   `PORT`: Server listening port (default: `7001` or first argument).
*   `MONGO_URI`: MongoDB connection string.
*   `LOGIN_LIMIT`: Maximum concurrent users (default: `200`).
*   `SSL_CERT_FILE`: Path to SSL CA certificate (overrides auto-detection).
*   `MONGO_LOG_LEVEL`: Minimum log level for MongoDB (`DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`; default: `INFO`).

### Client (`qkmj`)
*   `HOME`: Used to locate the `.qkmjrc` configuration file.
*   `AI_MODE`: Enable AI features (`1` to enable).
*   `AI_ENDPOINT`: Endpoint URL for the AI backend.
*   `AI_DEBUG`: Enable verbose AI debugging logs (`1` to enable).

### Recorder (`mjrec`)
*   `MONGO_URI`: MongoDB connection string.

## Protocol

See [COMMANDS.md](COMMANDS.md) for details on the client-server communication protocol.

## Original Credits

*   Original Author: sywu (吳先祐)
*   Forked from: TonyQ (0.94 beta)
*   Remaster/Docker: gjchen.tw@gmail.com