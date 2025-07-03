/*
 * Server 
 */
#include "mjgps.h"

// 常數定義，使用 MACRO 避免 magic number
#define ADMIN_USER  "mjgps"
#define MIN_JOIN_MONEY 0  // 允許使用者負債加入，可設定為 -999999

// 全域變數
int timeup = 0;
extern int errno;
fd_set rfds, afds;
int login_limit;
char gps_ip[20];
int gps_port;
int log_level;
// 使用 char array 避免潛在的字串長度問題
char number_map[10][5] = { "０", "１", "２", "３", "４", "５", "６", "７", "８", "９" };

int gps_sockfd;
char climark[30];
struct player_info player[MAX_PLAYER];
struct player_record record;
struct record_index_type record_index;
FILE *fp, *log_fp;
struct ask_mode_info ask;
struct rlimit fd_limit;


// 錯誤處理函式，附加時間戳記
int err(const char *errmsg) {
    time_t now;
    time(&now);
    char timestamp[20];  // 時間戳記字串
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if ((log_fp = fopen(LOG_FILE, "a")) == NULL) {
        perror("無法開啟記錄檔"); // 使用 perror 提供更詳細的錯誤訊息
        return -1;
    }

    fprintf(stderr, "[%s] %s", timestamp, errmsg); // 輸出到 stderr
    if (log_level == 0) {
        fprintf(log_fp, "[%s] %s", timestamp, errmsg);
    }

    log_level = 0;
    fclose(log_fp);
    return -1; // 返回錯誤碼表示失敗
}


// 遊戲記錄函式，附加時間戳記
int game_log(const char *gamemsg) {
    time_t now;
    time(&now);
    char timestamp[20]; // 時間戳記字串
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));


    if ((log_fp = fopen(GAME_FILE, "a")) == NULL) {
        perror("無法開啟遊戲記錄檔"); // 使用 perror 提供更詳細的錯誤訊息
        return -1;
    }

    fprintf(log_fp, "[%s] %s", timestamp, gamemsg);
    fclose(log_fp);
    return 0; //  返回成功碼
}


// 讀取訊息，使用 select() 處理 timeout，避免阻塞
int read_msg(int fd, char *msg) {
    int n = 0;
    char msg_buf[1000];
    int read_code;

    if (Check_for_data(fd) == 0) {
        err("讀取錯誤\n");
        return 2;
    }

    timeup = 0;
    alarm(5); // 設定 timeout 時間

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;


    while (1) { // 使用 while(1) 取代 goto
        int select_result = select(fd + 1, &readfds, NULL, NULL, &tv);

        if (select_result == -1) {
            if (errno != EINTR) {
                snprintf(msg_buf, sizeof(msg_buf), "read_msg 讀取失敗，錯誤碼：%d", errno);
                err(msg_buf);
                alarm(0);
                return 0;
            } // EINTR 表示被信號中斷，繼續 select()
        } else if (select_result == 0) {
            alarm(0);
            err("逾時！\n");
            return 0;
        } else if (FD_ISSET(fd, &readfds)) {
          read_code = read(fd, msg + n, 1);
            if (read_code == -1) {
                if (errno != EWOULDBLOCK) {
                    snprintf(msg_buf, sizeof(msg_buf), "read_msg 讀取失敗，錯誤碼：%d", errno);
                    err(msg_buf);
                    alarm(0);
                    return 0;
                } // EWOULDBLOCK 表示非阻塞模式下沒有數據可讀，繼續 select()
            } else if (read_code == 0) {
                alarm(0);
                return 0; // 連線關閉
            } else {
                n++;
                if (msg[n -1] == '\0') {
                  alarm(0);
                  return 1;
                }
                if (n >= 999) { // 限制訊息長度，避免緩衝區溢位
                  alarm(0);
                  err("訊息過長！\n");
                  return 0;
                }
            }
        }
    }
}

// 寫入訊息，確保完整寫入
void write_msg(int fd, const char *msg) {
    size_t n = strlen(msg);
    ssize_t bytes_written = 0;

    while (bytes_written < (ssize_t)n) {
        ssize_t result = write(fd, msg + bytes_written, n - bytes_written);
        if (result < 0) {
            if (errno == EINTR) {
                continue; // 被信號中斷，重新 write()
            }
            perror("寫入訊息失敗");
            close(fd);
            FD_CLR(fd, &afds);
            return;
        }
        bytes_written += result;
    }

    if (write(fd, "\0", 1) < 0) { // 寫入 null terminator
        perror("寫入 null terminator 失敗");
        close(fd);
        FD_CLR(fd, &afds);
    }
}

// 顯示訊息給玩家
void display_msg(int player_id, const char *msg) {
    char msg_buf[1000];
    snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);
    write_msg(player[player_id].sockfd, msg_buf);
}

// 檢查是否有數據可讀，使用 select() 避免 busy waiting
int Check_for_data(int fd) {
    int status;
    fd_set readfds;
    struct timeval tv;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    status = select(fd + 1, &readfds, NULL, NULL, &tv);

    // if (status < 0) { ... }  // 錯誤處理

    return status;
}

// 檢查密碼
int checkpasswd(const char *passwd, const char *test) {
    char pwbuf[14];
    char *pw;

    strncpy(pwbuf, test, sizeof(pwbuf) -1); // 避免 buffer overflow
    pwbuf[sizeof(pwbuf) - 1] = '\0'; //確保null termination

    pw = crypt(pwbuf, passwd); // 使用crypt() 函式
    return (strncmp(pw, passwd, strlen(passwd)) == 0); // 使用 strncmp() 避免 buffer overflow
}


//  產生密碼，使用更安全的鹽值生成方式
char *genpasswd(const char *pw) {
    char saltc[3];  // 鹽值字元，改用三個字元以符合 crypt() 需求
    char pwbuf[14]; // 使用 char array 避免潛在的字串長度問題

    // 若密碼為空字串，則回傳空字串
    if (strlen(pw) == 0) return "";

    // 使用 time() 和 getpid() 產生更隨機的鹽值，初始化亂數產生器
    srand(time(NULL) ^ getpid());

    // 產生兩個隨機字元作為鹽值
    for (int i = 0; i < 2; i++) {
        // 確保鹽值字元在 crypt() 可接受的範圍內 (./0-9A-Za-z)
        saltc[i] = (rand() % (75)) + 46; 
        // 排除 :;<=>? 等特殊符號
        if (saltc[i] >= 58 && saltc[i] <= 63){ 
            i--;
        }
    }
	saltc[2] = '\0'; //確保null termination

    // 將密碼複製到 pwbuf，避免修改原始密碼，並防止 buffer overflow
    strncpy(pwbuf, pw, sizeof(pwbuf) -1); 
    pwbuf[sizeof(pwbuf) - 1] = '\0'; //確保null termination

    // 使用 crypt() 函式產生加密密碼，並回傳
    return crypt(pwbuf, saltc); 
}


