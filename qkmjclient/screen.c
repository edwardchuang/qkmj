#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mjdef.h"
#include "qkmj.h"
#include "protocol_def.h"
#include "ai_client.h"

void draw_global_screen();
void draw_playing_screen();

void set_color(int fore, int back) {
  wrefresh(stdscr);
  if (color) {
    fprintf(stdout, "\033[%d;%dm", fore, back);
    fflush(stdout);
  }
}

void set_mode(int mode) {
  wrefresh(stdscr);
  if (color) {
    printf("\033[%dm", mode);
    fflush(stdout);
  }
}

void mvprintstr(WINDOW* win, int y, int x, char* msg) {
  wmove(win, y, x);
  printstr(win, msg);
}

void printstr(WINDOW* win, char* str) {
  int len = (int)strlen(str);
  for (int i = 0; i < len; i++) {
    printch(win, str[i]);
  }
}

void printch(WINDOW* win, char ch) {
  char msg[3];
  msg[0] = ch;
  msg[1] = '\0';
  waddstr(win, msg);
}

void mvprintch(WINDOW* win, int y, int x, char ch) {
  wmove(win, y, x);
  printch(win, ch);
}

void clear_screen_area(int ymin, int xmin, int height, int width) {
  char line_buf[255];

  for (int i = 0; i < width; i++) line_buf[i] = ' ';
  line_buf[width] = '\0';
  for (int i = 0; i < height; i++) {
    wmvaddstr(stdscr, ymin + i, xmin, line_buf);
  }
  wrefresh(stdscr);
}

void clear_input_line() {
  werase(inputwin);
  talk_x = 0;
  wmove(inputwin, 0, talk_x);
  talk_buf_count = 0;
  talk_buf[0] = '\0';
  wrefresh(inputwin);
}

void wait_a_key(char* msg) {
  int ch;
  werase(inputwin);
  wmvaddstr(inputwin, 0, 0, msg);
  wrefresh(inputwin);
  beep1();
  do {
    ch = my_getch();
  } while (ch != KEY_ENTER && ch != ENTER);
  werase(inputwin);
  mvwaddstr(inputwin, 0, 0, talk_buf);
  wrefresh(inputwin);
}

void ask_question(char* question, char* answer, int ans_len, int type) {
  werase(inputwin);
  wmvaddstr(inputwin, 0, 0, question);
  wrefresh(inputwin);
  mvwgetstring(inputwin, 0, (int)strlen(question), ans_len,
               (unsigned char*)answer, type);
  werase(inputwin);
  wmvaddstr(inputwin, 0, 0, talk_buf);
  wrefresh(inputwin);
}

void draw_index(int max_item) {
  for (int i = 1; i < max_item; i++) {
    wmvaddstr(stdscr, INDEX_Y, INDEX_X + (17 - max_item + i) * 2 - 2,
              menu_item[i]);
  }
  wmvaddstr(stdscr, INDEX_Y, INDEX_X + 16 * 2 + 1, menu_item[max_item]);
  return_cursor();
}

void current_index(int current) {
  wmove(stdscr, INDEX_Y, INDEX_X + current * 2);
  wrefresh(stdscr);
}

void show_cardback(int sit) {
  int i;

  switch ((sit - my_sit + 4) % 4) {
    case 0:
      break;
    case 1:
      for (i = 0; i < pool[sit].num; i++) {
        show_card(40, INDEX_X1, INDEX_Y1 - i, 0);
      }
      break;
    case 2:
      for (i = 0; i < pool[sit].num; i++) {
        show_card(30, INDEX_X2 - i * 2, INDEX_Y2, 1);
      }
      break;
    case 3:
      for (i = 0; i < pool[sit].num; i++) {
        show_card(40, INDEX_X3, INDEX_Y3 + i, 0);
      }
      break;
  }
  return_cursor();
}

void show_allcard(int sit) {
  int i;

  switch ((sit - my_sit + 4) % 4) {
    case 0:
      break;
    case 1:
      for (i = 0; i < pool[sit].num; i++) {
        show_card(pool[sit].card[i], INDEX_X1,
                  INDEX_Y1 - (16 - pool[sit].num + i), 0);
      }
      break;
    case 2:
      for (i = 0; i < pool[sit].num; i++) {
        show_card(pool[sit].card[i], INDEX_X2 - (16 - pool[sit].num + i) * 2,
                  INDEX_Y2, 1);
      }
      break;
    case 3:
      for (i = 0; i < pool[sit].num; i++) {
        show_card(pool[sit].card[i], INDEX_X3,
                  INDEX_Y3 + (16 - pool[sit].num + i), 0);
      }
      break;
  }
  return_cursor();
}

