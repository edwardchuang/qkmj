#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <unistd.h>
#include <locale.h>

#include "qkmj.h"

/*gloable variables*/
fd_set rfds,afds;
int nfds;
extern int errno;
//extern char *sys_errlist[];
char GPS_IP[255];
int GPS_PORT;
char my_username[20];
char my_address[70];
int PLAYER_NUM=4;
char QKMJ_VERSION[]="094";
char menu_item[25][5]
={"  ","A ","B ","C ","D ","E ","F ","G ","H ","I ","J ",
  "K ","L ","M ","N ","O ","P ","Q ","R ","  "};
char number_item[30][3]
={"０","１","２","３","４","５","６","７","８","９","10","11","12","13",
  "14","15","16","17","18","19","20"};
char mj_item[100][6]
={"＊＊","一萬","二萬","三萬","四萬","五萬","六萬","七萬","八萬","九萬",
  "摸牌","一索","二索","三索","四索","五索","六索","七索","八索","九索",
  "    ","一筒","二筒","三筒","四筒","五筒","六筒","七筒","八筒","九筒",
  "▉▉","東風","南風","西風","北風","    ","    ","    ","    ","    ",
  "▇▇","紅中","白板","青發","    ","    ","    ","    ","    ","    ",
  "    ","春１","夏２","秋３","冬４","梅１","蘭２","菊３","竹４"
 };
struct tai_type tai[100]={
  {"莊家",1,0},
  {"門清",1,0},
  {"自摸",1,0},
  {"斷么九",1,0},
  {"一杯口",1,0},
  {"杠上開花",1,0},
  {"海底摸月",1,0},
  {"河底撈魚",1,0},
  {"搶槓",1,0},
  {"東風",1,0},
  {"南風",1,0},
  {"西風",1,0},
  {"北風",1,0},
  {"紅中",1,0},
  {"白板",1,0},
  {"青發",1,0},
  {"花牌",1,0},
  {"東風東",2,0},
  {"南風南",2,0},
  {"西風西",2,0},
  {"北風北",2,0},
  {"春夏秋冬",2,0},
  {"梅蘭菊竹",2,0},
  {"全求人",2,0},
  {"平胡",2,0},
  {"混全帶么",2,0},
  {"三色同順",2,0},
  {"一條龍",2,0},
  {"二杯口",2,0},
  {"三暗刻",2,0},
  {"三槓子",2,0},
  {"三色同刻",2,0},
  {"門清自摸",3,0},
  {"碰碰胡",4,0},
  {"混一色",4,0},
  {"純全帶么",4,0},
  {"混老頭",4,0},
  {"小三元",4,0},
  {"四暗刻",6,0},
  {"四槓子",6,0},
  {"大三元",8,0},
  {"小四喜",8,0},
  {"清一色",8,0},
  {"字一色",8,0},
  {"七搶一",8,0},
  {"五暗刻",8,0},
  {"清老頭",8,0},
  {"大四喜",16,0},
  {"八仙過海",16,0},
  {"天胡",16,0},
  {"地胡",16,0},
  {"人胡",16,0},
  {"連  拉  ",2,0}
  };
struct card_comb_type card_comb[30];
int comb_num;
char mj[150];
char sit_name[5][3]
={"  ","東","南","西","北"};
char check_name[7][3]
={"無","吃","碰","槓","胡","聽"};

int SERV_PORT=DEFAULT_SERV_PORT;
int set_beep;
int pass_login;
int pass_count;
int current_item;
int card_in_pool[5];
int pos_x, pos_y;
int current_check;
int check_number;
int check_x,check_y;
int eat_x,eat_y;
int card_count=0;
int card_point;
int card_index;
int gps_sockfd,serv_sockfd = -1,table_sockfd = -1;
int in_serv,in_join;
char talk_buf[255]="\0";
int talk_buf_count=0;
char history[HISTORY_SIZE+1][255];
int h_head,h_tail,h_point;
int talk_x,talk_y;
int talk_left,talk_right;
int comment_x,comment_y;
int comment_left, comment_right, comment_bottom, comment_up;
char comment_lines[24][255];
int talk_mode;
int play_mode;
int screen_mode;
unsigned char key_buf[255];
char wait_hit[5];
int waiting;
unsigned char *str;
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
struct ask_mode_info ask;
struct player_info player[MAX_PLAYER];
struct pool_info pool[5];
struct table_info info;
struct timeval before,after;
int table[5];
int new_client;
char new_client_name[30];
long new_client_money;
unsigned int new_client_id;
int player_num;
WINDOW *commentwin, *inputwin,*global_win,*playing_win;
int turn;
int card_owner;
int in_kang;
int current_id;
int current_card;
int in_play;
int on_seat;
int player_in_table;
int check_flag[5][8],check_on,in_check[6],check_for[6];
int go_to_check;
int send_card_on;
int send_card_request;
int getting_card;
int next_player_on;
int next_player_request;
int color=1;
int cheat_mode=0;
char table_card[6][17];