// Convert message ID
int convert_msg_id(int player_id, const char *msg) {
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

// 初始化 socket
void init_socket() {
    struct sockaddr_in serv_addr;
    int on = 1;
    int ret;

    // 建立 socket
    if ((gps_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err("Server: cannot open stream socket");
        return;
    }

    // 設定 socket 選項，允許重複使用地址
    ret = setsockopt(gps_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0) {
        perror("setsockopt failed");
        close(gps_sockfd);
        return;
    }

    // 設定伺服器地址
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(gps_port);


    // 繫結 socket
    ret = bind(gps_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        perror("bind failed");
        close(gps_sockfd);
        return;
    }

    // 開始監聽連線
    ret = listen(gps_sockfd, 10);
    if (ret < 0) {
        perror("listen failed");
        close(gps_sockfd);
        return;
    }

    printf("Listen for client...\n");
}

// 查詢主機名稱
char *lookup(const struct sockaddr_in *cli_addrp) {
  	return inet_ntoa(cli_addrp->sin_addr); //回傳IP位址當作備案
}

// 初始化變數
void init_variable() {
    int i;

    login_limit = LOGIN_LIMIT;
    for (i = 0; i < MAX_PLAYER; i++) {
        player[i].login = 0;
        player[i].serv = 0;
        player[i].money = 0;
        player[i].join = 0;
        player[i].type = 16;
        player[i].note[0] = '\0';
        player[i].username[0] = '\0';
        player[i].version[0] = '\0';
        player[i].name[0] = '\0';
    }
}


// 讀取使用者名稱
int read_user_name(const char *name) {
    struct player_record tmp_rec;
    char msg_buf[1000];

    if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "(read_user_name) Cannot open file!\n");
        err(msg_buf);
        return 0;
    }

    while (fread(&tmp_rec, sizeof(tmp_rec), 1, fp) == 1) { //檢查fread回傳值
        if (strncmp(name, tmp_rec.name, sizeof(tmp_rec.name) -1) == 0) { //避免buffer overflow
            record = tmp_rec;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}


// 更新使用者名稱
int read_user_name_update(const char *name, int player_id) {
    struct player_record tmp_rec;
    char msg_buf[1000];

    if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "(read_user_name_update) Cannot open file!\n");
        err(msg_buf);
        return 0;
    }

    while (fread(&tmp_rec, sizeof(tmp_rec), 1, fp) == 1) { //檢查fread回傳值
        if (strncmp(name, tmp_rec.name, sizeof(tmp_rec.name) -1) == 0) {  //避免buffer overflow
            record = tmp_rec;
            if (player[player_id].id == record.id) { // double check
                player[player_id].id = record.id;
                player[player_id].money = record.money;
            }
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}


// 讀取使用者 ID
int read_user_id(unsigned int id) {
    char msg_buf[1000];

    if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "(read_user_id) Cannot open file!\n");
        err(msg_buf);
        return 0;
    }

    fseek(fp, (long)sizeof(record) * id, SEEK_SET); // 使用 fseek() 移動檔案指標
    if (fread(&record, sizeof(record), 1, fp) != 1) { //檢查fread回傳值
      perror("fread failed");
    }
    fclose(fp);
    return 1;
}


// 新增使用者
int add_user(int player_id, const char *name, const char *passwd) {
    struct stat status;
    char msg_buf[1000];

    stat(RECORD_FILE, &status);
    if (!read_user_name("")) {
      record.id = status.st_size / sizeof(record);
    }
    
    strncpy(record.name, name, sizeof(record.name) -1); // 避免 buffer overflow
    record.name[sizeof(record.name) - 1] = '\0'; //確保null termination
    strncpy(record.password, genpasswd(passwd), sizeof(record.password) -1); // 避免 buffer overflow
    record.password[sizeof(record.password) - 1] = '\0'; //確保null termination
    record.money = DEFAULT_MONEY;
    record.level = 0;
    record.login_count = 1;
    record.game_count = 0;
    time(&record.regist_time);
    record.last_login_time = record.regist_time;
    record.last_login_from[0] = '\0';
    
    if (player[player_id].username[0] != '\0') {
        snprintf(record.last_login_from, sizeof(record.last_login_from), "%s@", player[player_id].username);
    }
    strncat(record.last_login_from, lookup(&(player[player_id].addr)), sizeof(record.last_login_from) - strlen(record.last_login_from) -1); // 避免 buffer overflow
    record.last_login_from[sizeof(record.last_login_from) - 1] = '\0'; //確保null termination

    if (check_user(player_id)) {
        write_record();
        return 1;
    } else {
        return 0;
    }
}


// 檢查使用者
int check_user(int player_id) {
    char msg_buf[1000];
    char from[80];
    char email[80];
    FILE *baduser_fp;

    if ((baduser_fp = fopen(BADUSER_FILE, "r")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "Cannot open file %s", BADUSER_FILE);
        err(msg_buf);
        return 1;
    }

    strncpy(from, lookup(&(player[player_id].addr)), sizeof(from) -1); // 避免 buffer overflow
    from[sizeof(from) - 1] = '\0'; //確保null termination
    snprintf(email, sizeof(email), "%s@", player[player_id].username);
    strncat(email, from, sizeof(email) - strlen(email) -1); // 避免 buffer overflow
    email[sizeof(email) - 1] = '\0'; //確保null termination

    while (fgets(msg_buf, sizeof(msg_buf) - 1, baduser_fp) != NULL) {
        msg_buf[strcspn(msg_buf, "\n")] = 0; //移除換行字元
        if (strcmp(email, msg_buf) == 0 || strcmp(player[player_id].username, msg_buf) == 0) {
            display_msg(player_id, "你已被限制進入");
            fclose(baduser_fp);
            return 0;
        }
    }
    fclose(baduser_fp);
    return 1;
}


// 寫入記錄
void write_record() {
    char msg_buf[1000];

    fp = fopen(RECORD_FILE, "wb");
    if (fp == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "(write_record) Cannot open file!");
        err(msg_buf);
        return;
    }

    fseek(fp, (long)sizeof(record) * record.id, SEEK_SET); // 使用 fseek() 移動檔案指標
    if (fwrite(&record, sizeof(record), 1, fp) != 1) { //檢查fwrite回傳值
      perror("fwrite failed");
    }

    fclose(fp);
}



