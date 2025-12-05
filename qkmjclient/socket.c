#include "socket.h"

struct passwd *userdata;

// 檢查是否有資料可讀
// 傳回值: 1: 有資料, 0: 沒有資料, -1: 錯誤
int check_for_data(int fd)
{
  fd_set read_fds;
  struct timeval timeout;

  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  timeout.tv_sec = 0;
  timeout.tv_usec = 500; // 0.5 毫秒

  return select(fd + 1, &read_fds, NULL, NULL, &timeout);
}

// 初始化伺服器 socket
void init_serv_socket()
{
  struct sockaddr_in serv_addr;
  int bind_result;

  // 建立 TCP socket
  if ((serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err("伺服器: 無法建立 stream socket");
  }

  // 綁定本地地址
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(SERV_PORT);

  bind_result = bind(serv_sockfd, (struct sockaddr *)&serv_addr,
                     sizeof(serv_addr));
  if (bind_result < 0) {
    err("伺服器: 無法綁定 socket"); // 錯誤處理待完善
  }

  listen(serv_sockfd, 10);
  FD_SET(serv_sockfd, &afds);
}

// 取得自己的使用者名稱
void get_my_info()
{
  userdata = getpwuid(getuid());
  if (userdata != NULL) {
    strncpy(my_username, userdata->pw_name, sizeof(my_username) - 1);
    my_username[sizeof(my_username) -1] = '\0'; //確保 null-terminated
  } else {
    err("無法取得使用者名稱");
  }
}

// 初始化 client socket
// 傳回值: 0: 成功, -1: 失敗
int init_socket(const char *host, int portnum, int *sockfd)
{
  struct sockaddr_in serv_addr;
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int error = getaddrinfo(host, NULL, &hints, &res);
  if (error) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    return -1;
  }

  memcpy(&serv_addr, res->ai_addr, res->ai_addrlen);
  serv_addr.sin_port = htons(portnum);
  freeaddrinfo(res);

  // 建立 TCP socket
  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err("客戶端: 無法建立 stream socket");
  }

  // 連線到伺服器
  if (connect(*sockfd, (struct sockaddr *)&serv_addr,
              sizeof(serv_addr)) < 0) {
    return -1;
  }

  return 0;
}

// 接受新的 client 連線
void accept_new_client()
{
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int i, player_id;
  char msg_buf[MAX_MSG_LENGTH]; // Use MAX_MSG_LENGTH for consistency
  char error_message[1024]; // For error messages

  // 尋找空位
  for (i = 2; i < MAX_PLAYER; i++) {
    if (!player[i].in_table) break;
  }

  // 處理桌子已滿的情況
  if (i == MAX_PLAYER) {
    snprintf(error_message, sizeof(error_message), "桌子已滿，無法加入新玩家！");
    err(error_message);
    return;
  }

  player_id = i;

  // 接受新的連線
  player[player_id].sockfd = accept(serv_sockfd,
                                    (struct sockaddr *)&client_addr,
                                    &client_addr_len);
  if (player[player_id].sockfd < 0) {
    err("接受新連線失敗");
    return;
  }

  FD_SET(player[player_id].sockfd, &afds);
  player[player_id].in_table = 1;
  player_in_table++;

  // 驗證並設定玩家資訊 (Validating input is crucial here)
  if (strlen(new_client_name) >= sizeof(player[player_id].name)) {
    snprintf(error_message, sizeof(error_message), "玩家名稱過長");
    err(error_message);
    close_client(player_id); // Close the connection on error
    return;
  }
  strncpy(player[player_id].name, new_client_name, sizeof(player[player_id].name) - 1);
  player[player_id].name[sizeof(player[player_id].name) - 1] = '\0';
  player[player_id].id = new_client_id;
  player[player_id].money = new_client_money;

  // 分配座位
  for (i = 1; i <= 4; i++) {
    if (!table[i]) {
      player[player_id].sit = i;
      table[i] = player_id;
      break;
    }
  }

  // 發送訊息
  snprintf(msg_buf, sizeof(msg_buf), "%s 加入此桌，目前人數 %d",
           player[player_id].name, player_in_table);
  send_gps_line(msg_buf);

  // 廣播新玩家資訊
  snprintf(msg_buf, sizeof(msg_buf), "201%c%c%c%s",
           (char)player_id, player[player_id].sit, player_in_table, player[player_id].name);
  broadcast_msg(1, msg_buf); // 包含新玩家


  // 發送訊息給新玩家
  snprintf(msg_buf, sizeof(msg_buf), "205%c%c%c%s",
           (char)player_id, player[player_id].sit, player_in_table, player[player_id].name);
  write_msg(player[player_id].sockfd, msg_buf);

  snprintf(msg_buf, sizeof(msg_buf), "202%c%5d%ld", player_id,
           new_client_id, new_client_money);
  broadcast_msg(1, msg_buf);

  new_client = 0;  // 處理完畢
  write_msg(gps_sockfd, "111");  // 通知遊戲伺服器

// 發送現有玩家資訊給新玩家 (This is the corrected and essential part)
    for (i = 1; i < MAX_PLAYER; i++) {
      if (player[i].in_table && i != player_id) {
        // 通知新玩家其他玩家的資訊
        snprintf(msg_buf, sizeof(msg_buf), "203%c%c%s", i, player[i].sit, player[i].name);
        write_msg(player[player_id].sockfd, msg_buf);
        snprintf(msg_buf, sizeof(msg_buf), "202%c%5d%ld", i, player[i].id, player[i].money);
        write_msg(player[player_id].sockfd, msg_buf);
      }
    }

  // 檢查遊戲是否可以開始
  if (player_in_table == PLAYER_NUM) {
    init_playing_screen();
    broadcast_msg(1, "300");
    opening();
    open_deal();
  }
}


