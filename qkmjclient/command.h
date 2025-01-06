#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "mjdef.h"

#define TABLE 1
#define LIST 2
#define PLAYER 3
#define JOIN 4
#define SERV 5
#define QUIT 6
#define EXIT 7
#define WHO 8
#define SIT 9
#define NUM 10
#define NEW 11
#define SHOW 12
#define LEAVE 13
#define HELP 14
#define NOTE 15
#define STAT 16
#define LOGIN 17
#define PASSWD 18
#define preserve1 19
#define BROADCAST 20
#define KICK 21
#define KILL 22
#define INVITE 23
#define MSG 24
#define SHUTDOWN 25
#define LURKER 26
#define FIND 27
#define BEEP 28
#define EXEC 29
#define FREE 30
#define S_PLAYER 31
#define S_JOIN 32
#define S_HELP 33
#define S_WHO 34
#define S_SERV 35
#define S_LEAVE 36
#define S_TABLE 37
#define S_QUIT 38

void Tokenize(char *strinput);
void my_strupr(char* upper,char* org);
int command_mapper(char *cmd);
void who(char *name);
void list_players();
void help();
void command_parser(char *msg);

#endif
