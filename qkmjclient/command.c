#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "mjdef.h"

#include <cjson/cJSON.h>

#include "ai_client.h"
#include "misc.h"
#include "protocol.h"
#include "protocol_def.h"
#include "qkmj.h"

#define TABLE 1
#define LIST 2
#define PLAYER 3
#define JOIN 4
#define SERV 5
#define QUIT 6
#define EXIT 7
#define WHO 8
#define SIT 9
#define NUM 10
#define NEW 11
#define SHOW 12
#define LEAVE 13
#define HELP 14
#define NOTE 15
#define STAT 16
#define LOGIN 17
#define PASSWD 18
#define preserve1 19
#define BROADCAST 20
#define KICK 21
#define KILL 22
#define INVITE 23
#define MSG 24
#define SHUTDOWN 25
#define LURKER 26
#define FIND 27
#define BEEP 28
#define EXEC 29
#define FREE 30
#define S_PLAYER 31
#define S_JOIN 32
#define S_HELP 33
#define S_WHO 34
#define S_SERV 35
#define S_LEAVE 36
#define S_TABLE 37
#define S_QUIT 38
#define AI_CMD 39

int enable_kick = 1;
int enable_exec = 0;
char commands[100][10] = {
    "",     "TABLE", "LIST",   "PLAYER", "JOIN",     "SERV",      "QUIT",
    "EXIT", "WHO",   "SIT",    "NUM",    "NEW",      "SHOW",      "LEAVE",
    "HELP", "NOTE",  "STAT",   "LOGIN",  "PASSWD",   "preserve1", "BROADCAST",
    "KICK", "KILL",  "INVITE", "MSG",    "SHUTDOWN", "LURKER",    "FIND",
    "BEEP", "EXEC",  "FREE",   "P",      "J",        "H",         "W",
    "S",    "L",     "T",      "Q",      "AI"};

void Tokenize(char* strinput) {
  int Ltok;
  char* token;
  char str[255];
  narg = 1;
  strncpy(str, strinput, sizeof(str) - 1);
  str[sizeof(str) - 1] = '\0';
  token = strtok(str, " \n\t\r");
  if (token == NULL) {
    narg = 0;
    return;
  }
  Ltok = (int)strlen(token);
  strncpy((char*)cmd_argv[narg], token, sizeof(cmd_argv[narg]) - 1);
  cmd_argv[narg][sizeof(cmd_argv[narg]) - 1] = '\0';
  arglenv[narg] = Ltok;
  while (1) {
    token = strtok(NULL, " \n\t\r");
    if (token == NULL) break;
    narg++;
    Ltok = (int)strlen(token);
    strncpy((char*)cmd_argv[narg], token, sizeof(cmd_argv[narg]) - 1);
    cmd_argv[narg][sizeof(cmd_argv[narg]) - 1] = '\0';
    arglenv[narg] = Ltok;
  }
}

void my_strupr(char* upper, char* org) {
  int len = (int)strlen(org);
  for (int i = 0; i < len; i++) {
    upper[i] = (char)toupper(org[i]);
  }
  upper[len] = '\0';
}

int command_mapper(char* cmd) {
  char cmd_upper[255];
  my_strupr(cmd_upper, cmd);
  int i = 1;
  while (commands[i][0] != '\0') {
    if (strcmp(cmd_upper, commands[i]) == 0) return (i);
    i++;
  }
  return 0;
}

void who(char* name) {
  char msg_buf[255];
  cJSON* payload;

  if (name[0] != 0) {
    payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "table_leader", name);
    send_json(gps_sockfd, MSG_WHO_IN_TABLE, payload);
    return;
  }
  if (!in_join && !in_serv) {
    display_comment("你要查看那一桌?");
    return;
  }
  display_comment("----------------   此桌使用者   ------------------");
  msg_buf[0] = 0;
  for (int i = 1; i < MAX_PLAYER; i++) {
    if (strlen(msg_buf) > 49) {
      display_comment(msg_buf);
      msg_buf[0] = 0;
    }
    if (player[i].in_table) {
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) - 1);
      strncat(msg_buf, "  ", sizeof(msg_buf) - strlen(msg_buf) - 1);
    }
  }
  if (msg_buf[0] != 0) display_comment(msg_buf);
  display_comment("--------------------------------------------------");
}

