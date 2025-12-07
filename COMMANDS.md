# QKMJ Protocol Commands (QKMJ 通訊協定指令)

This document describes the communication protocol between the QKMJ client and server.
本文件描述 QKMJ 客戶端與伺服器之間的通訊協定。

## Message Format (訊息格式)

Messages are generally sent as text strings or binary packets.
Generally: `ID` + `Content`
通常格式為：`指令ID` + `內容`

ID is usually a 3-digit number (e.g., "101").
ID 通常為 3 位數字（例如 "101"）。

## Client to GPS Server Commands (客戶端 -> GPS 伺服器)

| ID | Command (指令) | Parameter (參數) | Description (說明) |
| :--- | :--- | :--- | :--- |
| 99 | Get Username | `username` | 傳送使用者帳號名稱 |
| 100 | Check Version | `version` | 檢查客戶端版本 |
| 101 | Login | `name` | 使用者登入 (暱稱) |
| 102 | Check Password | `password` | 驗證密碼 |
| 103 | Create Account | `password` | 建立新帳號 |
| 104 | Change Password | `new_password` | 修改密碼 |
| 105 | Kill Duplicate | - | 強制踢除重複登入的連線 |
| 2 | List Players | - | 列出線上使用者 (/WHO) |
| 3 | List Tables | - | 列出目前牌桌 (/List) |
| 4 | Set Note | `note` | 設定附註 (/Note) |
| 5 | User Info | `name` | 查詢使用者資訊 (/Query) |
| 6 | Who in Table | `table_leader` | 查詢牌桌內的使用者 |
| 7 | Broadcast | `msg` | 廣播訊息 (GM Only) |
| 8 | Invite | `name` | 邀請使用者 (/Invite) |
| 9 | Send Message | `name msg` | 傳送私訊 (/Tell) |
| 10 | Lurker List | - | 列出閒置使用者 |
| 11 | Join Table | `table_leader` | 加入牌桌 (/Join) |
| 12 | Open Table | `port` | 開設新牌桌 (/Open) |
| 13 | List Tables (Free) | - | 列出有空位的牌桌 |
| 14 | Check Open | - | 檢查是否符合開桌資格 |
| 20 | Win Game | `id money` | 回報贏牌結果 (更新金錢) |
| 21 | Find User | `name` | 尋找使用者 (/Find) |
| 200 | Leave | - | 離開遊戲 (/Leave) |
| 201 | Status | - | 顯示目前狀態 |
| 205 | Leave Table | - | 離開牌桌 |

## GPS Server to Client Messages (GPS 伺服器 -> 客戶端)

| ID | Message (訊息) | Description (說明) |
| :--- | :--- | :--- |
| 101 | Text Message | 一般文字訊息顯示 |
| 002 | Login OK | 登入成功 (需要密碼) |
| 003 | Welcome | 歡迎訊息 (完成登入程序) |
| 004 | Password Fail | 密碼錯誤 |
| 005 | New User | 新使用者 (需要建立帳號) |
| 006 | Duplicate Login | 重複登入警告 |
| 010 | Version Error | 版本不符 |
| 011 | Join Info | `IP Port` 牌桌伺服器位址 |
| 012 | Open OK | 同意開桌 |
| 120 | Update Money | 更新金錢顯示 |
| 200 | Kick | 被踢除 |

## Client to Table Server / Peer (客戶端 -> 牌桌/其他客戶端)

| ID | Command (指令) | Description (說明) |
| :--- | :--- | :--- |
| 101 | Chat | 聊天訊息 |
| 301 | Change Card | 交換手牌 (配牌階段) |
| 303 | Can Get | 通知可以摸牌 |
| 304 | Get Card | 摸牌 |
| 401 | Throw Card | 打出牌 |
| 402 | Card Thrown | 廣播有人打出牌 |
| 501 | Check Card | 詢問是否吃/碰/槓/胡 |
| 520 | Epk (Eat/Pong/Kang)| 執行 吃/碰/槓 |
| 522 | Make (Win) | 胡牌 |
| 530 | Show Epk | 顯示 吃/碰/槓 的結果 |

## Notes (附註)

*   **Encoding**: All messages are encoded in UTF-8 (refactored from Big5).
*   **Security**: Passwords are hashed using `crypt()`.
*   **Transport**: TCP/IP sockets.