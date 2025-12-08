#ifndef QKMJ_H
#define QKMJ_H

#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#if defined(HAVE_NCURSESW_CURSES_H)
#include <ncursesw/curses.h>
#elif defined(HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#include <curses.h>
#else
#error "No curses header found"
#endif

#include "mjdef.h"

/* Global Variables */
extern char menu_item[25][10];
extern char mj_item[100][10];
extern char number_item[30][10];

struct tai_type {
  char name[20];
  int score;
  char flag;
};
extern struct tai_type tai[100];

struct card_comb_type {
  char info[10][20];
  int set_count;
  int tai_sum;
  int tai_score[100];
};
extern struct card_comb_type card_comb[20];

extern int comb_num;
extern char mj[150];
extern char sit_name[5][10];
extern char check_name[7][10];
extern fd_set rfds, afds;
extern char GPS_IP[50];
extern int GPS_PORT;
extern char my_username[20];
extern char my_address[70];
extern int SERV_PORT;
extern int set_beep;
extern int pass_login;
extern int pass_count;
extern int card_in_pool[5];
extern int card_point, card_index;
extern int current_item;
extern int pos_x, pos_y;
extern int check_number;
extern int current_check;
extern int check_x, check_y;
extern int eat_x, eat_y;
extern int card_count;
extern int gps_sockfd, serv_sockfd, table_sockfd;
extern int in_serv, in_join;
extern char a[2];
extern char talk_buf[255];
extern int talk_buf_count;
extern char history[HISTORY_SIZE + 1][255];
extern int h_head, h_tail, h_point;
extern int talk_x, talk_y;
extern int talk_left, talk_right;
extern int comment_x, comment_y;
extern int comment_left, comment_right, comment_bottom, comment_up;
extern char comment_lines[24][255];
extern int talk_mode;
extern int screen_mode;
extern int play_mode;
extern unsigned char key_buf[255];
extern char wait_hit[5];
extern int waiting;
extern unsigned char* str;
extern int key_num;
extern int input_mode;
extern int current_mode;
extern unsigned char cmd_argv[40][100];
extern int arglenv[40];
extern int narg;
extern int my_id;
extern int my_sit;
extern long my_money;
extern unsigned int my_gps_id;
extern unsigned char my_name[50];
extern unsigned char my_pass[10];
extern unsigned char my_note[255];

struct ask_mode_info {
  int question;
  int answer_ok;
  char* answer;
};
extern struct ask_mode_info ask;

struct player_info {
  int sockfd;
  int in_table;
  int sit;
  unsigned int id;
  char name[30];
  long money;
  char pool[20];
  struct sockaddr_in addr;
  int is_ai;
};
extern struct player_info player[MAX_PLAYER];

struct pool_info {
  char name[30];
  int num;
  char card[20];
  char out_card_index;
  char out_card[10][6];
  char flower[10];
  char door_wind;
  int first_round;
  long money;
  float time;
};
extern struct pool_info pool[5];

struct table_info {
  int cardnum;
  int wind;
  int dealer;
  int cont_dealer;
  int base_value;
  int tai_value;
};
extern struct table_info info;

extern struct timeval before, after;
extern int table[5];
extern int new_client;
extern char new_client_name[30];
extern long new_client_money;
extern unsigned new_client_id;
extern int player_num;
extern WINDOW *commentwin, *inputwin, *global_win, *playing_win;
extern int turn;
extern int card_owner;
extern int in_kang;
extern int current_id;
extern int current_card;
extern int on_seat;
extern int player_in_table;
extern int PLAYER_NUM;
extern int check_flag[5][8], check_on, in_check[6], check_for[6];
extern int go_to_check;
extern int send_card_on, send_card_request;
extern int getting_card;
extern int next_player_on, next_player_request;
extern int color;
extern int cheat_mode;
extern char table_card[6][17];

/* Function Prototypes */

/* qkmj.c */
void write_msg(int fd, char* msg);
void show_card(int card, int x, int y, int color);
void show_cardmsg(int sit, int card);
void return_cursor();
int check_flower(int sit, int card);
void show_num(int x, int y, int num, int color);
void broadcast_msg(int player_id, char* msg);
void show_newcard(int sit, int type);
void clear_check_flag(int sit);
int check_kang(int sit, int card);
int check_make(int sit, int card, int method);
void init_check_mode();
void display_point(int sit);
void display_comment(char* msg);
void clear_screen_area(int y, int x, int h, int w);
void draw_table();
void display_info();
void draw_index(int num);
void show_cardback(int sit);
int generate_random(int range);
void generate_card();
int check_begin_flower(int sit, int card, int position);
int init_socket(char* host, int port, int* sockfd);
void init_global_screen();
void get_my_info();
void ask_question(char* prompt, char* ans, int len, int mode);
int read_msg_id(int fd, char* msg);
void close_join();
void close_serv();
void process_msg(int player_id, unsigned char* id_buf, int msg_type);
void accept_new_client();
int read_msg(int fd, char* msg);
void close_client(int player_id);
void compare_check();
void show_allcard(int sit);
void show_kang(int sit);
void send_pool_card();
void wmvaddstr(WINDOW* win, int y, int x, char* str);
void wait_a_key(char* msg);
void Tokenize(char* strinput);
void my_strupr(char* upper, char* org);
void init_serv_socket();
void init_playing_screen();
void send_gps_line(char* msg);
void display_news(int fd);
void display_time(int sit);
int leave();
void process_key();
void opening();
void open_deal();
void sort_card(int mode);
int next_turn(int current_turn);
void sort_pool(int sit);
void beep1();
void change_card(int position, int card);
void get_card(int card);
void process_new_card(int sit, int card);
void throw_card(int card);
void send_one_card(int id);
void next_player();
void display_pool(int sit);
void new_game();
void err(char* errmsg);
void init_variable();
void clear_variable();
void gps();
void read_qkmjrc();

/* screen.c */
void set_color(int fore, int back);
void set_mode(int mode);
void mvprintstr(WINDOW* win, int y, int x, char* msg);
void printstr(WINDOW* win, char* str);
void printch(WINDOW* win, char ch);
void mvprintch(WINDOW* win, int y, int x, char ch);
void clear_input_line();
void current_index(int current);
void draw_title();
void find_point(int pos);
void send_talk_line(char* talk);
int intlog10(int num);
void convert_num(char* str, int number, int digit);
void redraw_screen();
void reset_cursor();
void mvwgetstring(WINDOW* win, int y, int x, int max_len,
                  unsigned char* str_buf, int mode);
int my_getch();

/* command.c */
int command_mapper(char* cmd);
void who(char* name);
void help();
void command_parser(char* msg);

/* check.c */
int search_card(int sit, int card);
int check_eat(int sit, int card);
void check_card(int sit, int card);
void write_check(int check);
int check_pong(int sit, int card);

/* chkmake.c */
void full_check(int sit, int make_card);
int valid_type(int type);
void check_tai(int sit, int comb, int make_card);

/* misc.c */
float thinktime();

/* socket.c */
int Check_for_data(int fd);

/* input.c */
// process_key already in qkmj.c list

/* chkmake.c */
void process_epk(int check);
void draw_epk(int id, int kind, int card1, int card2, int card3);
void draw_flower(int sit, int card);
void process_make(int sit, int card);

#endif