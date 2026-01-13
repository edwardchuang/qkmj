/*
 * Server
 */
#include "mjgps.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <ifaddrs.h>
#include <net/if.h>
#include "mjgps_mongo_helpers.h"
#include "mongo.h"
#include "session_manager.h"
#include "protocol.h"
#include "protocol_def.h"
#include "logger.h"

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

#define MJGPS_VERSION "2.00 AI"

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
char number_map[20][5] = {"０", "１", "２", "３", "４",
                          "５", "６", "７", "８", "９"};

#define ADMIN_USER "mjgps"

#define MIN_JOIN_MONEY 0  // use -999999 if you allow user to join for debt

int gps_sockfd;

char climark[30];

struct player_info player[MAX_PLAYER];

struct player_record record;

struct record_index_type record_index;

FILE *fp, *log_fp;

struct ask_mode_info ask;

struct rlimit fd_limit;

/* Log local IP addresses */
void log_local_ips() {
  struct ifaddrs *ifaddr, *ifa;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    LOG_ERROR("getifaddrs failed");
    return;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) continue;
    if (ifa->ifa_flags & IFF_LOOPBACK) continue;

    if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST) == 0) {
      LOG_INFO("Local IP (%s): %s", ifa->ifa_name, host);
    }
  }
  freeifaddrs(ifaddr);
}

/* Helper functions for JSON extraction */
static const char* j_str(cJSON *json, const char *name) {
    cJSON *item = cJSON_GetObjectItem(json, name);
    return item ? cJSON_GetStringValue(item) : "";
}

static int j_int(cJSON *json, const char *name) {
    cJSON *item = cJSON_GetObjectItem(json, name);
    return item ? item->valueint : 0;
}

static double j_double(cJSON *json, const char *name) {
    cJSON *item = cJSON_GetObjectItem(json, name);
    return item ? item->valuedouble : 0.0;
}

// Old err() and game_log() removed. Replaced by logger.h functions.

void display_msg(int player_id, char* msg) {
  cJSON *payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", msg);
  send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
}

void list_player(int fd) {
  int i;
  char msg_buf[1000];
  int total_num = 0;
  
  cJSON *payload;

  // Hacky way to find id? 'fd' passed here is actually sockfd.
  // Note: The original code called find_user_name(player[fd].name) inside display_msg call?
  // No, list_player is called with player[player_id].sockfd.
  // But player[] is indexed by player_id, not sockfd.
  // Accessing player[fd] is dangerous if fd >= MAX_PLAYER.
  // However, in typical unix, fd can be small.
  // But strictly, we should pass player_id to this function, not fd.
  // I'll keep it as is for now to avoid breaking too much, but it's a smell.
  
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", "-------------    目前上線使用者    --------------- ");
  send_json(fd, MSG_TEXT_MESSAGE, payload);

  memset(msg_buf, 0, sizeof(msg_buf));
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2) {
      total_num++;
      if ((strlen(msg_buf) + strlen(player[i].name)) > 50) {
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "text", msg_buf);
        send_json(fd, MSG_TEXT_MESSAGE, payload);
        memset(msg_buf, 0, sizeof(msg_buf));
      }
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) - 1);
      strncat(msg_buf, "  ", sizeof(msg_buf) - strlen(msg_buf) - 1);
    }
  }
  
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", "--------------------------------------------------");
  send_json(fd, MSG_TEXT_MESSAGE, payload);
  
  snprintf(msg_buf, sizeof(msg_buf), "共 %d 人", total_num);
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", msg_buf);
  send_json(fd, MSG_TEXT_MESSAGE, payload);
}

void list_table(int fd, int mode) {
  int i;
  char msg_buf[1000];
  int total_num = 0;
  cJSON *payload;

  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "   桌長       人數  附註"); send_json(fd, MSG_TEXT_MESSAGE, payload);
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "--------------------------------------------------"); send_json(fd, MSG_TEXT_MESSAGE, payload);

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login && player[i].serv > 0) {
      if (player[i].serv > 4) {
        if (player[i].serv == 5)
          LOG_ERROR("SERV=5");
        else {
          LOG_ERROR("LIST TABLE ERROR!");
          snprintf(msg_buf, sizeof(msg_buf), "serv=%d", player[i].serv);
          close_id(i);
          LOG_ERROR("%s", msg_buf);
        }
      }
      if (mode == 2 && player[i].serv >= 4) continue;
      total_num++;
      snprintf(msg_buf, sizeof(msg_buf), "   %-10s %-4s  %s", player[i].name,
               number_map[player[i].serv], player[i].note);
      
      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "text", msg_buf);
      send_json(fd, MSG_TEXT_MESSAGE, payload);
    }
  }
  
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "--------------------------------------------------"); send_json(fd, MSG_TEXT_MESSAGE, payload);
  
  snprintf(msg_buf, sizeof(msg_buf), "共 %d 桌", total_num);
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", msg_buf);
  send_json(fd, MSG_TEXT_MESSAGE, payload);
}

