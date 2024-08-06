#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <sqlite3.h>

const char *sql_player_record = 
    "CREATE TABLE IF NOT EXISTS player_record ("
	"id INTEGER,"
	"name TEXT,"
	"password TEXT,"
	"money INTEGER,"
	"\"level\" INTEGER,"
	"login_count INTEGER,"
	"game_count INTEGER,"
	"regist_time INTEGER,"
	"last_login_time INTEGER,"
	"last_login_from TEXT,"
	"CONSTRAINT PLAYER_RECORD_PK PRIMARY KEY (id),"
	"CONSTRAINT player_record_player_info_FK FOREIGN KEY (id) REFERENCES player_info(id)"
    ");"
    "CREATE INDEX IF NOT EXISTS player_record_name_IDX ON player_record (name);";

const char  *sql_player_info =
    "CREATE TABLE IF NOT EXISTS player_info ("
    "sockfd INTEGER,"
    "login INTEGER,"
    "id INTEGER PRIMARY KEY,"
    "name TEXT,"
    "username TEXT,"
    "money INTEGER,"
    "serv INTEGER,"
    "\"join\" INTEGER,"
    "\"type\" INTEGER,"
    "note TEXT,"
    "input_mode INTEGER,"
    "prev_request INTEGER,"
    "addr TEXT,"
    "port INTEGER,"
    "version TEXT"
    ");"
    "CREATE INDEX IF NOT EXISTS player_info_name_IDX ON player_info (name);";

sqlite3 *init_database();

void handle_db_error(sqlite3 *db);
int create_tables(sqlite3 *db);

int create_player(sqlite3 *db, struct player_record *player);
struct player_record *read_player(sqlite3 *db, unsigned int id);
int update_player(sqlite3 *db, struct player_record *player);
int delete_player(sqlite3 *db, unsigned int id);

int create_info(sqlite3 *db, struct player_info *info);
struct player_info *read_info(sqlite3 *db, unsigned int id);
int update_info(sqlite3 *db, struct player_info *info);
int delete_info(sqlite3 *db, unsigned int id);

#endif/*_DATABASE_H_*/