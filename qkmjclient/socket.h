extern int Check_for_data(int fd);
extern void init_serv_socket();
extern int init_socket(char *host, int port, int *sockfd);
extern int read_msg(int fd, char *msg);
extern void write_msg(int fd, char *msg);
extern void broadcast_msg(int player_id, char *msg);
extern void close_join();
extern void close_serv();
extern int leave();