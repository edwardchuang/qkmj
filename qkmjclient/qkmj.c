#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mjdef.h"

#include "input.h"
#include "protocol.h"
#include "protocol_def.h"
#include "qkmj.h"  // Ensure qkmj.h is included to see prototypes
#include "socket.h"
#include "ai_client.h"

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

/* Display local IP addresses on splash screen */
void display_local_ips() {
  struct ifaddrs *ifaddr, *ifa;
  char host[NI_MAXHOST];
  char msg[256];

  if (getifaddrs(&ifaddr) == -1) return;

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) continue;
    if (ifa->ifa_flags & IFF_LOOPBACK) continue;

    if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST) == 0) {
      snprintf(msg, sizeof(msg), "本機 IP (%s): %s", ifa->ifa_name, host);
      display_comment(msg);
    }
  }
  freeifaddrs(ifaddr);
}

/*gloable variables*/
fd_set rfds, afds;
int nfds;
extern int errno;
char GPS_IP[50];
int GPS_PORT;
char my_username[20];
char my_address[70];
int PLAYER_NUM = 4;
char QKMJ_VERSION[] = "200";
char menu_item[25][10] = {"  ", "A ", "B ", "C ", "D ", "E ", "F ",
                          "G ", "H ", "I ", "J ", "K ", "L ", "M ",
                          "N ", "O ", "P ", "Q ", "R ", "  "};
char number_item[30][10] = {"０", "１", "２", "３", "４", "５", "６",
                            "７", "８", "９", "10", "11", "12", "13",
                            "14", "15", "16", "17", "18", "19", "20"};
char mj_item[100][10] = {
    "＊＊", "一萬", "二萬", "三萬", "四萬", "五萬", "六萬", "七萬", "八萬",
    "九萬", "摸牌", "一索", "二索", "三索", "四索", "五索", "六索", "七索",
    "八索", "九索", "    ", "一筒", "二筒", "三筒", "四筒", "五筒", "六筒",
    "七筒", "八筒", "九筒", "▉▉",   "東風", "南風", "西風", "北風", "    ",
    "    ", "    ", "    ", "    ", "▇▇",   "紅中", "白板", "青發", "    ",
    "    ", "    ", "    ", "    ", "    ", "    ", "春１", "夏２", "秋３",
    "冬４", "梅１", "蘭２", "菊３", "竹４"};
struct tai_type tai[100] = {
    {"莊家", 1, 0},      {"門清", 1, 0},     {"自摸", 1, 0},
    {"斷么九", 1, 0},    {"一杯口", 1, 0},   {"槓上開花", 1, 0},
    {"海底摸月", 1, 0},  {"河底撈魚", 1, 0}, {"搶槓", 1, 0},
    {"東風", 1, 0},      {"南風", 1, 0},     {"西風", 1, 0},
    {"北風", 1, 0},      {"紅中", 1, 0},     {"白板", 1, 0},
    {"青發", 1, 0},      {"花牌", 1, 0},     {"東風東", 2, 0},
    {"南風南", 2, 0},    {"西風西", 2, 0},   {"北風北", 2, 0},
    {"春夏秋冬", 2, 0},  {"梅蘭菊竹", 2, 0}, {"全求人", 2, 0},
    {"平胡", 2, 0},      {"混全帶么", 2, 0}, {"三色同順", 2, 0},
    {"一條龍", 2, 0},    {"二杯口", 2, 0},   {"三暗刻", 2, 0},
    {"三槓子", 2, 0},    {"三色同刻", 2, 0}, {"門清自摸", 3, 0},
    {"碰碰胡", 4, 0},    {"混一色", 4, 0},   {"純全帶么", 4, 0},
    {"混老頭", 4, 0},    {"小三元", 4, 0},   {"四暗刻", 6, 0},
    {"四槓子", 6, 0},    {"大三元", 8, 0},   {"小四喜", 8, 0},
    {"清一色", 8, 0},    {"字一色", 8, 0},   {"七搶一", 8, 0},
    {"五暗刻", 8, 0},    {"清老頭", 8, 0},   {"大四喜", 16, 0},
    {"八仙過海", 16, 0}, {"天胡", 16, 0},    {"地胡", 16, 0},
    {"人胡", 16, 0},     {"連  拉  ", 2, 0}};
