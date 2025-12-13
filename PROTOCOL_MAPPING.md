# Protocol Refactoring Checklist & Mapping

This document tracks the migration of the QKMJ communication protocol from legacy String/Binary format to JSON.

## Client -> GPS Server (mjgps)

| ID | Command | Old Format | New JSON Format | Status |
| :--- | :--- | :--- | :--- | :--- |
| 99 | Get Username | `099<username>` | `{"id": 99, "data": {"username": "..."}}` | Implemented |
| 100 | Check Version | `100<version>` | `{"id": 100, "data": {"version": "..."}}` | Implemented |
| 101 | Login | `101<name>` | `{"id": 101, "data": {"name": "..."}}` | Implemented |
| 102 | Check Password | `102<password>` | `{"id": 102, "data": {"password": "..."}}` | Implemented |
| 103 | Create Account | `103<password>` | `{"id": 103, "data": {"password": "..."}}` | Implemented |
| 104 | Change Password | `104<new_password>` | `{"id": 104, "data": {"new_password": "..."}}` | Implemented |
| 105 | Kill Duplicate | `105` | `{"id": 105}` | Implemented |
| 2 | List Players | `002` | `{"id": 2}` | Implemented |
| 3 | List Tables | `003` | `{"id": 3}` | Implemented |
| 4 | Set Note | `004<note>` | `{"id": 4, "data": {"note": "..."}}` | Implemented |
| 5 | User Info | `005<name>` | `{"id": 5, "data": {"name": "..."}}` | Implemented |
| 6 | Who in Table | `006<leader>` | `{"id": 6, "data": {"table_leader": "..."}}` | Implemented |
| 7 | Broadcast | `007<msg>` | `{"id": 7, "data": {"msg": "..."}}` | Implemented |
| 8 | Invite | `008<name>` | `{"id": 8, "data": {"name": "..."}}` | Implemented |
| 9 | Send Message | `009<name msg>` | `{"id": 9, "data": {"to": "...", "msg": "..."}}` | Implemented |
| 10 | Lurker List | `010` | `{"id": 10}` | Implemented |
| 11 | Join Table | `011<leader>` | `{"id": 11, "data": {"table_leader": "..."}}` | Implemented |
| 12 | Open Table | `012<port>` | `{"id": 12, "data": {"port": 1234}}` | Implemented |
| 13 | List Tables (Free) | `013` | `{"id": 13}` | Implemented |
| 14 | Check Open | `014` | `{"id": 14}` | Implemented |
| 20 | Win Game | `020<id money>` | `{"id": 20, "data": {"winner_id": 1, "money": 100}}` | Implemented |
| 21 | Find User | `021<name>` | `{"id": 21, "data": {"name": "..."}}` | Implemented |
| 900 | Game Record | `900<json>` | `{"id": 900, "data": {"record": {...}}}` | Implemented |
| 901 | AI Log | `901<json>` | `{"id": 901, "data": {...}}` | Implemented |
| 200 | Leave | `200` | `{"id": 200}` | Implemented |
| 201 | Status | `201` | `{"id": 201}` | Implemented |
| 202 | Kick User | `202<name>` | `{"id": 202, "data": {"name": "..."}}` | Implemented |
| 205 | Leave Table | `205` | `{"id": 205}` | Implemented |
| 500 | Shutdown | `500` | `{"id": 500}` | Implemented |

## GPS Server -> Client

| ID | Message | Old Format | New JSON Format | Status |
| :--- | :--- | :--- | :--- | :--- |
| 101 | Text Message | `101<msg>` | `{"id": 101, "data": {"text": "..."}}` | Implemented |
| 002 | Login OK | `002` | `{"id": 2}` | Implemented |
| 003 | Welcome | `003` | `{"id": 3}` | Implemented |
| 004 | Password Fail | `004` | `{"id": 4}` | Implemented |
| 005 | New User | `005` | `{"id": 5}` | Implemented |
| 006 | Duplicate Login | `006` | `{"id": 6}` | Implemented |
| 010 | Version Error | `010` | `{"id": 10}` | Implemented |
| 011 | Join Info | `011<0/1/2><msg>` | `{"id": 11, "data": {"status": 0, "ip": "...", "port": ...}}` | Implemented |
| 012 | Open OK | `012` | `{"id": 12}` | Implemented |
| 120 | Update Money | `120<id money>` | `{"id": 120, "data": {"user_id": ..., "money": ...}}` | Implemented |
| 200 | Kick | `200` | `{"id": 200}` | Implemented |
| 211 | Join Notify | `211<name>` | `{"id": 211, "data": {"name": "..."}}` | Implemented |

## Client <-> Table Server (P2P)