void list_stat(int fd, char* name) {
  char msg_buf[1000];
  char msg_buf1[1000];
  char order_buf[30];
  int total_num = 0;
  int order = 1;
  cJSON *payload;

  /* MongoDB variables */
  bson_t* query;

  if (!read_user_name(name)) {
    payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "text", "找不到這個人!");
    send_json(fd, MSG_TEXT_MESSAGE, payload);
    return;
  }
  snprintf(msg_buf, sizeof(msg_buf), "◇名稱:%s ", record.name); 

  if (record.game_count >= 16) {
    /* Count players with more money and >= 16 games */
    query = BCON_NEW("game_count", BCON_INT32(16), "money", "{", "$gt",
                     BCON_INT64(record.money), "}");
    order = (int)mongo_count_documents(MONGO_DB_NAME, MONGO_COLLECTION_USERS,
                                       query) +
            1;
    bson_destroy(query);

    /* Count total players with >= 16 games */
    query = BCON_NEW("game_count", BCON_INT32(16));
    total_num = (int)mongo_count_documents(MONGO_DB_NAME,
                                           MONGO_COLLECTION_USERS, query);
    bson_destroy(query);
  }

  if (record.game_count < 16) {
    strncpy(order_buf, "無", sizeof(order_buf) - 1);
    order_buf[sizeof(order_buf) - 1] = '\0';
  } else
    snprintf(order_buf, sizeof(order_buf), "%d/%d", order, total_num);
  snprintf(msg_buf1, sizeof(msg_buf1),
           "◇金額:%ld 排名:%s 上線次數:%d 已玩局數:%d", record.money,
           order_buf, record.login_count, record.game_count);
  
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf1); send_json(fd, MSG_TEXT_MESSAGE, payload);
}

void who(int fd, char* name) {
  char msg_buf[1000];
  int i;
  int serv_id = -1;
  cJSON *payload;

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login && player[i].serv) {
      if (strcmp(player[i].name, name) == 0) {
        serv_id = i;
        break;
      }
    }
  }
  if (serv_id == -1) {
    payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "找不到此桌"); send_json(fd, MSG_TEXT_MESSAGE, payload);
    return;
  }
  snprintf(msg_buf, sizeof(msg_buf), "%s  ", player[serv_id].name); 
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "----------------   此桌使用者   ------------------"); send_json(fd, MSG_TEXT_MESSAGE, payload);
  
  memset(msg_buf, 0, sizeof(msg_buf));
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].join == serv_id) {
      if ((strlen(msg_buf) + strlen(player[i].name)) > 53) {
        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
        memset(msg_buf, 0, sizeof(msg_buf));
      }
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) - 1);
      strncat(msg_buf, "   ", sizeof(msg_buf) - strlen(msg_buf) - 1);
    }
  }
  if (strlen(msg_buf) > 0) {
    payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
  }
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "--------------------------------------------------"); send_json(fd, MSG_TEXT_MESSAGE, payload);
}

void lurker(int fd) {
  int i, total_num = 0;
  char msg_buf[1000];
  cJSON *payload;

  memset(msg_buf, 0, sizeof(msg_buf));
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "-------------   目前□置之使用者   --------------- "); send_json(fd, MSG_TEXT_MESSAGE, payload);
  for (i = 1; i < MAX_PLAYER; i++)
    if (player[i].login == 2 && (player[i].join == 0 && player[i].serv == 0)) {
      total_num++;
      if ((strlen(msg_buf) + strlen(player[i].name)) > 53) {
        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
        memset(msg_buf, 0, sizeof(msg_buf));
      }
      strncat(msg_buf, player[i].name, sizeof(msg_buf) - strlen(msg_buf) - 1);
      strncat(msg_buf, "  ", sizeof(msg_buf) - strlen(msg_buf) - 1);
    }
  if (strlen(msg_buf) > 0) {
      payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
  }
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "--------------------------------------------------"); send_json(fd, MSG_TEXT_MESSAGE, payload);
  
  snprintf(msg_buf, sizeof(msg_buf), "共 %d 人", total_num);
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
}

void find_user(int fd, char* name) {
  char msg_buf[1000];
  int id;
  char last_login_time[80];
  cJSON *payload;

  id = find_user_name(name);
  if (id > 0) {
    if (player[id].login == 2) {
      if (player[id].join == 0 && player[id].serv == 0) {
        snprintf(msg_buf, sizeof(msg_buf), "◇%s □置中", name);
        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
      }
      if (player[id].join) {
        snprintf(msg_buf, sizeof(msg_buf), "◇%s 在 %s 桌內", name,
                 player[player[id].join].name);
        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
      }
      if (player[id].serv) {
        snprintf(msg_buf, sizeof(msg_buf), "◇%s 在 %s 桌內", name,
                 player[id].name);
        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
      }
      return;
    }
  }
  if (!read_user_name(name)) {
    snprintf(msg_buf, sizeof(msg_buf), "◇沒有 %s 這個人", name);
    payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
  } else {
    snprintf(msg_buf, sizeof(msg_buf), "◇%s 不在線上", name);
    payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
    
    snprintf(last_login_time, sizeof(last_login_time), "%s", ctime(&record.last_login_time));
    if (strlen(last_login_time) > 0 && last_login_time[strlen(last_login_time) - 1] == '\n') {
        last_login_time[strlen(last_login_time) - 1] = 0;
    }
    snprintf(msg_buf, sizeof(msg_buf), "◇上次連線時間: %s", last_login_time);
    payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(fd, MSG_TEXT_MESSAGE, payload);
  }
}

void broadcast(int player_id, char* msg) {
  int i;
  cJSON *payload;

  if (strcmp(player[player_id].name, ADMIN_USER) != 0) return;
  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2) {
      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "text", msg);
      send_json(player[i].sockfd, MSG_TEXT_MESSAGE, payload);
    }
  }
}

