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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "qkmj.h"
#include "input.h"

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
  {"槓上開花",1,0},
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
unsigned char my_pass[15];
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

#define MAX_GPS_IP_LEN 50
#define MAX_USERNAME_LEN 20
#define MAX_ADDRESS_LEN 70

// 設定檔參數 (使用結構體封裝)
typedef struct {
  char gps_ip[MAX_GPS_IP_LEN];
  int gps_port;
  char username[MAX_USERNAME_LEN];
  char address[MAX_ADDRESS_LEN];
} Config;

static Config config; // 使用 static

//------------------------------------------------------------------------------
// 函式: err
// 目的: 顯示錯誤訊息
// 參數:
//   errmsg: 要顯示的錯誤訊息字串
//------------------------------------------------------------------------------
void err(const char *errmsg) {
  display_comment(errmsg); // 使用 display_comment 函式顯示訊息
}

//------------------------------------------------------------------------------
// 函式: init_variable
// 目的: 初始化全域變數
//------------------------------------------------------------------------------
void init_variable() {
  int i, j;

  my_name[0] = 0;       // 初始化名稱
  my_pass[0] = 0;       // 初始化密碼
  pass_login = 0;        // 重置登入狀態
  set_beep = 1;          // 預設啟用提示音
  in_play = 0;           // 預設不在遊戲中
  in_serv = 0;           // 預設不在伺服器模式
  in_join = 0;           // 預設不在加入遊戲模式
  in_kang = 0;           // 預設不在槓牌模式
  new_client = 0;        // 重置新客戶端標誌
  player_num = 0;        // 重置玩家人數
  check_x = org_check_x;   // 設定檢查視窗 X 座標
  check_y = org_check_y;   // 設定檢查視窗 Y 座標
  check_number = 5;      // 設定檢查選項數量
  input_mode = ASK_MODE; // 設定預設輸入模式
  info.wind = 1;         // 初始化風圈
  info.dealer = 1;       // 初始化莊家
  info.cont_dealer = 0;   // 初始化連莊次數
  info.base_value = DEFAULT_BASE; // 設定底
  info.tai_value = DEFAULT_TAI;   // 設定台

  // 清空牌桌資訊
  for (i = 0; i < 5; i++) {
    table[i] = 0;
  }

  player[0].money = 0; // 初始化玩家金錢

  on_seat = 0;           // 重置座位狀態
  check_on = 0;          // 重置檢查狀態
  send_card_on = 0;      // 重置發牌狀態
  send_card_request = 0; // 重置發牌請求狀態
  next_player_on = 0;    // 重置下一位玩家狀態

  global_win = newwin(1, 63, org_talk_y, 11);   // 建立全域視窗
  playing_win = newwin(1, 43, org_talk_y, 11); // 建立遊戲視窗

  ask.question = 1;    // 初始化詢問模式
  h_head = 0;            // 初始化歷史記錄
  h_tail = h_point = 1; // 初始化歷史記錄指標

  keypad(stdscr, TRUE); // 啟用鍵盤
  meta(stdscr, TRUE);   // 啟用 Meta 鍵
}

//------------------------------------------------------------------------------
// 函式: clear_variable
// 目的: 清除遊戲相關的變數，用於重置遊戲狀態
//------------------------------------------------------------------------------
void clear_variable() {
  int i;

  // 清除所有玩家的遊戲狀態
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table) {
      player[i].in_table = 0; // 將玩家從牌桌上移除
    }
  }

  // 清空牌桌上的座位
  for (i = 1; i <= 4; i++) {
    table[i] = 0; // 將座位設為空位
  }

  player_in_table = 0; // 重置牌桌上的玩家數量
}

