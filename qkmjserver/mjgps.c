/*
 * Server 
 */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <string.h>
#include <netdb.h>
#include <sys/errno.h>
#if defined(HAVE_TERMIOS_H)
  #include <termios.h>
#else
  #include <termio.h>
#endif
#include <fcntl.h>
#include <sys/param.h>
#include <sys/file.h>
#include <pwd.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "mjgps.h"

/*
 * Global variables 
 */
int timeup = 0;
extern int errno;
fd_set rfds, afds;
int login_limit;
char gps_ip[20];
int gps_port;
int log_level;
char number_map[20][5] = { "０", "１", "２", "３", "４", "５", "６", "７", "８", "９" };

#define ADMIN_USER  "mjgps"

#define MIN_JOIN_MONEY 0  //use -999999 if you allow user to join for debt

int gps_sockfd;

struct player_info player[MAX_PLAYER];

struct player_record record;

struct record_index_type record_index;

FILE *fp, *log_fp;

struct ask_mode_info ask;

struct rlimit fd_limit;

// Error handling function
int err(char *errmsg) {
  // 嘗試開啟日誌檔案
  FILE *log_fp = fopen(LOG_FILE, "a");
  if (log_fp == NULL) {
    // 顯示錯誤訊息
    printf("Cannot open logfile\n");
    return -1;
  }

// 顯示錯誤訊息
  time_t now = time(0);
  struct tm *t = localtime(&now);
  char datetime[80];
  strftime(datetime, sizeof(datetime), "%FT%T%z", t);
  printf("[mjgps][%s] %s", datetime, errmsg);

  // 檢查日誌等級
  if (log_level == 0) {
    // 將錯誤訊息寫入日誌檔案
    fprintf(log_fp, "%s", errmsg);
  }

  // 重置日誌等級
  log_level = 0;

  // 關閉日誌檔案
  fclose(log_fp);
  return 0;
}

// Game log function
int game_log(char *gamemsg) {
  // 嘗試開啟遊戲日誌檔案
  FILE *log_fp = fopen(GAME_FILE, "a");
  if (log_fp == NULL) {
    // 顯示錯誤訊息
    printf("Cannot open GAME_FILE\n");
    return -1;
  }

  // 將遊戲訊息寫入遊戲日誌檔案
  fprintf(log_fp, "%s", gamemsg);

  // 關閉遊戲日誌檔案
  fclose(log_fp);
  return 0;
}

// Read message from a socket
int read_msg(int fd, char *msg) {
  // 初始化變數
  int n = 0;
  char msg_buf[1000];
  int read_code;

  // 檢查是否有資料
  if (Check_for_data(fd) == 0) {
    // 顯示錯誤訊息
    err("WRONG READ\n");
    return 2;
  }

  // 重置時間超時標記
  timeup = 0;

  // 設定 5 秒的鬧鐘
  alarm(5);

  // 迴圈讀取資料
  while (n <= 8000) {
    // 嘗試讀取資料
    read_code = read(fd, msg, 1);

    // 檢查讀取結果
    if (read_code == -1) {
      // 檢查錯誤代碼
      if (errno != EWOULDBLOCK) {
        // 顯示錯誤訊息
        snprintf(msg_buf, sizeof(msg_buf), "fail in read_msg,errno = %d", errno);
        err(msg_buf);

        // 取消鬧鐘
        alarm(0);
        return 0;
      } else if (timeup) {
        // 取消鬧鐘
        alarm(0);

        // 顯示錯誤訊息
        err("TIME UP!\n");
        return 0;
      }
    } else if (read_code == 0) {
      // 取消鬧鐘
      alarm(0);
      return 0;
    } else {
      // 檢查是否為結束符號
      if (*msg == '\0') {
        // 取消鬧鐘
        alarm(0);
        return 1;
      }

      // 遞增資料計數器
      n++;

      // 指向下一位元組
      msg++;
    }
  }

  // 取消鬧鐘
  alarm(0);
  return 0;
}

// Write message to a socket
void write_msg(int fd, char *msg) {
  // 取得訊息長度
  int n = strlen(msg);

  // 嘗試寫入訊息
  if (write(fd, msg, n) < 0) {
    // 關閉連線
    close(fd);

    // 從檔案描述符集合中移除連線
    FD_CLR(fd, &afds);
  }

  // 嘗試寫入結束符號
  if (write(fd, msg + n, 1) < 0) {
    // 關閉連線
    close(fd);

    // 從檔案描述符集合中移除連線
    FD_CLR(fd, &afds);
  }
}

// Display message to a player
void display_msg(int player_id, char *msg) {
  // 建立訊息緩衝區
  char msg_buf[1000];

  // 組成訊息
  snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);

  // 將訊息寫入玩家的 socket
  write_msg(player[player_id].sockfd, msg_buf);
}

// Check for data on a socket
int Check_for_data(int fd) {
  // 初始化變數
  int status;
  fd_set wait_set;
  struct timeval tm;

  // 清空檔案描述符集合
  FD_ZERO(&wait_set);

  // 將檔案描述符加入集合
  FD_SET(fd, &wait_set);

  // 設定時間超時為 0 秒
  tm.tv_sec = 0;
  tm.tv_usec = 0;

  // 檢查是否有資料
  status = select(FD_SETSIZE, &wait_set, (fd_set *) 0, (fd_set *) 0, &tm);

  // 回傳檢查結果
  return (status);
}

// Convert message ID
int convert_msg_id(int player_id, char *msg) {
  // 初始化變數
  int i;
  char msg_buf[1000];

  // 檢查訊息長度
  if (strlen(msg) < 3) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "Error msg: %s", msg);
    err(msg_buf);
    return 0;
  }

  // 檢查訊息 ID 是否為數字
  for (i = 0; i < 3; i++) {
    if (msg[i] < '0' || msg[i] > '9') {
      // 顯示錯誤訊息
      snprintf(msg_buf, sizeof(msg_buf), "%d", msg[i]);
      err(msg_buf);
    }
  }

  // 將訊息 ID 轉換為整數
  return (msg[0] - '0') * 100 + (msg[1] - '0') * 10 + (msg[2] - '0');
}