void send_msg(int player_id, char* msg) {
  char *str1, *str2;
  int i;
  char msg_buf[1000];
  cJSON *payload;

  /* msg format: "name message" */
  str1 = strtok(msg, " "); /* name */
  if (str1 == NULL) return;
  str2 = msg + strlen(str1) + 1; /* message */
  
  for (i = 1; i < MAX_PLAYER; i++)
    if (player[i].login == 2 && strcmp(player[i].name, str1) == 0) {
      snprintf(msg_buf, sizeof(msg_buf), "*%s* %s", player[player_id].name, str2);
      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "text", msg_buf);
      send_json(player[i].sockfd, MSG_TEXT_MESSAGE, payload);
      return;
    }
  
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", "找不到這個人");
  send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
}

void invite(int player_id, char* name) {
  int i;
  char msg_buf[1000];
  cJSON *payload;

  for (i = 1; i < MAX_PLAYER; i++)
    if (player[i].login == 2 && strcmp(player[i].name, name) == 0) {
      snprintf(msg_buf, sizeof(msg_buf), "%s 邀請你加入 %s",
               player[player_id].name,
               (player[player_id].join == 0)
                   ? player[player_id].name
                   : player[player[player_id].join].name);
      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "text", msg_buf);
      send_json(player[i].sockfd, MSG_TEXT_MESSAGE, payload);
      return;
    }
  payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "text", "找不到這個人");
  send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
}

void init_socket() {
  struct sockaddr_in serv_addr;
  int on = 1;

  /*
   * open a TCP socket for internet stream socket
   */
  if ((gps_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    LOG_FATAL("Server: cannot open stream socket");

  /*
   * bind our local address
   */
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(gps_port);
  setsockopt(gps_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
  if (bind(gps_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    LOG_FATAL("server: cannot bind local address");
    exit(1);
  }
  listen(gps_sockfd, 10);
  LOG_INFO("Listen for client...");
}

char* lookup(struct sockaddr_in* cli_addrp) {
  struct hostent* hp;
  char* hostname;

  hp = gethostbyaddr((char*)&cli_addrp->sin_addr, sizeof(struct in_addr),
                     cli_addrp->sin_family);

  if (hp)
    hostname = (char*)hp->h_name;
  else
    hostname = inet_ntoa(cli_addrp->sin_addr);
  return hostname;
}

void init_variable() {
  int i;

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

int read_user_name(char* name) {
  bson_t* query;
  bson_t* doc;

  query = BCON_NEW("name", BCON_UTF8(name));
  doc = mongo_find_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  bson_destroy(query);

  if (doc) {
    bson_to_record(doc, &record);
    bson_destroy(doc);
    return 1;
  }
  return 0;
}

int read_user_name_update(char* name, int player_id) {
  if (read_user_name(name)) {
    if (player[player_id].id == record.id) {  /* double check */
      player[player_id].id = record.id;
      player[player_id].money = record.money;
    }
    return 1;
  }
  return 0;
}

void read_user_id(unsigned int id) {
  bson_t* query;
  bson_t* doc;

  query = BCON_NEW("user_id", BCON_INT64(id));
  doc = mongo_find_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  bson_destroy(query);

  if (doc) {
    bson_to_record(doc, &record);
    bson_destroy(doc);
  }
}

int add_user(int player_id, char* name, char* passwd) {
  int64_t new_id;

  new_id = mongo_get_next_sequence(MONGO_DB_NAME, MONGO_SEQUENCE_USERID);
  if (new_id < 0) {
    LOG_ERROR("Failed to generate user ID");
    return 0;
  }
  record.id = (unsigned int)new_id;

  strncpy(record.name, name, sizeof(record.name) - 1);
  record.name[sizeof(record.name) - 1] = '\0';
  strncpy(record.password, genpasswd(passwd), sizeof(record.password) - 1);
  record.password[sizeof(record.password) - 1] = '\0';
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
          sizeof(record.last_login_from) - strlen(record.last_login_from) - 1);

  if (check_user(player_id)) {
    write_record();
    return 1;
  } else
    return 0;
}

int check_user(int player_id) {
  char from[80];
  char email[80];

  strncpy(from, lookup(&(player[player_id].addr)), sizeof(from) - 1);
  from[sizeof(from) - 1] = '\0';
  snprintf(email, sizeof(email), "%s@", player[player_id].username);
  strncat(email, from, sizeof(email) - strlen(email) - 1);

  bson_t* query = BCON_NEW("pattern", "{", "$in", "[", BCON_UTF8(email),
                           BCON_UTF8(player[player_id].username), "]", "}");
  int64_t count = mongo_count_documents(MONGO_DB_NAME, "badusers", query);
  bson_destroy(query);

  if (count > 0) {
    display_msg(player_id, "你已被限制進入");
    return 0;
  }
  return 1;
}

void write_record() {
  bson_t* query;
  bson_t* doc;

  query = BCON_NEW("user_id", BCON_INT64(record.id));
  doc = record_to_bson(&record);

  mongo_replace_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query, doc);

  bson_destroy(query);
  bson_destroy(doc);
}

void print_news(int fd, char* name) {
  /* Ignore 'name' (filename) and read from 'news' collection */
  bson_t* query = bson_new();
  bson_t* opts =
      BCON_NEW("sort", "{", "date", BCON_INT32(-1), "}");  /* Show newest first */

  mongoc_cursor_t* cursor = mongo_find_documents(MONGO_DB_NAME, "news", query);
  const bson_t* doc;
  cJSON *payload;

  if (cursor) {
    while (mongoc_cursor_next(cursor, &doc)) {
      bson_iter_t iter;
      if (bson_iter_init_find(&iter, doc, "content")) {
        const char* content = bson_iter_utf8(&iter, NULL);
        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "text", content);
        send_json(fd, MSG_TEXT_MESSAGE, payload);
      }
    }
    mongoc_cursor_destroy(cursor);
  }
  bson_destroy(query);
  bson_destroy(opts);
}

void welcome_user(int player_id) {
  char msg_buf[1000];
  cJSON *payload;

  if (strcmp(player[player_id].version, "093") < 0 ||
      player[player_id].version[0] == 0) {
    payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "請使用 QKMJ Ver 0.93 Beta 以上版本上線"); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
    send_json(player[player_id].sockfd, MSG_VERSION_ERROR, NULL);
    return;
  }
  snprintf(msg_buf, sizeof(msg_buf), "★★★★★　歡迎 %s 來到ＱＫ麻將  ★★★★★",
           player[player_id].name);
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
  
  print_news(player[player_id].sockfd, NEWS_FILE);

  player[player_id].id = record.id;
  player[player_id].money = record.money;
  player[player_id].login = 2;
  
  if (!session_create(player[player_id].name, "gps-server", inet_ntoa(player[player_id].addr.sin_addr), player[player_id].money)) {
      LOG_WARN("Failed to create session for %s", player[player_id].name);
  }

  player[player_id].note[0] = 0;
  show_online_users(player_id);
  list_stat(player[player_id].sockfd, player[player_id].name);
  send_json(player[player_id].sockfd, MSG_WELCOME, NULL);
  
  /* 120 Update Money */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player[player_id].id);
  cJSON_AddNumberToObject(payload, "money", player[player_id].money);
  send_json(player[player_id].sockfd, MSG_UPDATE_MONEY, payload);
  
  player[player_id].input_mode = CMD_MODE;
}