struct card_comb_type card_comb[20];
int comb_num;
char mj[150];
char sit_name[5][10] = {"  ", "東", "南", "西", "北"};
char check_name[7][10] = {"無", "吃", "碰", "槓", "胡", "聽"};

int SERV_PORT = DEFAULT_SERV_PORT;
int set_beep;
int pass_login;
int pass_count;
int current_item;
int card_in_pool[5];
int pos_x, pos_y;
int current_check;
int check_number;
int check_x, check_y;
int eat_x, eat_y;
int card_count = 0;
int card_point;
int card_index;
int gps_sockfd, serv_sockfd = -1, table_sockfd = -1;
int in_serv, in_join;
char talk_buf[255] = "\0";
int talk_buf_count = 0;
char history[HISTORY_SIZE + 1][255];
int h_head, h_tail, h_point;
int talk_x, talk_y;
int talk_left, talk_right;
int comment_x, comment_y;
int comment_left, comment_right, comment_bottom, comment_up;
char comment_lines[24][255];
int talk_mode;
int play_mode;
int screen_mode;
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
int my_sit;
long my_money;
unsigned int my_gps_id;
unsigned char my_name[50];
unsigned char my_pass[10];
unsigned char my_note[255];
char current_match_id[64] = "";
int move_serial = 0;
struct ask_mode_info ask;
struct player_info player[MAX_PLAYER];
struct pool_info pool[5];
struct table_info info;
struct timeval before, after;
int table[5];
int new_client;
char new_client_name[30];
long new_client_money;
unsigned int new_client_id;
int player_num;
WINDOW *commentwin, *inputwin, *global_win, *playing_win;
int turn;
int card_owner;
int in_kang;
int current_id;
int current_card;
int in_play;
int on_seat;
int player_in_table;
int check_flag[5][8], check_on, in_check[6], check_for[6];
int go_to_check;
int send_card_on;
int send_card_request;
int getting_card;
int next_player_on;
int next_player_request;
int color = 1;
int cheat_mode = 0;
char table_card[6][17];

/* Request a card */
int request_card() {
  if (in_join) {
    send_json(table_sockfd, MSG_REQUEST_CARD, NULL);
    return 0;
  } else {
    return mj[card_point++];
  }
}

/* Change a card */
void change_card(int position, int card) {
  int i = (16 - pool[my_sit].num + position) * 2;
  if (position == pool[my_sit].num) i++;
  pool[my_sit].card[position] = (char)card;
  show_card(20, INDEX_X + i, INDEX_Y + 1, 1);
  wrefresh(stdscr);
  show_card(card, INDEX_X + i, INDEX_Y + 1, 1);
  wrefresh(stdscr);
}

/* Get a card */
void get_card(int card) {
  pool[my_sit].card[pool[my_sit].num] = (char)card;
  show_card(20, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
  wrefresh(stdscr);
  show_card(card, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
  wrefresh(stdscr);

  send_game_log("Draw", card, NULL);

  pool[my_sit].time += thinktime();
  display_time(my_sit);

  getting_card = 1;
}

void process_new_card(int sit, int card) {
  current_card = card;
  show_cardmsg(sit, 0);
  pool[sit].card[pool[sit].num] = (char)card;
  get_card(card);
  return_cursor();
  if (!check_flower(sit, card)) play_mode = THROW_CARD;
}

/* Throw cards to the table */
void throw_card(int card) {
  int x, y;
  if (card == 20) {
    card_count--;
  }
  table_card[card_count / 17][card_count % 17] = (char)card;
  x = THROW_X + (card_count % 17) * 2;
  y = THROW_Y + card_count / 17 * 2;
  if (y % 4 == 3) {
    if (!color) attron(A_BOLD); 
    show_card(card, x, y, 1);
    if (!color) attroff(A_BOLD);
  } else
    show_card(card, x, y, 1);
  if (card != 20) {
    card_count++;
  }
}

void send_one_card(int id) {
  char card;
  char sit;
  int i;
  cJSON *payload;

  sit=(char)player[id].sit;
  card = mj[card_point++];
  current_card = card;
  pool[sit].card[pool[sit].num] = card;
  show_num(2, 70, 144 - card_point - 16, 2);

  /* 314 MSG_SHOW_NEW_CARD */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", sit);
  cJSON_AddNumberToObject(payload, "mode", 2);
  /* Broadcast to everyone except 'id' (Wait, original said broadcast_msg(id, ...)) 
     broadcast_msg(id, msg) means send to everyone EXCEPT id.
     So we broadcast to everyone except id.
  */
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != id) {
        send_json(player[i].sockfd, MSG_SHOW_NEW_CARD, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  show_newcard(sit, 2);
  return_cursor();

  /* 306 MSG_CARD_POINT */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "count", card_point);
  /* broadcast_msg(1, ...) -> except me (1) */
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_CARD_POINT, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  show_cardmsg(sit, 0);
  card_owner = sit;

  /* 305 MSG_CARD_OWNER */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", sit);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_CARD_OWNER, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  clear_check_flag(sit);
  check_flag[sit][3] = check_kang(sit, card);
  check_flag[sit][4] = check_make(sit, card, 0);
  in_check[sit] = 0;
  for (i = 1; i < check_number; i++)
    if (check_flag[sit][i]) {
      getting_card = 1;
      in_check[sit] = 1;
      check_on = 1;
      
      /* 501 MSG_CHECK_CARD */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "eat", check_flag[sit][1]);
      cJSON_AddNumberToObject(payload, "pong", check_flag[sit][2]);
      cJSON_AddNumberToObject(payload, "kang", check_flag[sit][3]);
      cJSON_AddNumberToObject(payload, "win", check_flag[sit][4]);
      send_json(player[table[sit]].sockfd, MSG_CHECK_CARD, payload);
      break;
    }
  
  /* 304 MSG_GET_CARD */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "card", card);
  send_json(player[id].sockfd, MSG_GET_CARD, payload);

  gettimeofday(&before, (struct timezone*)0);
}