// 列出線上玩家
void list_player(int fd) {
  // 初始化變數
  int i;
  char msg_buf[1000];
  int total_num = 0;

  // 顯示線上玩家標題
  write_msg(fd, "101-------------    目前上線使用者    ---------------");

  // 初始化訊息緩衝區
  snprintf(msg_buf, sizeof(msg_buf), "101");

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入
    if (player[i].login == 2) {
      // 遞增線上玩家數量
      total_num++;

      // 檢查訊息緩衝區是否已滿
      if ((strlen(msg_buf) + strlen(player[i].name)) > 50) {
        // 將訊息緩衝區寫入 socket
        write_msg(fd, msg_buf);

        // 重置訊息緩衝區
        snprintf(msg_buf, sizeof(msg_buf), "101");
      }

      // 將玩家名稱追加到訊息緩衝區
      snprintf(msg_buf, sizeof(msg_buf), "%s%s  ", msg_buf, player[i].name);
    }
  }

  // 檢查訊息緩衝區是否還有資料
  if (strlen(msg_buf) > 4) {
    // 將訊息緩衝區寫入 socket
    write_msg(fd, msg_buf);
  }

  // 顯示線上玩家數量
  write_msg(fd, "101--------------------------------------------------");
  snprintf(msg_buf, sizeof(msg_buf), "101共 %d 人", total_num);
  write_msg(fd, msg_buf);
}

// 列出牌桌
void list_table(int fd, int mode) {
  // 初始化變數
  int i;
  char msg_buf[1000];
  int total_num = 0;

  // 顯示牌桌標題
  write_msg(fd, "101   桌長       人數  附註");
  write_msg(fd, "101--------------------------------------------------");

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入且有牌桌
    if (player[i].login && player[i].serv > 0) {
      // 檢查牌桌人數是否超過上限
      if (player[i].serv > 4) {
        // 檢查牌桌人數是否為 5
        if (player[i].serv == 5) {
          // 顯示錯誤訊息
          err("SERV=5\n");
        } else {
          // 顯示錯誤訊息
          err("LIST TABLE ERROR!");
          snprintf(msg_buf, sizeof(msg_buf), "serv=%d\n", player[i].serv);
          close_id(i);
          err(msg_buf);
        }
      }

      // 檢查是否為詳細模式且牌桌人數已滿
      if (mode == 2 && player[i].serv >= 4) {
        // 跳過此牌桌
        continue;
      }

      // 遞增牌桌數量
      total_num++;

      // 組成牌桌資訊訊息
      snprintf(msg_buf, sizeof(msg_buf), "101   %-10s %-4s  %s", player[i].name,
              number_map[player[i].serv], player[i].note);

      // 將牌桌資訊訊息寫入 socket
      write_msg(fd, msg_buf);
    }
  }

  // 顯示牌桌數量
  write_msg(fd, "101--------------------------------------------------");
  snprintf(msg_buf, sizeof(msg_buf), "101共 %d 桌", total_num);
  write_msg(fd, msg_buf);
}

// 列出玩家統計資訊
void list_stat(int fd, char *name) {
  // 初始化變數
  char msg_buf[1000];
  char msg_buf1[1000];
  char order_buf[30];
  int i;
  int total_num;
  int order;
  struct player_record tmp_rec;

  // 初始化變數
  total_num = 0;
  order = 1;

  // 檢查玩家是否存在
  if (!read_user_name(name)) {
    // 顯示錯誤訊息
    write_msg(fd, "101找不到這個人!");
    return;
  }

  // 組成玩家名稱訊息
  snprintf(msg_buf, sizeof(msg_buf), "101◇名稱:%s ", record.name);

  // 嘗試開啟玩家記錄檔案
  if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
    // 顯示錯誤訊息
    err("(stat) Cannot open file\n");
    return;
  }

  // 重置檔案指標
  rewind(fp);

  // 檢查玩家已玩局數是否大於等於 16
  if (record.game_count >= 16) {
    // 迴圈遍歷所有玩家記錄
    while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
      // 檢查玩家記錄是否有效
      if (tmp_rec.name[0] != 0 && tmp_rec.game_count >= 16) {
        // 遞增玩家總數
        total_num++;

        // 檢查玩家金額是否大於當前玩家金額
        if (tmp_rec.money > record.money) {
          // 遞增玩家排名
          order++;
        }
      }
    }
  }

  // 檢查玩家已玩局數是否小於 16
  if (record.game_count < 16) {
    // 設定玩家排名為 "無"
    strncpy(order_buf, "無", sizeof("無"));
  } else {
    // 組成玩家排名訊息
    snprintf(order_buf, sizeof(order_buf), "%d/%d", order, total_num);
  }

  // 組成玩家統計資訊訊息
  snprintf(msg_buf1, sizeof(msg_buf1), "101◇金額:%ld 排名:%s 上線次數:%d 已玩局數:%d", record.money,
          order_buf, record.login_count, record.game_count);

  // 將玩家統計資訊訊息寫入 socket
  write_msg(fd, msg_buf);
  write_msg(fd, msg_buf1);

  // 關閉玩家記錄檔案
  fclose(fp);
}

// 顯示牌桌玩家
void who(int fd, char *name) {
  // 初始化變數
  char msg_buf[1000];
  int i;
  int serv_id = -1;

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入且有牌桌
    if (player[i].login && player[i].serv) {
      // 檢查玩家名稱是否匹配
      if (strncmp(player[i].name, name, sizeof(player[i].name)) == 0) {
        // 取得牌桌伺服器 ID
        serv_id = i;
        break;
      }
    }
  }

  // 檢查是否找到牌桌伺服器
  if (serv_id == -1) {
    // 顯示錯誤訊息
    write_msg(fd, "101找不到此桌");
    return;
  }

  // 組成牌桌伺服器名稱訊息
  snprintf(msg_buf, sizeof(msg_buf), "101%s  ", player[serv_id].name);

  // 顯示牌桌玩家標題
  write_msg(fd, "101----------------   此桌使用者   ------------------");

  // 初始化訊息緩衝區
  snprintf(msg_buf, sizeof(msg_buf), "101");

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否加入此牌桌
    if (player[i].join == serv_id) {
      // 檢查訊息緩衝區是否已滿
      if ((strlen(msg_buf) + strlen(player[i].name)) > 53) {
        // 將訊息緩衝區寫入 socket
        write_msg(fd, msg_buf);

        // 重置訊息緩衝區
        snprintf(msg_buf, sizeof(msg_buf), "101");
      }

      // 將玩家名稱追加到訊息緩衝區
      snprintf(msg_buf, sizeof(msg_buf), "%s%s   ", msg_buf, player[i].name);
    }
  }

  // 檢查訊息緩衝區是否還有資料
  if (strlen(msg_buf) > 4) {
    // 將訊息緩衝區寫入 socket
    write_msg(fd, msg_buf);
  }

  // 顯示牌桌玩家標題
  write_msg(fd, "101--------------------------------------------------");
}