void show_online_users(int player_id) {
  char msg_buf[1000];
  int total_num = 0;
  int online_num = 0;
  int i;
  cJSON *payload;

  bson_t* query = bson_new();
  total_num =
      (int)mongo_count_documents(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  bson_destroy(query);

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login == 2) online_num++;
  }
  snprintf(msg_buf, sizeof(msg_buf),
           "◇目前上線人數: %d 人       註冊人數: %d 人", online_num,
           total_num);
  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
}

void show_current_state(int player_id) {
  cJSON *payload;
  show_online_users(player_id);
  list_stat(player[player_id].sockfd, player[player_id].name);
  send_json(player[player_id].sockfd, MSG_WELCOME, NULL);
  
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player[player_id].id);
  cJSON_AddNumberToObject(payload, "money", player[player_id].money);
  send_json(player[player_id].sockfd, MSG_UPDATE_MONEY, payload);
}

void update_client_money(int player_id) {
  cJSON *payload;
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player[player_id].id);
  cJSON_AddNumberToObject(payload, "money", player[player_id].money);
  send_json(player[player_id].sockfd, MSG_UPDATE_MONEY, payload);
}

int find_user_name(char* name) {
  int i;

  for (i = 1; i < MAX_PLAYER; i++) {
    if (strcmp(player[i].name, name) == 0) return i;
  }
  return -1;
}

void close_id(int player_id) {
  char msg_buf[1000];

  close_connection(player_id);
  snprintf(msg_buf, sizeof(msg_buf), "Connection to %s closed",
           lookup(&(player[player_id].addr)));
  LOG_INFO("%s", msg_buf);
}

