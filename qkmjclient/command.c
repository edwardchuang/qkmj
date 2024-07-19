
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

#if defined(HAVE_LIBNCURSES)
  #include  <ncurses.h>
#endif

#include "qkmj.h"

int enable_kick=1;
int enable_exec=0;
char commands[100][10]
={"","TABLE","LIST","PLAYER","JOIN","SERV","QUIT","EXIT","WHO","SIT","NUM",
  "NEW","SHOW","LEAVE","HELP","NOTE","STAT","LOGIN","PASSWD","preserve1",
  "BROADCAST","KICK","KILL","INVITE","MSG","SHUTDOWN","LURKER","FIND","BEEP",
  "EXEC","FREE", "P", "J", "H", "W", "S", "L", "T", "Q"};

void Tokenize(char *strinput) {
  // 宣告變數
  char *token;
  char str[255];
  narg = 1;

  // 複製輸入字串到 str 陣列，並確保安全
  strncpy(str, strinput, sizeof(str) - 1);
  str[sizeof(str) - 1] = '\0';

  // 使用 strtok_r 進行分詞，並使用 saveptr 儲存狀態
  char *saveptr;
  token = strtok_r(str, " \n\t\r", &saveptr);

  // 檢查是否找到任何分詞
  if (token == NULL) {
    narg = 0;
    return;
  }

  // 儲存第一個分詞及其長度
  size_t Ltok = strlen(token);
  strncpy((char *)cmd_argv[narg], token, Ltok + 1); // 安全複製
  arglenv[narg] = Ltok;

  // 繼續分詞直到沒有更多分詞
  while ((token = strtok_r(NULL, " \n\t\r", &saveptr)) != NULL) {
    narg++;
    Ltok = strlen(token);
    strncpy((char *)cmd_argv[narg], token, Ltok + 1); // 安全複製
    arglenv[narg] = Ltok;
  }
}

void my_strupr(char* upper, char* org) {
  // 計算原始字串長度
  size_t len = strlen(org);

  // 使用 `strncpy` 避免緩衝區溢出
  strncpy(upper, org, len);

  // 將字串轉為大寫
  for (size_t i = 0; i < len; i++) {
    upper[i] = toupper(upper[i]);
  }

  // 添加字串結尾符號
  upper[len] = '\0';
}

int command_mapper(char *cmd) {
  // 將指令轉為大寫
  char cmd_upper[255];
  my_strupr(cmd_upper, cmd);

  // 遍歷指令列表
  for (int i = 1; commands[i][0] != '\0'; i++) {
    // 檢查指令是否匹配
    if (strncmp(cmd_upper, commands[i], sizeof(commands[i])) == 0) {
      return i;
    }
  }

  // 指令未找到
  return 0;
}

void who(char *name)
{
  char msg_buf[255];
  int i;

  if(name[0]!=0)
  {
    sprintf(msg_buf,"006%s",name);
    write_msg(gps_sockfd,msg_buf);
    return;
  }
  if(!in_join && !in_serv)
  {
    display_comment("你要查看那一桌?");
    return;
  }
  display_comment("----------------   此桌使用者   ------------------");
  msg_buf[0]=0;
  for(i=1;i<MAX_PLAYER;i++)
  {
    if(strlen(msg_buf)>49)
    {
      display_comment(msg_buf);
      msg_buf[0]=0;
    }
    if(player[i].in_table)
    {
      strcat(msg_buf,player[i].name);
      strcat(msg_buf,"  ");
    }
  }
  if(msg_buf[0]!=0)
    display_comment(msg_buf); 
  display_comment("--------------------------------------------------");
  return;
}

void list_players() {
  // 檢查是否已加入或開桌
  if (!in_join && !in_serv) {
    display_comment("你要查看那一桌?");
    return;
  }

  // 顯示標題
  display_comment("----------------   此桌使用者   ------------------");

  // 建立訊息緩衝區
  char msg_buf[255] = {0};
  int msg_buf_len = 0;

  // 遍歷所有玩家
  for (int i = 1; i < MAX_PLAYER; i++) {
    // 檢查訊息緩衝區長度
    if (msg_buf_len > 49) {
      // 顯示訊息緩衝區
      display_comment(msg_buf);
      // 重置訊息緩衝區
      msg_buf_len = 0;
      msg_buf[0] = 0;
    }

    // 檢查玩家是否在桌子上
    if (player[i].in_table) {
      // 將玩家名稱加入訊息緩衝區
      msg_buf_len += snprintf(msg_buf + msg_buf_len, sizeof(msg_buf) - msg_buf_len, "%s  ", player[i].name);
    }
  }

  // 檢查訊息緩衝區是否為空
  if (msg_buf[0] != 0) {
    // 顯示訊息緩衝區
    display_comment(msg_buf);
  }

  // 顯示結束標題
  display_comment("--------------------------------------------------");
}