// 列出閒置玩家
void lurker(int fd) {
  // 初始化變數
  int i, total_num = 0;
  char msg_buf[1000];

  // 初始化訊息緩衝區
  snprintf(msg_buf, sizeof(msg_buf), "101");

  // 顯示閒置玩家標題
  write_msg(fd, "101-------------   目前閒置之使用者   ---------------");

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入且未加入任何牌桌
    if (player[i].login == 2 && (player[i].join == 0 && player[i].serv == 0)) {
      // 遞增閒置玩家數量
      total_num++;

      // 檢查訊息緩衝區是否已滿
      if ((strlen(msg_buf) + strlen(player[i].name)) > 53) {
        // 將訊息緩衝區寫入 socket
        write_msg(fd, msg_buf);

        // 重置訊息緩衝區
        snprintf(msg_buf, sizeof(msg_buf), "101");
      }

      // 將玩家名稱追加到訊息緩衝區
      snprintf(msg_buf, sizeof(msg_buf), "%s%s  ", msg_buf, player[i].name);
    }
  }

  // 檢查訊息緩衝區是否還有資料
  if (strlen(msg_buf) > 4) {
    // 將訊息緩衝區寫入 socket
    write_msg(fd, msg_buf);
  }

  // 顯示閒置玩家數量
  write_msg(fd, "101--------------------------------------------------");
  snprintf(msg_buf, sizeof(msg_buf), "101共 %d 人", total_num);
  write_msg(fd, msg_buf);
}

// 尋找使用者
void find_user(int fd, char *name) {
  // 初始化變數
  int i;
  char msg_buf[1000];
  int id;
  char last_login_time[80];

  // 尋找使用者 ID
  id = find_user_name(name);

  // 檢查使用者 ID 是否有效
  if (id > 0) {
    // 檢查使用者是否已登入
    if (player[id].login == 2) {
      // 檢查使用者是否為閒置狀態
      if (player[id].join == 0 && player[id].serv == 0) {
        // 顯示使用者狀態訊息
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s □置中", name);
        write_msg(fd, msg_buf);
      }

      // 檢查使用者是否已加入牌桌
      if (player[id].join) {
        // 顯示使用者狀態訊息
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s 在 %s 桌內", name,
                player[player[id].join].name);
        write_msg(fd, msg_buf);
      }

      // 檢查使用者是否為牌桌伺服器
      if (player[id].serv) {
        // 顯示使用者狀態訊息
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s 在 %s 桌內", name, player[id].name);
        write_msg(fd, msg_buf);
      }

      // 退出函式
      return;
    }
  }

  // 檢查使用者是否存在
  if (!read_user_name(name)) {
    // 顯示使用者不存在訊息
    snprintf(msg_buf, sizeof(msg_buf), "101◇沒有 %s 這個人", name);
    write_msg(fd, msg_buf);
  } else {
    // 顯示使用者不在線訊息
    snprintf(msg_buf, sizeof(msg_buf), "101◇%s 不在線上", name);
    write_msg(fd, msg_buf);

    // 取得使用者上次連線時間
    strncpy(last_login_time, ctime(&record.last_login_time), sizeof(last_login_time) - 1);

    // 顯示使用者上次連線時間訊息
    snprintf(msg_buf, sizeof(msg_buf), "101◇上次連線時間: %s", last_login_time);
    write_msg(fd, msg_buf);
  }
}

// 廣播訊息給所有線上玩家
void broadcast(int player_id, char *msg) {
  // 初始化變數
  int i;
  char msg_buf[1000];

  // 檢查玩家是否為管理員
  if (strcmp(player[player_id].name, ADMIN_USER) != 0) {
    // 退出函式
    return;
  }

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入
    if (player[i].login == 2) {
      // 組成廣播訊息
      snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);

      // 將廣播訊息寫入玩家的 socket
      write_msg(player[i].sockfd, msg_buf);
    }
  }
}

// 傳送私訊給特定玩家
void send_msg(int player_id, char *msg) {
  // 初始化變數
  char *str1, *str2;
  int i;
  char msg_buf[1000];

  // 分割訊息
  str1 = strtok(msg, " ");
  str2 = msg + strlen(str1) + 1;

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入且名稱匹配
    if (player[i].login == 2 && strcmp(player[i].name, str1) == 0) {
      // 組成私訊訊息
      snprintf(msg_buf, sizeof(msg_buf), "101*%s* %s", player[player_id].name, str2);

      // 將私訊訊息寫入玩家的 socket
      write_msg(player[i].sockfd, msg_buf);

      // 退出函式
      return;
    }
  }

  // 顯示找不到使用者訊息
  write_msg(player[player_id].sockfd, "101找不到這個人");
}

// 邀請玩家加入牌桌
void invite(int player_id, char *name) {
  // 初始化變數
  int i;
  char msg_buf[1000];

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入且名稱匹配
    if (player[i].login == 2 && strcmp(player[i].name, name) == 0) {
      // 組成邀請訊息
      snprintf(msg_buf, sizeof(msg_buf), "101%s 邀請你加入 %s",
               player[player_id].name,
               (player[player_id].join == 0) ? player[player_id].name
                                            : player[player[player_id].join].name);

      // 將邀請訊息寫入玩家的 socket
      write_msg(player[i].sockfd, msg_buf);

      // 退出函式
      return;
    }
  }

  // 顯示找不到使用者訊息
  write_msg(player[player_id].sockfd, "101找不到這個人");
}

