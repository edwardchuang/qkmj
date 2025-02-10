#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>

#include "qkmj.h"
#include "mjdef.h"

// 設定顏色
void set_color(int fore, int back);
// 設定顯示模式
void set_mode(int mode);
// 在視窗中指定位置列印字串
void mvprintstr(WINDOW *win, int y, int x, const char *msg);
// 在視窗中列印字串
void printstr(WINDOW *win, const char *str);
// 在視窗中列印單一字元
void printch(WINDOW *win, char ch);
// 在視窗中指定位置列印單一字元
void mvprintch(WINDOW *win, int y, int x, char ch);
// 清除螢幕區域
void clear_screen_area(int ymin, int xmin, int height, int width);
// 清除輸入列
void clear_input_line(void);
// 等待使用者按下按鍵
void wait_a_key(const char *msg);
// 詢問使用者問題
void ask_question(const char *question, char *answer, int ans_len, int type);
// 繪製選單索引
void draw_index(int max_item);
// 設定目前索引
void current_index(int current);
// 顯示牌背
void show_cardback(char sit);
// 顯示所有牌
void show_allcard(char sit);
// 顯示槓牌
void show_kang(char sit);
// 顯示新牌
void show_newcard(char sit, char type);
// 顯示牌
void show_card(char card, int x, int y, int type);
// 繪製標題
void draw_title(void);
// 初始化遊戲畫面
void init_playing_screen(void);
// 初始化全局畫面
void init_global_screen(void);
// 在視窗中指定位置列印字串
int wmvaddstr(WINDOW *win, int y, int x, const char *str);
// 繪製桌子
void draw_table(void);
// 繪製全局畫面
void draw_global_screen(void);
// 繪製遊戲畫面
void draw_playing_screen(void);
// 尋找點數位置
void find_point(int pos);
// 顯示點數
void display_point(int current_turn);
// 顯示時間
void display_time(char sit);
// 顯示資訊
void display_info(void);
// 讀取一行資料
int readln(int fd, char *buf, int *end_flag);
// 顯示新聞
void display_news(int fd);
// 顯示留言
void display_comment(const char *comment);
// 送出對話
void send_talk_line(const char *talk);
// 送出遊戲訊息
void send_gps_line(const char *msg);
// 計算數字位數
int intlog10(int num);
// 將數字轉換為字串
void convert_num(char *str, int number, int digit);
// 顯示數字
void show_num(int y, int x, int number, int digit);
// 顯示牌訊息
void show_cardmsg(int sit, char card);
// 重繪螢幕
void redraw_screen(void);
// 重設游標
void reset_cursor(void);
// 返回游標
void return_cursor(void);

#endif