//------------------------------------------------------------------------------
// 函式: opening
// 目的: 開始新的一局遊戲前的初始化
//------------------------------------------------------------------------------
void opening() {
  char msg_buf[255];
  int i, j;

  new_game();                // 清除螢幕並繪製桌面
  input_mode = PLAY_MODE;    // 設定輸入模式為遊戲模式

  // 初始化每個玩家的狀態
  for (i = 1; i <= 4; i++) {
    in_check[i] = 0;         // 清除檢查標誌
    pool[i].first_round = 1; // 設定為第一輪
    wait_hit[i] = 0;         // 清除等待標誌
    pool[i].num = 16;        // 設定手牌數量為 16
    check_for[i] = 0;        // 清除檢查目標
    pool[i].out_card_index = 0; // 重置出牌索引

    // 清除花牌
    for (j = 0; j < 8; j++) {
      pool[i].flower[j] = 0;
    }
    pool[i].time = 0;        // 重置時間
  }

  display_info();              // 顯示遊戲資訊
  current_item = pool[my_sit].num; // 設定當前選項
  draw_index(pool[my_sit].num + 1);  // 繪製索引

  // 清空手牌
  for (i = 0; i < 16; i++) {
    pool[my_sit].card[i] = 0;
  }

  pos_x = INDEX_X + current_item * 2 + 1; // 設定游標位置
  pos_y = INDEX_Y;
  play_mode = 0;               // 設定遊戲模式
  card_count = 0;              // 重置牌數
  check_on = 0;                // 清除檢查標誌

  // 顯示其他玩家的牌背
  for (i = 1; i <= 4; i++) {
    if (i != my_sit && table[i]) {
      show_cardback(i);
    }
  }
}

