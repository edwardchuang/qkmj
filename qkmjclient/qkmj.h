
#ifndef _QKMJ_H_
#define _QKMJ_H_

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
#endif

#ifdef HAVE_LIBNCURSESW
  #include <wchar.h>
  #include <wctype.h>
#endif

#include <netinet/in.h>
#include "mjdef.h"
#include "screen.h"
#include "check.h"
#include "chkmake.h"
#include "checkscr.h"
#include "mjdef.h"
#include "socket.h"
#include "input.h" 
#include "card.h"
#include "chkmake.h"
#include "misc.h"
#include "socket.h"
#include "message.h"
#include "command.h"

/*gloable variables*/
extern char menu_item[25][5];
extern char mj_item[100][6];
extern char number_item[30][3];
extern struct tai_type {
  char name[20];
  int score;
  char flag;
} tai[100];
extern struct card_comb_type {
  char info[10][20];
  int set_count;
  int tai_sum;
  int tai_score[100];
} card_comb[30];
extern int comb_num;
extern char mj[150];
extern char sit_name[5][3];
extern char check_name[7][3];
extern fd_set rfds, afds;
extern char GPS_IP[255];
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
extern unsigned char my_pass[15];
extern unsigned char my_note[255];
extern struct ask_mode_info {
  int question;
  int answer_ok;
  char* answer;
} ask;
extern struct player_info {
  int sockfd;
  int in_table;
  int sit;
  unsigned int id;
  char name[30];
  long money;
  char pool[20];
  struct sockaddr_in addr;
} player[MAX_PLAYER];
extern struct pool_info {
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
} pool[5];
extern struct table_info {
  int cardnum;
  int wind;
  int dealer;
  int cont_dealer;
  int base_value;
  int tai_value;
} info;
extern struct timeval before, after;
extern int table[5];
extern int new_client;
extern char new_client_name[30];
extern long new_client_money;
extern unsigned new_client_id;
extern int player_num;
extern WINDOW* commentwin, *inputwin, *global_win, *playing_win;
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

// 函式宣告 (移至這裡)
int request_card();
void change_card(char position, char card);
void get_card(char card);
void process_new_card(char sit, char card);
void throw_card(char card);
void send_one_card(int id);
void next_player();
int next_turn(int current_turn);
void display_pool(int sit);
void sort_pool(int sit);
void sort_card(int mode);
void new_game();
void opening();
void open_deal();
void err(const char* errmsg);
void init_variable();
void clear_variable();
void gps();
static void read_qkmjrc();
static void init_config();
static void check_game_state();
static void handle_server_data();
static void handle_client_data(int player_id);
static void process_gps_data(int gps_sockfd);
static void handle_waiting_state();

#endif/*_QKMJ_H_*/