void show_kang(int sit) {
  int i;

  switch ((sit - my_sit + 4) % 4) {
    case 0:
      break;
    case 1:
      for (i = 0; i < pool[sit].out_card_index; i++)
        if (pool[sit].out_card[i][0] == 11) /* 暗槓 */
        {
          show_card(pool[sit].out_card[i][2], INDEX_X1, INDEX_Y1 - i * 3 - 1,
                    0);
        }
      break;
    case 2:
      for (i = 0; i < pool[sit].out_card_index; i++)
        if (pool[sit].out_card[i][0] == 11) {
          show_card(pool[sit].out_card[i][2], INDEX_X2 - i * 6 - 2, INDEX_Y2,
                    1);
        }
      break;
    case 3:
      for (i = 0; i < pool[sit].out_card_index; i++)
        if (pool[sit].out_card[i][0] == 11) {
          show_card(pool[sit].out_card[i][2], INDEX_X3, INDEX_Y3 + i * 3 + 1,
                    0);
        }
      break;
  }
}

void show_newcard(int sit, int type) {
  /*  type 1 : 摸牌  */
  /*  type 2 : 摸入  */
  /*  type 3 : 丟出  */
  /*  type 4 : 顯示  */

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
          break;
        case 2:
          /* 為處理 color 的問題 */
          show_card(40, INDEX_X3, INDEX_Y3 + 17, 0);
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
void show_card(int card, int x, int y, int type) {
  char card1[5];
  char card2[5];

  reset_cursor();
  mvwaddstr(stdscr, y, x, "  ");
  wrefresh(stdscr);
  wmove(stdscr, y, x);
  if (card == 30 || card == 40) set_color(32, 40);
  if (card >= 1 && card <= 9) {
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
  if (type == 1) {
    if (mj_item[card][0] == ' ') {
      snprintf(card1, sizeof(card1), "  ");
      snprintf(card2, sizeof(card2), "  ");
    } else {
      strncpy(card1, mj_item[card], 3);
      card1[3] = '\0';
      strncpy(card2, mj_item[card] + 3, 3);
      card2[3] = '\0';
    }
    mvwaddstr(stdscr, y, x, card1);
    mvwaddstr(stdscr, y + 1, x, card2);
  } else
    mvwaddstr(stdscr, y, x, mj_item[card]);
  wrefresh(stdscr);
  set_color(37, 40);
  set_mode(0);
}

void draw_title() {
  int x, y;

  for (y = 0; y < 24; y++) {
    mvprintstr(stdscr, y, 0, "│");
    mvprintstr(stdscr, y, 76, "│");
  }
  mvprintstr(stdscr, 22, 0, "├");
  for (x = 2; x <= 75; x += 2) {
    printstr(stdscr, "─");
  }
  mvprintstr(stdscr, 22, 76, "┤");
  wmvaddstr(stdscr, 23, 2, "【對話】");
  wrefresh(stdscr);
}

void init_playing_screen() {
  info.wind = 1;
  info.dealer = 1;
  info.cont_dealer = 0;
  talk_right = 55;
  inputwin = playing_win;
  talk_x = 0;
  talk_y = 0;
  talk_buf_count = 0;
  talk_buf[0] = '\0';
  comment_right = 55;
  comment_up = 19;
  comment_bottom = 22;
  comment_y = 19;
  commentwin =
      newwin(comment_bottom - comment_up + 1,
             comment_right - comment_left + 1, comment_up, comment_left);
  scrollok(commentwin, TRUE);
  screen_mode = PLAYING_SCREEN_MODE;
  draw_playing_screen();
  wmove(inputwin, talk_y, talk_x);
  wrefresh(inputwin);
}

void init_global_screen() {
  comment_left = comment_x = org_comment_x;
  comment_right = 72;
  comment_up = 0;
  comment_bottom = 21;
  comment_y = org_comment_y;
  talk_left = 11;
  talk_right = 74;
  talk_x = 0;
  talk_buf_count = 0;
  talk_buf[0] = '\0';
  screen_mode = GLOBAL_SCREEN_MODE;
  commentwin = newwin(22, 74, 0, 2);
  scrollok(commentwin, TRUE);
  inputwin = newwin(1, talk_right - talk_left, org_talk_y, talk_left);
  draw_global_screen();
  input_mode = TALK_MODE;
  talk_left = 11;
  inputwin = global_win;
  wmvaddstr(stdscr, 23, 2, "【對話】");
  return_cursor();
}

void draw_table() {
  int x, y;

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
  clear();
  draw_title();
}

void draw_playing_screen() {
  int x, y;
  clear();
  /* Draw outline */
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
  /* Draw table outline */
  /* Information section */
  wmvaddstr(stdscr, 1, 56, "├");
  for (x = 58; x <= 74; x += 2) waddstr(stdscr, "─");
  waddstr(stdscr, "┤");
  wmvaddstr(stdscr, 3, 56, "├");
  for (x = 58; x <= 74; x += 2) waddstr(stdscr, "─");
  waddstr(stdscr, "┤");
  wmvaddstr(stdscr, 12, 56, "├");
  for (x = 58; x <= 74; x += 2) waddstr(stdscr, "─");
  waddstr(stdscr, "┤");
  wmvaddstr(stdscr, 0, 66, "│");
  wmvaddstr(stdscr, 1, 66, "┼");
  wmvaddstr(stdscr, 2, 66, "│");
  wmvaddstr(stdscr, 3, 66, "┴");
  /* Characters */
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
  wmvaddstr(stdscr, 14, 60, "無 吃 碰 槓 胡");
  wmvaddstr(stdscr, 20, 64, "┌  ┐");
  wmvaddstr(stdscr, 22, 64, "└  ┘");
  wrefresh(stdscr);
  wmove(inputwin, talk_y, talk_x);
  wrefresh(inputwin);
}

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

void display_point(int current_turn) {
  static int last_turn = 0;

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

void display_time(int sit) {
  char msg_buf[255];
  char pos;
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

void display_info() {
  int i;

  wmvaddstr(stdscr, 0, 58, "  ");
  wmvaddstr(stdscr, 0, 62, "  ");
  wmvaddstr(stdscr, 0, 70, "    ");
  wrefresh(stdscr);
  wmvaddstr(stdscr, 0, 58, sit_name[info.wind]);
  wmvaddstr(stdscr, 0, 62, sit_name[info.dealer]);
  if (info.cont_dealer < 10)
    show_num(0, 71, info.cont_dealer, 1);
  else
    show_num(0, 70, info.cont_dealer, 2);
  wmvaddstr(stdscr, 22, 66, sit_name[my_sit]);
  wmvaddstr(stdscr, 21, 68, sit_name[(my_sit) % 4 + 1]);
  wmvaddstr(stdscr, 20, 66, sit_name[(my_sit + 1) % 4 + 1]);
  wmvaddstr(stdscr, 21, 64, sit_name[(my_sit + 2) % 4 + 1]);

  char name_buf[40];
  snprintf(name_buf, sizeof(name_buf), "%s%s", player[table[EAST]].name,
           player[table[EAST]].is_ai ? "(AI)" : "");
  wmvaddstr(stdscr, 4, 64, name_buf);
  snprintf(name_buf, sizeof(name_buf), "%s%s", player[table[SOUTH]].name,
           player[table[SOUTH]].is_ai ? "(AI)" : "");
  wmvaddstr(stdscr, 6, 64, name_buf);
  snprintf(name_buf, sizeof(name_buf), "%s%s", player[table[WEST]].name,
           player[table[WEST]].is_ai ? "(AI)" : "");
  wmvaddstr(stdscr, 8, 64, name_buf);
  snprintf(name_buf, sizeof(name_buf), "%s%s", player[table[NORTH]].name,
           player[table[NORTH]].is_ai ? "(AI)" : "");
  wmvaddstr(stdscr, 10, 64, name_buf);

  for (i = 1; i <= 4; i++) {
    display_time(i);
  }
  mvwprintw(stdscr, 5, 64, "         ");
  mvwprintw(stdscr, 5, 64, "%ld", player[table[EAST]].money);
  mvwprintw(stdscr, 7, 64, "         ");
  mvwprintw(stdscr, 7, 64, "%ld", player[table[SOUTH]].money);
  mvwprintw(stdscr, 9, 64, "         ");
  mvwprintw(stdscr, 9, 64, "%ld", player[table[WEST]].money);
  mvwprintw(stdscr, 11, 64, "         ");
  mvwprintw(stdscr, 11, 64, "%ld", player[table[NORTH]].money);
  mvwprintw(stdscr, 4, 74, "  ");
  mvwprintw(stdscr, 6, 74, "  ");
  mvwprintw(stdscr, 8, 74, "  ");
  mvwprintw(stdscr, 10, 74, "  ");
  attron(A_REVERSE);
  mvwprintw(stdscr, 2 + info.dealer * 2, 74, "莊");
  attroff(A_REVERSE);

  wmove(stdscr, 12, 60);
  if (ai_is_enabled()) {
    attron(A_REVERSE);
    waddstr(stdscr, " AI ");
    attroff(A_REVERSE);
  } else {
    waddstr(stdscr, "    ");
  }

  wrefresh(stdscr);
  wmove(inputwin, talk_y, talk_x);
  wrefresh(inputwin);
}

int more_size = 0, more_num = 0;
char more_buf[4096];
int readln(int fd, char* buf, int* end_flag) {
  int len, bytes, ch;

  len = bytes = 0;
  *end_flag = 0;
  while (1) {
    if (more_num >= more_size) {
      more_size = (int)read(fd, more_buf, 1);
      if (more_size == 0) {
        break;
      }
      more_num = 0;
    }
    ch = more_buf[more_num++];
    bytes++;
    if (ch == '\n' || len >= 74) {
      break;
    } else if (ch == '\0') {
      *end_flag = 1;
      *buf = '\0';
      return bytes;
    } else {
      len++;
      *buf++ = (char)ch;
    }
  }
  *buf++ = (char)ch;
  *buf = '\0';
  return bytes;
}

void display_news(int fd) {
  char buf[256];
  int line_count;
  int end_flag;
  WINDOW* news_win;

  line_count = 0;
  news_win = newwin(22, 74, 0, 2);
  wclear(news_win);
  wmove(news_win, 0, 0);
  while (readln(fd, buf, &end_flag)) {
    if (end_flag == 1) break;
    if (line_count == 22) {
      wrefresh(news_win);
      wait_a_key("請按 <ENTER> 至下一頁......");
      line_count = 0;
      wclear(news_win);
      wmove(news_win, 0, 0);
    }
    printstr(news_win, buf);
    wrefresh(news_win);
    line_count++;
  }
  wrefresh(news_win);
  wait_a_key("請按 <ENTER> 繼續.......");
  delwin(news_win);
  redraw_screen();
}

void display_comment(char* comment) {
  waddstr(commentwin, "\n");
  wattrset(commentwin, COLOR_PAIR(1));
  wattron(commentwin, COLOR_PAIR(1));
  printstr(commentwin, comment);
  wrefresh(commentwin);
  return_cursor();
}

void send_talk_line(char* talk)  // User Talks
{
  char comment[255];
  cJSON* payload;
  int i;

  snprintf(comment, sizeof(comment), "<%s> ", my_name);
  strncat(comment, talk, sizeof(comment) - strlen(comment) - 1);
  display_comment(comment);

  /* 102 MSG_ACTION_CHAT */
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", comment);

  if (in_serv) {
    /* Broadcast to all clients (except 1) */
    for (i = 2; i < MAX_PLAYER; i++) {
      if (player[i].in_table && i != 1) {
        send_json(player[i].sockfd, MSG_ACTION_CHAT,
                  cJSON_Duplicate(payload, 1));
      }
    }
  }
  if (in_join) {
    /* Send to table server */
    send_json(table_sockfd, MSG_ACTION_CHAT, payload);
  } else {
    cJSON_Delete(payload);
  }
}

void send_gps_line(char* msg) {
  char comment[255];
  strncpy(comment, msg, sizeof(comment) - 1);
  comment[sizeof(comment) - 1] = '\0';
  display_comment(comment);
}

int intlog10(int num) {
  int i;

  i = 0;
  do {
    num /= 10;
    if (num >= 1) i++;
  } while (num >= 1);
  return (i);
}

void convert_num(char* str, int number, int digit) {
  int i;
  int tmp[10];
  str[0] = '\0';
  for (i = digit - 1; i >= 0; i--) {
    tmp[i] = number % 10;
    number /= 10;
  }
  for (i = 0; i < digit; i++) strcat(str, number_item[tmp[i]]);
}

void show_num(int y, int x, int number, int digit) {
  int i;
  char msg_buf[255];
  wmove(stdscr, y, x);
  for (i = 0; i < digit; i++) waddstr(stdscr, "  ");
  wrefresh(stdscr);
  convert_num(msg_buf, number, digit);
  wmvaddstr(stdscr, y, x, msg_buf);
  wrefresh(stdscr);
}

void show_cardmsg(int sit, int card) {
  int pos;

  pos = (sit - my_sit + 4) % 4;
  clear_screen_area(15, 58, 3, 18);
  if (card) {
    wmvaddstr(stdscr, 15, 58, "┌───────┐");
    wmvaddstr(stdscr, 16, 58, "│              │");
    wmvaddstr(stdscr, 17, 58, "└───────┘");
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
    wmvaddstr(stdscr, 16, 62, "家打（    ）");
    wrefresh(stdscr);
    show_card(card, 68, 16, 0);
  }
  return_cursor();
}

void redraw_screen() {
  clearok(stdscr, TRUE);
  wrefresh(stdscr);
  touchwin(commentwin);
  wrefresh(commentwin);
  touchwin(inputwin);
  wrefresh(inputwin);
  return_cursor();
}

void reset_cursor() {
  mvwaddstr(stdscr, 23, 75, " ");
  wrefresh(stdscr);
}

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