//------------------------------------------------------------------------------
// 函式: open_deal
// 目的: 發牌，開始一局遊戲
//------------------------------------------------------------------------------
void open_deal() {
  char msg_buf[255];
  int i, j, sit;
  char card;

  turn = info.dealer; // 設定輪到哪位玩家
  i = generate_random(4); // 隨機決定門風

  // 設定門風
  pool[i % 4 + 1].door_wind = 1;
  pool[(i + 1) % 4 + 1].door_wind = 2;
  pool[(i + 2) % 4 + 1].door_wind = 3;
  pool[(i + 3) % 4 + 1].door_wind = 4;

  // 發送門風資訊給所有玩家
  snprintf(msg_buf, sizeof(msg_buf), "518%c%c%c%c", pool[1].door_wind,
           pool[2].door_wind, pool[3].door_wind, pool[4].door_wind);
  broadcast_msg(1, msg_buf);

  // 顯示自己的門風
  wmvaddstr(stdscr, 2, 64, sit_name[pool[my_sit].door_wind]);

  // 如果莊家不在牌桌上，則輪到下一位玩家
  if (!table[turn]) {
    turn = next_turn(turn);
  }

  card_owner = turn; // 設定發牌者
  snprintf(msg_buf, sizeof(msg_buf), "310%c", turn); // 訊息 310 是設定 turn
  broadcast_msg(1, msg_buf);
  snprintf(msg_buf, sizeof(msg_buf), "305%c", card_owner); // 訊息 305 是設定
                                                              // card_owner
  broadcast_msg(1, msg_buf);
  display_point(turn); // 顯示目前輪到哪位玩家

  card_point = 0; // 重置牌堆索引
  card_index = 143; // 設定剩餘牌數
  // strcpy(msg_buf, "302"); // 訊息 302 是發牌訊息，原本是複製字串，現在先註解

  generate_card(); // 產生牌堆

  // 發送 16 張牌給 4 位玩家
  for (j = 1; j <= 4; j++) {
    if (table[j]) {
      pool[j].first_round = 1; // 設定為第一輪

      snprintf(msg_buf, sizeof(msg_buf), "302"); // 訊息 302 是發牌訊息
      for (i = 0; i < 16; i++) {
        card = mj[card_point++]; // 從牌堆中取出一張牌
        pool[j].card[i] = card;  // 將牌加入手牌
        msg_buf[3 + i] = card;  // 構建發牌訊息
      }
      sort_pool(j);          // 將手牌排序
      msg_buf[3 + 16] = '\0'; // 設定字串結束符

      if (table[j] != 1) { // 如果不是伺服器
        write_msg(player[table[j]].sockfd, msg_buf); // 發送發牌訊息
      } else {
        sort_card(0); // 伺服器排序手牌
      }
    }
  }

  // 發送額外一張牌給莊家
  card = mj[card_point++]; // 從牌堆中取出一張牌
  current_card = card;    // 設定當前牌

  snprintf(msg_buf, sizeof(msg_buf), "304%c", card); // 訊息 304 是發送單張牌

  if (table[turn] != 1) { // 如果不是伺服器
    show_newcard(turn, 2); // 顯示新牌
    return_cursor();
    write_msg(player[table[turn]].sockfd, msg_buf); // 發送單張牌的訊息
    pool[turn].card[pool[turn].num] = card;  // 將牌加入手牌
  }

  if (table[turn] == 1) {  // 如果是伺服器
    pool[1].card[pool[1].num] = card;  // 將牌加入手牌
    show_card(pool[1].card[pool[1].num], INDEX_X + 16 * 2 + 1, pos_y + 1, 1);
    wrefresh(stdscr);
    return_cursor();
    play_mode = THROW_CARD; // 設定遊戲模式為丟牌
  }

  snprintf(msg_buf, sizeof(msg_buf), "314%c%c", turn, 2); // 訊息 314
                                                            // 是設定玩家狀態
  broadcast_msg(table[turn], msg_buf); // 通知玩家目前狀態

  in_play = 1;              // 設定為遊戲中

  // 檢查花牌
  sit = turn;               // 從莊家開始檢查
  do {
  } while (check_begin_flower(sit, pool[sit].card[16], 16));

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      if (table[sit])
        do {
        } while (check_begin_flower(sit, pool[sit].card[j], j));
    }
    sort_pool(sit); // 排序手牌
    sit = next_turn(sit); // 換下一位玩家
  }

  current_card = pool[turn].card[16]; // 設定當前牌
  show_num(2, 70, 144 - card_point - 16, 2); // 顯示剩餘牌數
  return_cursor();

  snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point); // 訊息 306
                                                            // 是設定剩餘牌數
  broadcast_msg(1, msg_buf);

  clear_check_flag(turn); // 清除檢查標誌
  check_flag[turn][3] = check_kang(turn, current_card); // 檢查是否可槓
  check_flag[turn][4] = check_make(turn, current_card, 0); // 檢查是否可胡
  in_check[turn] = 0;       // 清除檢查狀態

  // 檢查是否有任何可執行的動作
  for (i = 1; i < check_number; i++) {
    if (check_flag[turn][i]) {
      getting_card = 1;   // 設定為取得牌
      in_check[turn] = 1; // 設定為檢查狀態
      check_on = 1;       // 設定為可檢查

      if (in_serv) {
        init_check_mode();
      } else {
        snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c", '0', '0',
                 check_flag[turn][3] + '0',
                 check_flag[turn][4] + '0'); // 訊息 501 是傳送檢查結果
        write_msg(player[table[turn]].sockfd, msg_buf);
      }
    }
  }

  broadcast_msg(table[turn], "3080"); // 廣播訊息 3080
  if (turn != my_sit)
    write_msg(player[table[turn]].sockfd, "3081"); // 訊息 3081
  if (turn == my_sit)
    sort_card(1);
  else
    sort_card(0);

  gettimeofday(&before, (struct timezone *)0); // 記錄時間
}

// 取得下一位玩家
int next_turn(int current_turn) {
  current_turn++; // 輪到下一位玩家
  if (current_turn == 5) {
    current_turn = 1; // 如果超過 4 位玩家，則回到第一位
  }
  if (!table[current_turn]) {
    current_turn = next_turn(current_turn); // 如果該位置沒有玩家，則繼續找下一位
  }
  return current_turn; // 回傳下一位玩家的座位號碼
}

// 顯示指定座位的牌池
void display_pool(int sit) {
  int i;
  char buf[5];
  char msg_buf[255] = {0}; // 初始化字串

  // 組合牌池中的牌
  for (i = 0; i < pool[sit].num; i++) {
    snprintf(buf, sizeof(buf), "%2d", pool[sit].card[i]);
    strcat(msg_buf, buf);
  }
  display_comment(msg_buf); // 顯示訊息
}

// 將指定座位的牌池進行排序
void sort_pool(int sit) {
  int i, j, max;
  char tmp;

  max = pool[sit].num; // 取得牌池的牌數
  for (i = 0; i < max; i++) {
    for (j = 0; j < max - i - 1; j++) {
      // 冒泡排序
      if (pool[sit].card[j] > pool[sit].card[j + 1]) {
        tmp = pool[sit].card[j];
        pool[sit].card[j] = pool[sit].card[j + 1];
        pool[sit].card[j + 1] = tmp;
      }
    }
  }
}

