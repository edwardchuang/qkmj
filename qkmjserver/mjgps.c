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
  if ((log_fp = fopen(LOG_FILE, "a")) == NULL) {
    printf("Cannot open logfile\n");
    return -1;
  }
  printf("%s", errmsg);

  if (log_level == 0) {
    fprintf(log_fp, "%s", errmsg);
  }

  log_level = 0;
  fclose(log_fp);
  return 0;
}

// Game log function
int game_log(char *gamemsg) {
  if ((log_fp = fopen(GAME_FILE, "a")) == NULL) {
    printf("Cannot open GAME_FILE\n");
    return -1;
  }
  fprintf(log_fp, "%s", gamemsg);

  fclose(log_fp);
  return 0;
}

// Read message from a socket
int read_msg(int fd, char *msg) {
  int n = 0;
  char msg_buf[1000];
  int read_code;

  if (Check_for_data(fd) == 0) {
    err("WRONG READ\n");
    return 2;
  }
  timeup = 0;
  alarm(5);
  while (n <= 8000) {
    // Recheck for data if read fails with EWOULDBLOCK
    read_code = read(fd, msg, 1);
    if (read_code == -1) {
      if (errno != EWOULDBLOCK) {
        snprintf(msg_buf, sizeof(msg_buf), "fail in read_msg,errno = %d", errno);
        err(msg_buf);
        alarm(0);
        return 0;
      } else if (timeup) {
        alarm(0);
        err("TIME UP!\n");
        return 0;
      }
    } else if (read_code == 0) {
      alarm(0);
      return 0;
    } else {
      if (*msg == '\0') {
        alarm(0);
        return 1;
      }
      n++;
      msg++;
    }
  }
  alarm(0);
  return 0;
}

// Write message to a socket
void write_msg(int fd, char *msg) {
  int n = strlen(msg);
  if (write(fd, msg, n) < 0) {
    close(fd);
    FD_CLR(fd, &afds);
  }
  if (write(fd, msg + n, 1) < 0) {
    close(fd);
    FD_CLR(fd, &afds);
  }
}

// Display message to a player
void display_msg(int player_id, char *msg) {
  char msg_buf[1000];
  snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);
  write_msg(player[player_id].sockfd, msg_buf);
}

// Check for data on a socket
int Check_for_data(int fd) {
  int status;
  fd_set wait_set;
  struct timeval tm;

  FD_ZERO(&wait_set);
  FD_SET(fd, &wait_set);

  tm.tv_sec = 0;
  tm.tv_usec = 0;
  status = select(FD_SETSIZE, &wait_set, (fd_set *) 0, (fd_set *) 0, &tm);

  return (status);
}

// Convert message ID
int convert_msg_id(int player_id, char *msg) {
  int i;
  char msg_buf[1000];

  if (strlen(msg) < 3) {
    snprintf(msg_buf, sizeof(msg_buf), "Error msg: %s", msg);
    err(msg_buf);
    return 0;
  }
  for (i = 0; i < 3; i++) {
    if (msg[i] < '0' || msg[i] > '9') {
      snprintf(msg_buf, sizeof(msg_buf), "%d", msg[i]);
      err(msg_buf);
    }
  }
  return (msg[0] - '0') * 100 + (msg[1] - '0') * 10 + (msg[2] - '0');
}

// List online players
void list_player(int fd) {
  int i;
  char msg_buf[1000];
  int total_num = 0;

  write_msg(fd, "101-------------    目前上線使用者    ---------------");
  strncpy(msg_buf, "101", sizeof("101"));
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2) {
      total_num++;
      if ((strlen(msg_buf) + strlen(player[i].name)) > 50) {
        write_msg(fd, msg_buf);
        strncpy(msg_buf, "101", sizeof("101"));
      }
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - 1);
      strncat(msg_buf, "  ", sizeof(msg_buf) - 1);
    }
  }
  if (strlen(msg_buf) > 4) {
    write_msg(fd, msg_buf);
  }

  write_msg(fd, "101--------------------------------------------------");
  snprintf(msg_buf, sizeof(msg_buf), "101共 %d 人", total_num);
  write_msg(fd, msg_buf);
}

