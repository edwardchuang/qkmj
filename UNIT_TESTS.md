# Unit Tests Documentation

This document describes the unit tests available in the QKMJ project and how to execute them.

## Prerequisites

Ensure the project is built with testing enabled (default configuration).

```bash
mkdir -p build
cd build
cmake ..
make
```

## Available Test Suites

### 1. AI Client Tests (`AIClientTest`)

Tests the functionality of the AI Client module (`qkmjclient/ai_client.c`), specifically JSON parsing and state serialization.

*   **Source File**: `tests/test_ai_client.cpp`
*   **Target Name**: `test_ai`
*   **Test Framework**: GoogleTest

#### Test Cases
*   `ParseDecisionDiscard`: Verifies parsing of a simple "discard" action from JSON.
*   `ParseDecisionEat`: Verifies parsing of an "eat" action with meld cards.
*   `ParseDecisionWrappedArray`: Verifies parsing of nested JSON structures (Gemini API format).
*   `ParseDecisionPass`: Verifies parsing of a "pass" action.
*   `SerializeStateDiscardPhase`: Verifies that game state is correctly serialized to JSON during the discard phase.
*   `SerializeStateClaimPhase`: Verifies serialization during the claim phase (e.g., when a card is discarded by others).

#### Execution
```bash
./build/test_ai
# OR
ctest -R AIClientTest
```

---

### 2. Session Manager Tests (`SessionManagerTest`)

Tests the MongoDB-backed session management layer (`qkmjserver/session_manager.c`). **Requires a running MongoDB instance.**

*   **Source File**: `tests/test_session_manager.cpp`
*   **Target Name**: `session_test`
*   **Test Framework**: GoogleTest

#### Test Cases
*   `CreateAndCheckOnline`: Verifies creating a session and checking `session_is_online`.
*   `DestroySession`: Verifies that destroying a session removes the user from online status.
*   `UpdateStatus`: Verifies updating a user's status (e.g., to "PLAYING") and table association.
*   `Heartbeat`: Verifies updating the session heartbeat timestamp.
*   `DuplicateSessionUpsert`: Verifies that logging in again updates the existing session without errors.

#### Execution
```bash
./build/session_test
# OR
ctest -R SessionManagerTest
```

---

### 3. Connection Integration Test (`conn_test`)

A standalone integration test for the AI Client's initialization flow. It mocks the Ncurses environment to verify logic without a UI.

*   **Source File**: `tests/connection_test.c`
*   **Target Name**: `conn_test`
*   **Test Framework**: Custom (Manual execution)

#### Coverage
*   `ai_init()`: Initialization logic.
*   `ai_get_decision()`: End-to-end decision flow check.

#### Execution
```bash
./build/conn_test
```
