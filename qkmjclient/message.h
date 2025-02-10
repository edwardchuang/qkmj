#ifndef _MESSAGE_H_
#define _MESSAGE_H_

// 常數定義
#define MAX_MSG_LEN 256 // 訊息緩衝區最大長度
#define NAME_LENGTH 30  // 玩家名稱最大長度
#define PASSWORD_LENGTH 15 // 密碼最大長度
#define OUT_CARD_SIZE 6   // 明槓時 out_card 的大小

void process_msg(int player_id, const unsigned char *id_buf, int msg_type);
// 函式宣告
int convert_msg_id(const unsigned char *msg, int player_id);
// 內部函式宣告：處理來自 GPS 伺服器的訊息
static void process_msg_from_gps(int player_id, unsigned char *buf,
                                 int msg_id);
// 內部函式宣告：處理來自客戶端的訊息
static void process_msg_from_client(int player_id, unsigned char *buf,
                                    int msg_id);
// 內部函式宣告：處理來自伺服器的訊息
static void process_msg_from_server(int player_id, unsigned char *buf,
                                    int msg_id);
#endif
