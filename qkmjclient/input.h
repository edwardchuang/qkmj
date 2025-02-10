#ifndef _INPUT_H_
#define _INPUT_H_

// 函式宣告：處理遊戲模式的按鍵事件
static void handle_play_mode_key(int key);
// 函式宣告：處理檢查模式的按鍵事件
static void handle_check_mode_key(int key);
// 函式宣告：處理吃牌模式的按鍵事件
static void handle_eat_mode_key(int key);
// 函式宣告：處理聊天模式的按鍵事件
static void handle_talk_mode_key(int key);
int my_getch();
void process_key();

#endif/*_INPUT_H_*/