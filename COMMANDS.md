# QKMJ Client-Server Commands

This document outlines the communication protocol between the QKMJ client and server.

## Table of Contents

1.  [General Message Format](#general-message-format)
2.  [Authentication Commands](#authentication-commands)
3.  [Informational Commands](#informational-commands)
4.  [Game Action Commands](#game-action-commands)
5.  [Communication Commands](#communication-commands)
6.  [Client Control Commands](#client-control-commands)
7.  [Administrative Commands](#administrative-commands)
8.  [Server Response Messages](#server-response-messages)
9.  [Login Flow Chart](#login-flow-chart)

---

## General Message Format

The communication between the client and the server is done via ASCII strings. Each message is terminated by a null character (`\0`).

**Client to Server:**

Most messages from the client to the server have the following format:

`<3-digit-command-ID><payload>`

*   **3-digit-command-ID:** A three-digit number that represents the command.
*   **payload:** The arguments for the command.

**Server to Client:**

Responses from the server to the client also follow a similar format, often with a 3-digit status/command code.

---

## Authentication Commands

### Login

*   **English:**
    *   **Command:** `/LOGIN`
    *   **Description:** Initiates the login process. The client sends the username, and the server responds by asking for a password. After receiving the password, the server validates the credentials and, if successful, logs the user in.
    *   **Client -> Server:** `101<username>`
    *   **Server Response:**
        *   `002`: Password required.
        *   `005`: User not found, create a new account.
        *   `004`: Incorrect password.
        *   `006`: User already logged in.
        *   On success: Welcome messages, user stats, and table list.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/LOGIN`
    *   **描述:** 啟動登入流程。客戶端發送使用者名稱，伺服器回應要求輸入密碼。收到密碼後，伺服器會驗證憑證，如果成功，則登入使用者。
    *   **客戶端 -> 伺服器:** `101<使用者名稱>`
    *   **伺服器回應:**
        *   `002`: 需要密碼。
        *   `005`: 找不到使用者，建立新帳號。
        *   `004`: 密碼錯誤。
        *   `006`: 使用者已登入。
        *   成功時: 歡迎訊息、使用者狀態和桌子列表。

### Send User ID

*   **English:**
    *   **Command:** (No direct user command, part of login handshake)
    *   **Description:** The client sends its username to the server during the initial connection phase.
    *   **Client -> Server:** `099<username>`

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** (無直接使用者指令，登入握手的一部分)
    *   **描述:** 客戶端在初始連線階段向伺服器發送其使用者名稱。
    *   **客戶端 -> 伺服器:** `099<使用者名稱>`

### Send Client Version

*   **English:**
    *   **Command:** (No direct user command, part of login handshake)
    *   **Description:** The client sends its version information to the server during the initial connection phase.
    *   **Client -> Server:** `100<version_string>`

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** (無直接使用者指令，登入握手的一部分)
    *   **描述:** 客戶端在初始連線階段向伺服器發送其版本資訊。
    *   **客戶端 -> 伺服器:** `100<版本字串>`

### Change Password

*   **English:**
    *   **Command:** `/PASSWD`
    *   **Description:** Allows a logged-in user to change their password.
    *   **Client -> Server:** `104<new_password>`
    *   **Server Response:** `101Password changed.`

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/PASSWD`
    *   **描述:** 允許已登入的使用者變更密碼。
    *   **客戶端 -> 伺服器:** `104<新密碼>`
    *   **伺服器回應:** `101密碼已變更。`

---

## Informational Commands

### List Tables

*   **English:**
    *   **Command:** `/TABLE` or `/T`
    *   **Description:** Requests a list of all available game tables.
    *   **Client -> Server:** `003`
    *   **Server Response:** A formatted list of tables.

*   **繁體中文 (Traditional Chinese)::**
    *   **指令:** `/TABLE` 或 `/T`
    *   **描述:** 要求所有可用的遊戲桌列表。
    *   **客戶端 -> 伺服器:** `003`
    *   **伺服器回應:** 格式化的桌子列表。

### List Players

*   **English:**
    *   **Command:** `/PLAYER` or `/P`
    *   **Description:** Requests a list of all online players.
    *   **Client -> Server:** `002`
    *   **Server Response:** A formatted list of players.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/PLAYER` 或 `/P`
    *   **描述:** 要求所有在線玩家列表。
    *   **客戶端 -> 伺服器:** `002`
    *   **伺服器回應:** 格式化的玩家列表。

### Player Statistics

*   **English:**
    *   **Command:** `/STAT <username>`
    *   **Description:** Retrieves statistics for a specific player. If no username is provided, it retrieves the statistics of the current user.
    *   **Client -> Server:** `005<username>`
    *   **Server Response:** The player's statistics.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/STAT <使用者名稱>`
    *   **描述:** 檢索特定玩家的統計資訊。如果未提供使用者名稱，則檢索目前使用者的統計資訊。
    *   **客戶端 -> 伺服器:** `005<使用者名稱>`
    *   **伺服器回應:** 玩家的統計資訊。

### Who is at a Table

*   **English:**
    *   **Command:** `/WHO <table_owner_name>`
    *   **Description:** Lists the players currently at a specific table. If no name is provided, it lists players at the current table.
    *   **Client -> Server:** `006<table_owner_name>`
    *   **Server Response:** A list of players at the specified table.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/WHO <桌長名稱>`
    *   **描述:** 列出特定牌桌上的玩家。如果未提供名稱，則列出目前牌桌的玩家。
    *   **客戶端 -> 伺服器:** `006<桌長名稱>`
    *   **伺服器回應:** 指定牌桌上的玩家列表。

### Find Player

*   **English:**
    *   **Command:** `/FIND <username>`
    *   **Description:** Find the location of a user (e.g., which table they are at).
    *   **Client -> Server:** `021<username>`
    *   **Server Response:** The user's location or a "not found" message.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/FIND <使用者名稱>`
    *   **描述:** 尋找使用者在哪裡 (例如，在哪個牌桌)。
    *   **客戶端 -> 伺服器:** `021<使用者名稱>`
    *   **伺服器回應:** 使用者的位置或「找不到」的訊息。

### List Lurkers

*   **English:**
    *   **Command:** `/LURKER`
    *   **Description:** Lists players who are online but not currently at a table.
    *   **Client -> Server:** `010`
    *   **Server Response:** A list of lurkers.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/LURKER`
    *   **描述:** 列出在線但目前不在任何牌桌上的玩家。
    *   **客戶端 -> 伺服器:** `010`
    *   **伺服器回應:** 潛水者列表。

### List Joinable Tables

*   **English:**
    *   **Command:** `/FREE`
    *   **Description:** Lists tables that are not full and can be joined.
    *   **Client -> Server:** `013`
    *   **Server Response:** A list of joinable tables.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/FREE`
    *   **描述:** 列出未滿且可以加入的牌桌。
    *   **客戶端 -> 伺服器:** `013`
    *   **伺服器回應:** 可加入的牌桌列表。

### Show Current State

*   **English:**
    *   **Command:** (No direct user command, triggers server to send updates)
    *   **Description:** The client sends this message to request an update on the current online user count and player statistics.
    *   **Client -> Server:** `201`
    *   **Server Response:** The server will send `003` (table list) and `120` (money/ID update) messages.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** (無直接使用者指令，觸發伺服器發送更新)
    *   **描述:** 客戶端發送此訊息以請求更新目前的在線使用者數量和玩家統計資訊。
    *   **客戶端 -> 伺服器:** `201`
    *   **伺服器回應:** 伺服器將發送 `003` (牌桌列表) 和 `120` (金錢/ID更新) 訊息。

---

## Game Action Commands

### Create a Table

*   **English:**
    *   **Command:** `/SERV` or `/S`
    *   **Description:** Creates a new game table, making the user the table owner.
    *   **Client -> Server:** `014` (check if can create) -> `012` (ok) -> `012<port>` (create table with port)
    *   **Server Response:**
        *   `012`: OK to create a table.
        *   `101<error_message>`: Error creating table.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/SERV` 或 `/S`
    *   **描述:** 建立一個新的遊戲桌，並讓使用者成為桌長。
    *   **客戶端 -> 伺服器:** `014` (檢查是否可開桌) -> `012` (可以) -> `012<port>` (使用指定埠號開桌)
    *   **伺服器回應:**
        *   `012`: 可以開桌。
        *   `101<錯誤訊息>`: 開桌時發生錯誤。

### Join a Table

*   **English:**
    *   **Command:** `/JOIN <table_owner_name>` or `/J <table_owner_name>`
    *   **Description:** Joins an existing game table.
    *   **Client -> Server:** `011<table_owner_name>`
    *   **Server Response:**
        *   `0110<ip_address> <port>`: Success, with the IP and port of the table.
        *   `0111`: Table not found.
        *   `101<error_message>`: Other errors (e.g., table full).

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/JOIN <桌長名稱>` 或 `/J <桌長名稱>`
    *   **描述:** 加入一個現有的遊戲桌。
    *   **客戶端 -> 伺服器:** `011<桌長名稱>`
    *   **伺服器回應:**
        *   `0110<IP位址> <埠號>`: 成功，包含牌桌的IP和埠號。
        *   `0111`: 找不到牌桌。
        *   `101<錯誤訊息>`: 其他錯誤 (例如，牌桌已滿)。

### Leave a Table

*   **English:**
    *   **Command:** `/LEAVE` or `/L`
    *   **Description:** Leaves the current game table.
    *   **Client -> Server:** `205`
    *   **Server Response:** Confirmation of leaving the table.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/LEAVE` 或 `/L`
    *   **描述:** 離開目前的遊戲桌。
    *   **客戶端 -> 伺服器:** `205`
    *   **伺服器回應:** 確認離開牌桌。

### Set Table Note

*   **English:**
    *   **Command:** `/NOTE <text>`
    *   **Description:** Sets a descriptive note for the user's table (if they are the owner).
    *   **Client -> Server:** `004<text>`
    *   **Server Response:** `101Note updated.`

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/NOTE <文字>`
    *   **描述:** 為使用者的桌子設定描述性附註（如果他們是桌長）。
    *   **客戶端 -> 伺服器:** `004<文字>`
    *   **伺服器回應:** `101附註已更新。`

### Report Win Game

*   **English:**
    *   **Command:** (No direct user command, sent by client after game ends)
    *   **Description:** The client sends this message to the server to report the outcome of a game, including the player's updated money.
    *   **Client -> Server:** `020<player_id_5_digits><money_long>`

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** (無直接使用者指令，遊戲結束後由客戶端發送)
    *   **描述:** 客戶端發送此訊息給伺服器，報告遊戲結果，包括玩家更新後的金錢。
    *   **客戶端 -> 伺服器:** `020<玩家ID_5位數字><金錢_長整數>`

### Send Game Record

*   **English:**
    *   **Command:** (No direct user command, sent by client for logging)
    *   **Description:** The client sends game record data to the server for logging purposes.
    *   **Client -> Server:** `900<game_record_data>`

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** (無直接使用者指令，由客戶端發送用於記錄)
    *   **描述:** 客戶端發送遊戲記錄資料給伺服器，用於記錄目的。
    *   **客戶端 -> 伺服器:** `900<遊戲記錄資料>`

---

## Communication Commands

### Send a Message

*   **English:**
    *   **Command:** `/MSG <username> <message>`
    *   **Description:** Sends a private message to a specific user.
    *   **Client -> Server:** `009<username> <message>`
    *   **Server Response:** `101<message>` (to the recipient)

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/MSG <使用者名稱> <訊息>`
    *   **描述:** 向特定使用者發送私人訊息。
    *   **客戶端 -> 伺服器:** `009<使用者名稱> <訊息>`
    *   **伺服器回應:** `101<訊息>` (給接收者)

### Invite a Player

*   **English:**
    *   **Command:** `/INVITE <username>`
    *   **Description:** Invites a player to the user's current table.
    *   **Client -> Server:** `008<username>`
    *   **Server Response:** `101<invitation_message>` (to the invited player)

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/INVITE <使用者名稱>`
    *   **描述:** 邀請玩家到使用者目前的牌桌。
    *   **客戶端 -> 伺服器:** `008<使用者名稱>`
    *   **伺服器回應:** `101<邀請訊息>` (給被邀請的玩家)

---

## Client Control Commands

### Quit

*   **English:**
    *   **Command:** `/QUIT` or `/Q`
    *   **Description:** Disconnects the client from the server.
    *   **Client -> Server:** `200`
    *   **Server Response:** The server closes the connection.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/QUIT` 或 `/Q`
    *   **描述:** 中斷客戶端與伺服器的連線。
    *   **客戶端 -> 伺服器:** `200`
    *   **伺服器回應:** 伺服器關閉連線。

### Toggle Beep Sound

*   **English:**
    *   **Command:** `/BEEP [ON|OFF]`
    *   **Description:** Toggles the beep sound on or off. This is a client-side command and does not involve the server.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/BEEP [ON|OFF]`
    *   **描述:** 開啟或關閉提示音。這是客戶端指令，不涉及伺服器。

### Help

*   **English:**
    *   **Command:** `/HELP` or `/H`
    *   **Description:** Displays the help message with a list of commands. This is a client-side command and does not involve the server.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/HELP` 或 `/H`
    *   **描述:** 顯示包含指令列表的說明訊息。這是客戶端指令，不涉及伺服器。

---

## Administrative Commands

### Broadcast Message

*   **English:**
    *   **Command:** `/BROADCAST <message>`
    *   **Description:** Sends a message to all online players (admin only).
    *   **Client -> Server:** `007<message>`
    *   **Server Response:** `101<message>` (to all players)

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/BROADCAST <訊息>`
    *   **描述:** 向所有在線玩家發送訊息（僅限管理員）。
    *   **客戶端 -> 伺服器:** `007<訊息>`
    *   **伺服器回應:** `101<訊息>` (給所有玩家)

### Kick Player

*   **English:**
    *   **Command:** `/KICK <username>`
    *   **Description:** Kicks a player from the current table (table owner only).
    *   **Client -> Server:** (This command is handled by the table server, not the main GPS server)

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/KICK <使用者名稱>`
    *   **描述:** 將玩家踢出目前的牌桌（僅限桌長）。
    *   **客戶端 -> 伺服器:** (此指令由牌桌伺服器處理，而非主GPS伺服器)

### Force-Kick Player

*   **English:**
    *   **Command:** `/KILL <username>`
    *   **Description:** Forcibly removes a player from the server (admin only).
    *   **Client -> Server:** `202<username>`
    *   **Server Response:** The specified user is disconnected.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/KILL <使用者名稱>`
    *   **描述:** 強制將玩家從伺服器中移除（僅限管理員）。
    *   **客戶端 -> 伺服器:** `202<使用者名稱>`
    *   **伺服器回應:** 指定的使用者已中斷連線。

### Shutdown Server

*   **English:**
    *   **Command:** `/SHUTDOWN`
    *   **Description:** Shuts down the server (admin only).
    *   **Client -> Server:** `500`
    *   **Server Response:** The server disconnects all clients and shuts down.

*   **繁體中文 (Traditional Chinese):**
    *   **指令:** `/SHUTDOWN`
    *   **描述:** 關閉伺服器（僅限管理員）。
    *   **客戶端 -> 伺服器:** `500`
    *   **伺服器回應:** 伺服器中斷所有客戶端連線並關閉。

---

## Server Response Messages

### Update Client Money and ID

*   **English:**
    *   **Description:** Sent by the server to update the client's displayed money and user ID.
    *   **Server -> Client:** `120<user_id_5_digits><money_long>`
    *   **Triggered by:** Successful login, game actions that change money, or client request `201`.

*   **繁體中文 (Traditional Chinese):**
    *   **描述:** 伺服器發送此訊息以更新客戶端顯示的金錢和使用者ID。
    *   **伺服器 -> 客戶端:** `120<使用者ID_5位數字><金錢_長整數>`
    *   **觸發時機:** 成功登入、改變金錢的遊戲動作，或客戶端請求 `201`。

---

## Login Flow Chart

```
+----------+                                +----------+
|  Client  |                                |  Server  |
+----------+                                +----------+
     |                                            |
     | --- Send User ID (099<username>) --------> |
     | --- Send Client Version (100<version>) --> |
     | --- /LOGIN <username> (101<username>) ---> |
     |                                            |
     | <---------- 002 (Password Required) ------ |
     |                                            |
     | ---- /PASSWD <password> (102<password>) -->|
     |                                            |
     | <----------- Welcome Messages ----------- |
     | <------------ User Stats ---------------- |
     | <----------- Table List (003) ----------- |
     | <----------- Update Money (120) --------- |
     |                                            |
```