// 顯示新聞
void print_news(int fd, const char *name) {
    FILE *news_fp;
    char msg[255];
    char msg_buf[1000];

    if ((news_fp = fopen(name, "r")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "Cannot open file %s\n", NEWS_FILE);
        err(msg_buf);
        return;
    }

    while (fgets(msg, sizeof(msg) - 1, news_fp) != NULL) {
        msg[strcspn(msg, "\n")] = 0; //移除換行字元
        snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);
        write_msg(fd, msg_buf);
    }
    fclose(news_fp);
}


// 歡迎使用者
void welcome_user(int player_id) {
    char msg_buf[1000];
    int fd;
    int i;
    struct player_record tmp_rec;

    fd = player[player_id].sockfd;

    if (strncmp(player[player_id].version, "093", 3) < 0 || player[player_id].version[0] == '\0') {
        write_msg(player[player_id].sockfd, "101請使用 QKMJ Ver 0.93 Beta 以上版本上線");
        write_msg(player[player_id].sockfd, "010");
        return;
    }

    snprintf(msg_buf, sizeof(msg_buf), "101★★★★★　歡迎 %s 來到ＱＫ麻將  ★★★★★", player[player_id].name);
    write_msg(player[player_id].sockfd, msg_buf);
    print_news(player[player_id].sockfd, NEWS_FILE);

    player[player_id].id = record.id;
    player[player_id].money = record.money;
    player[player_id].login = 2;
    player[player_id].note[0] = '\0';
    show_online_users(player_id);
    list_stat(player[player_id].sockfd, player[player_id].name);
    write_msg(player[player_id].sockfd, "003");
    snprintf(msg_buf, sizeof(msg_buf), "120%05d%ld", player[player_id].id, player[player_id].money); // 使用 snprintf() 並且補零
    write_msg(player[player_id].sockfd, msg_buf);
    player[player_id].input_mode = CMD_MODE;
}


// 顯示線上使用者
void show_online_users(int player_id) {
    char msg_buf[1000];
    int fd;
    int total_num = 0;
    int online_num = 0;
    int i;
    struct player_record tmp_rec;
    char tmp_buf[1000];

    fd = player[player_id].sockfd;

    if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "(current) cannot open file\n");
        err(msg_buf);
    } else {
        while (fread(&tmp_rec, sizeof(tmp_rec), 1, fp) == 1) { //檢查fread回傳值
            if (tmp_rec.name[0] != '\0') total_num++;
        }
        fclose(fp);
    }

    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2) online_num++;
    }

    snprintf(msg_buf, sizeof(msg_buf), "101◇目前上線人數: %d 人       注冊人數: %d 人", online_num, total_num);
    write_msg(player[player_id].sockfd, msg_buf);
}


// 顯示目前狀態
void show_current_state(int player_id) {
    show_online_users(player_id);
    list_stat(player[player_id].sockfd, player[player_id].name);
    write_msg(player[player_id].sockfd, "003");
    char msg_buf[1000];
    snprintf(msg_buf, sizeof(msg_buf), "120%05d%ld", player[player_id].id, player[player_id].money); // 使用 snprintf() 並且補零
    write_msg(player[player_id].sockfd, msg_buf);
}


// 更新客戶端金額
void update_client_money(int player_id) {
    char msg_buf[1000];
    snprintf(msg_buf, sizeof(msg_buf), "120%05d%ld", player[player_id].id, player[player_id].money); // 使用 snprintf() 並且補零
    write_msg(player[player_id].sockfd, msg_buf);
}



// 尋找使用者名稱
int find_user_name(const char *name) {
    int i;

    for (i = 1; i < MAX_PLAYER; i++) {
        if (strncmp(player[i].name, name, sizeof(player[i].name) -1) == 0) { //避免buffer overflow
            return i;
        }
    }
    return -1;
}


// 關閉伺服器
void shutdown_server() {
    int i;
    char msg_buf[1000];

    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login) {
            close(player[i].sockfd); // 使用 close() 關閉 socket 連線
            FD_CLR(player[i].sockfd, &afds); // 清除檔案描述詞集合中的 socket
        }
    }

    snprintf(msg_buf, sizeof(msg_buf), "QKMJ 伺服器已關閉\n"); // 使用 snprintf() 避免緩衝區溢位
    err(msg_buf);
    exit(0);
}


// 核心傾印
void core_dump(int signo) {
    char buf[1024];
    char cmd[1024];
    FILE *fh;

    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
    if ((fh = fopen(buf, "r")) == NULL) {
        exit(0);
    }
    if (fgets(buf, sizeof(buf), fh) == NULL) {
        fclose(fh);
        exit(0);
    }
    fclose(fh);
    if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }
    snprintf(cmd, sizeof(cmd), "gdb %s %d", buf, getpid());
    system(cmd);

    err("核心傾印！\n");
    exit(0);
}


// 匯流排錯誤
void bus_err() {
    err("匯流排錯誤！\n");
    exit(0);
}


// 管道損毀
void broken_pipe() {
    err("管道損毀!!\n");
}


// 超時
void time_out() {
    err("逾時！");
    timeup = 1;
}


// 顯示玩家列表
void list_player(int fd) {
    int i;
    char msg_buf[1000];
    int total_num = 0;

    char prefix[] = "101-------------    目前上線使用者    ---------------\n"; // 使用 char array 避免潛在的字串長度問題

    write_msg(fd, prefix);

    snprintf(msg_buf, sizeof(msg_buf), "101"); // 使用 snprintf() 並且避免緩衝區溢位

    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2) {
            total_num++;
            if (strlen(msg_buf) + strlen(player[i].name) + 3 > sizeof(msg_buf) -1) { // 檢查緩衝區大小
                write_msg(fd, msg_buf);
                snprintf(msg_buf, sizeof(msg_buf), "101"); // 重置緩衝區
            }
            strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) -1); //避免buffer overflow
            strncat(msg_buf, "  ", sizeof(msg_buf) - strlen(msg_buf) -1); //避免buffer overflow
        }
    }
    if (strlen(msg_buf) > 4) {
        write_msg(fd, msg_buf);
    }

    snprintf(msg_buf, sizeof(msg_buf), "101--------------------------------------------------\n"); // 使用 snprintf() 並且避免緩衝區溢位
    write_msg(fd, msg_buf);
    snprintf(msg_buf, sizeof(msg_buf), "101共 %d 人\n", total_num); // 使用 snprintf() 並且避免緩衝區溢位
    write_msg(fd, msg_buf);
}