// List tables
void list_table(int fd, int mode) {
  int i;
  char msg_buf[1000];
  int total_num = 0;

  write_msg(fd, "101   桌長       人數  附註");
  write_msg(fd, "101--------------------------------------------------");
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login && player[i].serv > 0) {
      if (player[i].serv > 4) {
        if (player[i].serv == 5) {
          err("SERV=5\n");
        } else {
          err("LIST TABLE ERROR!");
          snprintf(msg_buf, sizeof(msg_buf), "serv=%d\n", player[i].serv);
          close_id(i);
          err(msg_buf);
        }
      }
      if (mode == 2 && player[i].serv >= 4) {
        continue;
      }
      total_num++;
      snprintf(msg_buf, sizeof(msg_buf), "101   %-10s %-4s  %s", player[i].name,
              number_map[player[i].serv], player[i].note);
      write_msg(fd, msg_buf);
    }
  }
  write_msg(fd, "101--------------------------------------------------");
  snprintf(msg_buf, sizeof(msg_buf), "101共 %d 桌", total_num);
  write_msg(fd, msg_buf);
}

// List player statistics
void list_stat(int fd, char *name) {
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
  snprintf(msg_buf, sizeof(msg_buf), "101◇名稱:%s ", record.name);
  if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
    err("(stat) Cannot open file\n");
    return;
  }
  rewind(fp);
  if (record.game_count >= 16) {
    while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
      if (tmp_rec.name[0] != 0 && tmp_rec.game_count >= 16) {
        total_num++;
        if (tmp_rec.money > record.money) {
          order++;
        }
      }
    }
  }
  if (record.game_count < 16) {
    strncpy(order_buf, "無", sizeof("無"));
  } else {
    snprintf(order_buf, sizeof(order_buf), "%d/%d", order, total_num);
  }
  snprintf(msg_buf1, sizeof(msg_buf1), "101◇金額:%ld 排名:%s 上線次數:%d 已玩局數:%d", record.money,
          order_buf, record.login_count, record.game_count);
  write_msg(fd, msg_buf);
  write_msg(fd, msg_buf1);
  fclose(fp);
}

// Show players at a table
void who(int fd, char *name) {
  char msg_buf[1000];
  int i;
  int serv_id = -1;

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login && player[i].serv) {
      if (strncmp(player[i].name, name, sizeof(player[i].name)) == 0) {
        serv_id = i;
        break;
      }
    }
  }
  if (serv_id == -1) {
    write_msg(fd, "101找不到此桌");
    return;
  }
  snprintf(msg_buf, sizeof(msg_buf), "101%s  ", player[serv_id].name);
  write_msg(fd, "101----------------   此桌使用者   ------------------");
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].join == serv_id) {
      if ((strlen(msg_buf) + strlen(player[i].name)) > 53) {
        write_msg(fd, msg_buf);
        snprintf(msg_buf, sizeof(msg_buf), "101");
      }
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - 1);
      strncat(msg_buf, "   ", sizeof(msg_buf) - 1);
    }
  }
  if (strlen(msg_buf) > 4) {
    write_msg(fd, msg_buf);
  }
  write_msg(fd, "101--------------------------------------------------");
}

// List lurkers
void lurker(int fd) {
  int i, total_num = 0;
  char msg_buf[1000];

  strncpy(msg_buf, "101", sizeof("101"));
  write_msg(fd, "101-------------   目前□置之使用者   ---------------");
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2
        && (player[i].join == 0 && player[i].serv == 0)) {
      total_num++;
      if ((strlen(msg_buf) + strlen(player[i].name)) > 53) {
        write_msg(fd, msg_buf);
        strncpy(msg_buf, "101", sizeof("101"));
      }
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - 1);
      strncat(msg_buf, "  ", sizeof(msg_buf) - 1);
    }
  }
  if (strlen(msg_buf) > 4) {
    write_msg(fd, msg_buf);
  }
  write_msg(fd, "101--------------------------------------------------");
  snprintf(msg_buf, sizeof(msg_buf), "101共 %d 人", total_num);
  write_msg(fd, msg_buf);
}

