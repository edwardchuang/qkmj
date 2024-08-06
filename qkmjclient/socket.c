
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>  // For errno
#include <string.h> // For memset

#include "qkmj.h"

struct passwd *userdata;

int Check_for_data(int fd)
/* Checks the socket descriptor fd to see if any incoming data has
   arrived.  If yes, then returns 1.  If no, then returns 0.
   If an error, returns -1 and stores the error message in socket_error.
*/
{
  int status;                 /* return code from Select call. */
  fd_set wait_set;     /* A set representing the connections that
				 have been established. */
  struct timeval tm;          /* A timelimit of zero for polling for new
				 connections. */

  FD_ZERO (&wait_set);
  FD_SET (fd, &wait_set);

  tm.tv_sec = 0;
  tm.tv_usec = 500;
  // 使用 select() 檢查是否有資料可讀取
  status = select (FD_SETSIZE, &wait_set, NULL, NULL, &tm);

  if (status < 0) {
    // 處理 select() 錯誤
    perror("Error in select()");
    return -1; 
  }

  // 回傳是否有資料可讀取
  return (status > 0); 

}

void init_serv_socket() {
  struct sockaddr_in serv_addr;

  // 建立 TCP socket
  serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (serv_sockfd < 0) {
    perror("Server: cannot open stream socket"); 
    exit(EXIT_FAILURE); 
  }

  // 設定 socket 選項：允許地址重複使用
  int optval = 1;
  if (setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    perror("Server: setsockopt failed");
    exit(EXIT_FAILURE);
  }

  // 初始化伺服器地址結構
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(0); // 將埠號設為 0，讓系統自動分配一個可用的埠。

  // 綁定 socket。
  if (bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Server: bind failed");
    exit(EXIT_FAILURE);
  }

  // 取得系統分配的埠號。
  socklen_t addrlen = sizeof(serv_addr);
  if (getsockname(serv_sockfd, (struct sockaddr *)&serv_addr, &addrlen) < 0) {
    perror("Server: getsockname failed");
    exit(EXIT_FAILURE);
  }

  SERV_PORT = ntohs(serv_addr.sin_port);

  // 開始監聽連線請求
  if (listen(serv_sockfd, 10) < 0) {
    perror("Server: listen failed");
    exit(EXIT_FAILURE);
  }

  // 將伺服器 socket 加入 afds 集合
  FD_SET(serv_sockfd, &afds);
}

void get_my_info() {
  // 取得使用者資訊
  userdata = getpwuid(getuid());
  if (userdata == NULL) {
    perror("Error getting user information");
    exit(EXIT_FAILURE);
  }

  // 複製使用者名稱，使用 strncpy 防止緩衝區溢位
  strncpy(my_username, userdata->pw_name, sizeof(my_username) - 1);
  my_username[sizeof(my_username) - 1] = '\0'; // Ensure null-termination
}

int init_socket(char *host, int portnum, int *sockfd) {
  struct sockaddr_in serv_addr;
  struct hostent *hp;

  // 初始化伺服器地址結構
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  // 取得主機資訊
  if ((hp = gethostbyname(host)) == NULL || hp->h_addrtype != AF_INET) {
    // 使用 inet_addr() 解析主機地址
    serv_addr.sin_addr.s_addr = inet_addr(host);
  } else {
    // 複製主機地址
    memcpy(&serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length);
  }

  serv_addr.sin_port = htons(portnum);

  // 建立 TCP socket
  *sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (*sockfd < 0) {
    perror("Client: cannot open stream socket");
    return -1;
  }

  // 連線到伺服器
  if (connect(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Client: connect failed");
    return -1;
  }

  return 0;
}

void accept_new_client() {
  socklen_t alen = sizeof(struct sockaddr_in); // Use socklen_t for address size
  int i, player_id;
  char msg_buf[255];

  // 尋找空位
  for (i = 2; i < MAX_PLAYER; i++) {
    if (!player[i].in_table) break;
  }
  if (i == MAX_PLAYER) {
    perror("Too many players!");
    exit(EXIT_FAILURE);
  }
  player_id = i;

  // 接受新的連線
  player[player_id].sockfd = accept(serv_sockfd, (struct sockaddr *)&player[player_id].addr, &alen);
  if (player[player_id].sockfd < 0) {
    perror("Server: accept failed");
    exit(EXIT_FAILURE);
  }

  // 設定新的玩家資訊
  FD_SET(player[player_id].sockfd, &afds);
  player[player_id].in_table = 1;
  player_in_table++;

  // 分配座位給新玩家
  if (player_in_table <= PLAYER_NUM) {
    for (i = 1; i <= 4; i++) {
      if (!table[i]) {
        player[player_id].sit = i;
        table[i] = player_id;
        break;
      }
    }
  }

  // 發送訊息通知新玩家加入
  snprintf(msg_buf, sizeof(msg_buf), "%s 加入此桌，目前人數 %d ", new_client_name, player_in_table);
  send_gps_line(msg_buf);

  // 設定新玩家資訊
  strncpy(player[player_id].name, new_client_name, sizeof(player[player_id].name) - 1);
  player[player_id].name[sizeof(player[player_id].name) - 1] = '\0'; // Ensure null-termination
  player[player_id].id = new_client_id;
  player[player_id].money = new_client_money;

  // 廣播新玩家資訊給所有玩家
  snprintf(msg_buf, sizeof(msg_buf), "201%c%c%c%s", (char)player_id, player[player_id].sit, player_in_table, player[player_id].name);
  broadcast_msg(1, msg_buf); 

  // 發送訊息給新玩家
  msg_buf[2] = '5';  // 設定訊息 ID 為 205
  write_msg(player[player_id].sockfd, msg_buf);

  // 發送更多新玩家資訊
  snprintf(msg_buf, sizeof(msg_buf), "202%c%5d%ld", player_id, new_client_id, new_client_money);
  broadcast_msg(1, msg_buf);

  // 更新遊戲狀態
  new_client = 0;
  write_msg(gps_sockfd, "111"); 

  // 發送所有玩家資訊給新玩家
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != player_id) {
      snprintf(msg_buf, sizeof(msg_buf), "203%c%c%s", i, player[i].sit, player[i].name);
      write_msg(player[player_id].sockfd, msg_buf);
      snprintf(msg_buf, sizeof(msg_buf), "202%c%5d%ld", i, player[i].id, player[i].money);
      write_msg(player[player_id].sockfd, msg_buf);
    }
  }

  // 檢查遊戲人數，開始遊戲
  if (player_in_table == PLAYER_NUM) {
    init_playing_screen();
    broadcast_msg(1, "300");
    opening();
    open_deal();
  }
}