void close_connection(int player_id) {
  if (player[player_id].login == 2) {
      session_destroy(player[player_id].name);
  }
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

void shutdown_server() {
  int i;
  char msg_buf[1000];

  for (i = 1; i < MAX_PLAYER; i++) {
    if (player[i].login) shutdown(player[i].sockfd, 2);
  }
  snprintf(msg_buf, sizeof(msg_buf), "QKMJ Server shutdown");
  LOG_WARN("%s", msg_buf);
  mongo_disconnect();
  exit(0);
}

/*
 * 主伺服器迴圈 (Main Loop)
 */
void gps_processing() {
  int alen;
  int nfds;
  int player_id;
  int player_num = 0;
  int i;
  int msg_id;
  char msg_buf[1000];
  struct timeval tm;
  long current_time;
  struct tm* tim;
  int find_duplicated;
  cJSON *data = NULL;
  cJSON *payload = NULL;

  log_level = 0;
  /* nfds = getdtablesize(); -- Removed for portability, hardcoded below */
  nfds = 256;
  printf("%d\n", nfds);
  FD_ZERO(&afds);
  FD_SET(gps_sockfd, &afds);
  memmove((char*)&rfds, (char*)&afds, sizeof(rfds));
  tm.tv_sec = 0;
  tm.tv_usec = 0;
  
  for (;;) {
    memmove((char*)&rfds, (char*)&afds, sizeof(rfds));
    if (select(nfds, &rfds, (fd_set*)0, (fd_set*)0, 0) < 0) {
      snprintf(msg_buf, sizeof(msg_buf), "select: %d %s", errno,
               strerror(errno));
      LOG_ERROR("%s", msg_buf);
      continue;
    }
    if (FD_ISSET(gps_sockfd, &rfds)) {
      /*
       * 處理新連線 (Handle new connection)
       */
      for (player_num = 1; player_num < MAX_PLAYER; player_num++)
        if (!player[player_num].login) break;
      if (player_num == MAX_PLAYER - 1) LOG_WARN("Too many users");
      player_id = player_num;
      alen = sizeof(player[player_num].addr);
      player[player_id].sockfd =
          accept(gps_sockfd, (struct sockaddr*)&player[player_num].addr,
                 (socklen_t*)&alen);

      if (player[player_id].sockfd < 0) {
        LOG_ERROR("accept failed");
        continue;  /* Don't break the loop */
      }

      FD_SET(player[player_id].sockfd, &afds);
      fcntl(player[player_id].sockfd, F_SETFL, FNDELAY);
      player[player_id].login = 1;
      strncpy(climark, lookup(&(player[player_id].addr)), sizeof(climark) - 1);
      climark[sizeof(climark) - 1] = '\0';
      snprintf(msg_buf, sizeof(msg_buf), "Connected with %s", climark);
      LOG_INFO("%s", msg_buf);

      time(&current_time);
      tim = localtime(&current_time);

      if (player_id > login_limit) {
        if (strcmp(climark, "ccsun34") != 0) {
          payload = cJSON_CreateObject();
          cJSON_AddStringToObject(payload, "text", "對不起,目前使用人數超過上限, 請稍後再進來.");
          send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
          print_news(player[player_id].sockfd, "server.lst");
          close_id(player_id);
        }
      }
    }
    for (player_id = 1; player_id < MAX_PLAYER; player_id++) {
      if (player[player_id].login) {
        if (FD_ISSET(player[player_id].sockfd, &rfds)) {
          /*
           * Processing the player's information
           */
          if (!recv_json(player[player_id].sockfd, &msg_id, &data)) {
            LOG_WARN("cant read code or conn closed!");
            close_id(player_id);
            if (data) cJSON_Delete(data);
          } else {
            // Debug Log for Protocol Flow
            LOG_DEBUG("Player [%s](%d) sent CMD: %d", player[player_id].name, player_id, msg_id);

            switch (msg_id) {
              case MSG_GET_USERNAME: /* 99 */
                strncpy(player[player_id].username, j_str(data, "username"),
                        sizeof(player[player_id].username) - 1);
                player[player_id].username[sizeof(player[player_id].username) - 1] = '\0';
                break;
              case MSG_CHECK_VERSION: /* 100 */
                strncpy(player[player_id].version, j_str(data, "version"),
                        sizeof(player[player_id].version) - 1);
                player[player_id].version[sizeof(player[player_id].version) - 1] = '\0';
                break;
              case MSG_LOGIN: /* 101 */
                {
                    const char *name = j_str(data, "name");
                    strncpy(player[player_id].name, name, sizeof(player[player_id].name) - 1);
                    player[player_id].name[sizeof(player[player_id].name) - 1] = '\0';
                    
                    for (i = 0; i < strlen(name); i++) {
                      if (name[i] <= 32 && name[i] != 0) {
                        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "Invalid username!"); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                        close_id(player_id);
                        break;
                      }
                    }
                    if (read_user_name(player[player_id].name)) {
                      send_json(player[player_id].sockfd, MSG_LOGIN_OK, NULL);
                    } else {
                      send_json(player[player_id].sockfd, MSG_NEW_USER, NULL);
                    }
                }
                break;
              case MSG_CHECK_PASSWORD: /* 102 */
                if (read_user_name(player[player_id].name)) {
                  if (checkpasswd(record.password, (char*)j_str(data, "password"))) {
                    find_duplicated = 0;
                    for (i = 1; i < MAX_PLAYER; i++) {
                      if ((player[i].login == 2 || player[i].login == 3) &&
                          strcmp(player[i].name, player[player_id].name) == 0) {
                        send_json(player[player_id].sockfd, MSG_DUPLICATE_LOGIN, NULL);
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
                               sizeof(record.last_login_from), "%s@",
                               player[player_id].username);
                    }
                    strncat(record.last_login_from,
                            lookup(&player[player_id].addr),
                            sizeof(record.last_login_from) -
                                strlen(record.last_login_from) - 1);
                    record.login_count++;
                    write_record();
                    if (check_user(player_id))
                      welcome_user(player_id);
                    else
                      close_id(player_id);
                  } else {
                    LOG_WARN("Login Failed: User [%s] from IP [%s]", player[player_id].name, inet_ntoa(player[player_id].addr.sin_addr));
                    send_json(player[player_id].sockfd, MSG_PASSWORD_FAIL, NULL);
                  }
                }
                break;
              case MSG_CREATE_ACCOUNT: /* 103 */
                if (!add_user(player_id, player[player_id].name,
                              (char*)j_str(data, "password"))) {
                  close_id(player_id);
                  break;
                }
                welcome_user(player_id);
                break;
              case MSG_CHANGE_PASSWORD: /* 104 */
                read_user_name(player[player_id].name);
                strncpy(record.password, genpasswd((char*)j_str(data, "new_password")),
                        sizeof(record.password) - 1);
                record.password[sizeof(record.password) - 1] = '\0';
                write_record();
                break;
              case MSG_KILL_DUPLICATE: /* 105 */
                if (read_user_name(player[player_id].name) &&
                    player[player_id].login == 3) {
                  for (i = 1; i < MAX_PLAYER; i++) {
                    if ((player[i].login == 2 || player[i].login == 3) &&
                        (i != player_id) &&
                        strcmp(player[i].name, player[player_id].name) == 0) {
                      close_id(i);
                      break;
                    }
                  }
                  time(&record.last_login_time);
                  record.last_login_from[0] = 0;
                  if (player[player_id].username[0] != 0) {
                    snprintf(record.last_login_from,
                             sizeof(record.last_login_from), "%s@",
                             player[player_id].username);
                  }
                  strncat(record.last_login_from,
                          lookup(&player[player_id].addr),
                          sizeof(record.last_login_from) -
                              strlen(record.last_login_from) - 1);
                  record.login_count++;
                  write_record();
                  if (check_user(player_id))
                    welcome_user(player_id);
                  else
                    close_id(player_id);
                }
                break;
              case MSG_LIST_PLAYERS: /* 2 */
                list_player(player[player_id].sockfd);
                break;
              case MSG_LIST_TABLES: /* 3 */
                list_table(player[player_id].sockfd, 1);
                break;
              case MSG_SET_NOTE: /* 4 */
                strncpy(player[player_id].note, j_str(data, "note"),
                        sizeof(player[player_id].note) - 1);
                player[player_id].note[sizeof(player[player_id].note) - 1] = '\0';
                break;
              case MSG_USER_INFO: /* 5 */
                show_online_users(player_id);
                list_stat(player[player_id].sockfd, (char*)j_str(data, "name"));
                break;
              case MSG_WHO_IN_TABLE: /* 6 */
                who(player[player_id].sockfd, (char*)j_str(data, "table_leader"));
                break;
              case MSG_BROADCAST: /* 7 */
                broadcast(player_id, (char*)j_str(data, "msg"));
                break;
              case MSG_INVITE: /* 8 */
                invite(player_id, (char*)j_str(data, "name"));
                break;
              case MSG_SEND_MESSAGE: /* 9 */
                /* Reconstruct message "name msg" for send_msg */
                {
                    char combined[1024];
                    snprintf(combined, sizeof(combined), "%s %s", j_str(data, "to"), j_str(data, "msg"));
                    send_msg(player_id, combined);
                }
                break;
              case MSG_LURKER_LIST: /* 10 */
                lurker(player[player_id].sockfd);
                break;
              case MSG_JOIN_TABLE: /* 11 */
                if (!read_user_name_update(player[player_id].name, player_id)) {
                  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "查無此人"); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                  break;
                }
                update_client_money(player_id);
                if (player[player_id].money <= MIN_JOIN_MONEY) {
                  LOG_INFO("Join Table Denied: Player [%s] has insufficient funds (%ld)", player[player_id].name, player[player_id].money);
                  snprintf(msg_buf, sizeof(msg_buf),
                           "您的賭幣（%ld）不足，必須超過 %d 元才能加入牌桌",
                           player[player_id].money, MIN_JOIN_MONEY);
                  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                  break;
                }
                for (i = 1; i < MAX_PLAYER; i++) {
                  if (player[i].login == 2 && player[i].serv) {
                    if (strcmp(player[i].name, j_str(data, "table_leader")) == 0) {
                      if (player[i].serv >= 4) {
                        LOG_INFO("Join Table Denied: Target table [%s] is full", j_str(data, "table_leader"));
                        payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "此桌人數已滿!"); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                        break;
                      }
                      
                      payload = cJSON_CreateObject();
                      cJSON_AddNumberToObject(payload, "user_id", player[player_id].id);
                      cJSON_AddNumberToObject(payload, "money", player[player_id].money);
                      send_json(player[i].sockfd, MSG_UPDATE_MONEY, payload);
                      
                      payload = cJSON_CreateObject();
                      cJSON_AddStringToObject(payload, "name", player[player_id].name);
                      send_json(player[i].sockfd, MSG_JOIN_NOTIFY, payload);
                      
                      payload = cJSON_CreateObject();
                      cJSON_AddNumberToObject(payload, "status", 0);
                      cJSON_AddStringToObject(payload, "ip", inet_ntoa(player[i].addr.sin_addr));
                      cJSON_AddNumberToObject(payload, "port", player[i].port);
                      send_json(player[player_id].sockfd, MSG_JOIN_INFO, payload);
                      
                      player[player_id].join = i;
                      player[player_id].serv = 0;
                      player[i].serv++;
                      break;
                    }
                  }
                }
                if (i == MAX_PLAYER) {
                  payload = cJSON_CreateObject();
                  cJSON_AddNumberToObject(payload, "status", 1); /* 1 = full/not found? Client says "查無此桌" */
                  send_json(player[player_id].sockfd, MSG_JOIN_INFO, payload);
                }
                break;
              case MSG_OPEN_TABLE: /* 12 */
                if (!read_user_name_update(player[player_id].name, player_id)) {
                  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "查無此人"); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                  break;
                }
                update_client_money(player_id);
                if (player[player_id].money <= MIN_JOIN_MONEY) {
                  snprintf(msg_buf, sizeof(msg_buf),
                           "您的賭幣（%ld）不足，必須超過 %d 元才能開桌",
                           player[player_id].money, MIN_JOIN_MONEY);
                  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                  break;
                }
                player[player_id].port = j_int(data, "port");
                if (player[player_id].join) {
                  if (player[player[player_id].join].serv > 0)
                    player[player[player_id].join].serv--;
                  player[player_id].join = 0;
                }
                /*
                 * clear all client
                 */
                for (i = 1; i < MAX_PLAYER; i++) {
                  if (player[i].join == player_id) player[i].join = 0;
                }
                player[player_id].serv = 1;
                break;
              case MSG_LIST_TABLES_FREE: /* 13 */
                list_table(player[player_id].sockfd, 2);
                break;
              case MSG_CHECK_OPEN: /* 14 */
                if (!read_user_name_update(player[player_id].name, player_id)) {
                  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", "查無此人"); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                  break;
                }
                update_client_money(player_id);
                if (player[player_id].money <= MIN_JOIN_MONEY) {
                  snprintf(msg_buf, sizeof(msg_buf),
                           "您的賭幣（%ld）不足，必須超過 %d 元才能開桌",
                           player[player_id].money, MIN_JOIN_MONEY);
                  payload = cJSON_CreateObject(); cJSON_AddStringToObject(payload, "text", msg_buf); send_json(player[player_id].sockfd, MSG_TEXT_MESSAGE, payload);
                  break;
                }
                send_json(player[player_id].sockfd, MSG_OPEN_OK, NULL);
                break;
              case MSG_GAME_START_REQ: /* 15 */
                {
                    char match_id[64];
                    unsigned int ts = (unsigned int)time(NULL);
                    unsigned int salt = (unsigned int)(rand() & 0xFFFF);
                    /* Format: 8-char Hex Time + 4-char Hex Salt/PID mix (12 chars total) */
                    snprintf(match_id, sizeof(match_id), "%08X%04X", ts, (unsigned int)((player_id ^ salt) & 0xFFFF));
                    
                    payload = cJSON_CreateObject();
                    cJSON_AddStringToObject(payload, "match_id", match_id);
                    send_json(player[player_id].sockfd, MSG_GAME_START_REQ, payload);
                }
                break;
              case MSG_WIN_GAME: /* 20 */
                {
                    int id = j_int(data, "winner_id");
                    read_user_id((unsigned int)id);
                    record.money = (long)j_double(data, "money");
                    record.game_count++;
                    write_record();
                    
                    LOG_INFO("Game End: Winner [%d] Money update: %ld", id, record.money);
                    
                    for (i = 1; i < MAX_PLAYER; i++) {
                      if (player[i].login == 2 && player[i].id == id) {
                        player[i].money = record.money;
                        break;
                      }
                    }
                }
                break;
              case MSG_FIND_USER: /* 21 */
                find_user(player[player_id].sockfd, (char*)j_str(data, "name"));
                break;
              case MSG_GAME_RECORD: /* 900 */
                {
                    /* We assume data has "record" which is a JSON object. 
                       Original code just passed string.
                       Now we might pass the JSON stringified?
                       Or we can just log the 'data' part. */
                    char *json_str = cJSON_PrintUnformatted(data);
                    if (json_str) {
                        log_game_record(json_str);
                        free(json_str);
                    }
                }
                break;
              case 901: /* AI Log */
                {
                    /* Just pass the whole data object to mongo helper */
                    /* bson_t *doc = bson_new_from_json ... */
                    /* Since we have cJSON, we can serialize it to string then to BSON, 
                       or just use the string. */
                    char *json_str = cJSON_PrintUnformatted(data);
                    if (json_str) {
                        bson_t *doc = bson_new_from_json((const uint8_t *)json_str, -1, NULL);
                        if (doc) {
                            mongo_insert_document(MONGO_DB_NAME, MONGO_COLLECTION_LOGS, doc);
                            bson_destroy(doc);
                        }
                        free(json_str);
                    }
                }
                break;
              case 111:
                /*
                 * player[player_id].serv++;
                 */
                break;
              case MSG_LEAVE: /* 200 */
                close_id(player_id);
                break;
              case MSG_KICK_USER: /* 202 */
                if (strcmp(player[player_id].name, ADMIN_USER) != 0) break;
                {
                    int id = find_user_name((char*)j_str(data, "name"));
                    if (id >= 0) {
                      LOG_WARN("ADMIN ACTION: [%s] kicked user [%s]", ADMIN_USER, j_str(data, "name"));
                      send_json(player[id].sockfd, MSG_LEAVE, NULL);
                      close_id(id);
                    }
                }
                break;
              case MSG_LEAVE_TABLE_GPS: /* 205 */
                if (player[player_id].serv) {
                  for (i = 1; i < MAX_PLAYER; i++) {
                    if (player[i].join == player_id) player[i].join = 0;
                  }
                  player[player_id].serv = 0;
                  player[player_id].join = 0;
                } else if (player[player_id].join) {
                  if (player[player[player_id].join].serv > 0)
                    player[player[player_id].join].serv--;
                  player[player_id].join = 0;
                }
                break;
              case MSG_STATUS: /* 201 */
                show_current_state(player_id);
                break;
              case MSG_SHUTDOWN: /* 500 */
                if (strcmp(player[player_id].name, ADMIN_USER) == 0) {
                  LOG_WARN("ADMIN ACTION: [%s] initiated shutdown", ADMIN_USER);
                  shutdown_server();
                }
                break;
              default:
                snprintf(msg_buf, sizeof(msg_buf),
                         "### cmd=%d player_id=%d sockfd=%d ###", msg_id,
                         player_id, player[player_id].sockfd);
                LOG_ERROR("%s", msg_buf);
                close_connection(player_id);
                snprintf(msg_buf, sizeof(msg_buf),
                         "Connection to %s error, closed it",
                         lookup(&(player[player_id].addr)));
                LOG_ERROR("%s", msg_buf);
                break;
            }
            if (data) cJSON_Delete(data);
          }
        }
      }
    }
  }
}

