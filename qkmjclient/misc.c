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
#include "ai_client.h"

#include <cjson/cJSON.h>
#include "protocol_def.h"
#include "protocol.h"

/* Prototypes */
int my_getch();

void send_game_log(const char* action, int card, cJSON* extra) {
  if (current_match_id[0] == '\0') return;

  cJSON* log = cJSON_CreateObject();
  cJSON_AddStringToObject(log, "level", "game_trace");
  cJSON_AddStringToObject(log, "match_id", current_match_id);
  cJSON_AddNumberToObject(log, "move_serial", ++move_serial);
  cJSON_AddStringToObject(log, "user_id", (char*)my_name);
  cJSON_AddNumberToObject(log, "sit", my_sit);
  cJSON_AddBoolToObject(log, "is_ai", ai_is_enabled());
  if (ai_is_enabled()) {
    cJSON_AddStringToObject(log, "ai_session_id", ai_get_session_id());
  }
  cJSON_AddStringToObject(log, "action", action);
  cJSON_AddNumberToObject(log, "card", card);
  cJSON_AddNumberToObject(log, "timestamp", (double)time(NULL) * 1000.0);

  /* Serialize current state */
  char* state_json = ai_serialize_state(play_mode == THROW_CARD ? AI_PHASE_DISCARD : AI_PHASE_CLAIM, card, card_owner);
  if (state_json) {
    cJSON* state_obj = cJSON_Parse(state_json);
    if (state_obj) {
      cJSON_AddItemToObject(log, "state", state_obj);
    }
    free(state_json);
  }

  if (extra) {
    cJSON_AddItemToObject(log, "extra", cJSON_Duplicate(extra, 1));
  }

  send_json(gps_sockfd, 901, log);
}

void write_check(int check) {
  const char *action_str = "Pass";
  if (check == EAT || (check >= 7 && check <= 9)) action_str = "Eat";
  else if (check == PONG) action_str = "Pong";
  else if (check == KANG) action_str = "Kang";
  else if (check == MAKE) action_str = "Win";
  else if (check == TING) action_str = "Ting";
  
  if (check == 0) {
      cJSON *avail = cJSON_CreateObject();
      if (check_flag[my_sit][1]) cJSON_AddBoolToObject(avail, "can_eat", true);
      if (check_flag[my_sit][2]) cJSON_AddBoolToObject(avail, "can_pong", true);
      if (check_flag[my_sit][3]) cJSON_AddBoolToObject(avail, "can_kang", true);
      if (check_flag[my_sit][4]) cJSON_AddBoolToObject(avail, "can_win", true);
      send_game_log(action_str, current_card, avail);
      cJSON_Delete(avail);
  } else {
      send_game_log(action_str, current_card, NULL);
  }

  if (in_join) {
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "action", check);
    send_json(table_sockfd, MSG_CHECK_RESULT, payload);
  } else {
    in_check[my_sit] = 0;
    check_for[my_sit] = check;
  }
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