void next_player() {
  cJSON *payload;
  int i;
  turn = next_turn(turn);

  /* 310 MSG_PLAYER_POINTER */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", turn);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_PLAYER_POINTER, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  display_point(turn);
  return_cursor();
  if (table[turn] != 1) {
    /* 303 MSG_CAN_GET */
    send_json(player[table[turn]].sockfd, MSG_CAN_GET, NULL);
    
    show_newcard(turn, 1);
    return_cursor();
  } else {
    attron(A_REVERSE);
    show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
    attroff(A_REVERSE);
    wrefresh(stdscr);
    return_cursor();
    play_mode = GET_CARD;
    beep1();
  }
  
  /* 314 MSG_SHOW_NEW_CARD */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", turn);
  cJSON_AddNumberToObject(payload, "mode", 1);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != table[turn]) {
        send_json(player[i].sockfd, MSG_SHOW_NEW_CARD, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);
}

int next_turn(int current_turn) {
  current_turn++;
  if (current_turn == 5) current_turn = 1;
  if (!table[current_turn]) current_turn = next_turn(current_turn);
  return (current_turn);
}

void display_pool(int sit) {
  int i;
  char buf[5], msg_buf[255];

  msg_buf[0] = 0;
  for (i = 0; i < pool[sit].num; i++) {
    snprintf(buf, sizeof(buf), "%2d", pool[sit].card[i]);
    strncat(msg_buf, buf, sizeof(msg_buf) - strlen(msg_buf) - 1);
  }
  display_comment(msg_buf);
}
void sort_pool(int sit) {
  int i, j, max;
  char tmp;

  max = pool[sit].num;
  for (i = 0; i < max; i++)
    for (j = 0; j < max - i - 1; j++)
      if (pool[sit].card[j] > pool[sit].card[j + 1]) {
        tmp = pool[sit].card[j];
        pool[sit].card[j] = pool[sit].card[j + 1];
        pool[sit].card[j + 1] = tmp;
      }
}

void sort_card(int mode) {
  int i, j, max;
  char tmp;

  if (mode)
    max = pool[my_sit].num + 1;
  else
    max = pool[my_sit].num;
  for (i = 0; i < max; i++)
    for (j = 0; j < max - i - 1; j++)
      if (pool[my_sit].card[j] > pool[my_sit].card[j + 1]) {
        tmp = pool[my_sit].card[j];
        pool[my_sit].card[j] = pool[my_sit].card[j + 1];
        pool[my_sit].card[j + 1] = tmp;
      }
  for (i = 0; i < pool[my_sit].num; i++) {
    show_card(20, INDEX_X + (16 - pool[my_sit].num + i) * 2, INDEX_Y + 1, 1);
    wrefresh(stdscr);
    show_card(pool[my_sit].card[i], INDEX_X + (16 - pool[my_sit].num + i) * 2,
              INDEX_Y + 1, 1);
    wrefresh(stdscr);
  }
  if (mode) {
    show_card(20, INDEX_X + (16 - pool[my_sit].num + i) * 2 + 1, INDEX_Y + 1,
              1);
    wrefresh(stdscr);
    show_card(pool[my_sit].card[pool[my_sit].num],
              INDEX_X + (16 - pool[my_sit].num + i) * 2 + 1, INDEX_Y + 1, 1);
    wrefresh(stdscr);
  }
  return_cursor();
}