/* Request a card */
int request_card() {
  // 檢查是否已加入遊戲
  if (in_join) {
    // 向伺服器發送請求牌訊息
    write_msg(table_sockfd, "313");
    return 0;
  } else {
    // 從牌堆中取得下一張牌
    return mj[card_point++];
  }
}

/* Change a card */
void change_card(char position, char card) {
  // 計算牌池中牌的位置
  int i = (16 - pool[my_sit].num + position) * 2;
  if (position == pool[my_sit].num) {
    i++;
  }
  // 更換牌池中的牌
  pool[my_sit].card[position] = card;
  // 顯示更換前的牌
  show_card(20, INDEX_X + i, INDEX_Y + 1, 1);
  wrefresh(stdscr);
  // 顯示更換後的牌
  show_card(card, INDEX_X + i, INDEX_Y + 1, 1);
  wrefresh(stdscr);
}

/* Get a card */
void get_card(char card) {
  // 將新取得的牌放入牌池
  pool[my_sit].card[pool[my_sit].num] = card;
  // 顯示取得前的牌
  show_card(20, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
  wrefresh(stdscr);
  // 顯示取得後的牌
  show_card(card, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
  wrefresh(stdscr);
}

void process_new_card(char sit, char card) {
  char msg_buf[255];

  // 更新當前牌
  current_card = card;
  // 顯示新牌訊息
  show_cardmsg(sit, 0);
  // 將新牌放入牌池
  pool[sit].card[pool[sit].num] = card;
  // 顯示新牌
  get_card(card);
  // 回復游標位置
  return_cursor();
  // 檢查是否為花牌
  if (!check_flower(sit, card)) {
    // 設定遊戲模式為出牌
    play_mode = THROW_CARD;
  }
}

/* Throw cards to the table */
void throw_card(char card) {
  int x, y;
  // 處理特殊牌 (20)
  if (card == 20) {
    card_count--;
  }
  // 將牌放入桌面牌陣列
  table_card[card_count / 17][card_count % 17] = card;
  // 計算牌在桌面上的位置
  x = THROW_X + (card_count % 17) * 2;
  y = THROW_Y + card_count / 17 * 2;
  // 顯示牌
  if (y % 4 == 3) {
    if (!color) {
      attron(A_BOLD);
    }
    show_card(card, x, y, 1);
    if (!color) {
      attroff(A_BOLD);
    }
  } else {
    show_card(card, x, y, 1);
  }
  // 更新牌計數
  if (card != 20) {
    card_count++;
  }
}

void send_one_card(int id) {
  char msg_buf[255];
  char card;
  char sit;
  int i;

  // 取得玩家座位號
  sit = player[id].sit;
  // 取得下一張牌
  card = mj[card_point++];
  // 更新當前牌
  current_card = card;
  // 將牌放入牌池
  pool[sit].card[pool[sit].num] = card;
  // 顯示牌堆剩餘牌數
  show_num(2, 70, 144 - card_point - 16, 2);
  // 通知所有玩家發牌訊息
  snprintf(msg_buf, sizeof(msg_buf), "314%c%c", sit, 2);
  broadcast_msg(id, msg_buf);
  // 顯示新牌
  show_newcard(sit, 2);
  // 回復游標位置
  return_cursor();
  // 通知所有玩家牌堆剩餘牌數
  snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point);
  broadcast_msg(1, msg_buf);
  // 顯示發牌訊息
  show_cardmsg(sit, 0);
  // 更新當前出牌者
  card_owner = sit;
  // 通知所有玩家當前出牌者
  snprintf(msg_buf, sizeof(msg_buf), "305%c", (char)sit);
  broadcast_msg(1, msg_buf);
  // 清除檢查標記
  clear_check_flag(sit);
  // 檢查是否可以槓
  check_flag[sit][3] = check_kang(sit, card);
  // 檢查是否可以胡
  check_flag[sit][4] = check_make(sit, card, 0);
  // 重置檢查狀態
  in_check[sit] = 0;
  // 檢查是否有可執行的動作
  for (i = 1; i < check_number; i++) {
    if (check_flag[sit][i]) {
      // 設定取得牌狀態
      getting_card = 1;
      // 設定檢查狀態
      in_check[sit] = 1;
      // 設定檢查模式
      check_on = 1;
      // 通知玩家可執行的動作
      snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c", check_flag[sit][1] + '0',
               check_flag[sit][2] + '0', check_flag[sit][3] + '0',
               check_flag[sit][4] + '0');
      write_msg(player[table[sit]].sockfd, msg_buf);
      break;
    }
  }
  // 通知玩家發牌
  snprintf(msg_buf, sizeof(msg_buf), "304%c", card);
  write_msg(player[id].sockfd, msg_buf);
  // 記錄發牌時間
  gettimeofday(&before, (struct timezone *)0);
}

