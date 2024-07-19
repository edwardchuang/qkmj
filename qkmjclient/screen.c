#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "qkmj.h"
  
// 設定文字顏色
// fore: 前景顏色
// back: 背景顏色
int set_color(int fore, int back) {
  char msg_buf[80];
  wrefresh(stdscr);
  if (color) {
    // 使用 ANSI escape code 設定顏色
    fprintf(stdout, "[%d;%dm", fore, back);
    fflush(stdout);
    /*
    sprintf(msg_buf,"[%d;%dm",fore,back);
    waddstr(stdscr,msg_buf);
    */
  }
  return 0;
}

// 設定文字模式
// mode: 文字模式
void set_mode(int mode) {
  char msg_buf[80];
  wrefresh(stdscr);
  if (color) {
    // 使用 ANSI escape code 設定文字模式
    printf("[%dm", mode);
    fflush(stdout);
    /*
    sprintf(msg_buf,"\033%dm",mode);
    waddstr(stdscr,msg_buf);
    */
  }
}

// 在指定位置顯示字串
// win: 窗口
// y: 垂直座標
// x: 水平座標
// msg: 字串
void mvprintstr(WINDOW *win, int y, int x, char *msg) {
  wmove(win, y, x);
  printstr(win, msg);
}

// 在窗口中顯示字串
// win: 窗口
// str: 字串
void printstr(WINDOW *win, char *str) {
  int i, len;
  len = strlen(str);
  for (i = 0; i < len; i++) {
    printch(win, str[i]);
  }
}

// 在窗口中顯示單個字元
// win: 窗口
// ch: 字元
void printch(WINDOW *win, char ch) {
  char msg[3];
  msg[0] = ch;
  msg[1] = '\0';
  waddstr(win, msg);
}

// 在指定位置顯示單個字元
// win: 窗口
// y: 垂直座標
// x: 水平座標
// ch: 字元
void mvprintch(WINDOW *win, int y, int x, char ch) {
  wmove(win, y, x);
  printch(win, ch);
}

// 清除螢幕區域
// ymin: 垂直起始座標
// xmin: 水平起始座標
// height: 高度
// width: 寬度
void clear_screen_area(int ymin, int xmin, int height, int width) {
  int i;
  char line_buf[255];

// 初始化 line_buf
  memset(line_buf, ' ', width);
  line_buf[width] = '\0';

  // 逐行清除
  for (i = 0; i < height; i++) {
    wmvaddstr(stdscr, ymin + i, xmin, line_buf);
  }
  wrefresh(stdscr);
}

// 清除輸入行
void clear_input_line() {
  werase(inputwin);
  talk_x = 0;
  wmove(inputwin, 0, talk_x);
  talk_buf_count = 0;
  talk_buf[0] = '\0';
  wrefresh(inputwin);
}

// 等待使用者按下鍵盤
// msg: 顯示訊息
void wait_a_key(char *msg) {
  int ch;
  werase(inputwin);
  wmvaddstr(inputwin, 0, 0, msg);
  wrefresh(inputwin);
  beep1();
  // 等待使用者按下 Enter 鍵
  do {
    ch = my_getch();
  } while (ch != KEY_ENTER && ch != ENTER);
  werase(inputwin);
  mvwaddstr(inputwin, 0, 0, talk_buf);
  wrefresh(inputwin);
}

// 詢問使用者問題
// question: 問題
// answer: 答案
// ans_len: 答案長度
// type: 輸入類型
void ask_question(char *question, char *answer, int ans_len, int type) {
  werase(inputwin);
  wmvaddstr(inputwin, 0, 0, question);
  wrefresh(inputwin);
  // 使用 mvwgetstring 取得使用者輸入
  mvwgetstring(inputwin, 0, strlen(question), ans_len, (unsigned char *)answer, type);
  werase(inputwin);
  wmvaddstr(inputwin, 0, 0, talk_buf);
  wrefresh(inputwin);
}

