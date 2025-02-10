#include "misc.h"

// 計算思考時間 (計算 gettimeofday() 兩次呼叫之間的時間差)
float thinktime(struct timeval before)
{
  struct timeval after;
  float t;
  char error_message[1024]; // For error messages

  if (gettimeofday(&after, NULL) == -1) {
    snprintf(error_message, sizeof(error_message), 
             "gettimeofday 失敗: %s", strerror(errno));
    err(error_message);
    return -1.0f; // Indicate an error
  }

  t = (float)(after.tv_sec - before.tv_sec) + 
      (float)(after.tv_usec - before.tv_usec) / 1000000.0f;

  return t;
}

// 從 ncurses 視窗讀取字串 (提供更進階的輸入功能，例如退格鍵和清除鍵)
void mvwgetstring(WINDOW *win, int y, int x, int max_len, 
                  char *str_buf, int mode)
{
  int ch;
  int org_x = x; // Store original x coordinate

  keypad(win, TRUE); // Enable keypad input
  echo(); // Enable echoing of input
  curs_set(1); // Show cursor
  
  wmvaddstr(win, y, x, str_buf);
  wrefresh(win);
  x += strlen(str_buf); // Move cursor to end

  while (1) {
    ch = getch();
    switch (ch) {
    case KEY_BACKSPACE:
    case '\b': // Backspace
      if (x > org_x) {
        x--;
        str_buf[x - org_x] = '\0'; // Null terminate at correct position
        mvwaddch(win, y, x, ' ');
        wmove(win, y, x);
        wrefresh(win);
      }
      break;
    case KEY_DL: // Delete
      if (x > org_x) {
        str_buf[x - org_x] = '\0'; // Null terminate at correct position
        mvwaddch(win, y, x, ' ');
        wrefresh(win);
      }
      break;
    case '\n': // Enter key
    case '\r':
      return;
    default:
      if (x - org_x >= max_len) break;
      str_buf[x - org_x] = ch;
      str_buf[x - org_x + 1] = '\0'; // Corrected null termination
      if (mode == 0) {
        mvwaddstr(win, y, x++, "*");
      } else {
        char ch_buf[2]; // Declare ch_buf
        ch_buf[0] = ch;
        ch_buf[1] = '\0';
        mvwaddstr(win, y, x++, ch_buf);
      }
      wrefresh(win);
      break;
    }
  }
}