// 初始化伺服器 socket
void init_socket() {
  // 初始化變數
  struct sockaddr_in serv_addr;
  int on = 1;

  // 開啟 TCP socket
  if ((gps_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    // 顯示錯誤訊息
    err("Server: cannot open stream socket");
  }

  // 綁定本地地址
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(gps_port);
  setsockopt(gps_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
  if (bind(gps_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    // 顯示錯誤訊息
    printf("server: cannot bind local address\n");
    exit(1);
  }

  // 監聽連線
  listen(gps_sockfd, 10);
  printf("Listen for client...\n");
}

// 查詢客戶端主機名稱或 IP 地址
char *lookup(struct sockaddr_in *cli_addrp) {
  // 初始化變數
  struct hostent *hp;
  char *hostname;

  // 查詢主機名稱
  hp = gethostbyaddr((char *) &cli_addrp->sin_addr, sizeof(struct in_addr),
                     cli_addrp->sin_family);

  // 檢查查詢結果
  if (hp) {
    // 取得主機名稱
    hostname = (char *) hp->h_name;
  } else {
    // 取得 IP 地址
    hostname = inet_ntoa(cli_addrp->sin_addr);
  }

  // 回傳主機名稱或 IP 地址
  return hostname;
}

// 初始化全域變數
void init_variable() {
  // 初始化變數
  int i;

  // 設定登入限制
  login_limit = LOGIN_LIMIT;

  // 初始化玩家資訊
  for (i = 0; i < MAX_PLAYER; i++) {
    player[i].login = 0;
    player[i].serv = 0;
    player[i].money = 0;
    player[i].join = 0;
    player[i].type = 16;
    player[i].note[0] = 0;
    player[i].username[0] = 0;
  }
}

// 讀取使用者名稱從記錄檔案
int read_user_name(char *name) {
  // 宣告變數
  struct player_record tmp_rec;
  char msg_buf[1000];

  // 嘗試開啟記錄檔案
  fp = fopen(RECORD_FILE, "a+b");
  if (fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_name) 无法打开文件！\n");
    err(msg_buf);
    return 0;
  }

  // 重置檔案指標
  rewind(fp);

  // 迴圈遍歷所有記錄
  while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
    // 檢查使用者名稱是否匹配
    if (strncmp(name, tmp_rec.name, sizeof(tmp_rec.name)) == 0) {
      // 將記錄複製到全域變數
      record = tmp_rec;

      // 關閉檔案
      fclose(fp);

      // 回傳成功
      return 1;
    }
  }

  // 關閉檔案
  fclose(fp);

  // 回傳失敗
  return 0;
}

// 讀取使用者名稱從記錄檔案並更新玩家資訊
int read_user_name_update(char *name, int player_id) {
  // 宣告變數
  struct player_record tmp_rec;
  char msg_buf[1000];

  // 嘗試開啟記錄檔案
  fp = fopen(RECORD_FILE, "a+b");
  if (fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_name) 无法打开文件！\n");
    err(msg_buf);
    return 0;
  }

  // 重置檔案指標
  rewind(fp);

  // 迴圈遍歷所有記錄
  while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
    // 檢查使用者名稱是否匹配
    if (strncmp(name, tmp_rec.name, sizeof(tmp_rec.name)) == 0) {
      // 將記錄複製到全域變數
      record = tmp_rec;

      // 檢查玩家 ID 是否匹配
      if (player[player_id].id == record.id) {
        // 更新玩家 ID 和金額
        player[player_id].id = record.id;
        player[player_id].money = record.money;
      }

      // 關閉檔案
      fclose(fp);

      // 回傳成功
      return 1;
    }
  }

  // 關閉檔案
  fclose(fp);

  // 回傳失敗
  return 0;
}

// 根據 ID 讀取使用者記錄
int read_user_id(unsigned int id) {
  // 宣告變數
  char msg_buf[1000];

  // 嘗試開啟記錄檔案
  fp = fopen(RECORD_FILE, "a+b");
  if (fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_id) 无法打开文件！\n");
    err(msg_buf);
    return -1;
  }

  // 重置檔案指標
  rewind(fp);

  // 定位到指定 ID 的記錄
  fseek(fp, sizeof(record) * id, 0);

  // 嘗試讀取記錄
  if (fread(&record, sizeof(record), 1, fp) == -1) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_id) 无法写入文件！\n");
    err(msg_buf);

    // 關閉檔案
    fclose(fp);

    // 回傳失敗
    return -1;
  }

  // 關閉檔案
  fclose(fp);

  // 回傳成功
  return 0;
}

// 將新使用者新增到記錄檔案
int add_user(int player_id, char *name, char *passwd) {
  // 宣告變數
  struct stat status;

  // 取得記錄檔案的狀態
  stat(RECORD_FILE, &status);

  // 檢查是否為第一個使用者
  if (!read_user_name("")) {
    // 設定使用者 ID
    record.id = status.st_size / sizeof(record);
  }

  // 複製使用者名稱到記錄
  strncpy(record.name, name, sizeof(record.name));

  // 產生密碼雜湊
  strncpy(record.password, genpasswd(passwd), sizeof(record.password));

  // 設定使用者初始金額
  record.money = DEFAULT_MONEY;

  // 設定使用者等級
  record.level = 0;

  // 設定使用者登入次數
  record.login_count = 1;

  // 設定使用者已玩局數
  record.game_count = 0;

  // 取得註冊時間
  time(&record.regist_time);

  // 設定上次登入時間
  record.last_login_time = record.regist_time;

  // 初始化上次登入來源
  record.last_login_from[0] = 0;

  // 檢查玩家是否有使用者名稱
  if (player[player_id].username[0] != 0) {
    // 將使用者名稱追加到上次登入來源
    snprintf(record.last_login_from, sizeof(record.last_login_from), "%s@",
             player[player_id].username);
  }

  // 將玩家 IP 地址追加到上次登入來源
  strncat(record.last_login_from, lookup(&(player[player_id].addr)),
          sizeof(record.last_login_from) - 1);

  // 檢查使用者是否被允許存取伺服器
  if (check_user(player_id)) {
    // 寫入記錄到檔案
    write_record();

    // 回傳成功
    return 1;
  } else {
    // 回傳失敗
    return 0;
  }
}

// 檢查使用者是否被允許存取伺服器
int check_user(int player_id) {
  // 宣告變數
  char msg_buf[1000];
  char from[80];
  char email[80];
  FILE *baduser_fp;

  // 嘗試開啟被封鎖使用者檔案
  baduser_fp = fopen(BADUSER_FILE, "r");
  if (baduser_fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "无法打开文件 %s", BADUSER_FILE);
    err(msg_buf);
    return 1;
  }

  // 取得玩家 IP 地址
  strncpy(from, lookup(&(player[player_id].addr)), sizeof(from) - 1);

  // 組成玩家電子郵件地址
  snprintf(email, sizeof(email), "%s@", player[player_id].username);
  strncat(email, from, sizeof(email) - 1);

  // 迴圈遍歷所有被封鎖使用者
  while (fgets(msg_buf, 80, baduser_fp) != NULL) {
    // 移除換行符號
    msg_buf[strlen(msg_buf) - 1] = 0;

    // 檢查玩家電子郵件地址或使用者名稱是否被封鎖
    if (strncmp(email, msg_buf, sizeof(msg_buf)) == 0 ||
        strncmp(player[player_id].username, msg_buf, sizeof(msg_buf)) == 0) {
      // 顯示被封鎖訊息
      display_msg(player_id, "你已被限制進入");

      // 關閉檔案
      fclose(baduser_fp);

      // 回傳失敗
      return 0;
    }
  }

  // 關閉檔案
  fclose(baduser_fp);

  // 回傳成功
  return 1;
}