// 繪製索引
// max_item: 最大項目數
void draw_index(int max_item) {
  int i;
  /* normal();  */
  // 繪製索引項目
  for (i = 1; i < max_item; i++) {
    wmvaddstr(stdscr, INDEX_Y, INDEX_X + (17 - max_item + i) * 2 - 2,
              menu_item[i]);
  }
  wmvaddstr(stdscr, INDEX_Y, INDEX_X + 16 * 2 + 1, menu_item[max_item]);
  return_cursor();
}

// 設定目前索引
// current: 目前索引
void current_index(int current) {
  wmove(stdscr, INDEX_Y, INDEX_X + current * 2);
  wrefresh(stdscr);
}

// 顯示牌背
// sit: 座位
void show_cardback(char sit) {
  int i;

  switch ((sit - my_sit + 4) % 4) {
    case 0:
      break;
    case 1:
      // 顯示牌背
      for (i = 0; i < pool[sit].num; i++) {
        show_card(40, INDEX_X1, INDEX_Y1 - i, 0);
      }
      break;
    case 2:
      // 顯示牌背
      for (i = 0; i < pool[sit].num; i++) {
        show_card(30, INDEX_X2 - i * 2, INDEX_Y2, 1);
      }
      break;
    case 3:
      // 顯示牌背
      for (i = 0; i < pool[sit].num; i++) {
        show_card(40, INDEX_X3, INDEX_Y3 + i, 0);
      }
      break;
  }
  return_cursor();
}

// 顯示所有牌
// sit: 座位
void show_allcard(char sit) {
  int i;

  switch ((sit - my_sit + 4) % 4) {
    case 0:
      break;
    case 1:
      // 顯示所有牌
      for (i = 0; i < pool[sit].num; i++) {
        show_card(pool[sit].card[i], INDEX_X1,
                  INDEX_Y1 - (16 - pool[sit].num + i), 0);
      }
      break;
    case 2:
      // 顯示所有牌
      for (i = 0; i < pool[sit].num; i++) {
        show_card(pool[sit].card[i], INDEX_X2 - (16 - pool[sit].num + i) * 2,
                  INDEX_Y2, 1);
      }
      break;
    case 3:
      // 顯示所有牌
      for (i = 0; i < pool[sit].num; i++) {
        show_card(pool[sit].card[i], INDEX_X3,
                  INDEX_Y3 + (16 - pool[sit].num + i), 0);
      }
      break;
  }
  return_cursor();
}

// 顯示槓牌
// sit: 座位
void show_kang(char sit) {
  int i;

  switch ((sit - my_sit + 4) % 4) {
    case 0:
      break;
    case 1:
      // 顯示槓牌
      for (i = 0; i < pool[sit].out_card_index; i++) {
        if (pool[sit].out_card[i][0] == 11) {  // 暗槓
          show_card(pool[sit].out_card[i][2], INDEX_X1,
                    INDEX_Y1 - i * 3 - 1, 0);
        }
      }
      break;
    case 2:
      // 顯示槓牌
      for (i = 0; i < pool[sit].out_card_index; i++) {
        if (pool[sit].out_card[i][0] == 11) {
          show_card(pool[sit].out_card[i][2], INDEX_X2 - i * 6 - 2,
                    INDEX_Y2, 1);
        }
      }
      break;
    case 3:
      // 顯示槓牌
      for (i = 0; i < pool[sit].out_card_index; i++) {
        if (pool[sit].out_card[i][0] == 11) {
          show_card(pool[sit].out_card[i][2], INDEX_X3,
                    INDEX_Y3 + i * 3 + 1, 0);
        }
      }
      break;
  }
}

