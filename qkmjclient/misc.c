#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "curses.h"
#include "mjdef.h"
#include "qkmj.h"
#define NO_SUN_HP 1

/* Prototypes */
int my_getch();

float thinktime() {
  float t;
  char msg_buf[80];

  gettimeofday(&after, (struct timezone*)0);
  after.tv_sec -= before.tv_sec;
  after.tv_usec -= before.tv_usec;
  if (after.tv_usec < 0) {
    after.tv_sec--;
    after.tv_usec += 1000000;
  }
  t = (float)after.tv_sec + (float)after.tv_usec / 1000000;
  return (t);
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
  int i;

  keypad(win, TRUE);
  meta(win, TRUE);
  org_y = y;
  org_x = x;
  wmvaddstr(win, y, x, (char*)str_buf);
  wrefresh(win);
  x = org_x + strlen((char*)str_buf);
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
        for (i = 0; i < x - org_x; i++) waddch(win, ' ');
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
        str_buf[x - org_x] = ch;
        str_buf[x + 1 - org_x] = 0;
        if (mode == 0)
          mvwaddstr(win, y, x++, "*");
        else {
          ch_buf[0] = ch;
          ch_buf[1] = 0;
          mvwaddstr(win, y, x++, (char*)ch_buf);
        }
        wrefresh(win);
        break;
    }
  }
}