// 下一位玩家
void next_player() {
  char msg_buf[255];
  // 取得下一輪的玩家
  turn = next_turn(turn);
  // 通知所有玩家輪到下一位玩家
  snprintf(msg_buf, sizeof(msg_buf), "310%c", turn);
  broadcast_msg(1, msg_buf);
  // 顯示當前玩家的計分
  display_point(turn);
  // 回復游標位置
  return_cursor();
  // 檢查是否為伺服器
  if (table[turn] != 1) {
    // 通知玩家可以出牌
    strncpy(msg_buf, "303", sizeof(msg_buf));
    write_msg(player[table[turn]].sockfd, msg_buf);
    // 顯示新牌
    show_newcard(turn, 1);
    // 回復游標位置
    return_cursor();
  } else {
    // 顯示伺服器取得牌的提示
    attron(A_REVERSE);
    show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
    attroff(A_REVERSE);
    wrefresh(stdscr);
    // 回復游標位置
    return_cursor();
    // 設定遊戲模式為取得牌
    play_mode = GET_CARD;
    // 播放提示音
    beep1();
  }
  // 通知玩家發牌
  snprintf(msg_buf, sizeof(msg_buf), "314%c%c", turn, 1);
  broadcast_msg(table[turn], msg_buf);
}

// 取得下一輪的玩家
int next_turn(int current_turn) {
  // 計算下一輪的玩家
  current_turn++;
  if (current_turn == 5) {
    current_turn = 1;
  }
  // 檢查下一輪的玩家是否在遊戲中
  if (!table[current_turn]) {
    current_turn = next_turn(current_turn);
  }
  return current_turn;
}

// 顯示牌池
void display_pool(int sit) {
  char msg_buf[255];
  // 初始化訊息緩衝區
  msg_buf[0] = 0;
  // 遍歷牌池中的所有牌
  for (int i = 0; i < pool[sit].num; i++) {
    char buf[5];
    // 將牌的數字轉換為字串
    snprintf(buf, sizeof(buf), "%2d", pool[sit].card[i]);
    // 將牌的數字字串附加到訊息緩衝區
    memmove(msg_buf + strlen(msg_buf), buf, strlen(buf));
  }
  // 顯示牌池的內容
  display_comment(msg_buf);
}

// 排序牌池
void sort_pool(int sit) {
  // 遍歷牌池中的所有牌
  for (int i = 0; i < pool[sit].num; i++) {
    // 遍歷牌池中尚未排序的牌
    for (int j = 0; j < pool[sit].num - i - 1; j++) {
      // 檢查是否需要交換牌
      if (pool[sit].card[j] > pool[sit].card[j + 1]) {
        // 交換牌
        char tmp = pool[sit].card[j];
        pool[sit].card[j] = pool[sit].card[j + 1];
        pool[sit].card[j + 1] = tmp;
      }
    }
  }
}

// 排序牌
void sort_card(int mode) {
  int i, j;
  // 計算排序的牌數
  int max = mode ? pool[my_sit].num + 1 : pool[my_sit].num;
  // 遍歷牌池中的所有牌
  for (i = 0; i < max; i++) {
    // 遍歷牌池中尚未排序的牌
    for (j = 0; j < max - i - 1; j++) {
      // 檢查是否需要交換牌
      if (pool[my_sit].card[j] > pool[my_sit].card[j + 1]) {
        // 交換牌
        char tmp = pool[my_sit].card[j];
        pool[my_sit].card[j] = pool[my_sit].card[j + 1];
        pool[my_sit].card[j + 1] = tmp;
      }
    }
  }
  // 顯示排序後的牌
  for (int i = 0; i < pool[my_sit].num; i++) {
    show_card(20, INDEX_X + (16 - pool[my_sit].num + i) * 2, INDEX_Y + 1, 1);
    wrefresh(stdscr);
    show_card(pool[my_sit].card[i], INDEX_X + (16 - pool[my_sit].num + i) * 2,
              INDEX_Y + 1, 1);
    wrefresh(stdscr);
  }
  // 顯示最後一張牌
  if (mode) {
    show_card(20, INDEX_X + (16 - pool[my_sit].num + i) * 2 + 1,
              INDEX_Y + 1, 1);
    wrefresh(stdscr);
    show_card(pool[my_sit].card[pool[my_sit].num],
              INDEX_X + (16 - pool[my_sit].num + i) * 2 + 1, INDEX_Y + 1, 1);
    wrefresh(stdscr);
  }
  // 回復游標位置
  return_cursor();
}