// 顯示牌桌列表
// 顯示牌桌列表
void list_table(int fd, int mode) {
    int i;
    char msg_buf[1000];
    int total_num = 0;

    // 使用 char array 避免潛在的字串長度問題
    char prefix[] = "101   桌長       人數  附註\n";
    char suffix[] = "101--------------------------------------------------\n";

    write_msg(fd, prefix);
    write_msg(fd, suffix);

    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login && player[i].serv > 0) {
            // 檢查 player[i].serv 的有效性
            if (player[i].serv > 5 || player[i].serv < 1) {  // 確保數值在合理範圍內
                snprintf(msg_buf, sizeof(msg_buf), "顯示牌桌列表錯誤！ serv=%d，玩家：%s\n", player[i].serv, player[i].name);
                err(msg_buf); // 記錄錯誤訊息
                // 採取適當的錯誤處理措施，例如重置 player[i].serv 或斷開連線
                player[i].serv = 0; // 或 close_id(i);
                continue; // 跳過此玩家
            }

            if (mode == 2 && player[i].serv >= 4) {
                continue; // 跳過已滿的牌桌
            }

            total_num++;
            snprintf(msg_buf, sizeof(msg_buf), "101   %-10s %-4s  %s\n", 
                     player[i].name, number_map[player[i].serv - 1], player[i].note);
            write_msg(fd, msg_buf);
        }
    }

    write_msg(fd, suffix);
    snprintf(msg_buf, sizeof(msg_buf), "101共 %d 桌\n", total_num);
    write_msg(fd, msg_buf);
}

// 顯示狀態
void list_stat(int fd, const char *name) {
    char msg_buf[1000];
    char msg_buf1[1000];
    char order_buf[30];
    int i;
    int total_num;
    int order;
    struct player_record tmp_rec;


    total_num = 0;
    order = 1;
    if (!read_user_name(name)) {
        write_msg(fd, "101找不到這個人!");
        return;
    }
    snprintf(msg_buf, sizeof(msg_buf), "101◇名稱:%s ", record.name); // 使用 snprintf() 並且避免緩衝區溢位


    if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
        snprintf(msg_buf, sizeof(msg_buf), "(顯示狀態) 無法開啟檔案\n"); // 使用 snprintf() 並且避免緩衝區溢位
        err(msg_buf);
        return;
    }
    rewind(fp);
    if (record.game_count >= 16) {
        while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp) == 1) { //檢查fread回傳值
            if (tmp_rec.name[0] != 0 && tmp_rec.game_count >= 16) {
                total_num++;
                if (tmp_rec.money > record.money) {
                    order++;
                }
            }
        }
    }
    if (record.game_count < 16) {
        strncpy(order_buf, "無", sizeof(order_buf)-1);
				order_buf[sizeof(order_buf)-1] = '\0';
    } else {
        snprintf(order_buf, sizeof(order_buf), "%d/%d", order, total_num); // 使用 snprintf() 並且避免緩衝區溢位
    }
    snprintf(msg_buf1, sizeof(msg_buf1), "101◇金額:%ld 排名:%s 上線次數:%d 已玩局數:%d\n", record.money, order_buf, record.login_count, record.game_count); // 使用 snprintf() 並且避免緩衝區溢位
    write_msg(fd, msg_buf);
    write_msg(fd, msg_buf1);
    fclose(fp);
}


// 顯示使用者資訊
void who(int fd, const char *name) {
    char msg_buf[1000] = {0};  // 初始化 msg_buf，避免未初始化數據
    int serv_id = -1; // 初始化 serv_id 為一個無效值

    // 使用迴圈取代 goto，並使用 strncmp 避免潛在的緩衝區溢位
    for (int i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login && player[i].serv && 
            strncmp(player[i].name, name, sizeof(player[i].name) - 1) == 0) {
            serv_id = i;
            break; // 找到後立即跳出迴圈
        }
    }

    if (serv_id == -1) { // 找不到此桌
        write_msg(fd, "101找不到此桌");
        return;
    }

    // 使用 snprintf 組合訊息，避免緩衝區溢位
    snprintf(msg_buf, sizeof(msg_buf), "101%s  \n", player[serv_id].name);
    write_msg(fd, "101----------------   此桌使用者   ------------------\n");

    // 優化訊息組合，減少 write_msg 呼叫次數
    size_t current_len = 0;
    for (int i = 1; i < MAX_PLAYER; i++) {
        if (player[i].join == serv_id) {
            size_t name_len = strlen(player[i].name);
            // 檢查剩餘空間是否足夠，不足則先寫入已有的訊息
            if (current_len + name_len + 3 > sizeof(msg_buf) - 1) {
                write_msg(fd, msg_buf);
                current_len = 0;
                msg_buf[0] = '\0'; // 清空緩衝區
            }

            // 使用 strncat 組合訊息，避免緩衝區溢位
            strncat(msg_buf, player[i].name, sizeof(msg_buf) - current_len - 1);
            strncat(msg_buf, "   ", sizeof(msg_buf) - current_len - name_len - 1);
            current_len += name_len + 3; 
        }
    }

    // 寫入剩餘訊息
    if (current_len > 0) {
        write_msg(fd, msg_buf);
    }

    write_msg(fd, "101--------------------------------------------------\n");
}

// 顯示潛水者列表
void lurker(int fd) {
    int i, total_num = 0;
    char msg_buf[1000];


    snprintf(msg_buf, sizeof(msg_buf), "101"); // 使用 snprintf() 並且避免緩衝區溢位
    write_msg(fd, "101-------------   目前潛水使用者   ---------------\n");
    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2 && (player[i].join == 0 && player[i].serv == 0)) {
            total_num++;
            if (strlen(msg_buf) + strlen(player[i].name) + 3 > sizeof(msg_buf) -1) { // 檢查緩衝區大小
                write_msg(fd, msg_buf);
                snprintf(msg_buf, sizeof(msg_buf), "101"); // 重置緩衝區
            }
            strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) -1); //避免buffer overflow
            strncat(msg_buf, "  ", sizeof(msg_buf) - strlen(msg_buf) -1); //避免buffer overflow
        }
    }
    if (strlen(msg_buf) > 4) {
        write_msg(fd, msg_buf);
    }
    write_msg(fd, "101--------------------------------------------------\n");
    snprintf(msg_buf, sizeof(msg_buf), "101共 %d 人\n", total_num); // 使用 snprintf() 並且避免緩衝區溢位
    write_msg(fd, msg_buf);
}