// 將更新後的使用者記錄寫入檔案
void write_record() {
  // 宣告變數
  char msg_buf[1000];

  // 嘗試開啟記錄檔案
  fp = fopen(RECORD_FILE, "r+b");
  if (fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "(write_record) 无法打开文件！");
    err(msg_buf);
    return;
  }

  // 定位到指定 ID 的記錄
  fseek(fp, sizeof(record) * record.id, 0);

  // 寫入記錄到檔案
  fwrite(&record, sizeof(record), 1, fp);

  // 關閉檔案
  fclose(fp);
}

// 將新聞訊息列印到客戶端
void print_news(int fd, char *name) {
  // 宣告變數
  FILE *news_fp;
  char msg[255];
  char msg_buf[1000];

  // 嘗試開啟新聞檔案
  news_fp = fopen(name, "r");
  if (news_fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "无法打开文件 %s\n", NEWS_FILE);
    err(msg_buf);
    return;
  }

  // 迴圈遍歷所有新聞訊息
  while (fgets(msg, 80, news_fp) != NULL) {
    // 移除換行符號
    msg[strlen(msg) - 1] = 0;

    // 組成新聞訊息
    snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);

    // 將新聞訊息寫入 socket
    write_msg(fd, msg_buf);
  }

  // 關閉檔案
  fclose(news_fp);
}

// 歡迎新使用者加入伺服器
void welcome_user(int player_id) {
  // 宣告變數
  char msg_buf[1000];
  int fd;

  // 取得玩家的 socket
  fd = player[player_id].sockfd;

  // 檢查玩家的版本是否符合要求
  if (strncmp(player[player_id].version, "093", sizeof(player[player_id].version)) < 0 ||
      player[player_id].version[0] == 0) {
    // 顯示錯誤訊息
    write_msg(player[player_id].sockfd, "101請使用 QKMJ Ver 0.93 Beta 以上版本上線");
    write_msg(player[player_id].sockfd, "010");
    return;
  }

  // 組成歡迎訊息
  snprintf(msg_buf, sizeof(msg_buf), "101★★★★★　歡迎 %s 來到ＱＫ麻將  ★★★★★",
           player[player_id].name);

  // 將歡迎訊息寫入玩家的 socket
  write_msg(player[player_id].sockfd, msg_buf);

  // 列印新聞訊息
  print_news(player[player_id].sockfd, NEWS_FILE);

  // 更新玩家資訊
  player[player_id].id = record.id;
  player[player_id].money = record.money;
  player[player_id].login = 2;
  player[player_id].note[0] = 0;

  // 顯示線上玩家資訊
  show_online_users(player_id);

  // 顯示玩家統計資訊
  list_stat(player[player_id].sockfd, player[player_id].name);

  // 通知玩家登入成功
  write_msg(player[player_id].sockfd, "003");

  // 傳送玩家 ID 和金額
  snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld", player[player_id].id,
           player[player_id].money);
  write_msg(player[player_id].sockfd, msg_buf);

  // 設定玩家輸入模式為命令模式
  player[player_id].input_mode = CMD_MODE;
}

// 顯示線上玩家和註冊玩家數量
void show_online_users(int player_id) {
  // 宣告變數
  char msg_buf[1000];
  int fd;
  int total_num = 0;
  int online_num = 0;

  // 取得玩家的 socket
  fd = player[player_id].sockfd;

  // 嘗試開啟記錄檔案
  fp = fopen(RECORD_FILE, "rb");
  if (fp == NULL) {
    // 顯示錯誤訊息
    snprintf(msg_buf, sizeof(msg_buf), "(current) 無法打開文件\n");
    err(msg_buf);
  } else {
    // 重置檔案指標
    rewind(fp);

    // 迴圈遍歷所有記錄
    while (!feof(fp) && fread(&record, sizeof(record), 1, fp)) {
      // 檢查使用者名稱是否有效
      if (record.name[0] != 0) {
        // 遞增註冊玩家數量
        total_num++;
      }
    }

    // 關閉檔案
    fclose(fp);
  }

  // 迴圈遍歷所有玩家
  for (int i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入
    if (player[i].login == 2) {
      // 遞增線上玩家數量
      online_num++;
    }
  }

  // 組成訊息
  snprintf(msg_buf, sizeof(msg_buf),
           "101◇目前上線人數: %d 人       注冊人數: %d 人", online_num,
           total_num);

  // 將訊息寫入玩家的 socket
  write_msg(player[player_id].sockfd, msg_buf);
}

// 顯示使用者的當前狀態
void show_current_state(int player_id) {
  // 宣告變數
  char msg_buf[1000];

  // 顯示線上玩家資訊
  show_online_users(player_id);

  // 顯示玩家統計資訊
  list_stat(player[player_id].sockfd, player[player_id].name);

  // 通知玩家更新成功
  write_msg(player[player_id].sockfd, "003");

  // 傳送玩家 ID 和金額
  snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld", player[player_id].id,
           player[player_id].money);
  write_msg(player[player_id].sockfd, msg_buf);
}

// 更新客戶端的金額
void update_client_money(int player_id) {
  // 宣告變數
  char msg_buf[1000];

  // 組成訊息
  snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld", player[player_id].id,
           player[player_id].money);

  // 將訊息寫入玩家的 socket
  write_msg(player[player_id].sockfd, msg_buf);
}

// 根據名稱尋找使用者
int find_user_name(char *name) {
  // 迴圈遍歷所有玩家
  for (int i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家名稱是否匹配
    if (strncmp(player[i].name, name, sizeof(player[i].name)) == 0) {
      // 回傳玩家 ID
      return i;
    }
  }

  // 找不到使用者
  return -1;
}