// 開始新遊戲
void new_game() {
  // 清除螢幕區域
  clear_screen_area(0, 2, 18, 54);
  // 顯示發牌訊息
  show_cardmsg(my_sit, 0);
  // 繪製桌子
  draw_table();
  // 更新螢幕
  wrefresh(stdscr);
}

// 開局
void opening() {
  char msg_buf[255];
  // 開始新遊戲
  new_game();
  // 設定輸入模式為遊戲模式
  input_mode = PLAY_MODE;
  // 初始化玩家資訊
  for (int i = 1; i <= 4; i++) {
    // 重置檢查狀態
    in_check[i] = 0;
    // 設定為第一輪
    pool[i].first_round = 1;
    // 重置等待狀態
    wait_hit[i] = 0;
    // 設定牌池數量
    pool[i].num = 16;
    // 重置檢查標記
    check_for[i] = 0;
    // 重置出牌索引
    pool[i].out_card_index = 0;
    // 重置花牌
    for (int j = 0; j < 8; j++) {
      pool[i].flower[j] = 0;
    }
    // 重置時間
    pool[i].time = 0;
  }
  // 顯示遊戲資訊
  display_info();
  // 設定當前項目
  current_item = pool[my_sit].num;
  // 繪製索引
  draw_index(pool[my_sit].num + 1);
  // 清除牌池
  for (int i = 0; i < 16; i++) {
    pool[my_sit].card[i] = 0;
  }
  // 設定游標位置
  pos_x = INDEX_X + current_item * 2 + 1;
  pos_y = INDEX_Y;
  // 設定遊戲模式為 0
  play_mode = 0;
  // 重置牌計數
  card_count = 0;
  // 重置檢查狀態
  check_on = 0;
  // 顯示其他玩家的牌背
  for (int i = 1; i <= 4; i++) {
    if (i != my_sit && table[i]) {
      show_cardback(i);
    }
  }
}

