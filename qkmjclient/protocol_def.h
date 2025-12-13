#ifndef QKMJ_PROTOCOL_DEF_H
#define QKMJ_PROTOCOL_DEF_H

/* Client -> GPS Server (mjgps) */
#define MSG_GET_USERNAME     99
#define MSG_CHECK_VERSION    100
#define MSG_LOGIN            101
#define MSG_CHECK_PASSWORD   102
#define MSG_CREATE_ACCOUNT   103
#define MSG_CHANGE_PASSWORD  104
#define MSG_KILL_DUPLICATE   105
#define MSG_LIST_PLAYERS     2
#define MSG_LIST_TABLES      3
#define MSG_SET_NOTE         4
#define MSG_USER_INFO        5
#define MSG_WHO_IN_TABLE     6
#define MSG_BROADCAST        7
#define MSG_INVITE           8
#define MSG_SEND_MESSAGE     9
#define MSG_LURKER_LIST      10
#define MSG_JOIN_TABLE       11
#define MSG_OPEN_TABLE       12
#define MSG_LIST_TABLES_FREE 13
#define MSG_CHECK_OPEN       14
#define MSG_WIN_GAME         20
#define MSG_FIND_USER        21
#define MSG_GAME_RECORD      900
#define MSG_LEAVE            200
#define MSG_STATUS           201
#define MSG_KICK_USER        202
#define MSG_LEAVE_TABLE_GPS  205
#define MSG_SHUTDOWN         500

/* GPS Server -> Client */
#define MSG_TEXT_MESSAGE     101
#define MSG_LOGIN_OK         2   /* Originally 002 */
#define MSG_WELCOME          3   /* Originally 003 */
#define MSG_PASSWORD_FAIL    4   /* Originally 004 */
#define MSG_NEW_USER         5   /* Originally 005 */
#define MSG_DUPLICATE_LOGIN  6   /* Originally 006 */
#define MSG_VERSION_ERROR    10  /* Originally 010 */
#define MSG_JOIN_INFO        11  /* Originally 011 */
#define MSG_OPEN_OK          12  /* Originally 012 */
#define MSG_UPDATE_MONEY     120
/* MSG_KICK (200) is same as Client->Server */
#define MSG_JOIN_NOTIFY      211

/* Client <-> Table Server (P2P) */
/* MSG_CHAT (101) same ID as Text Message */
#define MSG_ACTION_CHAT      102
#define MSG_AI_MODE          130
#define MSG_LEADER_LEAVE     199
/* MSG_LEAVE (200) same as above */
#define MSG_NEW_COMER_INFO   201
#define MSG_UPDATE_MONEY_P2P 202
#define MSG_OTHER_INFO       203
#define MSG_TABLE_INFO       204
#define MSG_MY_INFO          205
#define MSG_PLAYER_LEAVE     206
#define MSG_NEW_ROUND        290
#define MSG_INIT_SCREEN      300
#define MSG_CHANGE_CARD      301
#define MSG_DEAL_CARDS       302
#define MSG_CAN_GET          303
#define MSG_GET_CARD         304
#define MSG_CARD_OWNER       305
#define MSG_CARD_POINT       306
#define MSG_SORT_CARD        308
#define MSG_PLAYER_POINTER   310
#define MSG_TIME             312
#define MSG_REQUEST_CARD     313
#define MSG_SHOW_NEW_CARD    314
#define MSG_FINISH           315
#define MSG_SEA_BOTTOM       330
#define MSG_THROW_CARD       401
#define MSG_CARD_THROWN      402
#define MSG_WAIT_HIT         450
#define MSG_CHECK_CARD       501
#define MSG_CHECK_RESULT     510
#define MSG_NEXT_REQUEST     515
#define MSG_WIND_INFO        518
#define MSG_EPK              520
#define MSG_POOL_INFO        521
#define MSG_MAKE             522
#define MSG_FLOWER           525
#define MSG_SHOW_EPK         530

#endif /* PROTOCOL_DEF_H */