| ID | Command | Old Format | New JSON Format | Status |
| :--- | :--- | :--- | :--- | :--- |
| 101 | Chat | `101<msg>` | `{"id": 101, "data": {"text": "..."}}` | Implemented |
| 102 | Action Chat | `102<msg>` | `{"id": 102, "data": {"text": "..."}}` | Implemented |
| 130 | AI Mode | `130<sit><0/1>` | `{"id": 130, "data": {"sit": 1, "enabled": true}}` | Implemented |
| 199 | Leader Leave | `199` | `{"id": 199}` | Implemented |
| 200 | Leave | `200` | `{"id": 200}` | Implemented |
| 201 | New Comer Info | `201<id><sit><count><name>` | `{"id": 201, "data": {"user_id": ..., "sit": ..., "count": ..., "name": "..."}}` | Implemented |
| 202 | Update Money | `202<id><money>` | `{"id": 202, "data": {"user_id": ..., "money": ..., "db_id": ...}}` | Implemented |
| 203 | Other Info | `203<id><sit><name>` | `{"id": 203, "data": {"user_id": ..., "sit": ..., "name": "..."}}` | Implemented |
| 204 | Table Info | `204<sit1><sit2>...` | `{"id": 204, "data": {"table": [1, 2, 0, 4]}}` | Implemented |
| 205 | My Info | `205<id><sit>` | `{"id": 205, "data": {"user_id": ..., "sit": ..., "name": "..."}}` | Implemented |
| 206 | Player Leave | `206<id><count>` | `{"id": 206, "data": {"user_id": ..., "count": ...}}` | Implemented |
| 290 | New Round | `290` | `{"id": 290}` | Implemented |
| 300 | Init Screen | `300` | `{"id": 300}` | Implemented |
| 301 | Change Card | `301<pos><card>` | `{"id": 301, "data": {"pos": ..., "card": ...}}` | Implemented |
| 302 | Deal Cards | `302<card1>...<card16>` | `{"id": 302, "data": {"cards": [1, 2, ...]}}` | Implemented |
| 303 | Can Get | `303` | `{"id": 303}` | Implemented |
| 304 | Get Card | `304<card>` | `{"id": 304, "data": {"card": ...}}` | Implemented |
| 305 | Card Owner | `305<sit>` | `{"id": 305, "data": {"sit": ...}}` | Implemented |
| 306 | Card Point | `306<count>` | `{"id": 306, "data": {"count": ...}}` | Implemented |
| 308 | Sort Card | `308<mode>` | `{"id": 308, "data": {"mode": ...}}` | Implemented |
| 310 | Player Pointer | `310<sit>` | `{"id": 310, "data": {"sit": ...}}` | Implemented |
| 312 | Time | `312<sit><time>` | `{"id": 312, "data": {"sit": ..., "time": ...}}` | Implemented |
| 313 | Request Card | `313` | `{"id": 313}` | Implemented |
| 314 | Show New Card | `314<sit><mode>` | `{"id": 314, "data": {"sit": ..., "mode": ...}}` | Implemented |
| 315 | Finish | `315` | `{"id": 315}` | Implemented |
| 330 | Sea Bottom | `330` | `{"id": 330}` | Implemented |
| 401 | Throw Card | `401<card>` | `{"id": 401, "data": {"card": ...}}` | Implemented |
| 402 | Card Thrown | `402<id><card>` | `{"id": 402, "data": {"user_id": ..., "card": ...}}` | Implemented |
| 450 | Wait Hit | `450` | `{"id": 450}` | Implemented |
| 501 | Check Card | `501<eat><pong><kang><win>` | `{"id": 501, "data": {"actions": {"eat": bool, "pong": bool, ...}}}` | Implemented |
| 510 | Check Result | `510<action>` | `{"id": 510, "data": {"action": ...}}` | Implemented |
| 515 | Next Request | `515` | `{"id": 515}` | Implemented |
| 518 | Wind Info | `518<w1><w2><w3><w4>` | `{"id": 518, "data": {"winds": [1, 2, 3, 4]}}` | Implemented |
| 520 | Epk | `520<card>` | `{"id": 520, "data": {"card": ...}}` | Implemented |
| 521 | Pool Info | `521<cards...>` | `{"id": 521, "data": {"pool": [...]}}` | Implemented |
| 522 | Make | `522<id><type>` | `{"id": 522, "data": {"user_id": ..., "type": ...}}` | Implemented |
| 525 | Flower | `525<card><pos>` | `{"id": 525, "data": {"card": ..., "pos": ...}}` | Implemented |
| 530 | Show Epk | `530<sit><type>...` | `{"id": 530, "data": {"sit": ..., "type": ..., "cards": [...]}}` | Implemented |