// 顯示新牌
// sit: 座位
// type: 牌型
/*  type 1 : 摸牌  */
/*  type 2 : 摸入  */
/*  type 3 : 丟出  */
/*  type 4 : 顯示  */
void show_newcard(char sit, char type) {
  switch ((sit - my_sit + 4) % 4) {
    case 0:
      switch (type) {
        case 2:
          show_card(current_card, INDEX_X, INDEX_Y + 1, 1);
        case 4:
          show_card(current_card, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
          break;
      }
      break;
    case 1:
      switch (type) {
        case 1:
          /*
          attron(A_BOLD);
          show_card(10,INDEX_X1,INDEX_Y1-17,0);
          attroff(A_BOLD);
          */
          break;
        case 2:
          show_card(40, INDEX_X1, INDEX_Y1 - 17, 0);
          break;
        case 3:
          show_card(20, INDEX_X1, INDEX_Y1 - 17, 0);
          break;
        case 4:
          show_card(current_card, INDEX_X1, INDEX_Y1 - 17, 0);
          break;
      }
      break;
    case 2:
      switch (type) {
        case 1:
          /*
          attron(A_BOLD);
          show_card(10,INDEX_X2-16*2-1,INDEX_Y2,1);
          attroff(A_BOLD);
          */
          break;
        case 2:
          show_card(30, INDEX_X2 - 16 * 2 - 1, INDEX_Y2, 1);
          show_card(20, INDEX_X2 - 16 * 2 - 1, INDEX_Y2, 1);
          show_card(30, INDEX_X2 - 16 * 2 - 1, INDEX_Y2, 1);
          break;
        case 3:
          show_card(20, INDEX_X2 - 16 * 2 - 1, INDEX_Y2, 1);
          break;
        case 4:
          show_card(current_card, INDEX_X2 - 16 * 2 - 1, INDEX_Y2, 1);
          break;
      }
      break;
    case 3:
      switch (type) {
        case 1:
          /*
          attron(A_BOLD);
          show_card(10,INDEX_X3,INDEX_Y3+17,0);
          attroff(A_BOLD);
          */
          break;
        case 2:
          // 處理顏色問題
          show_card(40, INDEX_X3, INDEX_Y3 + 17, 0);
          /*  show_card(20,INDEX_X3,INDEX_Y3+17,0); */
          show_card(40, INDEX_X3, INDEX_Y3 + 17, 0);
          break;
        case 3:
          show_card(20, INDEX_X3, INDEX_Y3 + 17, 0);
          break;
        case 4:
          show_card(current_card, INDEX_X3, INDEX_Y3 + 17, 0);
          break;
      }
      break;
  }
}
      
/* Show cards on the screen. */
/* type   0: row             */
/* type   1: column          */
void show_card(char card, int x, int y, int type) {
  char card1[3];
  char card2[3];

  // 重置游標
  reset_cursor();
  // 清除顯示區域
  mvwaddstr(stdscr, y, x, "  ");
  wrefresh(stdscr);
  wmove(stdscr, y, x);

  // 設定顏色
  if (card == 30 || card == 40) {
    set_color(32, 40);
  } else if (card >= 1 && card <= 9) {
    set_mode(1);
    set_color(31, 40);
  } else if (card >= 11 && card <= 19) {
    set_mode(1);
    set_color(32, 40);
  } else if (card >= 21 && card <= 29) {
    set_mode(1);
    set_color(36, 40);
  } else if (card >= 31 && card <= 34) {
    set_mode(1);
    set_color(33, 40);
  } else if (card >= 41 && card <= 43) {
    set_mode(1);
    set_color(35, 40);
  }

  // 根據 type 顯示牌
  if (type == 1) {
    // 顯示兩行
    strncpy(card1, mj_item[card], 2);
    card1[2] = '\0';
    strncpy(card2, mj_item[card] + 2, 2);
    card2[2] = '\0';
    mvwaddstr(stdscr, y, x, card1);
    mvwaddstr(stdscr, y + 1, x, card2);
  } else {
    // 顯示一行
    mvwaddstr(stdscr, y, x, mj_item[card]);
  }

  // 刷新畫面
  wrefresh(stdscr);

  // 重置顏色和模式
  set_color(37, 40);
  set_mode(0);
}

void draw_title() {
  int x, y;

  // 繪製邊框
  for (y = 0; y < 24; y++) {
    mvprintstr(stdscr, y, 0, "│");
    mvprintstr(stdscr, y, 76, "│");
  }
  mvprintstr(stdscr, 22, 0, "├");
  for (x = 2; x <= 75; x += 2) {
    printstr(stdscr, "─");
  }
  mvprintstr(stdscr, 22, 76, "┤");

  // 顯示標題
  wmvaddstr(stdscr, 23, 2, "【對話】");
  wrefresh(stdscr);
}

void init_playing_screen() {
  // 初始化遊戲資訊
  info.wind = 1;
  info.dealer = 1;
  info.cont_dealer = 0;

  // 初始化對話框
  talk_right = 55;
  inputwin = playing_win;
  talk_x = 0;
  talk_y = 0;
  talk_buf_count = 0;
  talk_buf[0] = '\0';

  // 初始化評論框
  comment_right = 55;
  comment_up = 19;
  comment_bottom = 22;
  comment_y = 19;
  commentwin = newwin(comment_bottom - comment_up + 1,
                     comment_right - comment_left + 1, comment_up,
                     comment_left);
  scrollok(commentwin, TRUE);

  // 設定畫面模式
  screen_mode = PLAYING_SCREEN_MODE;

  // 繪製遊戲畫面
  draw_playing_screen();

  // 設定游標位置
  wmove(inputwin, talk_y, talk_x);
  wrefresh(inputwin);
}

void init_global_screen() {
  char msg_buf[255];
  char ans_buf[255];

  // 初始化評論框
  comment_left = comment_x = org_comment_x;
  comment_right = 72;
  comment_up = 0;
  comment_bottom = 21;
  comment_y = org_comment_y;

  // 初始化對話框
  talk_left = 11;
  talk_right = 74;
  talk_x = 0;
  talk_buf_count = 0;
  talk_buf[0] = '\0';

  // 設定畫面模式
  screen_mode = GLOBAL_SCREEN_MODE;

  // 建立評論框
  commentwin = newwin(22, 74, 0, 2);
  scrollok(commentwin, TRUE);

  // 建立對話框
  inputwin = newwin(1, talk_right - talk_left, org_talk_y, talk_left);

  // 繪製全局畫面
  draw_global_screen();

  // 設定輸入模式
  input_mode = TALK_MODE;
  talk_left = 11;
  inputwin = global_win;

  // 顯示對話標題
  wmvaddstr(stdscr, 23, 2, "【對話】");
  return_cursor();
}

void wmvaddstr(WINDOW *win, int y, int x, char *str) {
  wmove(win, y, x);
  waddstr(win, str);
}

void draw_table() {
  int x, y;

  // 繪製桌子邊框
  wmvaddstr(stdscr, 4, 10, "□");
  for (x = 13; x <= 45; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "□");
  for (y = 5; y <= 12; y++) {
    wmvaddstr(stdscr, y, 10, "│");
    wmvaddstr(stdscr, y, 46, "│");
  }
  wmvaddstr(stdscr, 13, 10, "□");
  for (x = 12; x <= 44; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "□");
}

void draw_global_screen() {
  // 清除畫面
  clear();
  // 繪製標題
  draw_title();
}

/* 繪製遊戲畫面 */
void draw_playing_screen() {
  int x, y;
  clear();
  /* 繪製外框 */
  for (y = 0; y <= 23; y++) {
    wmvaddstr(stdscr, y, 0, "│");
    wmvaddstr(stdscr, y, 56, "│");
    wmvaddstr(stdscr, y, 76, "│");
  }
  wmvaddstr(stdscr, 18, 0, "├");
  for (x = 3; x <= 77; x += 2) {
    waddstr(stdscr, "─");
  }
  wmvaddstr(stdscr, 18, 76, "┤");
  wmvaddstr(stdscr, 18, 56, "┼");
  wmvaddstr(stdscr, 23, 2, "【對話】 ");
  /* 繪製桌子外框 */
  /* 資訊區 */
  wmvaddstr(stdscr, 1, 56, "├");
  for (x = 58; x <= 74; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "┤");
  wmvaddstr(stdscr, 3, 56, "├");
  for (x = 58; x <= 74; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "┤");
  wmvaddstr(stdscr, 12, 56, "├");
  for (x = 58; x <= 74; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "┤");
  wmvaddstr(stdscr, 0, 66, "│");
  wmvaddstr(stdscr, 1, 66, "┼");
  wmvaddstr(stdscr, 2, 66, "│");
  wmvaddstr(stdscr, 3, 66, "┴");
  /* 字元 */
  wmvaddstr(stdscr, 0, 60, "風  局");
  wmvaddstr(stdscr, 0, 68, "連    莊");
  wmvaddstr(stdscr, 2, 58, "門風：");
  wmvaddstr(stdscr, 2, 68, "剩    張");
  wmvaddstr(stdscr, 4, 58, "東家：");
  wmvaddstr(stdscr, 6, 58, "南家：");
  wmvaddstr(stdscr, 8, 58, "西家：");
  wmvaddstr(stdscr, 10, 58, "北家：");
  wmvaddstr(stdscr, 5, 62, "＄");
  wmvaddstr(stdscr, 7, 62, "＄");
  wmvaddstr(stdscr, 9, 62, "＄");
  wmvaddstr(stdscr, 11, 62, "＄");
  wmvaddstr(stdscr, 13, 60, "0  1  2  3  4");
  wmvaddstr(stdscr, 14, 60, "無 吃 碰 杠 胡");
  wmvaddstr(stdscr, 20, 64, "┌  ┐");
  wmvaddstr(stdscr, 22, 64, "└  ┘");
  wrefresh(stdscr);
  wmove(inputwin, talk_y, talk_x);
  wrefresh(inputwin);
}

/* 尋找顯示點 */
void find_point(int pos) {
  switch (pos) {
    case 0:
      wmove(stdscr, 22, 66);
      break;
    case 1:
      wmove(stdscr, 21, 68);
      break;
    case 2:
      wmove(stdscr, 20, 66);
      break;
    case 3:
      wmove(stdscr, 21, 64);
      break;
  }
}

/* 顯示當前輪到誰 */
void display_point(int current_turn) {
  static int last_turn = 0;
  char msg_buf[255];

  if (last_turn) {
    find_point((last_turn + 4 - my_sit) % 4);
    waddstr(stdscr, "  ");
    wrefresh(stdscr);
    find_point((last_turn + 4 - my_sit) % 4);
    waddstr(stdscr, sit_name[last_turn]);
  }
  find_point((current_turn + 4 - my_sit) % 4);
  waddstr(stdscr, "  ");
  wrefresh(stdscr);
  find_point((current_turn + 4 - my_sit) % 4);
  attron(A_REVERSE);
  waddstr(stdscr, sit_name[current_turn]);
  attroff(A_REVERSE);
  last_turn = current_turn;
}

/* 顯示時間 */
void display_time(char sit) {
  char msg_buf[255];
  int pos;
  int min, sec;

  pos = (sit - my_sit + 4) % 4;
  min = (int)pool[sit].time / 60;
  min = min % 60;
  sec = (int)pool[sit].time % 60;
  snprintf(msg_buf, sizeof(msg_buf), "%2d:%02d", min, sec);
  switch (pos) {
    case 0:
      wmvaddstr(stdscr, 23, 64, "     ");
      wmvaddstr(stdscr, 23, 64, msg_buf);
      break;
    case 1:
      wmvaddstr(stdscr, 21, 71, "     ");
      snprintf(msg_buf, sizeof(msg_buf), "%d:%02d", min, sec);
      wmvaddstr(stdscr, 21, 71, msg_buf);
      break;
    case 2:
      wmvaddstr(stdscr, 19, 64, "     ");
      wmvaddstr(stdscr, 19, 64, msg_buf);
      break;
    case 3:
      wmvaddstr(stdscr, 21, 58, "     ");
      wmvaddstr(stdscr, 21, 58, msg_buf);
      break;
  }
  return_cursor();
}

/* 顯示遊戲資訊 */
void display_info() {
  int i;

  // 清除顯示區域
  wmvaddstr(stdscr, 0, 58, "  ");
  wmvaddstr(stdscr, 0, 62, "  ");
  wmvaddstr(stdscr, 0, 70, "    ");
  wrefresh(stdscr);

  // 顯示風向
  wmvaddstr(stdscr, 0, 58, sit_name[info.wind]);
  // 顯示莊家
  wmvaddstr(stdscr, 0, 62, sit_name[info.dealer]);
  // 顯示連莊次數
  if (info.cont_dealer < 10) {
    show_num(0, 71, info.cont_dealer, 1);
  } else {
    show_num(0, 70, info.cont_dealer, 2);
  }

  // 顯示座位資訊
  wmvaddstr(stdscr, 22, 66, sit_name[my_sit]);
  wmvaddstr(stdscr, 21, 68, sit_name[(my_sit) % 4 + 1]);
  wmvaddstr(stdscr, 20, 66, sit_name[(my_sit + 1) % 4 + 1]);
  wmvaddstr(stdscr, 21, 64, sit_name[(my_sit + 2) % 4 + 1]);

  // 顯示玩家姓名
  wmvaddstr(stdscr, 4, 64, player[table[EAST]].name);
  wmvaddstr(stdscr, 6, 64, player[table[SOUTH]].name);
  wmvaddstr(stdscr, 8, 64, player[table[WEST]].name);
  wmvaddstr(stdscr, 10, 64, player[table[NORTH]].name);

  // 顯示時間
  for (i = 1; i <= 4; i++) {
    display_time(i);
  }

  // 顯示玩家金錢
  mvwprintw(stdscr, 5, 64, "         ");
  mvwprintw(stdscr, 5, 64, "%ld", player[table[EAST]].money);
  mvwprintw(stdscr, 7, 64, "         ");
  mvwprintw(stdscr, 7, 64, "%ld", player[table[SOUTH]].money);
  mvwprintw(stdscr, 9, 64, "         ");
  mvwprintw(stdscr, 9, 64, "%ld", player[table[WEST]].money);
  mvwprintw(stdscr, 11, 64, "         ");
  mvwprintw(stdscr, 11, 64, "%ld", player[table[NORTH]].money);

  // 清除莊家標記
  mvwprintw(stdscr, 4, 74, "  ");
  mvwprintw(stdscr, 6, 74, "  ");
  mvwprintw(stdscr, 8, 74, "  ");
  mvwprintw(stdscr, 10, 74, "  ");

  // 顯示莊家標記
  attron(A_REVERSE);
  mvwprintw(stdscr, 2 + info.dealer * 2, 74, "莊");
  attroff(A_REVERSE);

  // 刷新畫面
  wrefresh(stdscr);

  // 設定游標位置
  wmove(inputwin, talk_y, talk_x);
  wrefresh(inputwin);
}

// 讀取一行文字
// fd: 檔案描述符
// buf: 緩衝區
// end_flag: 結束標誌
int readln(int fd, char *buf, int *end_flag) {
  int len = 0, bytes = 0, in_esc = 0, ch;
  int more_size = 0, more_num = 0;
  char more_buf[4096];

  // 初始化變數
  len = bytes = in_esc = 0;
  *end_flag = 0;

  // 循環讀取資料
  while (1) {
    // 檢查緩衝區是否已滿
    if (more_num >= more_size) {
      // 讀取資料到緩衝區
      more_size = read(fd, more_buf, sizeof(more_buf));
      // 檢查是否已讀取到檔案末尾
      if (more_size == 0) {
        break;
      }
      // 重置緩衝區指標
      more_num = 0;
    }

    // 取得下一個字元
    ch = more_buf[more_num++];
    // 計算已讀取的字元數
    bytes++;

    // 檢查是否已讀取到行尾或緩衝區已滿
    if (ch == '\n' || len >= 74) {
      break;
    } else if (ch == '\0') {
      // 設定結束標誌
      *end_flag = 1;
      // 清除緩衝區
      *buf = '\0';
      // 返回已讀取的字元數
      return bytes;
    } else {
      // 將字元複製到緩衝區
      *buf++ = ch;
      // 計算已讀取的字元數
      len++;
    }
  }

  // 將最後一個字元複製到緩衝區
  *buf++ = ch;
  // 將緩衝區結尾設定為空字元
  *buf = '\0';
  // 返回已讀取的字元數
  return bytes;
}

// 顯示新聞
// fd: 檔案描述符
void display_news(int fd) {
  char buf[256];
  int bytes;
  int line_count = 0;
  int end_flag;
  WINDOW *news_win;

  // 初始化變數
  line_count = 0;
  // 建立新聞窗口
  news_win = newwin(22, 74, 0, 2);
  // 清除新聞窗口
  wclear(news_win);
  // 設定游標位置
  wmove(news_win, 0, 0);

  // 循環讀取新聞資料
  while (readln(fd, buf, &end_flag)) {
    // 檢查是否已讀取到檔案末尾
    if (end_flag == 1) {
      break;
    }

    // 檢查是否已顯示到窗口末尾
    if (line_count == 22) {
      // 刷新新聞窗口
      wrefresh(news_win);
      // 等待使用者按下 Enter 鍵
      wait_a_key("請按 <ENTER> 至下一頁......");
      // 重置行數
      line_count = 0;
      // 清除新聞窗口
      wclear(news_win);
      // 設定游標位置
      wmove(news_win, 0, 0);
    }

    // 顯示新聞內容
    printstr(news_win, buf);
    // 刷新新聞窗口
    wrefresh(news_win);
    // 計算行數
    line_count++;
  }

  // 刷新新聞窗口
  wrefresh(news_win);
  // 等待使用者按下 Enter 鍵
  wait_a_key("請按 <ENTER> 繼續.......");
  // 刪除新聞窗口
  delwin(news_win);
  // 重新繪製畫面
  redraw_screen();
}

// 顯示評論
// comment: 評論內容
void display_comment(char *comment) {
  // 在評論窗口中添加換行符
  waddstr(commentwin, "\n");
  // 設定評論窗口的顏色
  wattrset(commentwin, COLOR_PAIR(1));
  wattron(commentwin, COLOR_PAIR(1));
  // 顯示評論內容
  printstr(commentwin, comment);
  // 刷新評論窗口
  wrefresh(commentwin);
  // 返回游標
  return_cursor();
}

// 發送對話訊息
// talk: 對話內容
void send_talk_line(char *talk) {  // User Talks
  char comment[255];
  char msg_buf[1024];

  // 組成評論訊息
  snprintf(comment, sizeof(comment), "<%s> %s", my_name, talk);
  // 顯示評論訊息
  display_comment(comment);
  // 組成發送訊息
  snprintf(msg_buf, sizeof(msg_buf), "102%s", comment);

  // 檢查是否已連接到伺服器
  if (in_serv) {
    // 廣播訊息
    broadcast_msg(1, msg_buf);
  }

  // 檢查是否已加入桌子
  if (in_join) {
    // 發送訊息到桌子
    write_msg(table_sockfd, msg_buf);
  }
}

// 發送 GPS 訊息
// msg: GPS 訊息內容
void send_gps_line(char *msg) {
  char comment[255];

  // 複製 GPS 訊息內容到評論緩衝區
  strncpy(comment, msg, sizeof(comment));
  // 顯示評論訊息
  display_comment(comment);
}

// 計算整數的位數
// num: 整數
int intlog10(int num) {
  int i = 0;

  // 循環計算位數
  do {
    // 除以 10
    num /= 10;
    // 檢查是否大於 1
    if (num >= 1) {
      // 計算位數
      i++;
    }
  } while (num >= 1);

  // 返回位數
  return i;
}

// 將數字轉換為字串
// str: 目標字串
// number: 數字
// digit: 位數
void convert_num(char *str, int number, int digit) {
  int i;
  int tmp[10];

  // 將數字拆解成個別位數
  for (i = digit - 1; i >= 0; i--) {
    tmp[i] = number % 10;
    number /= 10;
  }

  // 將個別位數轉換為字元並複製到目標字串
  for (i = 0; i < digit; i++) {
    strncpy(str + i * 2, number_item[tmp[i]], 2);
  }
}

// 在指定位置顯示數字
// y: 垂直座標
// x: 水平座標
// number: 數字
// digit: 位數
void show_num(int y, int x, int number, int digit) {
  int i;
  char msg_buf[255];

  // 設定游標位置
  wmove(stdscr, y, x);

  // 清除顯示區域
  for (i = 0; i < digit; i++) {
    waddstr(stdscr, "  ");
  }

  // 刷新畫面
  wrefresh(stdscr);

  // 將數字轉換為字串
  convert_num(msg_buf, number, digit);

  // 顯示數字
  wmvaddstr(stdscr, y, x, msg_buf);

  // 刷新畫面
  wrefresh(stdscr);
}

// 顯示牌訊息
// sit: 座位
// card: 牌
void show_cardmsg(int sit, char card) {
  int pos;

  // 計算座位位置
  pos = (sit - my_sit + 4) % 4;

  // 清除顯示區域
  clear_screen_area(15, 58, 3, 18);

  // 顯示牌訊息
  if (card) {
    // 繪製牌框
    wmvaddstr(stdscr, 15, 58, "┌───────┐");
    wmvaddstr(stdscr, 16, 58, "│              │");
    wmvaddstr(stdscr, 17, 58, "└───────┘");

    // 顯示座位資訊
    switch (pos) {
      case 0:
        wmvaddstr(stdscr, 16, 60, "玩");
        break;
      case 1:
        wmvaddstr(stdscr, 16, 60, "下");
        break;
      case 2:
        wmvaddstr(stdscr, 16, 60, "對");
        break;
      case 3:
        wmvaddstr(stdscr, 16, 60, "上");
        break;
    }

    // 顯示打牌訊息
    wmvaddstr(stdscr, 16, 62, "家打（    ）");

    // 刷新畫面
    wrefresh(stdscr);

    // 顯示牌
    show_card(card, 68, 16, 0);
  }

  // 返回游標
  return_cursor();
}

// 重新繪製畫面
void redraw_screen() {
  int i, j;

  // 清除畫面
  clearok(stdscr, TRUE);
  wrefresh(stdscr);

  // 刷新評論窗口
  touchwin(commentwin);
  wrefresh(commentwin);

  // 刷新輸入窗口
  touchwin(inputwin);
  wrefresh(inputwin);

  // 返回游標
  return_cursor();

  /*
  if(screen_mode==PLAYING_SCREEN_MODE)
  {
    for(i=1;i<=4;i++)
    {
      if(table[i] && i!=my_sit)
        show_cardback(i);
    }
    if(turn==card_owner)
      show_newcard(turn,2);
    for(i=0;i<3;i++)
      for(j=0;j<17;j++)
      {
        if(table_card[i][j]!=0 && table_card[i][j]!=20)
        {
          show_card(20,THROW_X+j*2,THROW_Y+i*2,1);
          show_card(table_card[i][j],THROW_X+j*2,THROW_Y+i*2,1);
        }
      }
    sort_card(0);
  }
  */
}

// 重置游標
void reset_cursor() {
  mvwaddstr(stdscr, 23, 75, " ");
  wrefresh(stdscr);
}

// 返回游標
void return_cursor() {
  switch (input_mode) {
    case ASK_MODE:
    case TALK_MODE:
      reset_cursor();
      wmove(inputwin, talk_y, talk_x);
      wrefresh(inputwin);
      break;
    case CHECK_MODE:
      wmove(stdscr, org_check_y, check_x);
      wrefresh(stdscr);
      break;
    case PLAY_MODE:
      wmove(stdscr, pos_y, pos_x);
      wrefresh(stdscr);
      break;
    case EAT_MODE:
      wmove(stdscr, eat_y, eat_x);
      wrefresh(stdscr);
      break;
  }
}