void core_dump(int signo) {
  char buf[1024];
  char cmd[1024];
  FILE* fh;

  snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
  if (!(fh = fopen(buf, "r"))) exit(0);
  if (!fgets(buf, sizeof(buf), fh)) exit(0);
  fclose(fh);
  if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = '\0';
  snprintf(cmd, sizeof(cmd), "gdb %s %d", buf, getpid());
  system(cmd);

  LOG_FATAL("CORE DUMP!");
  exit(0);
}

void bus_err(int signo) {
  LOG_FATAL("BUS ERROR!");
  exit(0);
}

void broken_pipe(int signo) { LOG_WARN("Broken PIPE!!"); }

void time_out(int signo) {
  LOG_WARN("timeout!");
  timeup = 1;
}

char* genpasswd(char* pw) {
  char saltc[2];
  long salt;
  int i, c;
  static char pwbuf[14];

  if (strlen(pw) == 0) return "";
  time(&salt);
  salt = 9 * getpid();
#ifndef lint
  saltc[0] = salt & 077;
  saltc[1] = (salt >> 6) & 077;
#endif
  for (i = 0; i < 2; i++) {
    c = saltc[i] + '.';
    if (c > '9') c += 7;
    if (c > 'Z') c += 6;
    saltc[i] = (char)c;
  }
  strncpy(pwbuf, pw, sizeof(pwbuf) - 1);
  pwbuf[sizeof(pwbuf) - 1] = '\0';
  return crypt(pwbuf, saltc);
}

