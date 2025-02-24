#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "mjdef.h"

//  使用 enum 定義指令 ID，提升可讀性和維護性
typedef enum {
    CMD_UNKNOWN = 0,
    CMD_TABLE, CMD_LIST, CMD_PLAYER, CMD_JOIN, CMD_SERV, CMD_QUIT, CMD_EXIT,
    CMD_WHO, CMD_SIT, CMD_NUM, CMD_NEW, CMD_SHOW, CMD_LEAVE, CMD_HELP,
    CMD_NOTE, CMD_STAT, CMD_LOGIN, CMD_PASSWD, CMD_BROADCAST, CMD_KICK,
    CMD_KILL, CMD_INVITE, CMD_MSG, CMD_SHUTDOWN, CMD_LURKER, CMD_FIND,
    CMD_BEEP, CMD_EXEC, CMD_FREE, CMD_S_PLAYER, CMD_S_JOIN, CMD_S_HELP,
    CMD_S_WHO, CMD_S_SERV, CMD_S_LEAVE, CMD_S_TABLE, CMD_S_QUIT
} CommandId;

//  使用結構體儲存指令名稱和 ID 的映射關係，提升可維護性
typedef struct {
  const char* name;
  CommandId id;
} CommandMapping;

//  函數原型宣告 (提升程式碼可讀性)
void Tokenize(const char* strinput);
void my_strupr(char* upper, const char* org);
static CommandId command_mapper(const char* cmd);
void who(const char* name);
static void help();
static void handle_command(CommandId cmd_id, int narg);
void command_parser(const char* msg);

#endif