// 開局發牌
void open_deal() {
  char msg_buf[255];
  int i, j, sit;
  char card;

  // 設定莊家
  turn = info.dealer;
  // 隨機決定門風
  i = generate_random(4);
  pool[i % 4 + 1].door_wind = 1;
  pool[(i + 1) % 4 + 1].door_wind = 2;
  pool[(i + 2) % 4 + 1].door_wind = 3;
  pool[(i + 3) % 4 + 1].door_wind = 4;
  // 通知所有玩家門風
  snprintf(msg_buf, sizeof(msg_buf), "518%c%c%c%c", pool[1].door_wind,
           pool[2].door_wind, pool[3].door_wind, pool[4].door_wind);
  broadcast_msg(1, msg_buf);
  // 顯示自己的門風
  wmvaddstr(stdscr, 2, 64, sit_name[pool[my_sit].door_wind]);
  // 檢查莊家是否在遊戲中
  if (!table[turn]) {
    // 取得下一輪的莊家
    turn = next_turn(turn);
  }
  // 設定當前出牌者
  card_owner = turn;
  // 通知所有玩家當前出牌者
  snprintf(msg_buf, sizeof(msg_buf), "310%c", turn);
  broadcast_msg(1, msg_buf);
  snprintf(msg_buf, sizeof(msg_buf), "305%c", card_owner);
  broadcast_msg(1, msg_buf);
  // 顯示當前玩家的計分
  display_point(turn);
  // 重置牌堆索引
  card_point = 0;
  card_index = 143;
  // 設定發牌訊息
  strncpy(msg_buf, "302", sizeof(msg_buf));
  // 生成牌堆
  generate_card();
  // 發 16 張牌給 4 個玩家
  for (j = 1; j <= 4; j++) {
    if (table[j]) {
      // 設定為第一輪
      pool[j].first_round = 1;
      // 發牌
      for (i = 0; i < 16; i++) {
        card = mj[card_point++];
        pool[j].card[i] = card;
        msg_buf[3 + i] = card;
      }
      // 排序牌池
      sort_pool(j);
      // 結束訊息
      msg_buf[3 + 16] = '\0';
      // 檢查是否為伺服器
      if (table[j] != 1) {
        // 發送牌給玩家
        write_msg(player[table[j]].sockfd, msg_buf);
      } else {
        // 排序自己的牌
        sort_card(0);
      }
    }
  }
  // 發一張牌給莊家
  card = mj[card_point++];
  current_card = card;
  snprintf(msg_buf, sizeof(msg_buf), "304%c", card);
  // 檢查是否為伺服器
  if (table[turn] != 1) {
    // 顯示新牌
    show_newcard(turn, 2);
    // 回復游標位置
    return_cursor();
    // 發送牌給玩家
    write_msg(player[table[turn]].sockfd, msg_buf);
    // 將牌放入牌池
    pool[turn].card[pool[turn].num] = card;
  }
  // 檢查是否為伺服器
  if (table[turn] == 1) {
    // 將牌放入牌池
    pool[1].card[pool[1].num] = card;
    // 顯示新牌
    show_card(pool[1].card[pool[my_sit].num], INDEX_X + 16 * 2 + 1,
              pos_y + 1, 1);
    wrefresh(stdscr);
    // 回復游標位置
    return_cursor();
    // 設定遊戲模式為出牌
    play_mode = THROW_CARD;
  }
  // 通知玩家發牌
  snprintf(msg_buf, sizeof(msg_buf), "314%c%c", turn, 2);
  broadcast_msg(table[turn], msg_buf);
  // 設定遊戲狀態為進行中
  in_play = 1;
  // 檢查所有玩家的花牌
  sit = turn;  // 檢查莊家
  // 檢查莊家的花牌
  do {
  } while (check_begin_flower(sit, pool[sit].card[16], 16));
  // 檢查其他玩家的花牌
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      if (table[sit]) {
        do {
        } while (check_begin_flower(sit, pool[sit].card[j], j));
      }
    }
    // 排序牌池
    sort_pool(sit);
    // 取得下一輪的玩家
    sit = next_turn(sit);
  }
  // 更新當前牌
  current_card = pool[turn].card[16];
  // 顯示牌堆剩餘牌數
  show_num(2, 70, 144 - card_point - 16, 2);
  // 回復游標位置
  return_cursor();
  // 通知所有玩家牌堆剩餘牌數
  snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point);
  broadcast_msg(1, msg_buf);
  // 清除檢查標記
  clear_check_flag(turn);
  // 檢查是否可以槓
  check_flag[turn][3] = check_kang(turn, current_card);
  // 檢查是否可以胡
  check_flag[turn][4] = check_make(turn, current_card, 0);
  // 重置檢查狀態
  in_check[turn] = 0;
  // 檢查是否有可執行的動作
  for (i = 1; i < check_number; i++) {
    if (check_flag[turn][i]) {
      // 設定取得牌狀態
      getting_card = 1;
      // 設定檢查狀態
      in_check[turn] = 1;
      // 設定檢查模式
      check_on = 1;
      // 檢查是否為伺服器
      if (in_serv) {
        // 初始化檢查模式
        init_check_mode();
      } else {
        // 通知玩家可執行的動作
        snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c",
                 check_flag[turn][1] + '0', check_flag[turn][2] + '0',
                 check_flag[turn][3] + '0', check_flag[turn][4] + '0');
        write_msg(player[table[turn]].sockfd, msg_buf);
      }
    }
  }
  /*
   sprintf(msg_buf,"304%c",card);
   if(table[turn]!=1)
   {
     show_newcard(turn,2);
     return_cursor();
     write_msg(player[table[turn]].sockfd,msg_buf);
     pool[turn].card[pool[turn].num]=card;
   }
  */
  // 通知玩家發牌
  broadcast_msg(table[turn], "3080");
  // 檢查是否為自己
  if (turn != my_sit) {
    // 通知玩家發牌
    write_msg(player[table[turn]].sockfd, "3081");
  }
  // 檢查是否為自己
  if (turn == my_sit) {
    // 排序自己的牌
    sort_card(1);
  } else {
    // 排序其他玩家的牌
    sort_card(0);
  }
  // 記錄發牌時間
  gettimeofday(&before, (struct timezone *)0);
}

// 錯誤處理函數
void err(char *errmsg) {
  // 顯示錯誤訊息
  display_comment(errmsg);
}

