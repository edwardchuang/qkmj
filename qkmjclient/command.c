
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>

#include "qkmj.h"
#include "command.h"

// 指令名稱與 ID 的映射表
const CommandMapping commandMappings[] = {
  {"", CMD_UNKNOWN},
  {"TABLE", CMD_TABLE}, {"LIST", CMD_LIST}, {"PLAYER", CMD_PLAYER},
  {"JOIN", CMD_JOIN}, {"SERV", CMD_SERV}, {"QUIT", CMD_QUIT},
  {"EXIT", CMD_EXIT}, {"WHO", CMD_WHO}, {"SIT", CMD_SIT}, {"NUM", CMD_NUM},
  {"NEW", CMD_NEW}, {"SHOW", CMD_SHOW}, {"LEAVE", CMD_LEAVE},
  {"HELP", CMD_HELP}, {"NOTE", CMD_NOTE}, {"STAT", CMD_STAT},
  {"LOGIN", CMD_LOGIN}, {"PASSWD", CMD_PASSWD}, {"preserve1", CMD_UNKNOWN},
  {"BROADCAST", CMD_BROADCAST}, {"KICK", CMD_KICK}, {"KILL", CMD_KILL},
  {"INVITE", CMD_INVITE}, {"MSG", CMD_MSG}, {"SHUTDOWN", CMD_SHUTDOWN},
  {"LURKER", CMD_LURKER}, {"FIND", CMD_FIND}, {"BEEP", CMD_BEEP},
  {"EXEC", CMD_EXEC}, {"FREE", CMD_FREE}, {"P", CMD_S_PLAYER},
  {"J", CMD_S_JOIN}, {"H", CMD_S_HELP}, {"W", CMD_S_WHO},
  {"S", CMD_S_SERV}, {"L", CMD_S_LEAVE}, {"T", CMD_S_TABLE},
  {"Q", CMD_S_QUIT},
  {NULL, CMD_UNKNOWN} //  標記陣列結束
};

#define MAX_ARGS 40

extern int arglenv[MAX_ARGS]; // 新增儲存參數長度的陣列

//  將輸入字串分割成 tokens
void Tokenize(const char* strinput) {
  char *str;
  const char* delimiters = " \n\t\r"; //  定義分隔符號
  char* token;
  size_t input_len = strlen(strinput);
  if (input_len >= 255) {
    err("指令過長");
    narg = 0;
    return;
  }

  str = (char*)malloc(input_len + 1); // +1 for null terminator
  if (str == NULL) {
      err("Error: Memory allocation failed\n");
      narg = 0;
      return;
  }

  memcpy(str, strinput, input_len + 1);
  narg = 0;
  token = strtok(str, delimiters);
  while (token != NULL && narg < MAX_ARGS) {
    int len = strlen(token);
    if (len >= sizeof((char *)cmd_argv[narg])) {
      free(str);
      err("指令參數過長");
      narg = 0;
      return;
    }
    snprintf((char *)cmd_argv[narg], sizeof(cmd_argv[narg]), "%s", token);
    arglenv[narg] = len;
    narg++;
    token = strtok(NULL, delimiters);
  }
  //  處理超過 MAX_ARGS 的參數
  if (narg >= MAX_ARGS) {
    err("指令參數過多");
    narg = 0;
  }
  free(str);
}

//  將字串轉換為大寫
void my_strupr(char* upper, const char* org) {
  for (const char* p = org; *p; ++p) {
      *upper++ = toupper((unsigned char)*p);
  }
  *upper = '\0'; // Add the null terminator manually
}

//  指令名稱映射函數 (使用更有效率的查找方式)
static CommandId command_mapper(const char* cmd) {
  char cmd_upper[255];
  my_strupr(cmd_upper, cmd);
  for (int i = 0; commandMappings[i].name != NULL; ++i) {
    if (strcmp(cmd_upper, commandMappings[i].name) == 0) {
      return commandMappings[i].id;
    }
  }
  return CMD_UNKNOWN;
}