void new_game() {
  clear_screen_area(0, 2, 18, 54);
  show_cardmsg(my_sit, 0);
  draw_table();
  wrefresh(stdscr);
}

void opening() {
  int i, j;

  new_game();
  input_mode = PLAY_MODE;
  for (i = 1; i <= 4; i++) {
    in_check[i] = 0;
    pool[i].first_round = 1;
    wait_hit[i] = 0;
    pool[i].num = 16;
    check_for[i] = 0;
    pool[i].out_card_index = 0;
    for (j = 0; j < 8; j++) pool[i].flower[j] = 0;
    pool[i].time = 0;
  }
  display_info();
  current_item = pool[my_sit].num;
  draw_index(pool[my_sit].num + 1);
  for (i = 0; i < 16; i++) pool[my_sit].card[i] = 0;
  pos_x = INDEX_X + current_item * 2 + 1;
  pos_y = INDEX_Y;
  play_mode = 0;
  card_count = 0;
  check_on = 0;
  for (i = 1; i <= 4; i++) {
    if (i != my_sit && table[i]) show_cardback(i);
  }
}

void open_deal() {
  int i, j, sit;
  char card;
  cJSON *payload, *winds, *cards;

  turn = info.dealer;
  i = generate_random(4);
  pool[i % 4 + 1].door_wind = 1;
  pool[(i + 1) % 4 + 1].door_wind = 2;
  pool[(i + 2) % 4 + 1].door_wind = 3;
  pool[(i + 3) % 4 + 1].door_wind = 4;
  
  /* 518 MSG_WIND_INFO */
  payload = cJSON_CreateObject();
  winds = cJSON_CreateArray();
  cJSON_AddItemToArray(winds, cJSON_CreateNumber(pool[1].door_wind));
  cJSON_AddItemToArray(winds, cJSON_CreateNumber(pool[2].door_wind));
  cJSON_AddItemToArray(winds, cJSON_CreateNumber(pool[3].door_wind));
  cJSON_AddItemToArray(winds, cJSON_CreateNumber(pool[4].door_wind));
  cJSON_AddItemToObject(payload, "winds", winds);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_WIND_INFO, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  wmvaddstr(stdscr, 2, 64, sit_name[pool[my_sit].door_wind]);
  if (!table[turn]) turn = next_turn(turn);
  card_owner = turn;
  
  /* 310 MSG_PLAYER_POINTER */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", turn);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_PLAYER_POINTER, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  /* 305 MSG_CARD_OWNER */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", card_owner);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_CARD_OWNER, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  display_point(turn);
  card_point = 0;
  card_index = 143;

  generate_card();
  /* send 16 cards to 4 people */
  for (j = 1; j <= 4; j++) {
    if (table[j]) {
      pool[j].first_round = 1;
      
      cards = cJSON_CreateArray();
      for (i = 0; i < 16; i++) {
        card = mj[card_point++];
        pool[j].card[i] = card;
        cJSON_AddItemToArray(cards, cJSON_CreateNumber(card));
      }
      sort_pool(j);
      
      if (table[j] != 1) /* not server */
      {
        /* 302 MSG_DEAL_CARDS */
        payload = cJSON_CreateObject();
        cJSON_AddItemToObject(payload, "cards", cards);
        send_json(player[table[j]].sockfd, MSG_DEAL_CARDS, payload);
      } else {
        cJSON_Delete(cards);
        sort_card(0);
      }
    }
  }
  /* send an additional card to dealer */
  card = mj[card_point++];
  current_card = card;

  /* 304 MSG_GET_CARD */
  if (table[turn] != 1) {
    show_newcard(turn, 2);
    return_cursor();
    
    payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "card", card);
    send_json(player[table[turn]].sockfd, MSG_GET_CARD, payload);

    pool[turn].card[pool[turn].num] = card;
  }
  if (table[turn] == 1) /* turn==server */
  {
    pool[1].card[pool[1].num] = card;
    show_card(pool[1].card[pool[my_sit].num], INDEX_X + 16 * 2 + 1, pos_y + 1,
              1);
    wrefresh(stdscr);
    return_cursor();
    play_mode = THROW_CARD;
  }
  
  /* 314 MSG_SHOW_NEW_CARD */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "sit", turn);
  cJSON_AddNumberToObject(payload, "mode", 2);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != table[turn]) {
        send_json(player[i].sockfd, MSG_SHOW_NEW_CARD, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  in_play = 1;
  /* check for flowers for 4 players */
  sit = turn; /* check dealer first */
  do {
  } while (check_begin_flower(sit, pool[sit].card[16], 16));
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      if (table[sit]) do {
        } while (check_begin_flower(sit, pool[sit].card[j], j));
    }
    sort_pool(sit);
    sit = next_turn(sit);
  }
  current_card = pool[turn].card[16];
  show_num(2, 70, 144 - card_point - 16, 2);
  return_cursor();
  
  /* 306 MSG_CARD_POINT */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "count", card_point);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_CARD_POINT, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  clear_check_flag(turn);
  check_flag[turn][3] = check_kang(turn, current_card);
  check_flag[turn][4] = check_make(turn, current_card, 0);
  in_check[turn] = 0;
  for (i = 1; i < check_number; i++) {
    if (check_flag[turn][i]) {
      getting_card = 1;
      in_check[turn] = 1;
      check_on = 1;
      if (in_serv) {
        init_check_mode();
      } else {
        /* 501 MSG_CHECK_CARD */
        payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(payload, "eat", 0);
        cJSON_AddNumberToObject(payload, "pong", 0);
        cJSON_AddNumberToObject(payload, "kang", check_flag[turn][3]);
        cJSON_AddNumberToObject(payload, "win", check_flag[turn][4]);
        send_json(player[table[turn]].sockfd, MSG_CHECK_CARD, payload);
      }
    }
  }
  
  /* 308 MSG_SORT_CARD */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "mode", 0);
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != table[turn]) {
        send_json(player[i].sockfd, MSG_SORT_CARD, cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  if (turn != my_sit) {
      /* 308 for turn player (mode 1) */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "mode", 1);
      send_json(player[table[turn]].sockfd, MSG_SORT_CARD, payload);
  }

  if (turn == my_sit)
    sort_card(1);
  else
    sort_card(0);
  gettimeofday(&before, (struct timezone*)0);
}

