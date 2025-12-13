#ifndef __MJGPS_H__
#define __MJGPS_H__

#include <netinet/in.h>
#include "cJSON.h" /* Need cJSON for prototypes if any */

#define DEFAULT_GPS_PORT 7001
#define DEFAULT_GPS_IP "0.0.0.0"
#define MAX_PLAYER 500
#define LOGIN_LIMIT 200
#define ASK_MODE 1
#define CMD_MODE 2

#define RECORD_FILE "/var/qkrecord/qkmj.rec"
#define INDEX_FILE "/var/qkrecord/qkmj.inx"
#define NEWS_FILE "/var/qkrecord/news.txt"
#define BADUSER_FILE "/var/qkrecord/baduser.txt"
#define LOG_FILE "/var/qkrecord/qkmj.log"
#define GAME_FILE "/var/qkrecord/qkmj_game.log"

#define DEFAULT_RECORD_FILE "/var/qkrecord/qkmj.rec"
#define DEFAULT_MONEY 40000

struct player_record {
  unsigned int id;
  char name[20];
  char password[15];
  long money;
  int level;
  int login_count;
  int game_count;
  long regist_time;
  long last_login_time;
  char last_login_from[60];
};

struct player_info {
  int sockfd;
  int login;
  unsigned int id;
  char name[20];
  char username[30];
  long money;
  int serv;
  int join;
  int type;
  char note[80];
  int input_mode;
  int prev_request;
  struct sockaddr_in addr;
  int port;
  char version[10];
  int is_ai;
};

struct record_index_type {
  char name[20];
  unsigned int id;
};

struct ask_mode_info {
  int question;
  int answer_ok;
  char* answer;
};

char* lookup(struct sockaddr_in*);
char* genpasswd(char*);
int checkpasswd(char*, char*);

int check_user(int);
void write_record();
void close_id(int);
void close_connection(int);
void shutdown_server();

/* Additional Prototypes */
void display_msg(int player_id, char* msg);
int Check_for_data(int fd);
void list_player(int fd);
void list_table(int fd, int mode);
void list_stat(int fd, char* name);
void who(int fd, char* name);
void lurker(int fd);
void find_user(int fd, char* name);
void broadcast(int player_id, char* msg);
void send_msg(int player_id, char* msg);
void invite(int player_id, char* name);
void init_socket();
void init_variable();
int read_user_name(char* name);
int read_user_name_update(char* name, int player_id);
void read_user_id(unsigned int id);
int add_user(int player_id, char* name, char* passwd);
void print_news(int fd, char* name);
void welcome_user(int player_id);
void show_online_users(int player_id);
void show_current_state(int player_id);
void update_client_money(int player_id);
int find_user_name(char* name);
void gps_processing();
void core_dump(int signo);
void bus_err(int signo);
void broken_pipe(int signo);
void time_out(int signo);

#endif