// Find a user
void find_user(int fd, char *name) {
  int i;
  char msg_buf[1000];
  int id;
  char last_login_time[80];

  id = find_user_name(name);
  if (id > 0) {
    if (player[id].login == 2) {
      if (player[id].join == 0 && player[id].serv == 0) {
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s □置中", name);
        write_msg(fd, msg_buf);
      }
      if (player[id].join) {
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s 在 %s 桌內", name,
                player[player[id].join].name);
        write_msg(fd, msg_buf);
      }
      if (player[id].serv) {
        snprintf(msg_buf, sizeof(msg_buf), "101◇%s 在 %s 桌內", name, player[id].name);
        write_msg(fd, msg_buf);
      }
      return;
    }
  }
  if (!read_user_name(name)) {
    snprintf(msg_buf, sizeof(msg_buf), "101◇沒有 %s 這個人", name);
    write_msg(fd, msg_buf);
  } else {
    snprintf(msg_buf, sizeof(msg_buf), "101◇%s 不在線上", name);
    write_msg(fd, msg_buf);
    strncpy(last_login_time, ctime(&record.last_login_time), sizeof(last_login_time) - 1);
    snprintf(msg_buf, sizeof(msg_buf), "101◇上次連線時間: %s", last_login_time);
    write_msg(fd, msg_buf);
  }
}

// Broadcast a message to all online players
void broadcast(int player_id, char *msg) {
  int i;
  char msg_buf[1000];

  if (strcmp(player[player_id].name, ADMIN_USER) != 0) {
    return;
  }
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2) {
      snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);
      write_msg(player[i].sockfd, msg_buf);
    }
  }
}

// Send a private message to a player
void send_msg(int player_id, char *msg) {
  char *str1, *str2;
  int i;
  char msg_buf[1000];

  str1 = strtok(msg, " ");
  str2 = msg + strlen(str1) + 1;
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2 && strcmp(player[i].name, str1) == 0) {
      snprintf(msg_buf, sizeof(msg_buf), "101*%s* %s", player[player_id].name, str2);
      write_msg(player[i].sockfd, msg_buf);
      return;
    }
  }
  write_msg(player[player_id].sockfd, "101找不到這個人");
}

// Invite a player to join a table
void invite(int player_id, char *name) {
  int i;
  char msg_buf[1000];

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2 && strcmp(player[i].name, name) == 0) {
      snprintf(msg_buf, sizeof(msg_buf), "101%s 邀請你加入 %s",
               player[player_id].name,
               (player[player_id].join == 0) ? player[player_id].name
                                            : player[player[player_id].join].name);
      write_msg(player[i].sockfd, msg_buf);
      return;
    }
  }
  write_msg(player[player_id].sockfd, "101找不到這個人");
}

