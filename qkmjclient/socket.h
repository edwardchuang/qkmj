extern int Check_for_data(int fd);
extern void init_serv_socket();
extern int init_socket(char* host, int port, int* sockfd);
extern void close_join();
extern void close_serv();
extern int leave();