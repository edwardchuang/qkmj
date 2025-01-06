#ifndef __MJGPS_H__
#define __MJGPS_H__

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
#include <stdbool.h>
#include <assert.h>
#define DEFAULT_GPS_PORT 7001
#define DEFAULT_GPS_IP "0.0.0.0"
#define MAX_PLAYER 500
#define LOGIN_LIMIT 200
#define ASK_MODE 1
#define CMD_MODE 2

#define HOMEDIR "/tmp/qkmj"
#define RECORD_FILE HOMEDIR"/qkmj.rec"
#define INDEX_FILE HOMEDIR"/qkmj.inx"
#define NEWS_FILE HOMEDIR"/news.txt"
#define BADUSER_FILE HOMEDIR"/baduser.txt"
#define LOG_FILE HOMEDIR"/qkmj.log"
#define GAME_FILE HOMEDIR"/qkmj_game.log"

#define DEFAULT_RECORD_FILE HOMEDIR"/qkmj.rec"
#define DEFAULT_MONEY 40000


struct player_record
{
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

struct player_info
{
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
};

struct record_index_type
{
    char name[20];
    unsigned int id;
};

struct ask_mode_info
{
    int question;
    int answer_ok;
    char *answer;
};

#define HASH_TABLE_SIZE 31 // Size of the hash table (adjust as needed)

// 訊息處理函式類型 (維持不變)
typedef void (*message_handler)(int player_id, char *msg);

// 訊息處理器結構
typedef struct {
    int msg_id;
    message_handler handler;
} msg_handler_entry;

// 函式宣告
int err(const char *errmsg);
int game_log(const char *gamemsg);
int read_msg(int fd, char *msg);
void write_msg(int fd, const char *msg);
void display_msg(int player_id, const char *msg);
int Check_for_data(int fd);
int checkpasswd(const char *passwd, const char *test);
char *genpasswd(const char *plaintext_password);
int convert_msg_id(int player_id, const char *msg);
void init_socket();
char *lookup(const struct sockaddr_in *cli_addrp);
void init_variable();
int read_user_name(const char *name);
int read_user_name_update(const char *name, int player_id);
int read_user_id(unsigned int id);
int add_user(int player_id, const char *name, const char *passwd);
int check_user(int player_id);
void write_record();
void print_news(int fd, const char *name);
void welcome_user(int player_id);
void show_online_users(int player_id);
void show_current_state(int player_id);
void update_client_money(int player_id);
int find_user_name(const char *name);
void shutdown_server();
void core_dump(int signo);
void bus_err();
void broken_pipe();
void time_out();
void list_player(int fd);
void list_table(int fd, int mode);
void list_stat(int fd, const char *name);
void who(int fd, const char *name);
void lurker(int fd);
void find_user(int fd, const char *name);
void broadcast(int player_id, const char *msg);
void send_msg(int player_id, const char *msg);
void invite(int player_id, const char *name);
void close_id(int player_id);
void close_connection(int player_id);
void gps_processing();
void init_message_handlers();
typedef void (*message_handler)(int player_id, char *msg);
void add_message_handler(int msg_id, message_handler handler);
message_handler get_message_handler(int msg_id);
static void handle_list_player(int player_id, char *msg);
static void handle_list_table(int player_id, char *msg);
static void handle_update_note(int player_id, char *msg);
static void handle_list_stat(int player_id, char *msg);
static void handle_who(int player_id, char *msg);
static void handle_broadcast(int player_id, char *msg);
static void handle_invite(int player_id, char *msg);
static void handle_send_msg(int player_id, char *msg);
static void handle_lurker(int player_id, char *msg);
static void handle_join_internal(int player_id, char *msg);
static void handle_create_table_internal(int player_id, char *msg);
static void handle_list_joinable_tables(int player_id, char* msg);
static void handle_check_create_table_internal(int player_id, char* msg);
static void handle_win_game_internal(int player_id, char* msg);
static void handle_find_user(int player_id, char* msg);
static void handle_game_record_internal(int player_id, char* msg);
static void handle_leave_game(int player_id, char* msg);
static void handle_force_leave_internal(int player_id, char* msg);
static void handle_leave_table_internal(int player_id, char* msg);
static void handle_show_current_state(int player_id, char* msg);
static void handle_shutdown_server_internal(int player_id, char* msg);
int main(int argc, char **argv);

#endif