// 將手牌進行排序 (mode: 0 - 不包含新摸的牌, 1 - 包含)
void sort_card(int mode) {
  int i, j, max;
  char tmp;

  if (mode) {
    max = pool[my_sit].num + 1; // 包含新摸的牌
  } else {
    max = pool[my_sit].num; // 不包含新摸的牌
  }

  // 冒泡排序
  for (i = 0; i < max; i++) {
    for (j = 0; j < max - i - 1; j++) {
      if (pool[my_sit].card[j] > pool[my_sit].card[j + 1]) {
        tmp = pool[my_sit].card[j];
        pool[my_sit].card[j] = pool[my_sit].card[j + 1];
        pool[my_sit].card[j + 1] = tmp;
      }
    }
  }

  // 重新顯示手牌
  for (i = 0; i < pool[my_sit].num; i++) {
    show_card(20, INDEX_X + (16 - pool[my_sit].num + i) * 2, INDEX_Y + 1, 1);
    wrefresh(stdscr);
    show_card(pool[my_sit].card[i],
              INDEX_X + (16 - pool[my_sit].num + i) * 2, INDEX_Y + 1, 1);
    wrefresh(stdscr);
  }

  // 顯示新摸的牌 (如果有的話)
  if (mode) {
    show_card(20, INDEX_X + (16 - pool[my_sit].num + i) * 2 + 1,
              INDEX_Y + 1, 1);
    wrefresh(stdscr);
    show_card(pool[my_sit].card[pool[my_sit].num],
              INDEX_X + (16 - pool[my_sit].num + i) * 2 + 1, INDEX_Y + 1, 1);
    wrefresh(stdscr);
  }
  return_cursor();
}

// 開始新的一局遊戲
void new_game() {
  clear_screen_area(0, 2, 18, 54); // 清除螢幕區域
  show_cardmsg(my_sit, 0);          // 顯示訊息
  draw_table();                      // 繪製桌面
  wrefresh(stdscr);                 // 刷新螢幕
}

// 丟牌到桌面
void throw_card(char card) {
  int x, y;

  // 如果丟棄的是空牌，則減少牌的計數
  if (card == 20) {
    card_count--;
  }

  // 將牌加入桌面牌堆
  table_card[card_count / 17][card_count % 17] = card;

  // 計算牌在桌面的位置
  x = THROW_X + (card_count % 17) * 2;
  y = THROW_Y + (card_count / 17) * 2;

  // 顯示牌
  if (y % 4 == 3) {
    if (!color)
      attron(A_BOLD);
    show_card(card, x, y, 1);
    if (!color)
      attroff(A_BOLD);
  } else {
    show_card(card, x, y, 1);
  }

  // 增加牌的計數
  if (card != 20) {
    card_count++;
  }
}

// 發送一張牌給指定玩家
void send_one_card(int id) {
  char msg_buf[255];
  char card;
  char sit;
  int i;

  // 取得玩家座位
  sit = player[id].sit;

  // 從牌堆中取出一張牌
  card = mj[card_point++];
  current_card = card;

  // 將牌加入玩家手牌
  pool[sit].card[pool[sit].num] = card;

  // 更新剩餘牌數
  show_num(2, 70, 144 - card_point - 16, 2);

  // 發送訊息給所有玩家，告知發牌事件
  snprintf(msg_buf, sizeof(msg_buf), "314%c%c", sit, 2);
  broadcast_msg(id, msg_buf);

  // 在客戶端顯示新牌
  show_newcard(sit, 2);
  return_cursor();

  // 發送訊息給所有玩家，告知剩餘牌數
  snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point);
  broadcast_msg(1, msg_buf);

  // 顯示發牌訊息
  show_cardmsg(sit, 0);
  card_owner = sit;

  // 發送訊息給所有玩家，告知發牌玩家
  snprintf(msg_buf, sizeof(msg_buf), "305%c", (char)sit);
  broadcast_msg(1, msg_buf);

  // 清除所有檢查標誌
  clear_check_flag(sit);

  // 檢查是否可以槓或胡
  check_flag[sit][3] = check_kang(sit, card);
  check_flag[sit][4] = check_make(sit, card, 0);
  in_check[sit] = 0;

  // 檢查是否有任何檢查動作
  for (i = 1; i < check_number; i++) {
    if (check_flag[sit][i]) {
      getting_card = 1;
      in_check[sit] = 1;
      check_on = 1;

      snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c",
               check_flag[sit][1] + '0', check_flag[sit][2] + '0',
               check_flag[sit][3] + '0', check_flag[sit][4] + '0');
      write_msg(player[table[sit]].sockfd, msg_buf);
      break;
    }
  }

  // 發送訊息給指定玩家，告知發送的牌
  snprintf(msg_buf, sizeof(msg_buf), "304%c", card);
  write_msg(player[id].sockfd, msg_buf);

  // 記錄發牌時間
  gettimeofday(&before, (struct timezone *)0);
}