// 從 socket 讀取訊息
int read_msg(int fd, char *msg)
{
  ssize_t bytes_read;
  size_t total_bytes_read = 0;
  char char_buf[2]; // Read one char at a time + null terminator.

    memset(msg, 0, MAX_MSG_LENGTH); // Initialize the buffer
    // Keep reading until we hit the null terminator or buffer limit.
    while (total_bytes_read < MAX_MSG_LENGTH - 1) {
      bytes_read = read(fd, char_buf, 1);
      if (bytes_read < 0) {
        if (errno == EINTR) {
            continue; // Interrupted by signal, retry
        }
        err("讀取訊息失敗");
        return 0; // Indicate an error
      } else if (bytes_read == 0) { 
        err("對方已關閉連線");
        return 0; // Indicate closed connection
      }

      msg[total_bytes_read] = char_buf[0];
      total_bytes_read++;

        if (char_buf[0] == '\0') {
            return 1; // Message successfully read
        }
    }
  // If we get here, the message was too long.
  err("訊息過長");
  msg[MAX_MSG_LENGTH - 1] = '\0'; // Ensure null-termination
  return 0; // Indicate an error (message truncated)
}


// 從 socket 讀取訊息 ID (前三個字元)
int read_msg_id(int fd, char *msg)
{
  ssize_t bytes_read;
  size_t total_read = 0;

  memset(msg, 0, 4);      // Initialize the buffer
  
  while (total_read < 3) {
      bytes_read = read(fd, msg + total_read, 3 - total_read);
      if (bytes_read < 0) {
          if (errno == EINTR) {
              continue;
          }
          err("讀取訊息 ID 失敗");
          return 0;
      } else if (bytes_read == 0) {
          err("對方已關閉連線");
          return 0;
      }
      total_read += bytes_read;
  }
  
  msg[3] = '\0';  // Null-terminate for safety
  return 1;
}
      
// 寫入訊息到 socket
int write_msg(int fd, const char *msg)
{
  size_t len = strlen(msg) + 1; // Include null terminator
  size_t total_written = 0;
  ssize_t bytes_written;

  while (total_written < len) {
      bytes_written = write(fd, msg + total_written, len - total_written);
      if (bytes_written < 0) {
          if (errno == EINTR) {
              continue;
          }
          err("寫入訊息失敗");
          return -1;
      }
      total_written += bytes_written;
  }
  return (int)total_written;
}

// 廣播訊息
void broadcast_msg(int id, const char *msg)
{
  int i;
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != id) {
      write_msg(player[i].sockfd, msg);
    }
  }
}

// 關閉 client 連線
void close_client(int player_id)
{
  char msg_buf[256]; // Increased for safety
  char error_message[1024];

  // 檢查遊戲是否正在進行，如果是，則初始化全局畫面
  if (player_in_table == 4) { // Assumes 4 players for a full game
    init_global_screen();
    input_mode = TALK_MODE;
  }

  player_in_table--;

  // 通知其他玩家此玩家已離開
  snprintf(msg_buf, sizeof(msg_buf), "206%c%c", player_id, player_in_table);
  broadcast_msg(player_id, msg_buf); // Exclude the leaving player

  // 顯示離開訊息
  snprintf(msg_buf, sizeof(msg_buf), "%s 離開此桌，目前人數剩下 %d 人",
           player[player_id].name, player_in_table);
  display_comment(msg_buf);

  // 關閉 client socket
  if (player[player_id].sockfd >= 0) { // Check if socket is valid
    FD_CLR(player[player_id].sockfd, &afds);
    if (close(player[player_id].sockfd) < 0) {
      err("關閉 client socket 失敗");
    }
  }

  // 更新遊戲狀態
  player[player_id].in_table = 0;
  table[player[player_id].sit] = 0;
}



// 關閉加入遊戲的連線
void close_join()
{
  char msg_buf[256]; // Increased for potential messages
  char error_message[1024];

  in_join = 0;

  //  通知遊戲伺服器
  snprintf(msg_buf, sizeof(msg_buf), "200離開遊戲");
  if (write_msg(table_sockfd, msg_buf) < 0) {
    err("寫入離開訊息失敗");
  }

  // 關閉 socket
  if (table_sockfd >= 0) { // Check for valid socket
    FD_CLR(table_sockfd, &afds);
    if (close(table_sockfd) < 0) {
      err("關閉加入遊戲 socket 失敗");
    }
  }
}

// 關閉伺服器
void close_serv()
{
  int i;
  char msg_buf[256]; // Increased for potential messages

  in_serv = 0;

  // 通知所有客戶端伺服器關閉
  for (i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table) {
      snprintf(msg_buf, sizeof(msg_buf), "199伺服器關閉中..."); // More info
      write_msg(player[i].sockfd, msg_buf); // Send before closing
      close_client(i); // Close client connections gracefully
    }
  }

  // 關閉伺服器 socket
  if (serv_sockfd >= 0) { // Check if the socket is valid
    FD_CLR(serv_sockfd, &afds);
    if (close(serv_sockfd) < 0) { // Handle potential close error
      snprintf(msg_buf, MAX_MSG_LENGTH,
               "關閉伺服器 socket 失敗: %s", strerror(errno));
      err(msg_buf); // Or other appropriate error handling
    }
  }
}

// 處理 Ctrl+C 中斷訊號
void leave()
{
  write_msg(gps_sockfd, "200");
  close(gps_sockfd);

  if (in_join) close_join();
  if (in_serv) close_serv();

  endwin();
  exit(0);
}

