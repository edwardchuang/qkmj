# Refactoring Plan: Migrating `qkmjserver` to MongoDB-backed Sessions

## Objective
Migrate the local, in-memory `player[MAX_PLAYER]` array state to the shared MongoDB database. This simplifies architecture by using a single data store for both persistence and session state, allowing for horizontal scaling and better observability.

## 1. Architecture Design

### Current State (Legacy)
*   **State**: Held in `struct player_info player[MAX_PLAYER]` global array.
*   **Identification**: Players are identified by their **Index** (0-499) in this array.
*   **Dependencies**: Code like `player[id].join` points to another index. This breaks if players are on different servers.

### Target State (MongoDB-Backed)
*   **Session State**: Held in the `active_sessions` collection in MongoDB.
*   **Abstraction**: A "Wrap Layer" (`session_manager.c/.h`) decouples logic from DB calls.
*   **Identification**: Players are identified by **Username**.
*   **Local Cache**: `player[]` array remains for *Socket Management* and *Buffer Handling*, but logical state (Money, Table, Status) is synced to MongoDB.

## 2. MongoDB Data Schema

### 2.1 Active Sessions (`active_sessions`)
**Query Key**: `username` (Unique)

| Field | Type | Description |
| :--- | :--- | :--- |
| `username` | String | Unique Player ID |
| `server_id` | String | Server Process ID (e.g., `gps-01:1234`) |
| `status` | String | `LOBBY`, `WAITING`, `PLAYING` |
| `current_table`| String | Leader's username (or null) |
| `money` | Int64 | Current cached money |
| `ip` | String | Client IP |
| `login_time` | Date | Session start time |
| `last_active` | Date | Last heartbeat (for cleanup) |

### 2.2 Active Tables (`active_tables`) (Future Phase)
**Query Key**: `leader`

| Field | Type | Description |
| :--- | :--- | :--- |
| `leader` | String | Table owner username |
| `port` | Int32 | Game server port |
| `players` | Array | List of usernames in table |

## 3. Implementation Steps

### Phase 1: The Wrap Layer (Completed)
*   **Defined Interface**: `qkmjserver/session_manager.h`
*   **Implemented Logic**: `qkmjserver/session_manager.c` using `mongo.c`.
*   **Build Config**: Updated `CMakeLists.txt`.

### Phase 2: Session Integration (Next Step)
*   **Goal**: Ensure every login/logout is reflected in MongoDB.
*   **Refactor `mjgps.c`**:
    *   In `MSG_LOGIN`: Call `session_create()`.
    *   In `close_connection`: Call `session_destroy()`.
    *   In `check_user` loop: Call `session_heartbeat()` periodically? (Or on every command).

### Phase 3: Decoupling Tables
*   **Goal**: List and Join tables using MongoDB data.
*   **Refactor `mjgps.c`**:
    *   `MSG_LIST_TABLES`: Query `active_tables` collection.
    *   `MSG_JOIN_TABLE`: Update `active_sessions.current_table` and `active_tables.players`.

## 4. Migration Strategy (Hybrid)
We will run in "Hybrid Mode". The `player[]` array continues to drive the legacy logic (socket handling, index-based references) while we "Shadow Write" all state changes to MongoDB. This validates the data layer without breaking the game.