// 初始化變數
void init_variable() {
  int i, j;

  // 初始化玩家名稱
  my_name[0] = 0;
  // 初始化玩家密碼
  my_pass[0] = 0;
  // 重置登入狀態
  pass_login = 0;
  // 設定提示音
  set_beep = 1;
  // 重置遊戲狀態
  in_play = 0;
  // 重置伺服器狀態
  in_serv = 0;
  // 重置加入遊戲狀態
  in_join = 0;
  // 重置槓牌狀態
  in_kang = 0;
  // 重置新客戶端狀態
  new_client = 0;
  // 重置玩家數量
  player_num = 0;
  // 初始化檢查區域座標
  check_x = org_check_x;
  check_y = org_check_y;
  // 設定檢查選項數量
  check_number = 5;
  // 設定輸入模式為詢問模式
  input_mode = ASK_MODE;
  // 初始化遊戲資訊
  info.wind = 1;
  info.dealer = 1;
  info.cont_dealer = 0;
  info.base_value = DEFAULT_BASE;
  info.tai_value = DEFAULT_TAI;
  // 初始化玩家座位
  for (i = 0; i < 5; i++) {
    table[i] = 0;
  }
  // 初始化玩家金錢
  player[0].money = 0;
  // 重置座位狀態
  on_seat = 0;
  // 重置檢查狀態
  check_on = 0;
  // 重置發牌狀態
  send_card_on = 0;
  send_card_request = 0;
  next_player_on = 0;
  // 初始化全局視窗
  global_win = newwin(1, 63, org_talk_y, 11);
  // 初始化遊戲視窗
  playing_win = newwin(1, 43, org_talk_y, 11);
  // 初始化詢問模式資訊
  ask.question = 1;
  // 初始化歷史訊息索引
  h_head = 0;
  h_tail = h_point = 1;
  // 設定鍵盤模式
  keypad(stdscr, TRUE);
  // 設定元字符模式
  meta(stdscr, TRUE);
}

// 清除變數
void clear_variable() {
  int i;

  // 清除玩家在桌子上的狀態
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table) {
      player[i].in_table = 0;
    }
  }
  // 清除玩家座位
  for (i = 1; i <= 4; i++) {
    table[i] = 0;
  }
  // 重置桌子上的玩家數量
  player_in_table = 0;
}