// 尋找使用者
void find_user(int fd, const char *name) {
    int i;
    char msg_buf[1000];
    int id;
    char last_login_time[80];

    id = find_user_name(name);
    if (id > 0) {
        if (player[id].login == 2) {
            if (player[id].join == 0 && player[id].serv == 0) {
                snprintf(msg_buf, sizeof(msg_buf), "101◇%s 潛水中\n", name); // 使用 snprintf() 並且避免緩衝區溢位
                write_msg(fd, msg_buf);
            }
            if (player[id].join) {
                snprintf(msg_buf, sizeof(msg_buf), "101◇%s 在 %s 桌內\n", name, player[player[id].join].name); // 使用 snprintf() 並且避免緩衝區溢位
                write_msg(fd, msg_buf);
            }
            if (player[id].serv) {
                snprintf(msg_buf, sizeof(msg_buf), "101◇%s 在 %s 桌內\n", name, player[id].name); // 使用 snprintf() 並且避免緩衝區溢位
                write_msg(fd, msg_buf);
            }
            return;
        }
    }
    if (!read_user_name(name)) {
        snprintf(msg_buf, sizeof(msg_buf), "101◇沒有 %s 這個人\n", name); // 使用 snprintf() 並且避免緩衝區溢位
        write_msg(fd, msg_buf);
    } else {
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s 不在線上\n", name); // 使用 snprintf() 並且避免緩衝區溢位
        write_msg(fd, msg_buf);
        snprintf(last_login_time, sizeof(last_login_time), "%s", ctime(&record.last_login_time)); // 使用 snprintf() 並且避免緩衝區溢位
        last_login_time[strcspn(last_login_time, "\n")] = 0; //移除換行字元
        snprintf(msg_buf, sizeof(msg_buf), "101◇上次連線時間: %s\n", last_login_time); // 使用 snprintf() 並且避免緩衝區溢位
        write_msg(fd, msg_buf);
    }
}


// 廣播訊息
void broadcast(int player_id, const char *msg) {
    int i;
    char msg_buf[1000];

    if (strncmp(player[player_id].name, ADMIN_USER, strlen(ADMIN_USER)) != 0) { //避免buffer overflow
        return;
    }
    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2) {
            snprintf(msg_buf, sizeof(msg_buf), "101%s\n", msg); // 使用 snprintf() 並且避免緩衝區溢位
            write_msg(player[i].sockfd, msg_buf);
        }
    }
}


// 傳送訊息給特定玩家
void send_msg(int player_id, const char *msg) {
    char *str1, *str2;
    int i;
    char msg_buf[1000];

    str1 = strtok(strdup(msg), " "); // 使用 strdup() 複製字串避免修改原始字串
    str2 = (char *)msg + strlen(str1) + 1;
    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2 && strncmp(player[i].name, str1, strlen(str1)) == 0) { //避免buffer overflow
            snprintf(msg_buf, sizeof(msg_buf), "101*%s* %s\n", player[player_id].name, str2); // 使用 snprintf() 並且避免緩衝區溢位
            write_msg(player[i].sockfd, msg_buf);
						free(str1);
            return;
        }
				free(str1);
    }
    write_msg(player[player_id].sockfd, "101找不到這個人\n");
		free(str1);
}


// 邀請加入牌桌
void invite(int player_id, const char *name) {
    int i;
    char msg_buf[1000];

    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2 && strncmp(player[i].name, name, strlen(name)) == 0) { //避免buffer overflow
            snprintf(msg_buf, sizeof(msg_buf), "101%s 邀請你加入 %s\n", player[player_id].name, (player[player_id].join == 0) ? player[player_id].name : player[player[player_id].join].name); // 使用 snprintf() 並且避免緩衝區溢位
            write_msg(player[i].sockfd, msg_buf);
            return;
        }
    }
    write_msg(player[player_id].sockfd, "101找不到這個人\n");
}

/* -- */

// 雜湊表
msg_handler_entry message_handlers[HASH_TABLE_SIZE];

// 雜湊函式 (簡單的範例，可根據需求調整)
unsigned int hash(int msg_id) {
    return msg_id % HASH_TABLE_SIZE;
}

// 新增訊息處理器
void add_message_handler(int msg_id, message_handler handler) {
    unsigned int index = hash(msg_id);

    // 處理碰撞 (使用線性探測法)
    while (message_handlers[index].handler != NULL) {
        index = (index + 1) % HASH_TABLE_SIZE;
    }

    message_handlers[index].msg_id = msg_id;
    message_handlers[index].handler = handler;
}

// --- 訊息處理器函數實作 (開始) ---

// 處理 list_player 訊息
static void handle_list_player(int player_id, char *msg) {
    list_player(player[player_id].sockfd);
}

// 處理 list_table 訊息 (處理 mode 參數)
static void handle_list_table(int player_id, char *msg) {
    int mode = atoi(msg + 3); // 從訊息中取得 mode
    list_table(player[player_id].sockfd, mode);
}

// 處理 update_note 訊息
static void handle_update_note(int player_id, char *msg) {
    strncpy(player[player_id].note, msg + 3, sizeof(player[player_id].note) - 1);
    player[player_id].note[sizeof(player[player_id].note) - 1] = '\0';
}

// 處理 list_stat 訊息
static void handle_list_stat(int player_id, char *msg) {
    list_stat(player[player_id].sockfd, msg + 3);
}

// 處理 who 訊息
static void handle_who(int player_id, char *msg) {
    who(player[player_id].sockfd, msg + 3);
}

// 處理 broadcast 訊息 (管理員權限)
static void handle_broadcast(int player_id, char *msg) {
    broadcast(player_id, msg + 3);
}

// 處理 invite 訊息
static void handle_invite(int player_id, char *msg) {
    invite(player_id, msg + 3);
}


// 處理 send_msg 訊息
static void handle_send_msg(int player_id, char *msg) {
    send_msg(player_id, msg + 3);
}

// 處理 lurker 訊息
static void handle_lurker(int player_id, char *msg) {
    lurker(player[player_id].sockfd);
}

// 處理 join 訊息
static void handle_join(int player_id, char *msg) {
    handle_join_internal(player_id, msg);
}

// 處理開桌訊息
static void handle_create_table(int player_id, char *msg){
    handle_create_table_internal(player_id, msg);
}

