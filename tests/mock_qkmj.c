#include "qkmj.h"

// Mock Global Variables
struct player_info player[MAX_PLAYER];
struct pool_info pool[5];
struct table_info info;
int table[5];
int my_sit = 1;
int turn = 1;
int card_point = 0;
int check_flag[5][8];
int gps_sockfd = 0; // For ai_client to link if it uses it? 
// ai_client uses gps_sockfd? No, it uses curl. 
// But it uses `player`, `table`, `pool`, `info`, `my_sit`, `turn`, `card_point`.

// Define other externs required by ai_client or headers
char menu_item[25][10];
char mj_item[100][10];
char number_item[30][10];
struct tai_type tai[100];
struct card_comb_type card_comb[20];
int comb_num;
char mj[150];
char sit_name[5][10];
char check_name[7][10];
fd_set rfds, afds;
char GPS_IP[50];
int GPS_PORT;
char my_username[20];
char my_address[70];
int SERV_PORT;
int set_beep;
int pass_login;
int pass_count;
int card_in_pool[5];
int card_index;
int current_item;
int pos_x, pos_y;
int check_number;
int current_check;
int check_x, check_y;
int eat_x, eat_y;
int card_count;
int serv_sockfd, table_sockfd;
int in_serv, in_join;
char a[2];
char talk_buf[255];
int talk_buf_count;
char history[HISTORY_SIZE + 1][255];
int h_head, h_tail, h_point;
int talk_x, talk_y;
int talk_left, talk_right;
int comment_x, comment_y;
int comment_left, comment_right, comment_bottom, comment_up;
char comment_lines[24][255];
int talk_mode;
int screen_mode;
int play_mode;
unsigned char key_buf[255];
char wait_hit[5];
int waiting;
unsigned char* str;
int key_num;
int input_mode;
int current_mode;
unsigned char cmd_argv[40][100];
int arglenv[40];
int narg;
int my_id;
long my_money;
unsigned int my_gps_id;
unsigned char my_name[50];
unsigned char my_pass[10];
unsigned char my_note[255];
struct ask_mode_info ask;
struct timeval before, after;
int new_client;
char new_client_name[30];
long new_client_money;
unsigned new_client_id;
int player_num;
WINDOW *commentwin, *inputwin, *global_win, *playing_win;
int card_owner;
int in_kang;
int current_id;
int current_card;
int on_seat;
int player_in_table;
int PLAYER_NUM;
int check_on, in_check[6], check_for[6];
int go_to_check;
int send_card_on, send_card_request;
int getting_card;
int next_player_on, next_player_request;
int color;
int cheat_mode;
char table_card[6][17];

void write_msg(int fd, char* msg) {
    // Mock implementation
}