// GPS 連線函數
void gps() {
  int status;
  int i;
  int key;
  int msg_id;
  int userfd;
  char msg_buf[1024];
  char buf[128];
  char ans_buf[255];

  // 初始化全局螢幕
  init_global_screen();
  // 設定輸入模式為 0
  input_mode = 0;
  // 顯示連線訊息
  snprintf(msg_buf, sizeof(msg_buf), "連往 QKMJ Server %s %d", GPS_IP,
           GPS_PORT);
  //snprintf(msg_buf,"連往 QKMJ Server 中 ");
  display_comment(msg_buf);
  // 初始化 GPS 連線
  status = init_socket(GPS_IP, GPS_PORT, &gps_sockfd);
  if (status < 0) {
    // 顯示連線錯誤訊息
    err("無法連往 QKMJ Server");
    endwin();
    fprintf(stderr, "無法連往 QKMJ Server QKMJ Server: %s port %d\n", GPS_IP, GPS_PORT);
    exit(0);
  }
  //send_gps_line("已連上");
  // 顯示歡迎訊息
  snprintf(msg_buf, sizeof(msg_buf),
           "歡迎來到 QKMJ 休閑麻將 Ver %c.%2s 特別板 ", QKMJ_VERSION[0],
           QKMJ_VERSION + 1);
  display_comment(msg_buf);
  // 顯示作者資訊
  display_comment("原作者 sywu (吳先祐 Shian-Yow Wu) ");
  //display_comment("本版本由  TonyQ (tonylovejava@gmail.com / TonyQ@ptt.cc ) 維護 ");
  // 顯示原始碼位置
  display_comment("Forked Source: https://github.com/gjchentw/qkmj");
  //display_comment("可以用^C退出");
  // 取得玩家資訊
  get_my_info();
  // 發送版本訊息
  snprintf(msg_buf, sizeof(msg_buf), "100%s", QKMJ_VERSION);
  write_msg(gps_sockfd, msg_buf);
  // 發送玩家名稱訊息
  snprintf(msg_buf, sizeof(msg_buf), "099%s", my_username);
  write_msg(gps_sockfd, msg_buf);
  // 重置密碼輸入次數
  pass_count = 0;
  // 檢查是否已儲存玩家名稱和密碼
  if (my_name[0] != 0 && my_pass[0] != 0) {
    // 使用儲存的玩家名稱
    strncpy(ans_buf, (char *)my_name, sizeof(ans_buf));
  } else {
    // 輸入玩家名稱
    strncpy(ans_buf, (char *)my_name, sizeof(ans_buf));
    do {
      ask_question("請輸入你的名字：", ans_buf, 10, 1);
    } while (ans_buf[0] == 0);
    ans_buf[10] = 0;
  }
  // 發送玩家名稱訊息
  snprintf(msg_buf, sizeof(msg_buf), "101%s", ans_buf);
  write_msg(gps_sockfd, msg_buf);
  // 更新玩家名稱
  strncpy((char *)my_name, ans_buf, sizeof(my_name));
  // 取得檔案描述符數量
  nfds = getdtablesize();

  // 初始化檔案描述符集合
  FD_ZERO(&afds);
  FD_SET(gps_sockfd, &afds);
  FD_SET(0, &afds);
  // nfds = gps_sockfd + 1;

  // 主迴圈
  for (;;) {
    // 複製檔案描述符集合
    bcopy((char *)&afds, (char *)&rfds, sizeof(rfds));

    // 更新檔案描述符數量
    if (table_sockfd >= nfds) {
      nfds = table_sockfd + 1;
    }
    if (serv_sockfd >= nfds) {
      nfds = serv_sockfd + 1;
    }

    // 檢查是否有資料可讀取
    if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, 0) < 0) {
      // 檢查錯誤是否為中斷訊號
      if (errno != EINTR) {
        // 顯示錯誤訊息
        display_comment("Select Error!");
        exit(0);
      }
      continue;
    }
    // 檢查標準輸入是否有資料
    if (FD_ISSET(0, &rfds)) {
      // 處理鍵盤輸入
      if (input_mode) {
        process_key();
      }
    }
    // 檢查 GPS 連線是否有資料
    if (FD_ISSET(gps_sockfd, &rfds)) {
      // 讀取 GPS 訊息
      if (!read_msg_id(gps_sockfd, buf)) {
        // 顯示連線中斷訊息
        display_comment("Closed by QKMJ Server.");
        shutdown(gps_sockfd, 2);
        // 檢查是否已加入遊戲
        if (in_join) {
          close_join();
        }
        // 檢查是否為伺服器
        if (in_serv) {
          close_serv();
        }
        endwin();
        exit(0);
      } else {
        // 處理 GPS 訊息
        process_msg(0, (unsigned char *)buf, FROM_GPS);
        buf[0] = '\0';
      }
    }
    // 檢查是否為伺服器
    if (in_serv) {
      // 檢查是否有新的連線
      if (FD_ISSET(serv_sockfd, &rfds)) {
        // 檢查是否有新的客戶端
        if (new_client) {
          accept_new_client();
        }
        /*
          else
            display_comment("Error from new client!");
        */
      }
      // 檢查客戶端是否有資料
      for (i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table) {
          // 檢查客戶端是否有資料
          if (FD_ISSET(player[i].sockfd, &rfds)) {
            // 讀取客戶端訊息
            if (read_msg_id(player[i].sockfd, buf) == 0) {
              // 關閉客戶端連線
              close_client(i);
            } else {
              // 處理客戶端訊息
              process_msg(i, (unsigned char *)buf, FROM_CLIENT);
            }
          }
        }
      }
      // 等待所有玩家的訊號
      if (waiting) {
        // 檢查所有玩家是否都已發送訊號
        for (i = 1; i <= 4; i++) {
          if (table[i] && !wait_hit[i]) {
            goto continue_waiting;
          }
        }
        // 重置等待狀態
        waiting = 0;
        // 通知所有玩家開始新遊戲
        broadcast_msg(1, "290");
        // 開局
        opening();
        // 發牌
        open_deal();
      }
    continue_waiting:;
      // 處理牌
      if (check_on) {
        // 檢查是否有玩家正在檢查
        for (i = 1; i <= 4; i++) {
          if (table[i] && in_check[i]) {
            goto still_in_check;
          }
        }
        // 重置檢查狀態
        check_on = 0;
        // 設定下一位玩家狀態
        next_player_on = 1;
        // 設定發牌狀態
        send_card_on = 1;
        // 比較檢查結果
        compare_check();
      still_in_check:;
      }
      // 檢查是否需要發牌
      if (next_player_request && next_player_on) {
        // 檢查牌堆是否還有牌
        if (144 - card_point <= 16) {
          // 顯示所有玩家的牌
          for (i = 1; i <= 4; i++) {
            if (table[i] && i != my_sit) {
              show_allcard(i);
              show_kang(i);
            }
          }
          // 更新連續莊家次數
          info.cont_dealer++;
          // 發送牌池中的牌
          send_pool_card();
          // 通知所有玩家流局
          broadcast_msg(1, "330");
          // 清除桌面區域
          clear_screen_area(THROW_Y, THROW_X, 8, 34);
          // 顯示流局訊息
          wmvaddstr(stdscr, THROW_Y + 3, THROW_X + 12, "海 底 流 局");
          // 回復游標位置
          return_cursor();
          // 等待玩家按下任意鍵
          wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
          // 通知所有玩家開始新遊戲
          broadcast_msg(1, "290");
          // 開局
          opening();
          // 發牌
          open_deal();
        } else {
          // 切換到下一位玩家
          next_player();
          // 重置發牌請求狀態
          next_player_request = 0;
        }
      }
      // 檢查是否需要發牌
      if (send_card_request && send_card_on) {
        // 發牌給當前玩家
        send_one_card(table[turn]);
        // 重置發牌請求狀態
        send_card_request = 0;
      }
    }
    // 檢查是否已加入遊戲
    if (in_join) {
      // 檢查伺服器是否有資料
      if (FD_ISSET(table_sockfd, &rfds)) {
        // 讀取伺服器訊息
        if (!read_msg_id(table_sockfd, buf)) {
          // 關閉伺服器連線
          close(table_sockfd);
          // 從檔案描述符集合中移除伺服器連線
          FD_CLR(table_sockfd, &afds);
          // 重置加入遊戲狀態
          in_join = 0;
          // 設定輸入模式為聊天模式
          input_mode = TALK_MODE;
          // 初始化全局螢幕
          init_global_screen();
        } else {
          // 處理伺服器訊息
          process_msg(1, (unsigned char *)buf, FROM_SERV);
        }
      }
    }
  }
}

