#ifndef _SOCKET_H_
#define _SOCKET_H_

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

#include "mjdef.h"
#include "qkmj.h"
#define MAX_MSG_LENGTH 256

int check_for_data(int fd);
void init_serv_socket(void);  // Use void for functions with no parameters
void get_my_info(void);
int init_socket(const char *host, int portnum, int *sockfd);
void accept_new_client(void);
void close_client(int player_id);
void close_join(void);
void close_serv(void);
void leave(void);
int read_msg(int fd, char *msg);
int read_msg_id(int fd, char *msg);
int write_msg(int fd, const char *msg);
void broadcast_msg(int id, const char *msg);

#endif/* _SOCKET_H_ */