#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "qkmj.h"
#include "mjdef.h"

int set_color(int fore,int back);
void set_mode(int mode);
void mvprintstr(WINDOW *win,int y,int x,char *msg);
void printstr(WINDOW *win,char *str);
void printch(WINDOW *win,char ch);
void mvprintch(WINDOW *win,int y,int x,char ch);
void clear_screen_area (int ymin, int xmin, int height, int width);
void clear_input_line();
void wait_a_key(char *msg);
void ask_question(char *question,char *answer,int ans_len,int type);
void draw_index(int max_item);
void current_index(int current);
void show_cardback(char sit);
void show_allcard(char sit);
void show_kang(char sit);
void show_newcard(char sit,char type);
void show_card(char card,int x,int y,int type);
void draw_title();
void init_playing_screen();
void init_global_screen();
void wmvaddstr(WINDOW *win,int y,int x,char *str);
void draw_table();
void draw_global_screen();
void draw_playing_screen();
void find_point(int pos);
void display_point(int current_turn);
void display_time(char sit);
void display_info();
int readln(int fd,char *buf,int *end_flag);
void display_news(int fd);
void display_comment(char *comment);
void send_talk_line(char *talk);
void send_gps_line(char *msg);
int intlog10(int num);
void convert_num(char *str,int number,int digit);
void show_num(int y,int x,int number,int digit);
void show_cardmsg(int sit,char card);
void redraw_screen();
void reset_cursor();
void return_cursor();

#endif