#if 0
void read_qkmjrc()
{
  FILE *qkmjrc_fp;
  char msg_buf[256];
  char msg_buf1[256];
  char event[80];
  char *str1;
  char rc_name[255];

  sprintf(rc_name,"%s/%s",getenv("HOME"),QKMJRC);
  if((qkmjrc_fp=fopen(rc_name,"r"))!=NULL)
  {
    while(fgets(msg_buf,80,qkmjrc_fp)!=NULL)
    {
      Tokenize(msg_buf);
      my_strupr(event,cmd_argv[1]);
      if(strcmp(event,"LOGIN")==0)
      {
        if(narg>1)
        {
          cmd_argv[2][10]=0;
          strcpy(my_name,cmd_argv[2]);
        }
      }
      else if(strcmp(event,"PASSWORD")==0)
      {
        if(narg>1)
        {
          cmd_argv[2][8]=0;
          strcpy(my_pass,cmd_argv[2]);
        }
      }
      else if(strcmp(event,"SERVER")==0)
      {
        if(narg>1)
        {
          strcpy(GPS_IP,cmd_argv[2]);
        }
        if(narg>2)
        {
          GPS_PORT=atoi(cmd_argv[3]);
        }
      }
      else if(strcmp(event,"NOTE")==0)
      {
        if(narg>1)
        {
          str1=strtok(msg_buf," \n\t\r");
          str1=strtok('\0',"\n\t\r");
          strcpy(my_note,str1);
        }
      } 
      else if(strcmp(event,"BEEP")==0)
      {
        if(narg>1)
        {
          my_strupr(msg_buf1, (char *)cmd_argv[2]);
          if(strncmp(msg_buf1,"OFF",3)==0)
          {
            set_beep=0;
          }        }
      }  
    }
    fclose(qkmjrc_fp);
  }
}
#endif

int main(int argc, char *argv[]) {
  // 設定終端機類型為 xterm
  setlocale(LC_ALL, "");
  setenv("TERM", "xterm", 1);

  // 初始化 curses 函式庫
  initscr();
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);

  // 設定 curses 函式庫的行為
  cbreak();
  noecho();
  nonl();

  // 清除螢幕
  clear();

  // 處理中斷訊號
#ifdef SIGIOT
  signal(SIGINT, leave);
  signal(SIGIOT, leave);
  signal(SIGPIPE, leave);
#endif

  // 初始化變數
  init_variable();

  // 設定預設 GPS 伺服器 IP 位址和端口
  strncpy(GPS_IP, DEFAULT_GPS_IP, sizeof(GPS_IP));
  GPS_PORT = DEFAULT_GPS_PORT;

  // 讀取 qkmjrc 檔案
  //read_qkmjrc();

  // 處理命令列參數
  if (argc >= 3) {
    strncpy(GPS_IP, argv[1], sizeof(GPS_IP));
    GPS_PORT = atoi(argv[2]);
  } else if (argc == 2) {
    strncpy(GPS_IP, argv[1], sizeof(GPS_IP));
    GPS_PORT = DEFAULT_GPS_PORT;
  }

  // 連線到 GPS 伺服器
  gps();

  // 結束程式
  exit(0);
}