void err(char* errmsg) { display_comment(errmsg); }

void init_variable() {
  int i, j;

  my_name[0] = 0;
  my_pass[0] = 0;
  pass_login = 0;
  set_beep = 1;
  in_play = 0;
  in_serv = 0;
  in_join = 0;
  in_kang = 0;
  new_client = 0;
  player_num = 0;
  check_x = org_check_x;
  check_y = org_check_y;
  check_number = 5;
  input_mode = ASK_MODE;
  info.wind = 1;
  info.dealer = 1;
  info.cont_dealer = 0;
  info.base_value = DEFAULT_BASE;
  info.tai_value = DEFAULT_TAI;
  for (i = 0; i < 5; i++) {
    table[i] = 0;
  }
  player[0].money = 0;
  on_seat = 0;
  check_on = 0;
  send_card_on = 0;
  send_card_request = 0;
  next_player_on = 0;
  global_win = newwin(1, 63, org_talk_y, 11);
  playing_win = newwin(1, 43, org_talk_y, 11);
  ask.question = 1;
  h_head = 0;
  h_tail = h_point = 1;
  keypad(stdscr, TRUE);
  meta(stdscr, TRUE);
}

void clear_variable() {
  int i;

  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table) player[i].in_table = 0;
  }
  for (i = 1; i <= 4; i++) table[i] = 0;
  player_in_table = 0;
}

/*
 * 客戶端主迴圈 (Main Loop)
 * 處理連線、輸入、遊戲流程控制。
 */