void gps_processing() {
  int alen;
  int player_id;
  int player_num = 0;
  int i, nfds;
  int msg_id;
  int read_code;
  char tmp_buf[80];
  char msg_buf[1000];
  unsigned char buf[256];
  struct timeval timeout;
  struct hostent *hp;
  long current_time;
  struct tm *tim;

  log_level = 0;
  nfds = getdtablesize();
  nfds = 256;
  printf("%d\n", nfds);
  FD_ZERO(&afds);
  FD_SET(gps_sockfd, &afds);
  memcpy((char *)&rfds, (char *)&afds, sizeof(rfds));
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  // Main loop to handle connections and messages
  for (;;) {
    memcpy((char *)&rfds, (char *)&afds, sizeof(rfds));
    if (select(nfds, &rfds, NULL, NULL, NULL) < 0) {
      snprintf(msg_buf, sizeof(msg_buf), "select: %d %s\n", errno,
               strerror(errno));
      err(msg_buf);
      continue;
    }

    // 檢查是否有新的連線
    if (FD_ISSET(gps_sockfd, &rfds)) {
      // 尋找第一個未登入的玩家
      for (player_num = 1; player_num < MAX_PLAYER; player_num++) {
        if (!player[player_num].login) {
          break;
        }
      }

      // 檢查是否已達玩家上限
      if (player_num == MAX_PLAYER - 1) {
        // 顯示錯誤訊息
        err("Too many users");
      } else {
        // 設定玩家 ID
        player_id = player_num;

        // 取得玩家地址大小
        alen = sizeof(player[player_num].addr);

        // 接受新的連線
        player[player_id].sockfd = accept(gps_sockfd,
            (struct sockaddr *)&player[player_num].addr,
            (socklen_t *)&alen);

        // 將玩家的 socket 加入檔案描述符集合
        FD_SET(player[player_id].sockfd, &afds);

        // 設定非阻塞模式
        fcntl(player[player_id].sockfd, F_SETFL, FNDELAY);

        // 設定玩家登入狀態
        player[player_id].login = 1;

        // 取得目前時間
        time(&current_time);

        // 取得當地時間
        tim = localtime(&current_time);

        // 檢查玩家數量是否超過限制
        if (player_id > login_limit) {
          // 顯示錯誤訊息
          write_msg(player[player_id].sockfd,
              "101對不起,目前使用人數超過上限, 請稍後再進來.");

          // 列印新聞訊息
          print_news(player[player_id].sockfd, "server.lst");

          // 關閉玩家連線
          close_id(player_id);
        }
      }
    }

    // Check for messages from connected players
    for (player_id = 1; player_id < MAX_PLAYER; player_id++) {
      if (!player[player_id].login) {
        continue;
      }
      if (!FD_ISSET(player[player_id].sockfd, &rfds)) {
        continue;
      }
      // Process the player's information
      read_code = read_msg(player[player_id].sockfd, (char *) buf);
      if (!read_code || read_code != 1) {
        err(("cant read code!"));
        close_id(player_id);
        continue;
      }
      msg_id = convert_msg_id(player_id, (char *) buf);
      switch (msg_id) {
        case 99:  // Get username
          buf[15] = 0;
          strncpy(player[player_id].username,
              (char *)(buf + 3),
              sizeof(player[player_id].username) - 1);
          break;
        case 100:  // Check version
          buf[6] = 0;
          strncpy(player[player_id].version,
              (char *)(buf + 3),
              sizeof(player[player_id].version) - 1);
          break;
        case 101:  // User login
          buf[13] = 0;
          strncpy(player[player_id].name,
              (char *)(buf + 3),
              sizeof(player[player_id].name) - 1);
          for (i = 0; i < strlen((char *)(buf)) - 3;
                i++) {
            if (buf[3 + i] <= 32 && buf[3 + i] != 0) {
              write_msg(player[player_id].sockfd,
                  "101Invalid username!");
              close_id(player_id);
              break;
            }
          }
          if (read_user_name(player[player_id].name)) {
            write_msg(player[player_id].sockfd, "002");
          } else {
            write_msg(player[player_id].sockfd, "005");
          }
          break;
        case 102:  // Check password
          if (read_user_name(player[player_id].name)) {
            buf[11] = 0;
            if (checkpasswd(record.password,
                (char *)(buf + 3))) {
              int find_duplicated = 0;
              for (int i = 1; i < MAX_PLAYER; i++) {
                if ((player[i].login == 2 || player[i].login == 3) &&
                    strncmp(player[i].name, player[player_id].name,
                            sizeof(player[i].name)) == 0) {
                  write_msg(player[player_id].sockfd, "006");
                  player[player_id].login = 3;
                  find_duplicated = 1;
                  break;
                }
              }
              if (find_duplicated) {
                break;
              }

              time(&record.last_login_time);
              record.last_login_from[0] = 0;
              if (player[player_id].username[0] != 0) {
                snprintf(record.last_login_from,
                        sizeof(record.last_login_from),
                        "%s@", player[player_id].username);
              }
              strncat(record.last_login_from,
                      lookup(&player[player_id].addr),
                      sizeof(record.last_login_from) - 1);
              record.login_count++;
              write_record();
              if (check_user(player_id)) {
                welcome_user(player_id);
              } else {
                close_id(player_id);
              }
            } else {
              write_msg(player[player_id].sockfd, "004");
            }
          }
          break;
        case 103:  // Create new account
          buf[11] = 0;
          if (!add_user(player_id, player[player_id].name,
              (char *)(buf + 3))) {
            close_id(player_id);
            break;
          }
          welcome_user(player_id);
          break;
        case 104:  // Change password
          buf[11] = 0;
          read_user_name(player[player_id].name);
          strncpy(record.password,
              genpasswd((char *)(buf + 3)),
              sizeof(record.password) - 1);
          write_record();
          break;
        case 105:
          if (read_user_name(player[player_id].name) &&
              player[player_id].login == 3) {
            buf[11] = 0;
            for (i = 1; i < MAX_PLAYER; i++) {
              if ((player[i].login == 2 || player[i].login == 3) &&
                  (i != player_id) &&
                  strncmp(player[i].name, player[player_id].name,
                          sizeof(player[i].name)) == 0) {
                close_id(i);
                break;
              }
            }
            time(&record.last_login_time);
            record.last_login_from[0] = 0;
            if (player[player_id].username[0] != 0) {
              snprintf(record.last_login_from,
                      sizeof(record.last_login_from),
                      "%s@", player[player_id].username);
            }
            strncat(record.last_login_from,
                    lookup(&player[player_id].addr),
                    sizeof(record.last_login_from) - 1);
            record.login_count++;
            write_record();
            if (check_user(player_id)) {
              welcome_user(player_id);
            } else {
              close_id(player_id);
            }
          }
          break;
        case 2:  // List players
          list_player(player[player_id].sockfd);
          break;
        case 3:  // List tables
          list_table(player[player_id].sockfd, 1);
          break;
        case 4:  // Set player note
          strncpy(player[player_id].note,
              (char *)(buf + 3),
              sizeof(player[player_id].note) - 1);
          break;
        case 5:  // Show online users and stats
          show_online_users(player_id);
          list_stat(player[player_id].sockfd,
              (char *)(buf + 3));
          break;
        case 6:  // Who query
          who(player[player_id].sockfd,
              (char *)(buf + 3));
          break;
        case 7:  // Broadcast message (GM function)
          broadcast(player_id, (char *)(buf + 3));
          break;
        case 8:  // Invite player
          invite(player_id, (char *)(buf + 3));
          break;
        case 9:  // Send message
          send_msg(player_id, (char *)(buf + 3));
          break;
        case 10:  // Lurker mode
          lurker(player[player_id].sockfd);
          break;
        case 11:  // Join table
          // Check for table server
          if (!read_user_name_update(player[player_id].name,
              player_id)) {
            snprintf(msg_buf, sizeof(msg_buf), "101查無此人");
            write_msg(player[player_id].sockfd, msg_buf);
            break;
          }
          update_client_money(player_id);
          if (player[player_id].money <= MIN_JOIN_MONEY) {
            snprintf(msg_buf, sizeof(msg_buf),
                "101您的賭幣（%ld）不足，必須超過 %d 元才能加入牌桌",
                player[player_id].money, MIN_JOIN_MONEY);
            write_msg(player[player_id].sockfd, msg_buf);
            break;
          }
          for (i = 1; i < MAX_PLAYER; i++) {
            if (player[i].login == 2 && player[i].serv) {
              continue;
            }
            // Find the name of table server
            if (strncmp(player[i].name,
                (char *)(buf + 3),
                sizeof(player[i].name)) == 0) {
              if (player[i].serv >= 4) {
                write_msg(player[player_id].sockfd,
                    "101此桌人數已滿!");
                break;
              }
              snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld",
                      player[player_id].id,
                      player[player_id].money);
              write_msg(player[i].sockfd, msg_buf);
              snprintf(msg_buf, sizeof(msg_buf), "211%s",
                      player[player_id].name);
              write_msg(player[i].sockfd, msg_buf);
              snprintf(msg_buf, sizeof(msg_buf),
                      "0110%s %d",
                      inet_ntoa(player[i].addr.sin_addr),
                      player[i].port);
              write_msg(player[player_id].sockfd, msg_buf);
              player[player_id].join = i;
              player[player_id].serv = 0;
              player[i].serv++;
              break;
            }
          }
          if (i == MAX_PLAYER) {
            write_msg(player[player_id].sockfd, "0111");
          }
          break;
        case 12:  // Create table
          if (!read_user_name_update(player[player_id].name,
              player_id)) {
            snprintf(msg_buf, sizeof(msg_buf), "101查無此人");
            write_msg(player[player_id].sockfd, msg_buf);
            break;
          }
          update_client_money(player_id);
          if (player[player_id].money <= MIN_JOIN_MONEY) {
            snprintf(msg_buf, sizeof(msg_buf),
                "101您的賭幣（%ld）不足，必須超過 %d 元才能開桌",
                player[player_id].money, MIN_JOIN_MONEY);
            write_msg(player[player_id].sockfd, msg_buf);
            break;
          }
          player[player_id].port =
              atoi((char *)(buf + 3));
          if (player[player_id].join) {
            if (player[player[player_id].join].serv > 0) {
              player[player[player_id].join].serv--;
            }
            player[player_id].join = 0;
          }
          // Clear all clients from the table
          for (i = 1; i < MAX_PLAYER; i++) {
            if (player[i].join == player_id) {
              player[i].join = 0;
            }
          }
          player[player_id].serv = 1;
          break;
        case 13:  // List tables (detailed)
          list_table(player[player_id].sockfd, 2);
          break;
        case 14:  // Check table creation eligibility
          if (!read_user_name_update(player[player_id].name,
              player_id)) {
            snprintf(msg_buf, sizeof(msg_buf), "101查無此人");
            write_msg(player[player_id].sockfd, msg_buf);
            break;
          }
          update_client_money(player_id);
          if (player[player_id].money <= MIN_JOIN_MONEY) {
            snprintf(msg_buf, sizeof(msg_buf),
                "101您的賭幣（%ld）不足，必須超過 %d 元才能開桌",
                player[player_id].money, MIN_JOIN_MONEY);
            write_msg(player[player_id].sockfd, msg_buf);
            break;
          }
          write_msg(player[player_id].sockfd, "012");  // Confirm table creation
          break;
        case 20:  // Win game
          strncpy(msg_buf, (char *)(buf + 3),
              sizeof(msg_buf) - 1);
          msg_buf[5] = 0;
          player_id = atoi(msg_buf);
          read_user_id(player_id);
          record.money = atol((char *)(buf + 8));
          record.game_count++;
          write_record();
          for (i = 1; i < MAX_PLAYER; i++) {
            if (player[i].login == 2 && player[i].id == player_id) {
              player[i].money = record.money;
              break;
            }
          }
          break;
        case 21:  // Find user
          find_user(player[player_id].sockfd, (char *)(buf + 3));
          break;
        case 900:  // Game record
          err("get game record\n");
          game_log((char *)(buf + 3));
          err("get game record end\n");
          break;
        case 111:
          // player[player_id].serv++;
          break;
        case 200:  // User leaves the game (/LEAVE)
          close_id(player_id);
          break;
        case 202:  // Kick user (admin function)
          if (strncmp(player[player_id].name, ADMIN_USER,
              sizeof(player[player_id].name)) != 0) {
            break;
          }
          player_id = find_user_name(
              (char *)(buf + 3));
          if (player_id >= 0) {
            write_msg(player[player_id].sockfd, "200");
            close_id(player_id);
          }
          break;
        case 205:  // Leave table
          if (player[player_id].serv) {
            // Clear all clients from the table
            for (i = 1; i < MAX_PLAYER; i++) {
              if (player[i].join == player_id) {
                player[i].join = 0;
              }
            }
            player[player_id].serv = 0;
            player[player_id].join = 0;
          } else if (player[player_id].join) {
            if (player[player[player_id].join].serv > 0) {
              player[player[player_id].join].serv--;
            }
            player[player_id].join = 0;
          }
          break;
        case 201:  // Show current state
          show_current_state(player_id);
          break;
        case 500:  // Shutdown server (admin function)
          if (strncmp(player[player_id].name, ADMIN_USER,
              sizeof(player[player_id].name)) == 0) {
            shutdown_server();
          }
          break;
        default:
          snprintf(msg_buf, sizeof(msg_buf),
              "### cmd=%d player_id=%d sockfd=%d ###\n",
              msg_id, player_id, player[player_id].sockfd);
          err(msg_buf);
          close_connection(player_id);
          snprintf(msg_buf, sizeof(msg_buf),
              "Connection to %s error, closed it\n",
              lookup(&(player[player_id].addr)));
          err(msg_buf);
          break;
      }
      buf[0] = '\0';
    }
  }
}