void help() {
  send_gps_line("-----------------   使用說明   -------------------");
  send_gps_line("/HELP          /H   查看使用說明");
  send_gps_line("/TABLE         /T   查看所有的桌");
  send_gps_line("/FREE               查看目前人數未滿的桌");
  send_gps_line("/PLAYER        /P   查看目前在線上的使用者");
  send_gps_line("/LURKER             查看閑置的使用者");
  send_gps_line("/FIND <名稱>        找尋此使用者");
  send_gps_line("/WHO <名稱>    /W   查看此桌的使用者");
  send_gps_line("/SERV          /S   開桌");
  send_gps_line("/JOIN <名稱>   /J   加入一桌");
  send_gps_line("/MSG <名稱> <訊息>  送訊息給特定使用者");
  send_gps_line("/INVITE <名稱>      邀請使用者到此桌");
  send_gps_line("/KICK <名稱>        將使用者踢出此桌 (桌長才可用)");
  send_gps_line("/NOTE <附註>        更改此桌的附註");
  send_gps_line("/LEAVE         /L   離開一桌");
  send_gps_line("/STAT <名稱>        看使用者狀態");
  send_gps_line("/BEEP [ON|OFF]      設定聲音開關");
  send_gps_line("/PASSWD             更改密碼");
  send_gps_line("/QUIT          /Q   離開");
  send_gps_line("--------------------------------------------------");
}