void gps() {
  int status;
  int i;
  int key;
  int msg_id;
  int userfd;
  char msg_buf[255];
  char buf[128];
  char ans_buf[255];
  cJSON *payload;

  init_global_screen();
  input_mode = 0;
  snprintf(msg_buf, sizeof(msg_buf), "連往 QKMJ Server %s %d", GPS_IP,
           GPS_PORT);
  display_comment(msg_buf);
  status = init_socket(GPS_IP, GPS_PORT, &gps_sockfd);
  if (status < 0) {
    endwin();
    fprintf(stderr, "無法連往 QKMJ Server %s:%d\n", GPS_IP, GPS_PORT);
    exit(1);
  }
  snprintf(msg_buf, sizeof(msg_buf), 
           "歡迎來到 QKMJ 休閑麻將 Ver %c.%2s AI (Build: %s)", QKMJ_VERSION[0],
           QKMJ_VERSION + 1, GIT_HASH);
  display_comment(msg_buf);
  display_comment("原作者 sywu (吳先祐 Shian-Yow Wu) ");
  display_comment("Forked Source: https://github.com/edwardchuang/qkmj");
  display_local_ips();
  get_my_info();

  /* 100 MSG_CHECK_VERSION */
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "version", QKMJ_VERSION);
  cJSON_AddStringToObject(payload, "build", GIT_HASH);
  send_json(gps_sockfd, MSG_CHECK_VERSION, payload);

  /* 99 MSG_GET_USERNAME */
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "username", my_username);
  send_json(gps_sockfd, MSG_GET_USERNAME, payload);

  pass_count = 0;
  if (my_name[0] != 0 && my_pass[0] != 0)
    strncpy(ans_buf, (char*)my_name, sizeof(ans_buf) - 1);
  else {
    strncpy(ans_buf, (char*)my_name, sizeof(ans_buf) - 1);
    do {
      ask_question("請輸入你的名字：", ans_buf, 10, 1);
    } while (ans_buf[0] == 0);
    ans_buf[10] = 0;
  }
  ans_buf[sizeof(ans_buf) - 1] = '\0';

  /* 101 MSG_LOGIN */
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "name", ans_buf);
  send_json(gps_sockfd, MSG_LOGIN, payload);

  strncpy((char*)my_name, ans_buf, sizeof(my_name) - 1);
  my_name[sizeof(my_name) - 1] = '\0';
  nfds = getdtablesize();

  FD_ZERO(&afds);
  FD_SET(gps_sockfd, &afds);
  FD_SET(0, &afds);
  // nfds = gps_sockfd + 1;

  for (;;) {
    memmove((char*)&rfds, (char*)&afds, sizeof(rfds));

    if (table_sockfd >= nfds) {
      nfds = table_sockfd + 1;
    }
    if (serv_sockfd >= nfds) {
      nfds = serv_sockfd + 1;
    }

    struct timeval ai_tv;
    struct timeval *tv_ptr = 0;

    if (ai_is_enabled()) {
        ai_tv.tv_sec = 0;
        ai_tv.tv_usec = 100000; // 0.1s timeout for AI polling
        tv_ptr = &ai_tv;
    }

    if (select(nfds, &rfds, (fd_set*)0, (fd_set*)0, tv_ptr) < 0) {
      if (errno != EINTR) {
        endwin();
        fprintf(stderr, "Select Error: %s\n", strerror(errno));
        exit(1);
      }
      continue;
    }
    if (FD_ISSET(0, &rfds)) {
      if (input_mode) process_key();
    }
    
    /* AI Hook: Auto-Draw */
    if (input_mode == PLAY_MODE && play_mode == GET_CARD && ai_is_enabled()) {
        action_draw_card();
    }

    /* AI Hook: Check Mode (Self-Draw Win/Kang) */
    if (input_mode == CHECK_MODE && in_check[1] && ai_is_enabled()) {
        ai_decision_t dec = ai_get_decision(AI_PHASE_DISCARD, current_card, 0);
        if (dec.action == AI_ACTION_WIN) {
             write_check(4);
             input_mode = PLAY_MODE;
             return_cursor();
        } else if (dec.action == AI_ACTION_KANG) {
             write_check(3);
             input_mode = PLAY_MODE;
             return_cursor();
        } else {
             write_check(0);
             input_mode = PLAY_MODE;
             return_cursor();
        }
    }
    
    /* AI Hook: Discard Phase */
    if (input_mode == PLAY_MODE && play_mode == THROW_CARD && ai_is_enabled()) {
        ai_decision_t dec = ai_get_decision(AI_PHASE_DISCARD, current_card, 0);
        if (dec.action == AI_ACTION_DISCARD) {
            int idx = -1;
            // Search in hand (cards 0 to num-1)
            for(i=0; i<pool[my_sit].num; i++) {
                if(pool[my_sit].card[i] == dec.card) { 
                    idx = i; 
                    break; 
                }
            }
            // If not found, check if it's the new card (which might be at index `num` or `current_item`?)
            // In qkmj, when we get a card, it is at `pool[my_sit].card[pool[my_sit].num]`.
            // But `pool[my_sit].num` is usually 16. The 17th card is at index 16.
            // `process_new_card` puts it there.
            if (idx == -1 && pool[my_sit].card[pool[my_sit].num] == dec.card) {
                idx = pool[my_sit].num;
            }

            if (idx != -1) {
                current_item = idx;
                action_throw_card(idx);
            }
        }
    }

    /* Check for data from GPS */
    if (FD_ISSET(gps_sockfd, &rfds)) {
      int msg_id_val = 0;
      cJSON *data = NULL;
      if (!recv_json(gps_sockfd, &msg_id_val, &data)) {
        shutdown(gps_sockfd, 2);
        if (in_join) close_join();
        if (in_serv) close_serv();
        endwin();
        fprintf(stderr, "Closed by QKMJ Server.\n");
        exit(0);
      } else {
        process_msg(0, msg_id_val, data, FROM_GPS);
        if (data) cJSON_Delete(data);
      }
    }
    if (in_serv) {
      /* Check for new connections */
      if (FD_ISSET(serv_sockfd, &rfds)) {
        if (new_client) {
          accept_new_client();
        }
      }
      /* Check for data from client */
      for (i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table) {
          if (FD_ISSET(player[i].sockfd, &rfds)) {
            int msg_id_val = 0;
            cJSON *data = NULL;
            if (!recv_json(player[i].sockfd, &msg_id_val, &data)) {
              close_client(i);
              if (data) cJSON_Delete(data);
            } else {
              process_msg(i, msg_id_val, data, FROM_CLIENT);
              if (data) cJSON_Delete(data);
            }
          }
        }
      }
      /* Waiting for signals from each sit */
      if (waiting) {
        for (i = 1; i <= 4; i++) {
          if (table[i] && !wait_hit[i]) goto continue_waiting;
        }
        waiting = 0;
        /* Broadcast 290 MSG_NEW_ROUND */
        for (i = 2; i < MAX_PLAYER; i++) {
            if (player[i].in_table && i != 1) {
                send_json(player[i].sockfd, MSG_NEW_ROUND, NULL);
            }
        }
        opening();
        open_deal();
      }
    continue_waiting:;
      /* Process the cards */
      if (check_on) {
        /* find if there are still player in check */
        for (i = 1; i <= 4; i++) {
          if (table[i] && in_check[i]) goto still_in_check;
        }
        check_on = 0;
        next_player_on = 1;
        send_card_on = 1;
        compare_check();
      still_in_check:;
      }
      if (next_player_request && next_player_on) {
        if (144 - card_point <= 16) {
          for (i = 1; i <= 4; i++) {
            if (table[i] && i != my_sit) {
              show_allcard(i);
              show_kang(i);
            }
          }
          info.cont_dealer++;
          send_pool_card();
          /* Broadcast 330 MSG_SEA_BOTTOM */
          for (i = 2; i < MAX_PLAYER; i++) {
             if (player[i].in_table && i != 1) send_json(player[i].sockfd, MSG_SEA_BOTTOM, NULL);
          }
          clear_screen_area(THROW_Y, THROW_X, 8, 34);
          wmvaddstr(stdscr, THROW_Y + 3, THROW_X + 12, "海 底 流 局");
          return_cursor();
          wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
          /* Broadcast 290 MSG_NEW_ROUND */
          for (i = 2; i < MAX_PLAYER; i++) {
             if (player[i].in_table && i != 1) send_json(player[i].sockfd, MSG_NEW_ROUND, NULL);
          }
          opening();
          open_deal();
        } else {
          next_player();
          next_player_request = 0;
        }
      }
      if (send_card_request && send_card_on) {
        send_one_card(table[turn]);
        send_card_request = 0;
      }
    }
    if (in_join) {
      if (FD_ISSET(table_sockfd, &rfds)) {
        int msg_id_val = 0;
        cJSON *data = NULL;
        if (!recv_json(table_sockfd, &msg_id_val, &data)) {
          close(table_sockfd);
          FD_CLR(table_sockfd, &afds);
          in_join = 0;
          input_mode = TALK_MODE;
          init_global_screen();
          if (data) cJSON_Delete(data);
        } else {
          process_msg(1, msg_id_val, data, FROM_SERV);
          if (data) cJSON_Delete(data);
        }
      }
    }
  }
}