// Initialize the socket for the server
void init_socket() {
  struct sockaddr_in serv_addr;
  int on = 1;

  // Open a TCP socket for internet stream socket
  if ((gps_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err("Server: cannot open stream socket");
  }

  // Bind our local address
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(gps_port);
  setsockopt(gps_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
  if (bind(gps_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    printf("server: cannot bind local address\n");
    exit(1);
  }
  listen(gps_sockfd, 10);
  printf("Listen for client...\n");
}

// Lookup the hostname or IP address of a client
char *lookup(struct sockaddr_in *cli_addrp) {
  struct hostent *hp;
  char *hostname;

  hp = gethostbyaddr((char *) &cli_addrp->sin_addr, sizeof(struct in_addr),
                     cli_addrp->sin_family);

  if (hp) {
    hostname = (char *) hp->h_name;
  } else {
    hostname = inet_ntoa(cli_addrp->sin_addr);
  }
  return hostname;
}

// Initialize global variables
void init_variable() {
  int i;

  login_limit = LOGIN_LIMIT;
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

// Read user name from the record file
int read_user_name(char *name) {
  struct player_record tmp_rec;
  char msg_buf[1000];

  if ((fp = fopen(RECORD_FILE, "a+b")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_name) Cannot open file!\n");
    err(msg_buf);
    return 0;
  }
  rewind(fp);
  while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
    if (strncmp(name, tmp_rec.name, sizeof(tmp_rec.name)) == 0) {
      record = tmp_rec;
      fclose(fp);
      return 1;
    }
  }
  fclose(fp);
  return 0;
}
// Read user name from the record file and update player info
int read_user_name_update(char *name, int player_id) {
  struct player_record tmp_rec;
  char msg_buf[1000];

  if ((fp = fopen(RECORD_FILE, "a+b")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_name) Cannot open file!\n");
    err(msg_buf);
    return 0;
  }
  rewind(fp);
  while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
    if (strncmp(name, tmp_rec.name, sizeof(tmp_rec.name)) == 0) {
      record = tmp_rec;
      if (player[player_id].id == record.id) {  // Double check
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

// Read user record by ID
int read_user_id(unsigned int id) {
  char msg_buf[1000];

  if ((fp = fopen(RECORD_FILE, "a+b")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_id) Cannot open file!\n");
    err(msg_buf);
    return -1;
  }
  rewind(fp);
  fseek(fp, sizeof(record) * id, 0);
  if (fread(&record, sizeof(record), 1, fp) == -1) {
    snprintf(msg_buf, sizeof(msg_buf), "(read_user_id) Cannot write file!\n");
    err(msg_buf);
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}

// Add a new user to the record file
int add_user(int player_id, char *name, char *passwd) {
  struct stat status;

  stat(RECORD_FILE, &status);
  if (!read_user_name("")) {
    record.id = status.st_size / sizeof(record);
  }
  strncpy(record.name, name, sizeof(record.name));
  strncpy(record.password, genpasswd(passwd), sizeof(record.password));
  record.money = DEFAULT_MONEY;
  record.level = 0;
  record.login_count = 1;
  record.game_count = 0;
  time(&record.regist_time);
  record.last_login_time = record.regist_time;
  record.last_login_from[0] = 0;
  if (player[player_id].username[0] != 0) {
    snprintf(record.last_login_from, sizeof(record.last_login_from), "%s@",
             player[player_id].username);
  }
  strncat(record.last_login_from, lookup(&(player[player_id].addr)),
          sizeof(record.last_login_from) - 1);
  if (check_user(player_id)) {
    write_record();
    return 1;
  } else {
    return 0;
  }
}

// Check if a user is allowed to access the server
int check_user(int player_id) {
  char msg_buf[1000];
  char from[80];
  char email[80];
  FILE *baduser_fp;

  // Open the file containing banned users
  if ((baduser_fp = fopen(BADUSER_FILE, "r")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "Cannot open file %s", BADUSER_FILE);
    err(msg_buf);
    return 1;
  }

  // Get the user's IP address and construct their email address
  strncpy(from, lookup(&(player[player_id].addr)), sizeof(from) - 1);
  snprintf(email, sizeof(email), "%s@", player[player_id].username);
  strncat(email, from, sizeof(email) - 1);

  // Check if the user is banned
  while (fgets(msg_buf, 80, baduser_fp) != NULL) {
    msg_buf[strlen(msg_buf) - 1] = 0;
    if (strncmp(email, msg_buf, sizeof(msg_buf)) == 0 ||
        strncmp(player[player_id].username, msg_buf, sizeof(msg_buf)) == 0) {
      display_msg(player_id, "你已被限制進入");
      fclose(baduser_fp);
      return 0;
    }
  }
  fclose(baduser_fp);
  return 1;
}

// Write the updated user record to the file
void write_record() {
  char msg_buf[1000];

  if ((fp = fopen(RECORD_FILE, "r+b")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "(write_record) Cannot open file!");
    err(msg_buf);
    return;
  }
  fseek(fp, sizeof(record) * record.id, 0);
  fwrite(&record, sizeof(record), 1, fp);
  fclose(fp);
}

// Print news messages to the client
void print_news(int fd, char *name) {
  FILE *news_fp;
  char msg[255];
  char msg_buf[1000];

  if ((news_fp = fopen(name, "r")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "Cannot open file %s\n", NEWS_FILE);
    err(msg_buf);
    return;
  }
  while (fgets(msg, 80, news_fp) != NULL) {
    msg[strlen(msg) - 1] = 0;
    snprintf(msg_buf, sizeof(msg_buf), "101%s", msg);
    write_msg(fd, msg_buf);
  }
  fclose(news_fp);
}

// Welcome a new user to the server
void welcome_user(int player_id) {
  char msg_buf[1000];
  int fd;
  int i;
  struct player_record tmp_rec;

  fd = player[player_id].sockfd;
  if (strncmp(player[player_id].version, "093", sizeof(player[player_id].version)) < 0 ||
      player[player_id].version[0] == 0) {
    write_msg(player[player_id].sockfd, "101請使用 QKMJ Ver 0.93 Beta 以上版本上線");
    write_msg(player[player_id].sockfd, "010");
    return;
  }
  snprintf(msg_buf, sizeof(msg_buf), "101★★★★★　歡迎 %s 來到ＱＫ麻將  ★★★★★",
           player[player_id].name);
  write_msg(player[player_id].sockfd, msg_buf);
  print_news(player[player_id].sockfd, NEWS_FILE);
  player[player_id].id = record.id;
  player[player_id].money = record.money;
  player[player_id].login = 2;
  player[player_id].note[0] = 0;
  show_online_users(player_id);
  list_stat(player[player_id].sockfd, player[player_id].name);
  write_msg(player[player_id].sockfd, "003");
  snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld", player[player_id].id,
           player[player_id].money);
  write_msg(player[player_id].sockfd, msg_buf);
  player[player_id].input_mode = CMD_MODE;
}

// Show the number of online and registered users
void show_online_users(int player_id) {
  char msg_buf[1000];
  int fd;
  int total_num = 0;
  int online_num = 0;
  int i;
  struct player_record tmp_rec;

  fd = player[player_id].sockfd;
  if ((fp = fopen(RECORD_FILE, "rb")) == NULL) {
    snprintf(msg_buf, sizeof(msg_buf), "(current) cannot open file\n");
    err(msg_buf);
  } else {
    rewind(fp);
    while (!feof(fp) && fread(&tmp_rec, sizeof(tmp_rec), 1, fp)) {
      if (tmp_rec.name[0] != 0) {
        total_num++;
      }
    }
    fclose(fp);
  }
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2) {
      online_num++;
    }
  }
  snprintf(msg_buf, sizeof(msg_buf),
           "101◇目前上線人數: %d 人       注冊人數: %d 人", online_num,
           total_num);
  write_msg(player[player_id].sockfd, msg_buf);
}

// Show the current state of the user
void show_current_state(int player_id) {
  char msg_buf[1000];
  int fd;
  int i;

  show_online_users(player_id);
  list_stat(player[player_id].sockfd, player[player_id].name);
  write_msg(player[player_id].sockfd, "003");
  snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld", player[player_id].id,
           player[player_id].money);
  write_msg(player[player_id].sockfd, msg_buf);
}

// Update the client's money
void update_client_money(int player_id) {
  char msg_buf[1000];
  snprintf(msg_buf, sizeof(msg_buf), "120%5d%ld", player[player_id].id,
           player[player_id].money);
  write_msg(player[player_id].sockfd, msg_buf);
}

// Find a user by name
int find_user_name(char *name) {
  int i;

  for (i = 1; i < MAX_PLAYER; i++) {
    if (strncmp(player[i].name, name, sizeof(player[i].name)) == 0) {
      return i;
    }
  }
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

    // Check for new connections
    if (FD_ISSET(gps_sockfd, &rfds)) {
      for (player_num = 1; player_num < MAX_PLAYER; player_num++) {
        if (!player[player_num].login) {
          break;
        }
      }
      if (player_num == MAX_PLAYER - 1) {
        err("Too many users");
      }
      player_id = player_num;
      alen = sizeof(player[player_num].addr);
      player[player_id].sockfd = accept(gps_sockfd,
          (struct sockaddr *)&player[player_num].addr,
          (socklen_t *)&alen);
      FD_SET(player[player_id].sockfd, &afds);
      fcntl(player[player_id].sockfd, F_SETFL, FNDELAY);
      player[player_id].login = 1;

      time(&current_time);
      tim = localtime(&current_time);

      if (player_id > login_limit) {
        write_msg(player[player_id].sockfd,
            "101對不起,目前使用人數超過上限, 請稍後再進來.");
        print_news(player[player_id].sockfd, "server.lst");
        close_id(player_id);
      }
    }

    // Check for messages from connected players
    for (player_id = 1; player_id < MAX_PLAYER; player_id++) {
      if (player[player_id].login) {
        if (FD_ISSET(player[player_id].sockfd, &rfds)) {
          // Process the player's information
          read_code = read_msg(player[player_id].sockfd, (char *) buf);
          if (!read_code) {
            err(("cant read code!"));
            close_id(player_id);
          } else if (read_code == 1) {
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
                write_msg(player[player_id].sockfd, "012");  // Confirm table
                                                            // creation
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
                find_user(player[player_id].sockfd,
                    (char *)(buf + 3));
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
    }
  }
}

// Close the connection for a specific player
void close_id(int player_id) {
  char msg_buf[1000];

  close_connection(player_id);
  snprintf(msg_buf, sizeof(msg_buf), "Connection to %s closed\n",
           lookup(&(player[player_id].addr)));
  err(msg_buf);
}

// Close the connection for a specific player
void close_connection(int player_id) {
  close(player[player_id].sockfd);
  FD_CLR(player[player_id].sockfd, &afds);
  if (player[player_id].join && player[player[player_id].join].serv) {
    player[player[player_id].join].serv--;
  }

  player[player_id].login = 0;
  player[player_id].serv = 0;
  player[player_id].join = 0;
  player[player_id].version[0] = 0;
  player[player_id].note[0] = 0;
  player[player_id].name[0] = 0;
  player[player_id].username[0] = 0;
}

// Shutdown the server
void shutdown_server() {
  int i;
  char msg_buf[1000];

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login) {
      shutdown(player[i].sockfd, 2);
    }
  }
  snprintf(msg_buf, sizeof(msg_buf), "QKMJ Server shutdown\n");
  err(msg_buf);
  close(gps_sockfd);
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

// Handle bus error signal
void bus_err() {
  err("BUS ERROR!\n");
  exit(0);
}

// Handle broken pipe signal
void broken_pipe() {
  err("Broken PIPE!!\n");
}

// Handle timeout signal
void time_out() {
  err("timeout!");
  timeup = 1;
}

char *genpasswd(char *pw) {
  char saltc[2];
  long salt;
  int i, c;
  static char pwbuf[14];

  // Return an empty string if the password is empty
  if (strlen(pw) == 0) {
    return "";
  }

  // Generate a salt based on time and process ID
  time(&salt);
  salt = 9 * getpid();

  // Create a salt string
#ifndef lint
  saltc[0] = salt & 077;
  saltc[1] = (salt >> 6) & 077;
#endif
  for (i = 0; i < 2; i++) {
    c = saltc[i] + '.';
    if (c > '9') {
      c += 7;
    }
    if (c > 'Z') {
      c += 6;
    }
    saltc[i] = c;
  }

  // Copy the password to a buffer
  strncpy(pwbuf, pw, sizeof(pwbuf) - 1);

  // Return the encrypted password
  return crypt(pwbuf, saltc);
}

// Check if a password matches a hash
int checkpasswd(char *passwd, char *test) {
  static char pwbuf[14];
  char *pw;

  // Copy the test password to a buffer
  strncpy(pwbuf, test, sizeof(pwbuf) - 1);

  // Generate the hash of the test password
  pw = crypt(pwbuf, passwd);

  // Compare the generated hash with the stored hash
  return (strncmp(pw, passwd, sizeof(pwbuf)) == 0);
}

// Main function
int main(int argc, char **argv) {
  int i;

  // Set the maximum number of file descriptors
  getrlimit(RLIMIT_NOFILE, &fd_limit);
  fd_limit.rlim_cur = fd_limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &fd_limit);
  i = getdtablesize();
  printf("FD_SIZE=%d\n", i);

  // Set signal handlers
  signal(SIGSEGV, &core_dump);
  signal(SIGBUS, bus_err);
  signal(SIGPIPE, broken_pipe);
  signal(SIGALRM, time_out);

  // Get the port number from command line arguments
  if (argc < 2) {
    gps_port = DEFAULT_GPS_PORT;
  } else {
    gps_port = atoi(argv[1]);
    printf("Using port %s\n", argv[1]);
  }

  // Set the default IP address
  strncpy(gps_ip, DEFAULT_GPS_IP, sizeof(gps_ip) - 1);

  // Initialize the socket and variables
  init_socket();
  init_variable();

  // Start the main processing loop
  gps_processing();

  return 0;
}