/* Constant-time string comparison to prevent timing attacks */
int safe_strcmp(const char* s1, const char* s2) {
  unsigned char result = 0;
  while (*s1 && *s2) {
    result |= (*s1 ^ *s2);
    s1++;
    s2++;
  }
  /* If lengths differ, *s1 or *s2 will be 0, result |= non-zero. */
  /* Also ensure both are at end. */
  result |= (*s1 ^ *s2);
  return result == 0;  /* Returns 1 (true) if equal, 0 (false) if not */
}

/*
 * 檢查密碼
 * 使用 crypt() 加密後比對。
 * 注意：使用 safe_strcmp 防止時序攻擊 (Timing Attack)。
 */
int checkpasswd(char* passwd, char* test) {
  static char pwbuf[14];
  char* pw;

  strncpy(pwbuf, test, 14);
  pw = crypt(pwbuf, passwd);
  return safe_strcmp(pw, passwd);
}

int main(int argc, char** argv) {
  int i;
  char* mongo_uri;

  setlocale(LC_ALL, "");

  /*
   * Set fd to be the maximum number
   */
  getrlimit(RLIMIT_NOFILE, &fd_limit);
  fd_limit.rlim_cur = fd_limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &fd_limit);
  i = getdtablesize();
  printf("FD_SIZE=%d\n", i);
  signal(SIGSEGV, &core_dump);
  signal(SIGBUS, bus_err);
  signal(SIGPIPE, broken_pipe);
  signal(SIGALRM, time_out);

  LOG_INFO("QKMJ MJGPS Server Ver %s (Build: %s) Starting...", MJGPS_VERSION, GIT_HASH);
  log_local_ips();
  
  /* Environment Variable Configuration (Cloud Native) */
  char *env_port = getenv("PORT");
  char *env_mongo = getenv("MONGO_URI");
  char *env_limit = getenv("LOGIN_LIMIT");

  if (env_port) {
    gps_port = atoi(env_port);
    LOG_INFO("Using PORT from environment: %d", gps_port);
  } else if (argc >= 2) {
    gps_port = atoi(argv[1]);
    LOG_INFO("Using PORT from argument: %d", gps_port);
  } else {
    gps_port = DEFAULT_GPS_PORT;
    LOG_INFO("Using default PORT: %d", gps_port);
  }

  if (env_limit) {
    login_limit = atoi(env_limit);
    LOG_INFO("Using LOGIN_LIMIT from environment: %d", login_limit);
  } else {
    login_limit = LOGIN_LIMIT;
  }

  strncpy(gps_ip, DEFAULT_GPS_IP, sizeof(gps_ip) - 1);
  gps_ip[sizeof(gps_ip) - 1] = '\0';

  /* Init Mongo */
  if (env_mongo) {
     mongo_uri = env_mongo;
     LOG_INFO("Using MONGO_URI from environment");
  } else {
     mongo_uri = "mongodb://localhost:27017";
     LOG_INFO("Using default MONGO_URI");
  }
  
  if (!mongo_connect(mongo_uri)) {
    LOG_FATAL("Failed to connect to MongoDB at %s", mongo_uri);
    exit(1);
  }

  session_mgmt_init();

  init_socket();
  init_variable();
  /* login_limit is set in init_variable() again, we need to fix that */
  /* init_variable() sets login_limit = LOGIN_LIMIT. We should move that logic or fix init_variable */
  /* Let's see init_variable implementation below... */
  
  gps_processing();

  mongo_disconnect();
  return 0;
}