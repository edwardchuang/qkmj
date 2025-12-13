#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "mjdef.h"
#include "qkmj.h"

#include <cjson/cJSON.h>
#include "protocol_def.h"
#include "protocol.h"

/* Prototypes */
int my_getch();

void send_game_log(const char *action, int card, const char *extra) {
    if (gps_sockfd <= 0) return;
    cJSON *record = cJSON_CreateObject();
    cJSON_AddStringToObject(record, "action", action);
    // my_username is global from qkmj.h
    cJSON_AddStringToObject(record, "player", my_username);
    cJSON_AddNumberToObject(record, "card", card);
    if (extra) {
        cJSON_AddStringToObject(record, "info", extra);
    }
    send_json(gps_sockfd, MSG_GAME_RECORD, record);
}

float thinktime() {
  gettimeofday(&after, (struct timezone*)0);
  after.tv_sec -= before.tv_sec;
  after.tv_usec -= before.tv_usec;
  if (after.tv_usec < 0) {
    after.tv_sec--;
    after.tv_usec += 1000000;
  }
  float t = (float)after.tv_sec + (float)after.tv_usec / 1000000;
  return t;
}

void beep1() {
  if (set_beep) beep();
}

void wmvaddstr(WINDOW* win, int y, int x, char* str) {
  mvwaddstr(win, y, x, str);
}

void mvwgetstring(WINDOW* win, int y, int x, int max_len,
                  unsigned char* str_buf, int mode) {
  int ch;
  unsigned char ch_buf[2];
  int org_x, org_y;

  keypad(win, TRUE);
  meta(win, TRUE);
  org_y = y;
  org_x = x;
  wmvaddstr(win, y, x, (char*)str_buf);
  wrefresh(win);
  x = org_x + (int)strlen((char*)str_buf);
  while (1) {
    ch = my_getch();
    switch (ch) {
      case KEY_UP:
      case KEY_DOWN:
      case KEY_LEFT:
      case KEY_RIGHT:
        break;
      case BACKSPACE:
      case KEY_BACKSPACE: /* ncurses */
      case CTRL_H:
        if (x > org_x) {
          x--;
          str_buf[x - org_x] = 0;
          mvwaddch(win, y, x, ' ');
          wmove(win, y, x);
          wrefresh(win);
        }
        break;
      case CTRL_U:
        wmove(win, y, org_x);
        for (int i = 0; i < x - org_x; i++) waddch(win, ' ');
        wmove(win, y, org_x);
        str_buf[0] = 0;
        x = org_x;
        wrefresh(win);
        break;
      case KEY_ENTER:
      case ENTER:
        return;
        break;
      default:
        if (x - org_x >= max_len) break;
        str_buf[x - org_x] = (unsigned char)ch;
        str_buf[x + 1 - org_x] = 0;
        if (mode == 0)
          mvwaddstr(win, y, x++, "*");
        else {
          ch_buf[0] = (unsigned char)ch;
          ch_buf[1] = 0;
          mvwaddstr(win, y, x++, (char*)ch_buf);
        }
        wrefresh(win);
        break;
    }
  }
}