void read_qkmjrc() {
  FILE* qkmjrc_fp;
  char msg_buf[256];
  char msg_buf1[256];
  char event[80];
  char* str1;
  char rc_name[255];

  snprintf(rc_name, sizeof(rc_name), "%s/%s", getenv("HOME"), QKMJRC);
  if ((qkmjrc_fp = fopen(rc_name, "r")) != NULL) {
    while (fgets(msg_buf, 80, qkmjrc_fp) != NULL) {
      Tokenize(msg_buf);
      my_strupr(event, (char*)cmd_argv[1]);
      if (strcmp(event, "LOGIN") == 0) {
        if (narg > 1) {
          cmd_argv[2][10] = 0;
          strncpy((char*)my_name, (char*)cmd_argv[2], sizeof(my_name) - 1);
          my_name[sizeof(my_name) - 1] = '\0';
        }
      } else if (strcmp(event, "PASSWORD") == 0) {
        if (narg > 1) {
          cmd_argv[2][8] = 0;
          strncpy((char*)my_pass, (char*)cmd_argv[2], sizeof(my_pass) - 1);
          my_pass[sizeof(my_pass) - 1] = '\0';
        }
      } else if (strcmp(event, "SERVER") == 0) {
        if (narg > 1) {
          strncpy(GPS_IP, (char*)cmd_argv[2], sizeof(GPS_IP) - 1);
          GPS_IP[sizeof(GPS_IP) - 1] = '\0';
        }
        if (narg > 2) {
          GPS_PORT = atoi((char*)cmd_argv[3]);
        }
      } else if (strcmp(event, "NOTE") == 0) {
        if (narg > 1) {
          str1 = strtok(msg_buf, " \n\t\r");
          str1 = strtok(NULL, "\n\t\r");
          strncpy((char*)my_note, str1, sizeof(my_note) - 1);
          my_note[sizeof(my_note) - 1] = '\0';
        }
      } else if (strcmp(event, "BEEP") == 0) {
        if (narg > 1) {
          my_strupr(msg_buf1, (char*)cmd_argv[2]);
          if (strcmp(msg_buf1, "OFF") == 0) {
            set_beep = 0;
          }
        }
      }
    }
    fclose(qkmjrc_fp);
  }
}