// 關閉特定玩家的連線
void close_id(int player_id) {
  // 宣告變數
  char msg_buf[1000];

  // 關閉玩家連線
  close_connection(player_id);

  // 顯示關閉連線訊息
  snprintf(msg_buf, sizeof(msg_buf), "Connection to %s closed\n",
           lookup(&(player[player_id].addr)));
  err(msg_buf);
}

// 關閉特定玩家的連線
void close_connection(int player_id) {
  // 關閉玩家的 socket
  close(player[player_id].sockfd);

  // 從檔案描述符集合中移除玩家的 socket
  FD_CLR(player[player_id].sockfd, &afds);

  // 檢查玩家是否已加入牌桌
  if (player[player_id].join && player[player[player_id].join].serv) {
    // 減少牌桌伺服器的玩家數量
    player[player[player_id].join].serv--;
  }

  // 重置玩家資訊
  player[player_id].login = 0;
  player[player_id].serv = 0;
  player[player_id].join = 0;
  player[player_id].version[0] = 0;
  player[player_id].note[0] = 0;
  player[player_id].name[0] = 0;
  player[player_id].username[0] = 0;
}

// 關閉伺服器
void shutdown_server() {
  // 宣告變數
  int i;
  char msg_buf[1000];

  // 迴圈遍歷所有玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    // 檢查玩家是否已登入
    if (player[i].login) {
      // 關閉玩家的 socket
      shutdown(player[i].sockfd, 2);
    }
  }

  // 顯示關閉伺服器訊息
  snprintf(msg_buf, sizeof(msg_buf), "QKMJ Server shutdown\n");
  err(msg_buf);

  // 關閉伺服器 socket
  close(gps_sockfd);

  // 退出程式
  exit(0);
}

