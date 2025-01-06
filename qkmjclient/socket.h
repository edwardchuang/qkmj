#ifndef _SOCKET_H_
#define _SOCKET_H_

int Check_for_data(int fd);
void init_serv_socket();
void get_my_info();
int init_socket(char *host,int portnum,int *sockfd);
void accept_new_client();
int read_msg(int fd,char *msg);
int read_msg_id(int fd,char *msg);
int write_msg(int fd,char *msg);
void broadcast_msg(int id,char *msg);
void close_client(int player_id);
void close_join();
void close_serv();
void leave();

#endif/* _SOCKET_H_ */