// 處理 list_joinable_tables 訊息
static void handle_list_joinable_tables(int player_id, char* msg){
    list_table(player[player_id].sockfd, 2);
}

// 處理檢查開桌資格訊息
static void handle_check_create_table(int player_id, char* msg){
    handle_check_create_table_internal(player_id, msg);
}

// 處理 win game 訊息
static void handle_win_game(int player_id, char* msg){
    handle_win_game_internal(player_id, msg);
}

// 處理 find user 訊息
static void handle_find_user(int player_id, char* msg){
    find_user(player[player_id].sockfd, msg + 3);
}

// 處理 game record 訊息
static void handle_game_record(int player_id, char* msg){
    handle_game_record_internal(player_id, msg);
}

// 處理 leave game 訊息
static void handle_leave_game(int player_id, char* msg){
    close_id(player_id);
}


// 處理強制離開訊息(管理員)
static void handle_force_leave(int player_id, char* msg){
    handle_force_leave_internal(player_id, msg);
}

// 處理離開牌桌訊息
static void handle_leave_table(int player_id, char* msg){
    handle_leave_table_internal(player_id, msg);
}

// 處理顯示目前狀態訊息
static void handle_show_current_state(int player_id, char* msg){
    show_current_state(player_id);
}

// 處理關閉伺服器訊息(管理員)
static void handle_shutdown_server(int player_id, char* msg){
    handle_shutdown_server_internal(player_id, msg);
}

static void handle_get_userid(int player_id, char* msg){
    handle_get_userid_internal(player_id, msg);
}

static void handle_version_check(int player_id, char* msg){
    handle_version_check_internal(player_id, msg);
}

static void handle_user_login(int player_id, char* msg){
    handle_user_login_internal(player_id, msg);
}

static void handle_check_password(int player_id, char* msg){
    handle_check_password_internal(player_id, msg);
}

static void handle_create_account(int player_id, char* msg){
    handle_create_account_internal(player_id, msg);
}

static void handle_change_password(int player_id, char* msg){
    handle_change_password_internal(player_id, msg);
}

static void handle_log_user(int player_id, char* msg){
    handle_log_user_internal(player_id, msg);
}
// --- 訊息處理器函數實作 (結束) ---



// 初始化訊息處理器
void init_message_handlers() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        message_handlers[i].handler = NULL;
    }

    add_message_handler(2, handle_list_player);
    add_message_handler(3, handle_list_table);
    add_message_handler(4, handle_update_note);
    add_message_handler(5, handle_list_stat);
    add_message_handler(6, handle_who);
    add_message_handler(7, handle_broadcast);
    add_message_handler(8, handle_invite);
    add_message_handler(9, handle_send_msg);
    add_message_handler(10, handle_lurker);
    add_message_handler(11, handle_join);
    add_message_handler(12, handle_create_table);
    add_message_handler(13, handle_list_joinable_tables);
    add_message_handler(14, handle_check_create_table);
    add_message_handler(20, handle_win_game);
    add_message_handler(21, handle_find_user);
    add_message_handler(99, handle_get_userid);
    add_message_handler(100, handle_version_check);
    add_message_handler(101, handle_user_login);
    add_message_handler(102, handle_check_password);
    add_message_handler(103, handle_create_account);
    add_message_handler(104, handle_change_password);
    add_message_handler(105, handle_log_user);
    add_message_handler(900, handle_game_record);
    add_message_handler(200, handle_leave_game);
    add_message_handler(202, handle_force_leave);
    add_message_handler(205, handle_leave_table);
    add_message_handler(201, handle_show_current_state);
    add_message_handler(500, handle_shutdown_server);

    // 檢查是否成功添加所有處理器
    assert(get_message_handler(2) == handle_list_player);
    assert(get_message_handler(500) == handle_shutdown_server);
}

// --- 內部函式實作 ---

static void handle_get_userid_internal(int player_id, char* msg) {
    strncpy(player[player_id].username, msg + 3, sizeof(player[player_id].username) - 1);
    player[player_id].username[sizeof(player[player_id].username) - 1] = '\0';
}

static void handle_version_check_internal(int player_id, char* msg) {
    strncpy(player[player_id].version, msg + 3, sizeof(player[player_id].version) - 1);
    player[player_id].version[sizeof(player[player_id].version) - 1] = '\0';
}

static void handle_user_login_internal(int player_id, char* msg) {
    strncpy(player[player_id].name, msg + 3, sizeof(player[player_id].name) - 1);
    player[player_id].name[sizeof(player[player_id].name) - 1] = '\0';
    for (int i = 0; i < strlen(msg) - 3; i++) {
        if (msg[3 + i] <= 32 && msg[3 + i] != 0) {
            write_msg(player[player_id].sockfd, "101Invalid username!");
            close_id(player_id);
            break;
        }
    }
    if (read_user_name(player[player_id].name)) {
        write_msg(player[player_id].sockfd, "002");
    } else {
        write_msg(player[player_id].sockfd, "005");
    }
}

static void handle_check_password_internal(int player_id, char* msg) {
    if (read_user_name(player[player_id].name)) {
        *(msg + 11) = 0;
        if (checkpasswd(record.password, msg + 3)) {
            int find_duplicated = 0;
            for (int i = 1; i < MAX_PLAYER; i++) {
                if ((player[i].login == 2 || player[i].login == 3) && strcmp(
                        player[i].name,
                        player[player_id].name) == 0) {
                    write_msg(player[player_id].sockfd, "006");
                    player[player_id].login = 3;
                    find_duplicated = 1;
                    break;
                }
            }
            if (find_duplicated){
                return;
            }
            
            time(&record.last_login_time);
            record.last_login_from[0] = 0;
            if (player[player_id].username[0] != 0) {
                snprintf(record.last_login_from, sizeof(record.last_login_from), "%s@", player[player_id].username);
            }
            strncat(record.last_login_from, lookup(&player[player_id].addr), sizeof(record.last_login_from) - strlen(record.last_login_from) - 1);
            record.login_count++;
            write_record();
            if (check_user(player_id))
                welcome_user(player_id);
            else
                close_id(player_id);
        } else {
            write_msg(player[player_id].sockfd, "004");
        }
    }
}

static void handle_create_account_internal(int player_id, char* msg) {
    *(msg + 11) = 0;
    if (!add_user(player_id, player[player_id].name, msg + 3)) {
        close_id(player_id);
        return;
    }
    welcome_user(player_id);
}