int main(int argc, char** argv) {
#ifdef DEBUG
  printf("Ready for debugger. PID: %d\n", getpid());
  printf("Press 'g' and Enter to start the game...\n");
  while (getchar() != 'g');
#endif

  setlocale(LC_ALL, "");
  setenv("TERM", "xterm", 1);

  /* init curses */
  initscr();
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  /* init curses end */
  cbreak();
  noecho();
  nonl();
  clear();
#ifdef SIGIOT
  signal(SIGINT, (void (*)(int))leave);
  signal(SIGIOT, (void (*)(int))leave);
  signal(SIGPIPE, (void (*)(int))leave);
#endif
  ai_init();
  init_variable();
  strncpy(GPS_IP, DEFAULT_GPS_IP, sizeof(GPS_IP) - 1);
  GPS_IP[sizeof(GPS_IP) - 1] = '\0';
  GPS_PORT = DEFAULT_GPS_PORT;
  read_qkmjrc();
  if (argc >= 3) {
    strncpy(GPS_IP, argv[1], sizeof(GPS_IP) - 1);
    GPS_IP[sizeof(GPS_IP) - 1] = '\0';
    GPS_PORT = atoi(argv[2]);
  } else if (argc == 2) {
    strncpy(GPS_IP, argv[1], sizeof(GPS_IP) - 1);
    GPS_IP[sizeof(GPS_IP) - 1] = '\0';
    GPS_PORT = DEFAULT_GPS_PORT;
  }
  gps();
  ai_cleanup();
  exit(0);
}