// Handle core dump signal
void core_dump(int signo) {
  char buf[1024];
  char cmd[1024];
  FILE *fh;

  snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
  if (!(fh = fopen(buf, "r"))) {
    exit(0);
  }
  if (!fgets(buf, sizeof(buf), fh)) {
    exit(0);
  }
  fclose(fh);
  if (buf[strlen(buf) - 1] == '\n') {
    buf[strlen(buf) - 1] = '\0';
  }
  snprintf(cmd, sizeof(cmd), "gdb %s %d", buf, getpid());
  if (system(cmd) != 0) {
    // Handle the error, e.g., print an error message
    err("Error executing gdb command!\n");
  }

  err("CORE DUMP!\n");
  exit(0);
}

// 處理匯流排錯誤訊號
void bus_err() {
  // 顯示錯誤訊息
  err("BUS ERROR!\n");

  // 退出程式
  exit(0);
}

// 處理中斷管線訊號
void broken_pipe() {
  // 顯示錯誤訊息
  err("Broken PIPE!!\n");
}

// 處理超時訊號
void time_out() {
  // 顯示錯誤訊息
  err("timeout!");

  // 設定超時標記
  timeup = 1;
}

// 產生密碼雜湊
char *genpasswd(char *pw) {
  // 宣告變數
  char saltc[2];
  long salt;
  int i, c;
  static char pwbuf[14];

  // 檢查密碼是否為空
  if (strlen(pw) == 0) {
    // 回傳空字串
    return "";
  }

  // 根據時間和程序 ID 產生鹽值
  time(&salt);
  salt = 9 * getpid();

  // 建立鹽值字串
#ifndef lint
  saltc[0] = salt & 077;
  saltc[1] = (salt >> 6) & 077;
#endif
  for (i = 0; i < 2; i++) {
    // 將鹽值轉換為可列印字元
    c = saltc[i] + '.';
    if (c > '9') {
      c += 7;
    }
    if (c > 'Z') {
      c += 6;
    }
    saltc[i] = c;
  }

  // 複製密碼到緩衝區
  strncpy(pwbuf, pw, sizeof(pwbuf) - 1);

  // 回傳加密後的密碼
  return crypt(pwbuf, saltc);
}

// 檢查密碼是否匹配雜湊
int checkpasswd(char *passwd, char *test) {
  // 宣告變數
  static char pwbuf[14];
  char *pw;

  // 複製測試密碼到緩衝區
  strncpy(pwbuf, test, sizeof(pwbuf) - 1);

  // 產生測試密碼的雜湊
  pw = crypt(pwbuf, passwd);

  // 比較產生的雜湊和儲存的雜湊
  return (strncmp(pw, passwd, sizeof(pwbuf)) == 0);
}

// 主函式
int main(int argc, char **argv) {
  // 宣告變數
  int i;

  // 設定檔案描述符的最大數量
  getrlimit(RLIMIT_NOFILE, &fd_limit);
  fd_limit.rlim_cur = fd_limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &fd_limit);
  i = getdtablesize();
  printf("FD_SIZE=%d\n", i);

  // 設定訊號處理函式
  //signal(SIGSEGV, &core_dump);
  signal(SIGBUS, bus_err);
  signal(SIGPIPE, broken_pipe);
  signal(SIGALRM, time_out);

  // 從命令列參數取得埠號
  if (argc < 2) {
    // 使用預設埠號
    gps_port = DEFAULT_GPS_PORT;
  } else {
    // 使用命令列參數指定的埠號
    gps_port = atoi(argv[1]);
  }
  printf("Using port %s\n", argv[1]);

  // 設定預設 IP 地址
  strncpy(gps_ip, DEFAULT_GPS_IP, sizeof(gps_ip) - 1);

  // 初始化 socket 和變數
  init_socket();
  init_variable();

  // 啟動主處理迴圈
  gps_processing();

  // 回傳成功
  return 0;
}