static void handle_change_password_internal(int player_id, char* msg) {
    *(msg + 11) = 0;
    read_user_name(player[player_id].name);
    strncpy(record.password, genpasswd(msg + 3), sizeof(record.password) - 1);
    record.password[sizeof(record.password) - 1] = '\0';
    write_record();
}

static void handle_log_user_internal(int player_id, char* msg) {
    if (read_user_name(player[player_id].name) && player[player_id].login == 3) {
        *(msg + 11) = 0;
        for (int i = 1; i < MAX_PLAYER; i++) {
            if ((player[i].login == 2 || player[i].login == 3) && (i != player_id) && strcmp(
                    player[i].name, player[player_id].name)
                    == 0) {
                close_id(i);
                break;
            }
        }
        time(&record.last_login_time);
        record.last_login_from[0] = 0;
        if (player[player_id].username[0] != 0) {
            snprintf(record.last_login_from, sizeof(record.last_login_from), "%s@",
                    player[player_id].username);
        }
        strncat(record.last_login_from,
                lookup(&player[player_id].addr), sizeof(record.last_login_from) - strlen(record.last_login_from) - 1);
        record.login_count++;
        write_record();
        if (check_user(player_id))
            welcome_user(player_id);
        else
            close_id(player_id);
    }
}

// 處理 JOIN 訊息的內部函式
static void handle_join_internal(int player_id, char *msg) {
    char msg_buf[1000];
    int i;

    if (!read_user_name_update(player[player_id].name, player_id)) {
        snprintf(msg_buf, sizeof(msg_buf), "101查無此人");
        write_msg(player[player_id].sockfd, msg_buf);
        return;
    }
    update_client_money(player_id);
    if (player[player_id].money <= MIN_JOIN_MONEY) {
        snprintf(msg_buf, sizeof(msg_buf), "101您的賭幣（%ld）不足，必須超過 %d 元才能加入牌桌",
                 player[player_id].money, MIN_JOIN_MONEY);
        write_msg(player[player_id].sockfd, msg_buf);
        return;
    }
    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2 && player[i].serv) {
            if (strncmp(player[i].name, msg + 3, strlen(msg + 3)) == 0) {
                if (player[i].serv >= 4) {
                    write_msg(player[player_id].sockfd, "101此桌人數已滿!");
                    return;
                }
                snprintf(msg_buf, sizeof(msg_buf), "120%05d%ld", player[player_id].id, player[player_id].money);
                write_msg(player[i].sockfd, msg_buf);
                snprintf(msg_buf, sizeof(msg_buf), "211%s", player[player_id].name);
                write_msg(player[i].sockfd, msg_buf);
                snprintf(msg_buf, sizeof(msg_buf), "0110%s %d", inet_ntoa(player[i].addr.sin_addr), player[i].port);
                write_msg(player[player_id].sockfd, msg_buf);
                player[player_id].join = i;
                player[player_id].serv = 0;
                player[i].serv++;
                return;
            }
        }
    }
    write_msg(player[player_id].sockfd, "0111");
}


// 處理開桌訊息的內部函式
static void handle_create_table_internal(int player_id, char *msg) {
    char msg_buf[1000];
    int i;

    if (!read_user_name_update(player[player_id].name, player_id)) {
        snprintf(msg_buf, sizeof(msg_buf), "101查無此人");
        write_msg(player[player_id].sockfd, msg_buf);
        return;
    }
    update_client_money(player_id);
    if (player[player_id].money <= MIN_JOIN_MONEY) {
        snprintf(msg_buf, sizeof(msg_buf), "101您的賭幣（%ld）不足，必須超過 %d 元才能開桌",
                 player[player_id].money, MIN_JOIN_MONEY);
        write_msg(player[player_id].sockfd, msg_buf);
        return;
    }
    player[player_id].port = atoi(msg + 3);
    if (player[player_id].join) {
        if (player[player[player_id].join].serv > 0)
            player[player[player_id].join].serv--;
        player[player_id].join = 0;
    }
    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].join == player_id)
            player[i].join = 0;
    }
    player[player_id].serv = 1;
}


// 處理 WIN GAME 訊息的內部函式
static void handle_win_game_internal(int player_id, char *msg) {
    char msg_buf[1000];
    char tmp_buf[1000];
    int id;
    int i;

    strncpy(msg_buf, msg + 3, 5); //避免buffer overflow
    msg_buf[5] = '\0'; //確保null termination
    id = atoi(msg_buf);
    read_user_id(id);
    strncpy(tmp_buf, msg + 8, sizeof(tmp_buf) - 1); //避免buffer overflow
    tmp_buf[sizeof(tmp_buf) - 1] = '\0'; //確保null termination
    record.money = atol(tmp_buf);
    record.game_count++;
    write_record();
    for (i = 1; i < MAX_PLAYER; i++) {
        if (player[i].login == 2 && player[i].id == id) {
            player[i].money = record.money;
            break;
        }
    }
}

// 處理 GAME RECORD 訊息的內部函式
static void handle_game_record_internal(int player_id, char* msg){
    err("get game record\n");
    game_log(msg + 3);
    err("get game record end\n");
}

// 處理強制離開訊息(管理員)的內部函式
static void handle_force_leave_internal(int player_id, char* msg){
    int id;
    if (strncmp(player[player_id].name, ADMIN_USER, strlen(ADMIN_USER)) != 0) {
        return;
    }
    id = find_user_name(msg + 3);
    if (id >= 0) {
        write_msg(player[id].sockfd, "200");
        close_id(id);
    }
}

// 處理離開牌桌訊息的內部函式
static void handle_leave_table_internal(int player_id, char* msg){
    int i;
    if (player[player_id].serv) {
        for (i = 1; i < MAX_PLAYER; i++) {
            if (player[i].join == player_id)
                player[i].join = 0;
        }
        player[player_id].serv = 0;
        player[player_id].join = 0;
    } else if (player[player_id].join) {
        if (player[player[player_id].join].serv > 0)
            player[player[player_id].join].serv--;
        player[player_id].join = 0;
    }
}

// 處理檢查開桌資格訊息的內部函式
static void handle_check_create_table_internal(int player_id, char* msg){
    char msg_buf[1000];
    if (!read_user_name_update(player[player_id].name, player_id)) {
        snprintf(msg_buf, sizeof(msg_buf), "101查無此人");
        write_msg(player[player_id].sockfd, msg_buf);
        return;
    }
    update_client_money(player_id);
    if (player[player_id].money <= MIN_JOIN_MONEY) {
        snprintf(msg_buf, sizeof(msg_buf), "101您的賭幣（%ld）不足，必須超過 %d 元才能開桌",
                 player[player_id].money, MIN_JOIN_MONEY);
        write_msg(player[player_id].sockfd, msg_buf);
        return;
    }
    write_msg(player[player_id].sockfd, "012");
}