//  顯示使用者列表函數 (和先前程式碼相同)
void who(const char* name) {
  char msg_buf[255];
  int i;

  if (name[0] != 0) {
    snprintf(msg_buf, sizeof(msg_buf), "006%s", name);
    write_msg(gps_sockfd, msg_buf);
    return;
  }
  if (!in_join && !in_serv) {
    display_comment("你要查看那一桌?");
    return;
  }
  display_comment("----------------   此桌使用者   ------------------");
  msg_buf[0] = 0;
  for (i = 1; i < MAX_PLAYER; i++) {
    if (strlen(msg_buf) > 49) {
      display_comment(msg_buf);
      msg_buf[0] = 0;
    }
    if (player[i].in_table) {
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) - 1);
      strncat(msg_buf, "  ", sizeof(msg_buf) - strlen(msg_buf) - 1);
    }
  }
  if (msg_buf[0] != 0)
    display_comment(msg_buf);
  display_comment("--------------------------------------------------");
}

//  顯示說明函數 (和先前程式碼相同)
static void help() {
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

//  指令解析函數 (使用更清晰的 switch 語句)
void command_parser(const char* msg) {
  if (msg[0] == '/') {
    Tokenize(msg + 1); //  解析指令
    if (narg > 0) {
      CommandId cmd_id = command_mapper((char *)cmd_argv[0]);
      handle_command(cmd_id, narg);
    } else {
      send_gps_line("無效指令"); //  處理無參數的指令
    }
  } else {
    send_talk_line(msg); //  處理聊天訊息
  }
}

//  處理個別指令的函數
static void handle_command(CommandId cmd_id, int narg) {
  char msg_buf[255]; //  使用更安全的字串處理方式
  int i;

  switch (cmd_id) {
    case CMD_UNKNOWN:
      send_gps_line("未知指令");
      break;
    case CMD_TABLE:
    case CMD_S_TABLE:
      write_msg(gps_sockfd, "003");
      break;
    case CMD_FREE:
      write_msg(gps_sockfd, "013");
      break;
    case CMD_LIST:
    case CMD_PLAYER:
    case CMD_S_PLAYER:
      write_msg(gps_sockfd, "002");
      break;
    case CMD_JOIN:
    case CMD_S_JOIN:
      //  加入 JOIN 指令的錯誤處理和安全性檢查
      if (!pass_login) {
        display_comment("請先登入");
        break;
      }
      if (in_join || in_serv) {
        display_comment("您已在牌桌中，請先離開");
        break;
      }
      if (narg < 2) {
        display_comment("請指定桌號");
        break;
      }
      clear_variable();
      if (in_join) {
        close_join();
        write_msg(gps_sockfd, "205");
        init_global_screen();
      }
      if (in_serv) {
        close_serv();
        write_msg(gps_sockfd, "205");
        init_global_screen();
      }
      snprintf(msg_buf, sizeof(msg_buf), "011%s", cmd_argv[1]);
      write_msg(gps_sockfd, msg_buf);
      break;
    case CMD_SERV:
    case CMD_S_SERV:
      //  加入 SERV 指令的錯誤處理和安全性檢查
      if (!pass_login) {
        display_comment("請先登入");
        break;
      }
      if (in_join || in_serv) {
        display_comment("請先離開此桌");
        break;
      }
      clear_variable();
      if (in_join) {
        close_join();
        write_msg(gps_sockfd, "205");
        init_global_screen();
      }
      if (in_serv) {
        close_serv();
        write_msg(gps_sockfd, "205");
        init_global_screen();
      }
      write_msg(gps_sockfd, "014"); // 檢查開桌條件
      break;
    case CMD_QUIT:
    case CMD_S_QUIT:
    case CMD_EXIT:
      leave();
      break;
    case CMD_SHOW:
      //  SHOW 指令的處理 (需要根據原始程式碼補充)
      break;
    case CMD_WHO:
    case CMD_S_WHO:
      if (narg == 2) {
        who((char *)cmd_argv[1]);
      } else {
        who("");
      }
      break;
    case CMD_NUM:
      i = cmd_argv[1][0] - '0';
      if (i >= 1 && i <= 4) {
        PLAYER_NUM = i;
      }
      break;
    case CMD_NEW:
      if (in_serv) {
        broadcast_msg(1, "290");
        opening();
        open_deal();
      } else {
        write_msg(table_sockfd, "290");
        opening();
      }
      break;
    case CMD_LEAVE:
    case CMD_S_LEAVE:
      if (in_join) {
        in_join = 0;
        close_join();
        write_msg(gps_sockfd, "205");
        init_global_screen();
        write_msg(gps_sockfd, "201"); // 更新線上人數
        display_comment("您已離開牌桌");
        display_comment("-------------------");
      }
      if (in_serv) {
        in_serv = 0;
        close_serv();
        write_msg(gps_sockfd, "205");
        init_global_screen();
        write_msg(gps_sockfd, "201"); // 更新線上人數
        display_comment("您已關閉牌桌");
        display_comment("-------------------");
      }
      input_mode = TALK_MODE;
      break;
    case CMD_HELP:
    case CMD_S_HELP:
      help();
      break;
    case CMD_NOTE:
      snprintf(msg_buf, sizeof(msg_buf), "004%s", cmd_argv[1]);
      write_msg(gps_sockfd, msg_buf);
      break;
    case CMD_STAT:
      if (narg < 2) {
        snprintf(msg_buf, sizeof(msg_buf), "005%s", my_name);
      } else {
        snprintf(msg_buf, sizeof(msg_buf), "005%s", cmd_argv[1]);
      }
      write_msg(gps_sockfd, msg_buf);
      break;
    case CMD_LOGIN:
      // LOGIN 指令的處理 (需要根據原始程式碼補充)
      break;
    case CMD_BROADCAST:
      snprintf(msg_buf, sizeof(msg_buf), "007%s", cmd_argv[1]);
      write_msg(gps_sockfd, msg_buf);
      break;
    case CMD_MSG:
      if (narg <= 2) break;
      if (in_join || in_serv) {
        for (i = 1; i <= 4; i++) {
          if (table[i] && strncmp((char *)cmd_argv[1], player[table[i]].name, sizeof(player[table[i]].name)) == 0) {
            snprintf(msg_buf, sizeof(msg_buf), "%s", cmd_argv[2]);
            send_talk_line(msg_buf);
            goto finish_msg;
          }
        }
      }
      snprintf(msg_buf, sizeof(msg_buf), "009%s", cmd_argv[1]);
      write_msg(gps_sockfd, msg_buf);
      snprintf(msg_buf, sizeof(msg_buf), "-> *%s* %s", cmd_argv[1], cmd_argv[2]);
      msg_buf[talk_right] = 0;
      display_comment(msg_buf);
    finish_msg:;
      break;
    case CMD_SHUTDOWN:
      write_msg(gps_sockfd, "500");
      break;
    case CMD_LURKER:
      write_msg(gps_sockfd, "010");
      break;
    case CMD_FIND:
      if (narg < 2) {
        display_comment("你要找誰呢?");
        break;
      }
      snprintf(msg_buf, sizeof(msg_buf), "021%s", cmd_argv[1]);
      write_msg(gps_sockfd, msg_buf);
      break;
    case CMD_EXEC:
      //  EXEC 指令已被移除，因為存在安全風險
      break;
    case CMD_BEEP:
      if (narg < 2) {
        snprintf(msg_buf, sizeof(msg_buf), "目前聲音設定為%s",
                 (set_beep == 1) ? "開啟" : "關閉");
        display_comment(msg_buf);
      } else {
        char ans_buf[255];
        my_strupr(ans_buf, (char *)cmd_argv[1]);
        if (strcmp(ans_buf, "ON") == 0) {
          set_beep = 1;
          display_comment("開啟聲音");
        } else if (strcmp(ans_buf, "OFF") == 0) {
          set_beep = 0;
          display_comment("關閉聲音");
        }
      }
      break;
    case CMD_PASSWD:
      // PASSWD 指令的處理 (需要根據原始程式碼補充)
      break;
    case CMD_KICK:
      // KICK 指令的處理 (需要根據原始程式碼補充)
      break;
    case CMD_KILL:
      if (narg >= 2) {
        snprintf(msg_buf, sizeof(msg_buf), "202%s", cmd_argv[1]);
        write_msg(gps_sockfd, msg_buf);
      }
      break;
    case CMD_INVITE:
      if (narg < 2) {
        display_comment("你打算邀請誰?");
        break;
      }
      snprintf(msg_buf, sizeof(msg_buf), "008%s", cmd_argv[1]);
      write_msg(gps_sockfd, msg_buf);
      snprintf(msg_buf, sizeof(msg_buf), "邀請 %s 加入此桌", cmd_argv[1]);
      display_comment(msg_buf);
      break;
    default:
      snprintf(msg_buf, sizeof(msg_buf), "未知指令: %s", cmd_argv[0]);
      err(msg_buf);
      break;
  }
}