int read_msg(int fd, char *msg) {
  // 讀取訊息直到遇到 null terminator
  char *msg_ptr = msg; 
  do {
    ssize_t bytes_read = read(fd, msg_ptr, 1);
    if (bytes_read <= 0) { 
      return bytes_read; // 回傳 0 表示 EOF，-1 表示錯誤
    }
    msg_ptr++; 
  } while (*(msg_ptr - 1) != '\0'); 

  return msg_ptr - msg; // 回傳讀取的位元組數（包含 null terminator）
}

int read_msg_id(int fd, char *msg) {
  // 讀取訊息 ID (3 個字元)
  for (int i = 0; i < 3; i++) {
    ssize_t bytes_read = read(fd, msg + i, 1); 
    if (bytes_read <= 0 || msg[i] == '\0') {
      return 0; // 讀取錯誤或遇到 null terminator
    }
  }
  return 1; 
}

int write_msg(int fd, char *msg) {
  // 寫入訊息到 socket
  size_t msg_len = strlen(msg);
  // 使用 write() 兩次確保訊息和 null terminator 都被寫入
  if (write(fd, msg, msg_len) != msg_len || write(fd, "\0", 1) != 1) {
    perror("Error writing to socket"); // 發生錯誤時輸出錯誤訊息
    return -1; 
  }
  return 0; 
}

/* Command for server */
void broadcast_msg(int id, char *msg) {
  // 廣播訊息給所有玩家 (除了指定的 id)
  for (int i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != id) {
      write_msg(player[i].sockfd, msg);
    }
  }
}

void close_client(int player_id) {
  char msg_buf[255];

  if (player_in_table == 4) {
    init_global_screen();
    input_mode = TALK_MODE;
  }
  player_in_table--;
  snprintf(msg_buf, sizeof(msg_buf), "206%c%c", player_id, player_in_table);
  broadcast_msg(player_id, msg_buf);
  snprintf(msg_buf, sizeof(msg_buf), "%s 離開此桌，目前人數剩下 %d 人", player[player_id].name, player_in_table);
  display_comment(msg_buf);

  // 關閉 client socket
  if (close(player[player_id].sockfd) < 0) {
    perror("Error closing client socket");
  }

  FD_CLR(player[player_id].sockfd, &afds);
  player[player_id].in_table = 0;
  table[player[player_id].sit] = 0;
}

void close_join() {
  in_join = 0;
  write_msg(table_sockfd, "200");

  // 關閉 table socket
  if (close(table_sockfd) < 0) {
    perror("Error closing table socket");
  }

  FD_CLR(table_sockfd, &afds);
  /*
    shutdown(table_sockfd,2);
  */
}

void close_serv() {
  in_serv = 0;
  for (int i = 2; i < MAX_PLAYER; i++) { // Note that i start from 2
    if (player[i].in_table) {
      write_msg(player[i].sockfd, "199");
      close_client(i);
      /*
        shutdown(player[i].sockfd,2);
      */
    }
  }
  FD_CLR(serv_sockfd, &afds);
}

void leave() /* the ^C trap. */ {
  write_msg(gps_sockfd, "200");
  /*
    shutdown(gps_sockfd,2);
  */

  // 關閉 gps socket
  if (close(gps_sockfd) < 0) {
    perror("Error closing gps socket");
  }

  if (in_join) {
    close_join();
  }
  if (in_serv) {
    close_serv();
  }
  endwin();
  exit(0);
}