void command_parser(char* msg) {
  int cmd_id;
  char msg_buf[255];
  char ans_buf[255];
  char ans_buf1[255];
  cJSON* payload;

  if (msg[0] == '/') {
    Tokenize(msg + 1);
    cmd_id = command_mapper((char*)cmd_argv[1]);
    switch (cmd_id) {
      case 0:
        send_gps_line("沒有這個指令");
        break;
      case TABLE:
      case S_TABLE:
        send_json(gps_sockfd, MSG_LIST_TABLES, NULL);
        break;
      case FREE:
        send_json(gps_sockfd, MSG_LIST_TABLES_FREE, NULL);
        break;
      case LIST:
      case PLAYER:
      case S_PLAYER:
        send_json(gps_sockfd, MSG_LIST_PLAYERS, NULL);
        break;
      case JOIN:
      case S_JOIN:
        if (!pass_login) {
          display_comment("請先 login 一個名稱");
          break;
        }
        if (in_join || in_serv) {
          display_comment("請先離開目前桌，才能加入別桌。(/Leave)");
          break;
        }
        clear_variable();
        if (in_join) {
          close_join();
          send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
          init_global_screen();
        }
        if (in_serv) {
          close_serv();
          send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
          init_global_screen();
        }
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "table_leader", (char*)cmd_argv[2]);
        send_json(gps_sockfd, MSG_JOIN_TABLE, payload);
        break;
      case SERV:
      case S_SERV:
        if (!pass_login) {
          display_comment("請先 login 一個名稱");
          break;
        }
        if (in_join || in_serv) {
          display_comment("請先離開此桌");
          break;
        }
        clear_variable();
        if (in_join) {
          close_join();
          send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
          init_global_screen();
        }
        if (in_serv) {
          close_serv();
          send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
          init_global_screen();
        }
        send_json(gps_sockfd, MSG_CHECK_OPEN, NULL);
        break;
      case QUIT:
      case S_QUIT:
      case EXIT:
        leave();
        break;
      case SHOW:
        break;
      case WHO:
      case S_WHO:
        if (narg == 2)
          who((char*)cmd_argv[2]);
        else
          who("");
        break;
      case NUM: {
        int i = cmd_argv[2][0] - '0';
        if (i >= 1 && i <= 4) PLAYER_NUM = i;
        break;
      }
      case NEW:
        if (in_serv) {
          /* Broadcast NEW ROUND */
          for (int i = 2; i < MAX_PLAYER; i++) {
            if (player[i].in_table && i != 1) { /* 1 is server (me) */
              send_json(player[i].sockfd, MSG_NEW_ROUND, NULL);
            }
          }
          opening();
          open_deal();
        } else {
          send_json(table_sockfd, MSG_NEW_ROUND, NULL);
          opening();
        }
        break;
      case LEAVE:
      case S_LEAVE:
        if (in_join) {
          in_join = 0;
          close_join();
          send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
          init_global_screen();
          send_json(gps_sockfd, MSG_STATUS, NULL);
          display_comment("您已離開牌桌");
          display_comment("-------------------");
        }
        if (in_serv) {
          in_serv = 0;
          close_serv();
          send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
          init_global_screen();
          send_json(gps_sockfd, MSG_STATUS, NULL);
          display_comment("您已關閉牌桌");
          display_comment("-------------------");
        }
        input_mode = TALK_MODE;
        break;
      case HELP:
      case S_HELP:
        help();
        break;
      case NOTE:
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "note", msg + 6);
        send_json(gps_sockfd, MSG_SET_NOTE, payload);
        break;
      case STAT:  // /STAT
        if (narg < 2)
          strncpy((char*)cmd_argv[2], (char*)my_name, sizeof(cmd_argv[2]) - 1);
        cmd_argv[2][sizeof(cmd_argv[2]) - 1] = '\0';

        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "name", (char*)cmd_argv[2]);
        send_json(gps_sockfd, MSG_USER_INFO, payload);
        break;
      case LOGIN:
        break;
      case BROADCAST:
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "msg", msg + 11);
        send_json(gps_sockfd, MSG_BROADCAST, payload);
        break;
      case MSG:
        if (narg <= 2) break;
        if (in_join || in_serv) {
          for (int i = 1; i <= 4; i++) {
            if (table[i] &&
                strcmp((char*)cmd_argv[2], player[table[i]].name) == 0) {
              snprintf(msg_buf, sizeof(msg_buf), "%s",
                       msg + 5 + strlen((char*)cmd_argv[2]) + 1);
              send_talk_line(msg_buf);
              goto finish_msg;
            }
          }
        }
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "to", (char*)cmd_argv[2]);
        cJSON_AddStringToObject(payload, "msg", msg + 5);
        {
          char* p = msg + 5;
          while (*p && *p != ' ') p++;
          if (*p == ' ') p++; /* Skip space */
          /* p now points to message part */
          cJSON_ReplaceItemInObject(payload, "msg", cJSON_CreateString(p));
        }
        send_json(gps_sockfd, MSG_SEND_MESSAGE, payload);

        snprintf(msg_buf, sizeof(msg_buf), "-> *%s* %s", (char*)cmd_argv[2],
                 msg + 5 + strlen((char*)cmd_argv[2]) + 1);
        msg_buf[talk_right] = 0;
        display_comment(msg_buf);
      finish_msg:;
        break;
      case SHUTDOWN:
        send_json(gps_sockfd, MSG_SHUTDOWN, NULL);
        break;
      case LURKER:
        send_json(gps_sockfd, MSG_LURKER_LIST, NULL);
        break;
      case FIND:
        if (narg < 2) {
          display_comment("你要找誰呢?");
          break;
        }
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "name", (char*)cmd_argv[2]);
        send_json(gps_sockfd, MSG_FIND_USER, payload);
        break;
      case EXEC:
        break;
      case BEEP:
        if (narg < 2) {
          snprintf(msg_buf, sizeof(msg_buf), "目前聲音設定為%s",
                   (set_beep == 1) ? "開啟" : "關閉");
          display_comment(msg_buf);
        } else {
          my_strupr(ans_buf, (char*)cmd_argv[2]);
          if (strcmp(ans_buf, "ON") == 0) {
            set_beep = 1;
            display_comment("開啟聲音");
          }
          if (strcmp(ans_buf, "OFF") == 0) {
            set_beep = 0;
            display_comment("關閉聲音");
          }
        }
        break;
      case PASSWD:
        ans_buf[0] = 0;
        ask_question("請輸入你原來的密碼：", ans_buf, 8, 0);
        ans_buf[8] = 0;
        if (strcmp((char*)my_pass, ans_buf) != 0) {
          wait_a_key("密碼錯誤,更改失敗!");
          break;
        }
        ans_buf[0] = 0;
        ask_question("請輸入你要更改的密碼：", ans_buf, 8, 0);
        ans_buf1[0] = 0;
        ask_question("請再輸入一次確認：", ans_buf1, 8, 0);
        ans_buf[8] = 0;
        ans_buf1[8] = 0;
        if (strcmp(ans_buf, ans_buf1) == 0) {
          payload = cJSON_CreateObject();
          cJSON_AddStringToObject(payload, "new_password", ans_buf);
          send_json(gps_sockfd, MSG_CHANGE_PASSWORD, payload);

          strncpy((char*)my_pass, ans_buf, sizeof(my_pass) - 1);
          my_pass[sizeof(my_pass) - 1] = '\0';
          wait_a_key("密碼已更改!");
        } else {
          wait_a_key("兩次密碼不同,更改失敗!");
        }
        break;
      case KICK:
        if (!enable_kick) break;
        if (in_serv) {
          if (narg < 2) {
            display_comment("要把誰踢出去呢?");
          } else {
            if (strcmp((char*)my_name, (char*)cmd_argv[2]) == 0) {
              display_comment("抱歉, 自己不能踢自己");
              break;
            }
            for (int i = 2; i < MAX_PLAYER; i++) {
              if (player[i].in_table &&
                  strcmp(player[i].name, (char*)cmd_argv[2]) == 0) {
                snprintf(msg_buf, sizeof(msg_buf), "101%s 被踢出此桌",
                         cmd_argv[2]);
                display_comment(msg_buf + 3);

                /* Broadcast kick msg */
                for (int k = 2; k < MAX_PLAYER; k++) {
                  if (player[k].in_table && k != 1) {
                    payload = cJSON_CreateObject();
                    cJSON_AddStringToObject(payload, "text", msg_buf + 3);
                    send_json(player[k].sockfd, MSG_TEXT_MESSAGE, payload);
                  }
                }

                send_json(player[i].sockfd, MSG_LEAVE, NULL);
                close_client(i);
                goto finish_kick;
              }
            }
            display_comment("此桌沒有這個人");
          }
        } else {
          display_comment("此命令只有桌長可用");
        }
      finish_kick:;
        break;
      case KILL:
        if (narg >= 2) {
          payload = cJSON_CreateObject();
          cJSON_AddStringToObject(payload, "name", (char*)cmd_argv[2]);
          send_json(gps_sockfd, MSG_KICK_USER, payload);
        }
        break;
      case INVITE:
        if (narg < 2) {
          display_comment("你打算邀請誰?");
          break;
        }
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "name", (char*)cmd_argv[2]);
        send_json(gps_sockfd, MSG_INVITE, payload);

        snprintf(msg_buf, sizeof(msg_buf), "邀請 %s 加入此桌",
                 (char*)cmd_argv[2]);
        display_comment(msg_buf);
        break;
      case AI_CMD:
        if (narg < 2) {
          snprintf(msg_buf, sizeof(msg_buf), "目前 AI 模式為%s",
                   (ai_is_enabled()) ? "開啟" : "關閉");
          display_comment(msg_buf);
        } else {
          my_strupr(ans_buf, (char*)cmd_argv[2]);
          int enabled = -1;
          if (strcmp(ans_buf, "ON") == 0) {
            enabled = 1;
            display_comment("開啟 AI 模式");
          } else if (strcmp(ans_buf, "OFF") == 0) {
            enabled = 0;
            display_comment("關閉 AI 模式");
          }

          if (enabled != -1) {
            ai_set_enabled(enabled);
            payload = cJSON_CreateObject();
            cJSON_AddNumberToObject(payload, "sit", my_sit);
            cJSON_AddNumberToObject(payload, "enabled", enabled);

            if (in_serv) {
              player[my_id].is_ai = enabled;
              /* Broadcast */
              for (int i = 2; i < MAX_PLAYER; i++) {
                if (player[i].in_table && i != my_id) {
                  send_json(player[i].sockfd, MSG_AI_MODE,
                            cJSON_Duplicate(payload, 1));
                }
              }
              cJSON_Delete(payload);
            } else if (in_join) {
              send_json(table_sockfd, MSG_AI_MODE, payload);
            } else {
              cJSON_Delete(payload);
            }
          }
        }
        break;
      default:
        err("Unknow command id");
        display_comment(msg);
        break;
    }
  } else {
    send_talk_line(msg);
  }
}