// 切換到下一位玩家
void next_player() {
  char msg_buf[255];

  // 取得下一位玩家
  turn = next_turn(turn);

  // 發送訊息給所有玩家，告知輪到哪位玩家
  snprintf(msg_buf, sizeof(msg_buf), "310%c", turn);
  broadcast_msg(1, msg_buf);

  // 顯示目前輪到哪位玩家
  display_point(turn);
  return_cursor();

  // 如果輪到的玩家不是伺服器
  if (table[turn] != 1) {
    // 發送訊息給該玩家，告知輪到他
    strcpy(msg_buf, "303");
    write_msg(player[table[turn]].sockfd, msg_buf);
    show_newcard(turn, 1);
    return_cursor();
  } else {
    // 如果輪到的玩家是伺服器
    attron(A_REVERSE);
    show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
    attroff(A_REVERSE);
    wrefresh(stdscr);
    return_cursor();
    play_mode = GET_CARD;
  }

  // 發送訊息給所有玩家，告知目前狀態
  snprintf(msg_buf, sizeof(msg_buf), "314%c%c", turn, 1);
  broadcast_msg(table[turn], msg_buf);
}

// 請求一張牌
int request_card() {
  if (in_join) {
    // 如果是加入遊戲的模式，向伺服器請求一張牌
    write_msg(table_sockfd, "313");
    return 0; // 返回 0 表示請求成功，實際的牌值由伺服器發送
  } else {
    // 如果不是加入遊戲的模式，從牌堆中取一張牌
    if (card_point >= 144) {
      // 如果牌堆已經沒有牌了，返回錯誤
      display_comment("牌堆已空!");
      return -1; // 使用 -1 表示錯誤
    }
    return mj[card_point++]; // 從牌堆中取出下一張牌
  }
}

// 更換一張牌
void change_card(char position, char card) {
  int i;

  // 計算要更換的牌在手牌中的索引
  i = (16 - pool[my_sit].num + position) * 2;
  if (position == pool[my_sit].num)
    i++;

  // 檢查索引是否有效
  if (position < 0 || position >= 17) {
    display_comment("無效的牌的位置!");
    return;
  }

  // 更換手牌中的牌
  pool[my_sit].card[position] = card;

  // 顯示更換後的牌
  show_card(20, INDEX_X + i, INDEX_Y + 1, 1);
  wrefresh(stdscr);
  show_card(card, INDEX_X + i, INDEX_Y + 1, 1);
  wrefresh(stdscr);
}