// 處理關閉伺服器訊息(管理員)的內部函式
static void handle_shutdown_server_internal(int player_id, char* msg){
    if (strncmp(player[player_id].name, ADMIN_USER, strlen(ADMIN_USER)) == 0) {
        shutdown_server();
    }
}

// 取得訊息處理器
message_handler get_message_handler(int msg_id) {
    unsigned int index = hash(msg_id);

    // 搜尋 msg_id，處理碰撞
    while (message_handlers[index].handler != NULL) {
        if (message_handlers[index].msg_id == msg_id) {
            return message_handlers[index].handler;
        }
        index = (index + 1) % HASH_TABLE_SIZE;
    }

    return NULL; // 找不到 msg_id
}

/* -- */

// 處理GPS訊息
void gps_processing() {
    socklen_t alen;
    int fd, nfds;
    int player_id;
    int player_num = 0;
    int i, j;
    int msg_id;
    int read_code;
    char tmp_buf[80];
    char msg_buf[1000];
    unsigned char buf[256];
    struct timeval timeout;
    struct hostent *hp;
    int id;
    struct timeval tm;
    long current_time;
    struct tm *tim;

    log_level = 0;
    nfds = getdtablesize();
    nfds = 256;
    printf("%d\n", nfds);
    FD_ZERO(&afds);
    FD_SET(gps_sockfd, &afds);
    bcopy((char *)&afds, (char *)&rfds, sizeof(rfds));
    tm.tv_sec = 0;
    tm.tv_usec = 0;

    init_message_handlers();
    /*
     * Waiting for connections 
     */
    for (;;) {
        bcopy((char *)&afds, (char *)&rfds, sizeof(rfds));
        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &tm) < 0) {
            snprintf(msg_buf, sizeof(msg_buf), "select: %d %s\n", errno, strerror(errno)); // 使用 snprintf() 並且避免緩衝區溢位
            err(msg_buf);
            continue;
        }
        if (FD_ISSET(gps_sockfd, &rfds)) {
            for (player_num = 1; player_num < MAX_PLAYER - 1; player_num++)
                if (!player[player_num].login)
                    break;
            if (player_num == MAX_PLAYER - 1)
                err("Too many users");
            player_id = player_num;
            alen = sizeof(player[player_num].addr);
            player[player_id].sockfd = accept(gps_sockfd,
                                              (struct sockaddr *)&player[player_num].addr, &alen);
            FD_SET(player[player_id].sockfd, &afds);
            fcntl(player[player_id].sockfd, F_SETFL, FNDELAY);
            player[player_id].login = 1;
            strncpy(climark, lookup(&(player[player_id].addr)), sizeof(climark) - 1); //避免buffer overflow
            climark[sizeof(climark) - 1] = '\0'; //確保null termination
            snprintf(msg_buf, sizeof(msg_buf), "Connectted with %s\n", climark); // 使用 snprintf() 並且避免緩衝區溢位
            err(msg_buf);

            time(&current_time);
            tim = localtime(&current_time);
            if (player_id > login_limit) {
                write_msg(player[player_id].sockfd,
                                    "101對不起,目前使用人數超過上限, 請稍後再進來.");
                print_news(player[player_id].sockfd, "server.lst");
                close_id(player_id);
            }
        }
        for (player_id = 1; player_id < MAX_PLAYER; player_id++) {
            if (player[player_id].login) {
                if (FD_ISSET(player[player_id].sockfd, &rfds)) {
                    /* 
                     * Processing the player's information 
                     */
                    read_code = read_msg(player[player_id].sockfd, (char *)buf);
                    if (!read_code) {
                        err(("cant read code!"));
                        close_id(player_id);
                    } else if (read_code == 1) {
                        msg_id = convert_msg_id(player_id, (char *)buf);
                        message_handler handler = get_message_handler(msg_id);
                        if (handler != NULL) {
                            handler(player_id, (char *)buf);
                        } else {
                            char msg_buf[1000];
                            snprintf(msg_buf, sizeof(msg_buf),
                                    "### 未知的訊息 ID: %d  player_id=%d sockfd=%d ###\n",
                                    msg_id, player_id, player[player_id].sockfd);
                            err(msg_buf);
                            close_connection(player_id);
                            snprintf(msg_buf, sizeof(msg_buf),
                                    "Connection to %s error, closed it\n",
                                    lookup(&(player[player_id].addr)));
                            err(msg_buf);
                        }
                        buf[0] = '\0'; // 清空buf
                    }
                }
            }
        }
    }
}

void close_id(int player_id) {
    char msg_buf[1000];

    close_connection(player_id);
    snprintf(msg_buf, sizeof(msg_buf), "Connection to %s closed\n",
             lookup(&(player[player_id].addr))); // 使用 snprintf() 並且避免緩衝區溢位
    err(msg_buf);
}

void close_connection(int player_id) {
    close(player[player_id].sockfd);
    FD_CLR(player[player_id].sockfd, &afds);
    if (player[player_id].join && player[player[player_id].join].serv)
        player[player[player_id].join].serv--;

    player[player_id].login = 0;
    player[player_id].serv = 0;
    player[player_id].join = 0;
    player[player_id].version[0] = 0;
    player[player_id].note[0] = 0;
    player[player_id].name[0] = 0;
    player[player_id].username[0] = 0;
}

int main(int argc, char **argv) {
    int i;

    getrlimit(RLIMIT_NOFILE, &fd_limit);
    fd_limit.rlim_cur = fd_limit.rlim_max;
    setrlimit(RLIMIT_NOFILE, &fd_limit);
    i = getdtablesize();
    printf("FD_SIZE=%d\n", i);
    signal(SIGSEGV, core_dump);
    signal(SIGBUS, bus_err);
    signal(SIGPIPE, broken_pipe);
    signal(SIGALRM, time_out);
    if (argc < 2)
        gps_port = DEFAULT_GPS_PORT;
    else {
        gps_port = atoi(argv[1]);
        printf("Using port %s\n", argv[1]);
    }
    strncpy(gps_ip, DEFAULT_GPS_IP, sizeof(gps_ip) -1); // 避免 buffer overflow
    gps_ip[sizeof(gps_ip) - 1] = '\0'; //確保null termination
    init_socket();
    init_variable();
    gps_processing();
}