void help()
{
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

void command_parser(char *msg) {
  int i;
  int cmd_id;
  char sit;
  char table_upper[255];
  char msg_buf[4096];
  char ans_buf[255];
  char ans_buf1[255];

  // 檢查指令是否以 '/' 開頭
  if (msg[0] == '/') {
    // 將指令字串分割成單詞
    Tokenize(msg + 1);
    // 尋找對應的指令 ID
    cmd_id = command_mapper((char *)cmd_argv[1]);
    switch (cmd_id) {
      case 0:
        // 找不到指令
        send_gps_line("沒有這個指令");
        break;
      case TABLE:
      case S_TABLE:
        // 查詢所有桌子
        write_msg(gps_sockfd, "003");
        break;
      case FREE:
        // 查詢空桌
        write_msg(gps_sockfd, "013");
        break;
      case LIST:
      case PLAYER:
      case S_PLAYER:
        // 查詢線上玩家
        write_msg(gps_sockfd, "002");
        break;
      case JOIN:
      case S_JOIN:
        // 檢查是否已登入
        if (!pass_login) {
          display_comment("請先 login 一個名稱");
          break;
        }
        // 檢查是否已在桌子上
        if (in_join || in_serv) {
          display_comment("請先離開目前桌，才能加入別桌。(/Leave)");
          break;
        }
        // 清除變數
        clear_variable();
        // 離開目前的桌子
        if (in_join) {
          close_join();
          write_msg(gps_sockfd, "205");
          init_global_screen();
        }
        // 離開目前的桌子
        if (in_serv) {
          close_serv();
          write_msg(gps_sockfd, "205");
          init_global_screen();
        }
        // 發送加入桌子的訊息
        snprintf(msg_buf, sizeof(msg_buf), "011%s", cmd_argv[2]);
        write_msg(gps_sockfd, msg_buf);
        // 等待 GPS 的回應
        break;
      case SERV:
      case S_SERV:
        // 檢查是否已登入
        if (!pass_login) {
          display_comment("請先 login 一個名稱");
          break;
        }
        // 檢查是否已在桌子上
        if (in_join || in_serv) {
          display_comment("請先離開此桌");
          break;
        }
        // 清除變數
        clear_variable();
        // 離開目前的桌子
        if (in_join) {
          close_join();
          write_msg(gps_sockfd, "205");
          init_global_screen();
        }
        // 離開目前的桌子
        if (in_serv) {
          close_serv();
          write_msg(gps_sockfd, "205");
          init_global_screen();
        }
        // 發送開桌的訊息
        write_msg(gps_sockfd, "014"); // 檢查開桌條件，將開桌的部份另外放到 message 去執行
        break;
      case QUIT:
      case S_QUIT:
      case EXIT:
        // 離開遊戲
        leave();
        break;
      case SHOW:
        // 顯示牌
        break;
      case WHO:
      case S_WHO:
        // 查詢玩家
        list_players();
        break;
      case NUM:
        // 設定每桌人數
        i = cmd_argv[2][0] - '0';
        if (i >= 1 && i <= 4) {
          PLAYER_NUM = i;
        }
        break;
      case NEW:
        // 開始新遊戲
        if (in_serv) {
          broadcast_msg(1, "290");
          opening();
          open_deal();
        } else {
          write_msg(table_sockfd, "290");
          opening();
        }
        break;
      case LEAVE:
      case S_LEAVE:
        // 離開桌子
        if (in_join) {
          in_join = 0;
          close_join();
          write_msg(gps_sockfd, "205");
          init_global_screen();
          write_msg(gps_sockfd, "201"); // 更新一下目前線上人數跟內容
          display_comment("您已離開牌桌");
          display_comment("-------------------");
        }
        if (in_serv) {
          in_serv = 0;
          close_serv();
          write_msg(gps_sockfd, "205");
          init_global_screen();
          write_msg(gps_sockfd, "201"); // 更新一下目前線上人數跟內容
          display_comment("您已關閉牌桌");
          display_comment("-------------------");
        }
        input_mode = TALK_MODE;
        break;
      case HELP:
      case S_HELP:
        // 顯示幫助訊息
        help();
        break;
      case NOTE:
        // 更新桌子備註
        snprintf(msg_buf, sizeof(msg_buf), "004%s", msg + 6);
        write_msg(gps_sockfd, msg_buf);
        break;
      case STAT: // /STAT
        // 查詢玩家狀態
        if (narg < 2) {
          strncpy((char *)cmd_argv[2], (char *)my_name, sizeof(cmd_argv[2]));
        }
        snprintf(msg_buf, sizeof(msg_buf), "005%s", cmd_argv[2]);
        write_msg(gps_sockfd, msg_buf);
        break;
      case LOGIN:
        // 登入
        break;
      case BROADCAST:
        // 廣播訊息
        snprintf(msg_buf, sizeof(msg_buf), "007%s", msg + 11);
        write_msg(gps_sockfd, msg_buf);
        break;
      case MSG:
        // 私人訊息
        if (narg <= 2) {
          break;
        }
        if (in_join || in_serv) {
          for (i = 1; i <= 4; i++) {
            if (table[i] && strcmp((char *)cmd_argv[2], player[table[i]].name) == 0) {
              snprintf(msg_buf, sizeof(msg_buf), "%s",
                       msg + 5 + strlen((char *)cmd_argv[2]) + 1);
              send_talk_line(msg_buf);
              // 使用 break 代替 goto finish_msg
              break;
            }
          }
        }
        snprintf(msg_buf, sizeof(msg_buf), "009%s", msg + 5);
        write_msg(gps_sockfd, msg_buf);
        snprintf(msg_buf, sizeof(msg_buf), "-> *%s* %s", cmd_argv[2],
                 msg + 5 + strlen((char *)cmd_argv[2]) + 1);
        msg_buf[talk_right] = 0;
        display_comment(msg_buf);
        // 使用 break 代替 goto finish_msg
        break;
      case SHUTDOWN:
        // 關閉伺服器
        write_msg(gps_sockfd, "500");
        break;
      case LURKER:
        // 查詢閒置玩家
        write_msg(gps_sockfd, "010");
        break;
      case FIND:
        // 查詢玩家
        if (narg < 2) {
          display_comment("你要找誰呢?");
          break;
        }
        snprintf(msg_buf, sizeof(msg_buf), "021%s", cmd_argv[2]);
        write_msg(gps_sockfd, msg_buf);
        break;
      case EXEC:
        // 執行系統指令
        break;
      case BEEP:
        // 設定聲音
        if (narg < 2) {
          snprintf(msg_buf, sizeof(msg_buf), "目前聲音設定為%s",
                   (set_beep == 1) ? "開啟" : "關閉");
          display_comment(msg_buf);
        } else {
          my_strupr(ans_buf, (char *)cmd_argv[2]);
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
        // 更改密碼
        ans_buf[0] = 0;
        ask_question("請輸入你原來的密碼：", ans_buf, sizeof(ans_buf) - 1, 0);
        if (strncmp((char *)my_pass, ans_buf, sizeof(my_pass)) != 0) {
          wait_a_key("密碼錯誤,更改失敗!");
          break;
        }
        ask_question("請輸入你要更改的密碼：", ans_buf, sizeof(ans_buf) - 1, 0);
        ask_question("請再輸入一次確認：", ans_buf1, sizeof(ans_buf1) - 1, 0);
        if (strncmp(ans_buf, ans_buf1, sizeof(ans_buf)) == 0) {
          snprintf(msg_buf, sizeof(msg_buf), "104%s", ans_buf);
          if (write_msg(gps_sockfd, msg_buf) < 0) {
            // 處理寫入錯誤
            wait_a_key("網路連線錯誤,更改失敗!");
            break;
          }
          strncpy((char *)my_pass, ans_buf, sizeof(my_pass));
          wait_a_key("密碼已更改!");
        } else {
          wait_a_key("兩次密碼不同,更改失敗!");
        }
        break;
      case KICK:
        // 踢出玩家
        if (!enable_kick) {
          break;
        }
        if (in_serv) {
          if (narg < 2) {
            display_comment("要把誰踢出去呢?");
          } else {
            // 檢查是否踢自己
            if (strncmp((char *)my_name, (char *)cmd_argv[2], sizeof(my_name)) == 0) {
              display_comment("抱歉, 自己不能踢自己");
              break;
            }
            // 遍歷所有玩家
            for (i = 2; i < MAX_PLAYER; i++) {
              // 檢查玩家是否在桌子上且名稱匹配
              if (player[i].in_table &&
                  strncmp(player[i].name, (char *)cmd_argv[2], sizeof(player[i].name)) == 0) {
                // 構建踢出訊息
                snprintf(msg_buf, sizeof(msg_buf), "101%s 被踢出此桌",
                         cmd_argv[2]);
                // 顯示踢出訊息
                display_comment(msg_buf + 3);
                // 廣播踢出訊息
                broadcast_msg(1, msg_buf);
                // 通知被踢玩家
                write_msg(player[i].sockfd, "200");
                // 關閉被踢玩家連線
                close_client(i);
                // 退出循環
                break;
              }
            }
            // 找不到玩家
            display_comment("此桌沒有這個人");
          }
        } else {
          display_comment("此命令只有桌長可用");
        }
        break;
      case KILL:
        // 強制斷線
        if (narg >= 2) {
          snprintf(msg_buf, sizeof(msg_buf), "202%s", cmd_argv[2]);
          write_msg(gps_sockfd, msg_buf);
        }
        break;
      case INVITE:
        // 邀請玩家
        if (narg < 2) {
          display_comment("你打算邀請誰?");
          break;
        }
        snprintf(msg_buf, sizeof(msg_buf), "008%s", cmd_argv[2]);
        write_msg(gps_sockfd, msg_buf);
        snprintf(msg_buf, sizeof(msg_buf), "邀請 %s 加入此桌", cmd_argv[2]);
        display_comment(msg_buf);
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