// 取得一張牌
void get_card(char card) {
  // 將牌加入手牌
  pool[my_sit].card[pool[my_sit].num] = card;

  // 顯示新取得的牌
  show_card(20, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
  wrefresh(stdscr);
  show_card(card, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
  wrefresh(stdscr);
}

// 處理新取得的牌
void process_new_card(char sit, char card) {
  char msg_buf[255];

  // 設定當前牌
  current_card = card;

  // 顯示訊息
  show_cardmsg(sit, 0);

  // 將牌加入牌池
  pool[sit].card[pool[sit].num] = card;

  // 顯示新取得的牌
  get_card(card);

  // 還原游標位置
  return_cursor();

  // 檢查是否為花牌
  if (!check_flower(sit, card)) {
    // 如果不是花牌，進入丟牌模式
    play_mode = THROW_CARD;
  }
}

// 處理與 GPS 伺服器的連線和通訊
void gps() {
  int status;
  int i;
  char buf[128];
  char msg_buf[50];
  char ans_buf[50];

  init_global_screen();
  input_mode = 0;

  snprintf(msg_buf, sizeof(msg_buf), "連往 QKMJ Server %s %d", config.gps_ip,
           config.gps_port);
  display_comment(msg_buf);

  status = init_socket(config.gps_ip, config.gps_port, &gps_sockfd);
  if (status < 0) {
    perror("無法連往 QKMJ Server");
    endwin();
    exit(EXIT_FAILURE);
  }

  snprintf(msg_buf, sizeof(msg_buf),
           "歡迎來到 QKMJ 休閑麻將 Ver %c.%2s 特別板 ", QKMJ_VERSION[0],
           QKMJ_VERSION + 1);
  display_comment(msg_buf);
  display_comment("原作者 sywu (吳先祐 Shian-Yow Wu) ");
  display_comment("Forked Source: https://github.com/gjchentw/qkmj");

  get_my_info();

  snprintf(msg_buf, sizeof(msg_buf), "100%s", QKMJ_VERSION);
  write_msg(gps_sockfd, msg_buf);

  snprintf(msg_buf, sizeof(msg_buf), "099%s", my_username);
  write_msg(gps_sockfd, msg_buf);

  pass_count = 0;
  if (my_name[0] != 0 && my_pass[0] != 0) {
    strncpy(ans_buf, (char *)my_name, sizeof(ans_buf) - 1);
    ans_buf[sizeof(ans_buf) - 1] = '\0';
  } else {
    strncpy(ans_buf, (char *)my_name, sizeof(ans_buf) - 1);
    ans_buf[sizeof(ans_buf) - 1] = '\0';

    do {
      ask_question("請輸入你的名字：", ans_buf, 10, 1);
    } while (ans_buf[0] == 0);
    ans_buf[10] = 0;
  }

  snprintf(msg_buf, sizeof(msg_buf), "101%s", ans_buf);
  write_msg(gps_sockfd, msg_buf);
  strncpy((char *)my_name, ans_buf, sizeof(my_name) - 1);
  my_name[sizeof(my_name) - 1] = '\0';

  nfds = getdtablesize();

  FD_ZERO(&afds);
  FD_SET(gps_sockfd, &afds);
  FD_SET(0, &afds);

  for (;;) {
    fd_set read_fds = afds; // 複製 afds，避免修改原始集合

    if (table_sockfd >= nfds) {
      nfds = table_sockfd + 1;
    }
    if (serv_sockfd >= nfds) {
      nfds = serv_sockfd + 1;
    }

    if (select(nfds, &read_fds, (fd_set *)0, (fd_set *)0, 0) < 0) {
      if (errno != EINTR) {
        perror("Select Error!");
        exit(EXIT_FAILURE);
      }
      continue;
    }

    if (FD_ISSET(0, &read_fds)) {
      if (input_mode)
        process_key(); // 處理鍵盤輸入
    }

    /* Check for data from GPS */
    if (FD_ISSET(gps_sockfd, &read_fds)) {
      process_gps_data(gps_sockfd);
    }

    if (in_serv) {
      /* Check for new connections */
      if (FD_ISSET(serv_sockfd, &read_fds)) {
        if (new_client) {
          accept_new_client();
        }
      }
      /* Check for data from client */
      for (i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table) {
          if (FD_ISSET(player[i].sockfd, &read_fds)) {
            handle_client_data(i);
          }
        }
      }
      /* Waiting for signals from each sit */
      handle_waiting_state();
      /* Process the cards */
      check_game_state();
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
          broadcast_msg(1, "330");
          clear_screen_area(THROW_Y, THROW_X, 8, 34);
          wmvaddstr(stdscr, THROW_Y + 3, THROW_X + 12, "海 底 流 局");
          return_cursor();
          wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
          broadcast_msg(1, "290");
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
      if (FD_ISSET(table_sockfd, &read_fds)) {
        handle_server_data();
      }
    }
  }
}

// 處理來自 GPS 伺服器的資料
static void process_gps_data(int gps_sockfd) {
  unsigned char buf[128];

  if (!read_msg_id(gps_sockfd, (char *)buf)) {
    display_comment("Closed by QKMJ Server.");
    shutdown(gps_sockfd, SHUT_RDWR);
    close(gps_sockfd); // 關閉 socket
    if (in_join)
      close_join();
    if (in_serv)
      close_serv();
    endwin();
    exit(EXIT_FAILURE);
  } else {
    process_msg(0, buf, FROM_GPS);
    buf[0] = '\0';
  }
}

// 處理來自客戶端的資料
static void handle_client_data(int player_id) {
  unsigned char buf[MAX_MSG_LEN];

  if (read_msg_id(player[player_id].sockfd, (char *)buf) == 0)
    close_client(player_id);
  else
    process_msg(player_id, buf, FROM_CLIENT);
}

// 處理來自伺服器的資料
static void handle_server_data() {
  unsigned char buf[MAX_MSG_LEN];

  if (!read_msg_id(table_sockfd, (char *)buf)) {
    close(table_sockfd);
    FD_CLR(table_sockfd, &afds);
    in_join = 0;
    input_mode = TALK_MODE;
    init_global_screen();
  } else
    process_msg(1, buf, FROM_SERV);
}

// 檢查遊戲狀態
static void check_game_state() {
  int i;
  // 尋找是否還有玩家處於檢查狀態
  for (i = 1; i <= 4; i++) {
    if (table[i] && in_check[i])
      return; // 還有玩家在檢查中，直接返回
  }
  check_on = 0;
  next_player_on = 1;
  send_card_on = 1;
  compare_check();
}

// 處理等待狀態
void handle_waiting_state() {
  int i;
  // 如果處於等待狀態，檢查是否所有玩家都已準備好
  if (waiting) {
    for (i = 1; i <= 4; i++) {
      if (table[i] && !wait_hit[i])
        return; // 還有玩家未準備好，直接返回
    }
    waiting = 0;
    broadcast_msg(1, "290");
    opening();
    open_deal();
  }
}

// 初始化設定檔
static void init_config() {
  strncpy(config.gps_ip, DEFAULT_GPS_IP,
          sizeof(config.gps_ip) - 1); // 使用 strncpy
  config.gps_ip[sizeof(config.gps_ip) - 1] = '\0'; // 確保字串結束符
  config.gps_port = DEFAULT_GPS_PORT;
  config.username[0] = '\0';
  config.address[0] = '\0';
}

// 讀取設定檔
static void read_qkmjrc() {
  FILE *qkmjrc_fp;
  char msg_buf[256];
  char msg_buf1[256];
  char event[80];
  char *str1;
  char rc_name[255];

  snprintf(rc_name, sizeof(rc_name), "%s/%s", getenv("HOME"),
           QKMJRC); // 使用 snprintf
  if ((qkmjrc_fp = fopen(rc_name, "r")) != NULL) {
    while (fgets(msg_buf, sizeof(msg_buf), qkmjrc_fp) != NULL) { // 使用
                                                                 // sizeof
      Tokenize(msg_buf);
      my_strupr(event, (char *)cmd_argv[1]);
      if (strcmp(event, "LOGIN") == 0) {
        if (narg > 1) {
          cmd_argv[2][10] = 0;
          strncpy(config.username, (char *)cmd_argv[2],
                  sizeof(config.username) - 1); // 使用 strncpy
          config.username[sizeof(config.username) - 1] = '\0'; // 確保字串結束符
        }
      } else if (strcmp(event, "PASSWORD") == 0) {
        if (narg > 1) {
          // 密碼處理邏輯 (省略)
        }
      } else if (strcmp(event, "SERVER") == 0) {
        if (narg > 1) {
          strncpy(config.gps_ip, (char *)cmd_argv[2],
                  sizeof(config.gps_ip) - 1); // 使用 strncpy
          config.gps_ip[sizeof(config.gps_ip) - 1] = '\0'; // 確保字串結束符
        }
        if (narg > 2) {
          config.gps_port = atoi((char *)cmd_argv[3]);
        }
      } else if (strcmp(event, "NOTE") == 0) {
        // 備註處理邏輯 (省略)
      } else if (strcmp(event, "BEEP") == 0) {
        // 提示音處理邏輯 (省略)
      }
    }
    fclose(qkmjrc_fp);
  }
}

// GPS 連線初始化
static int init_gps_connection() {
  int status;
  char msg_buf[256];

  snprintf(msg_buf, sizeof(msg_buf), "連往 QKMJ Server %s %d",
           config.gps_ip, config.gps_port); // 使用 snprintf
  display_comment(msg_buf);

  status = init_socket(config.gps_ip, config.gps_port, &gps_sockfd);
  if (status < 0) {
    perror("無法連往 QKMJ Server");
    endwin();
    exit(EXIT_FAILURE);
  }

  snprintf(msg_buf, sizeof(msg_buf),
           "歡迎來到 QKMJ 休閑麻將 Ver %c.%2s 特別板 ", QKMJ_VERSION[0],
           QKMJ_VERSION + 1); // 使用 snprintf
  display_comment(msg_buf);
  display_comment("原作者 sywu (吳先祐 Shian-Yow Wu) ");
  display_comment("Forked Source: https://github.com/gjchentw/qkmj");

  get_my_info(); // 獲取玩家資訊

  return 0;
}

// 發送初始訊息給 GPS 伺服器
static int send_initial_gps_messages() {
  char msg_buf[256];
  char ans_buf[256];

  snprintf(msg_buf, sizeof(msg_buf), "100%s", QKMJ_VERSION); // 使用
                                                             // snprintf
  write_msg(gps_sockfd, msg_buf);
  snprintf(msg_buf, sizeof(msg_buf), "099%s", my_username); // 使用
                                                            // snprintf
  write_msg(gps_sockfd, msg_buf);

  pass_count = 0;
  if (my_name[0] != 0 && my_pass[0] != 0) {
    strncpy(ans_buf, (char *)my_name, sizeof(ans_buf) - 1); // 使用 strncpy
    ans_buf[sizeof(ans_buf) - 1] = '\0'; // 確保字串結束符
  } else {
    strncpy(ans_buf, (char *)my_name, sizeof(ans_buf) - 1); // 使用 strncpy
    ans_buf[sizeof(ans_buf) - 1] = '\0'; // 確保字串結束符

    do {
      ask_question("請輸入你的名字：", ans_buf, sizeof(ans_buf) - 1, 1);
    } while (ans_buf[0] == 0);
    ans_buf[10] = 0;
  }

  snprintf(msg_buf, sizeof(msg_buf), "101%s", ans_buf); // 使用
                                                        // snprintf
  write_msg(gps_sockfd, msg_buf);
  strncpy((char *)my_name, ans_buf, sizeof(my_name) - 1); // 使用 strncpy
  my_name[sizeof(my_name) - 1] = '\0'; // 確保字串結束符

  return 0;
}

// 初始化 fd_set
static int init_fd_sets() {
  nfds = getdtablesize();

  FD_ZERO(&afds);
  FD_SET(gps_sockfd, &afds);
  FD_SET(0, &afds);

  return 0;
}

// 程式進入點
int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  setenv("TERM", "xterm", 1);

  // 初始化 curses
  initscr();
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  cbreak();
  noecho();
  nonl();
  clear();

  init_variable(); // 初始化變數
  init_config(); // 初始化設定檔
  read_qkmjrc(); // 讀取設定檔

  if (argc >= 3) {
    strncpy(config.gps_ip, argv[1],
            sizeof(config.gps_ip) - 1); // 使用 strncpy
    config.gps_ip[sizeof(config.gps_ip) - 1] = '\0';
    config.gps_port = atoi(argv[2]);
  } else if (argc == 2) {
    strncpy(config.gps_ip, argv[1],
            sizeof(config.gps_ip) - 1); // 使用 strncpy
    config.gps_ip[sizeof(config.gps_ip) - 1] = '\0';
    config.gps_port = DEFAULT_GPS_PORT;
  }

  gps(); // 啟動 GPS 連線

  exit(EXIT_SUCCESS);
}
