#ifndef _CHECK_H_
#define _CHECK_H_

void clear_check_flag(char sit);
int search_card(char sit, char card);
int check_kang(char sit,char card);
int check_pong(char sit,char card);
int check_eat(char sit,char card);
void check_card(char sit,char card);
int check_begin_flower(char sit,char card,char position);
int check_flower(char sit,char card);
void write_check(char check);
void send_pool_card();
void compare_check();

#endif
