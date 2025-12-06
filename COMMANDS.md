# QKMJ Protocol Documentation / QKMJ 通訊協定文件

This document outlines the communication protocol used between the QKMJ Client (`qkmj`) and the GPS Server (`mjgps`), as well as the communication between the Client and the Table Server (which is hosted by one of the clients).
本文件概述了 QKMJ 客戶端 (`qkmj`) 與 GPS 伺服器 (`mjgps`) 之間的通訊協定，以及客戶端與牌桌伺服器（由其中一個客戶端託管）之間的通訊。

## Client to GPS Server Commands (客戶端對 GPS 伺服器指令)

| Command ID | Description (English) | Description (Traditional Chinese) | Parameters |
| :--- | :--- | :--- | :--- |
| `099` | Send Username | 傳送使用者名稱 | `099<username>` |
| `100` | Check Version | 檢查版本 | `100<version>` |
| `101` | Login | 登入 | `101<name>` |
| `102` | Verify Password | 驗證密碼 | `102<password>` |
| `103` | Create Account | 建立新帳號 | `103<password>` |
| `104` | Change Password | 更改密碼 | `104<new_password>` |
| `105` | Kill Duplicate Login | 清除重複登入 | `105` |
| `002` | Request Player List | 請求玩家列表 | `002` (Triggered by `/LIST`, `/PLAYER`) |
| `003` | Request Table List | 請求牌桌列表 | `003` (Triggered by `/TABLE`) |
| `004` | Update Note | 更新備註 | `004<note>` |
| `005` | Request Stats | 請求統計資料 | `005<name>` |
| `006` | Request Who (Table Info) | 請求牌桌成員資訊 | `006<name>` |
| `007` | Broadcast Message | 廣播訊息 | `007<message>` |
| `008` | Invite Player | 邀請玩家 | `008<name>` |
| `009` | Private Message | 私人訊息 | `009<name> <message>` |
| `010` | Request Lurker List | 請求閒置玩家列表 | `010` |
| `011` | Join Table | 加入牌桌 | `011<table_host_name>` |
| `012` | Open Table (Register Port) | 開桌（註冊連接埠） | `012<port>` |
| `013` | Request Free Tables | 請求空桌列表 | `013` |
| `014` | Check Check Open Table | 檢查開桌資格 | `014` |
| `020` | Update Money (from Table) | 更新金額（來自牌桌） | `020<id><money>` |
| `021` | Find User | 尋找玩家 | `021<name>` |
| `111` | Add Player to Table (Internal)| 新增玩家至牌桌（內部） | `111` |
| `199` | Table Server Closing | 牌桌伺服器關閉 | `199` |
| `200` | Logout / Leave | 離開 / 登出 | `200` |
| `201` | Update State | 更新狀態 | `201` |
| `202` | Kill User (Admin) | 強制踢除玩家（管理員） | `202<name>` |
| `205` | Leave Table | 離開牌桌 | `205` |
| `500` | Shutdown Server (Admin) | 關閉伺服器（管理員） | `500` |
| `900` | Game Log | 遊戲紀錄 | `900<log_data>` |

## GPS Server to Client Messages (GPS 伺服器對客戶端訊息)

| Command ID | Description (English) | Description (Traditional Chinese) | Parameters |
| :--- | :--- | :--- | :--- |
| `101` | System Message / Broadcast | 系統訊息 / 廣播 | `101<message>` |
| `102` | User Message | 使用者訊息 | `102<message>` |
| `002` | Login Success (Existing) | 登入成功（既有帳號） | `002` |
| `003` | Login Success / Stats | 登入成功 / 統計 | `003` |
| `004` | Password Error | 密碼錯誤 | `004` |
| `005` | User Not Found | 查無此人 | `005` |
| `006` | Duplicate Login | 重複登入 | `006` |
| `010` | Version Mismatch | 版本不符 | `010` |
| `011` | Join Response | 加入回應 | `0110<ip> <port>` (Success), `0111` (Full) |
| `012` | Open Table Response | 開桌回應 | `012` |
| `120` | Player Info (ID/Money) | 玩家資訊（ID/金額） | `120<id><money>` |
| `200` | Force Logout / Kick | 強制登出 / 踢除 | `200` |
| `211` | Table Host Name | 桌長名稱 | `211<name>` |

## Client <-> Table Server Protocol (客戶端與牌桌伺服器通訊協定)

This protocol is used for the game logic itself.
此通訊協定用於遊戲邏輯本身。

| Command ID | Description (English) | Description (Traditional Chinese) | Parameters |
| :--- | :--- | :--- | :--- |
| `101` | System Message | 系統訊息 | `101<msg>` |
| `102` | Chat Message | 聊天訊息 | `102<msg>` |
| `199` | Host Left | 桌長離開 | `199` |
| `200` | Leave Table | 離開牌桌 | `200` |
| `201` | New Player Joined | 新玩家加入 | `201<id><sit><count><name>` |
| `202` | Player ID/Money | 玩家 ID/金額 | `202<id><gps_id><money>` |
| `203` | Existing Player Info | 現有玩家資訊 | `203<id><sit><name>` |
| `204` | Table Seat Info | 牌桌座位資訊 | `204...` |
| `205` | Self Info (Join Confirm) | 個人資訊（加入確認） | `205<id><sit><name>` |
| `206` | Player Left | 玩家離開 | `206<id><count>` |
| `290` | New Game / Opening | 新局 / 開局 | `290` |
| `300` | Init Playing Screen | 初始化遊戲畫面 | `300` |
| `301` | Change Card | 換牌 | `301<pos><card>` |
| `302` | Deal Cards (16) | 發牌（16張） | `302<cards...>` |
| `303` | Can Get Card? | 可摸牌？ | `303` |
| `304` | Get One Card | 摸一張牌 | `304<card>` |
| `305` | Card Owner Update | 持牌者更新 | `305<sit>` |
| `306` | Card Point/Count Update | 牌數更新 | `306<count>` |
| `308` | Sort Cards | 理牌 | `308<mode>` |
| `310` | Turn Change | 輪替 | `310<sit>` |
| `312` | Time Update | 時間更新 | `312<sit><time>` |
| `313` | Request Card | 請求摸牌 | `313` |
| `314` | Show New Card/Back | 顯示新牌/牌背 | `314<sit><type>` |
| `330` | Draw Game (Sea Floor) | 流局（海底撈月） | `330` |
| `401` | Throw Card (Client) | 出牌（客戶端發送） | `401<card>` |
| `402` | Throw Card (Broadcast) | 出牌（廣播） | `402<id><card>` |
| `450` | Wait Confirmation | 等待確認 | `450` |
| `501` | Check Card (Eat/Pong...) | 檢查牌（吃/碰...） | `501<flags...>` |
| `510` | Check Response | 檢查回應 | `510<choice>` |
| `518` | Door Wind Info | 門風資訊 | `518<winds...>` |
| `520` | Process EPK (Action) | 執行吃碰槓 | `520<type>` |
| `521` | Show Hands (End Game) | 攤牌（結算） | `521<cards...>` |
| `522` | Process Win (Make) | 處理胡牌 | `522<sit><card>` |
| `525` | Draw Flower | 補花 | `525<sit><card>` |
| `530` | EPK Broadcast | 吃碰槓廣播 | `530<id><type><cards...>` |
