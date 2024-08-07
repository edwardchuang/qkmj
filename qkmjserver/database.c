/*
    database.c, sqlite implementation in replacement of qkmj.rec and in-memory arrays
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include "mjgps.h"
#include "database.h"

// Function to handle database errors
void handle_db_error(sqlite3 *db) {
    char msg_buf[1024];
    snprintf(msg_buf, sizeof(msg_buf), "SQL error: %s\n", sqlite3_errmsg(db));
    err(msg_buf);
}

sqlite3 *init_database() {
    sqlite3 *db;
    if (sqlite3_open(DEFAULT_RECORD_DATABASE, &db)) {
        handle_db_error(db);
        return NULL;
    }

    // Create tables if they don't exist
    if (create_tables(db) != SQLITE_OK) {
        handle_db_error(db);
        sqlite3_close(db);
        return NULL;
    }

    return db;
}

// Create tables if they don't exist
int create_tables(sqlite3 *db) {
    int rc = sqlite3_exec(db, sql_player_record, NULL, 0, NULL);
    if (rc != SQLITE_OK) {
        return rc;
    }

    rc = sqlite3_exec(db, sql_player_info, NULL, 0, NULL);
    if (rc != SQLITE_OK) {
        return rc;
    }

    return SQLITE_OK;
}

// 建立新的玩家紀錄
int create_player(sqlite3 *db, struct player_record *player) {
  char *sql = "INSERT INTO players(name, password, money, level, login_count, "
              "game_count, regist_time, last_login_time, last_login_from) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, player->name, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, player->password, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 3, player->money);
  sqlite3_bind_int(stmt, 4, player->level);
  sqlite3_bind_int(stmt, 5, player->login_count);
  sqlite3_bind_int(stmt, 6, player->game_count);
  sqlite3_bind_int64(stmt, 7, player->regist_time);
  sqlite3_bind_int64(stmt, 8, player->last_login_time);
  sqlite3_bind_text(stmt, 9, player->last_login_from, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

// 透過 ID 讀取玩家紀錄
struct player_record *read_player(sqlite3 *db, unsigned int id) {
  char *sql = "SELECT * FROM players WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    struct player_record *player = malloc(sizeof(struct player_record));
    player->id = sqlite3_column_int(stmt, 0);
    strncpy(player->name, (char *)sqlite3_column_text(stmt, 1),
            sizeof(player->name));
    strncpy(player->password, (char *)sqlite3_column_text(stmt, 2),
            sizeof(player->password));
    player->money = sqlite3_column_int(stmt, 3);
    player->level = sqlite3_column_int(stmt, 4);
    player->login_count = sqlite3_column_int(stmt, 5);
    player->game_count = sqlite3_column_int(stmt, 6);
    player->regist_time = sqlite3_column_int64(stmt, 7);
    player->last_login_time = sqlite3_column_int64(stmt, 8);
    strncpy(player->last_login_from, (char *)sqlite3_column_text(stmt, 9),
            sizeof(player->last_login_from));

    sqlite3_finalize(stmt);
    return player;
  } else {
    sqlite3_finalize(stmt);
    return NULL;
  }
}

// 更新玩家紀錄
int update_player(sqlite3 *db, struct player_record *player) {
  char *sql =
      "UPDATE players SET name = ?, password = ?, money = ?, level = ?, "
      "login_count = ?, game_count = ?, regist_time = ?, last_login_time = ?, "
      "last_login_from = ? WHERE id = ?;";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, player->name, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, player->password, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 3, player->money);
  sqlite3_bind_int(stmt, 4, player->level);
  sqlite3_bind_int(stmt, 5, player->login_count);
  sqlite3_bind_int(stmt, 6, player->game_count);
  sqlite3_bind_int64(stmt, 7, player->regist_time);
  sqlite3_bind_int64(stmt, 8, player->last_login_time);
  sqlite3_bind_text(stmt, 9, player->last_login_from, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 10, player->id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

// 刪除玩家紀錄
int delete_player(sqlite3 *db, unsigned int id) {
  char *sql = "DELETE FROM players WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

// 建立新的玩家資訊紀錄
int create_info(sqlite3 *db, struct player_info *info) {
  char *sql = "INSERT INTO player_info(sockfd, login, id, name, username, money, "
              "serv, join, type, note, input_mode, prev_request, ip_address, port, version) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_int(stmt, 1, info->sockfd);
  sqlite3_bind_int(stmt, 2, info->login);
  sqlite3_bind_int(stmt, 3, info->id);
  sqlite3_bind_text(stmt, 4, info->name, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 5, info->username, -1, SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 6, info->money);
  sqlite3_bind_int(stmt, 7, info->serv);
  sqlite3_bind_int(stmt, 8, info->join);
  sqlite3_bind_int(stmt, 9, info->type);
  sqlite3_bind_text(stmt, 10, info->note, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 11, info->input_mode);
  sqlite3_bind_int(stmt, 12, info->prev_request);
  sqlite3_bind_text(stmt, 13, inet_ntoa(info->addr.sin_addr), -1, SQLITE_STATIC); 
  sqlite3_bind_int(stmt, 14, info->port);
  sqlite3_bind_text(stmt, 15, info->version, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

// 透過 ID 讀取玩家資訊紀錄
struct player_info *read_info(sqlite3 *db, unsigned int id) {
  char *sql = "SELECT * FROM player_info WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    struct player_info *info = malloc(sizeof(struct player_info));
    info->sockfd = sqlite3_column_int(stmt, 0);
    info->login = sqlite3_column_int(stmt, 1);
    info->id = sqlite3_column_int(stmt, 2);
    strncpy(info->name, (char *)sqlite3_column_text(stmt, 3), sizeof(info->name));
    strncpy(info->username, (char *)sqlite3_column_text(stmt, 4), sizeof(info->username));
    info->money = sqlite3_column_int64(stmt, 5);
    info->serv = sqlite3_column_int(stmt, 6);
    info->join = sqlite3_column_int(stmt, 7);
    info->type = sqlite3_column_int(stmt, 8);
    strncpy(info->note, (char *)sqlite3_column_text(stmt, 9), sizeof(info->note));
    info->input_mode = sqlite3_column_int(stmt, 10);
    info->prev_request = sqlite3_column_int(stmt, 11);
    inet_aton((char *)sqlite3_column_text(stmt, 12), &(info->addr.sin_addr)); // Convert IP string to in_addr
    info->port = sqlite3_column_int(stmt, 13);
    strncpy(info->version, (char *)sqlite3_column_text(stmt, 14), sizeof(info->version));

    sqlite3_finalize(stmt);
    return info;
  } else {
    sqlite3_finalize(stmt);
    return NULL;
  }
}

// 更新玩家資訊紀錄
int update_info(sqlite3 *db, struct player_info *info) {
  char *sql = "UPDATE player_info SET sockfd = ?, login = ?, id = ?, name = ?, username = ?, "
              "money = ?, serv = ?, join = ?, type = ?, note = ?, input_mode = ?, "
              "prev_request = ?, ip_address = ?, port = ?, version = ? WHERE id = ?;";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_int(stmt, 1, info->sockfd);
  sqlite3_bind_int(stmt, 2, info->login);
  sqlite3_bind_int(stmt, 3, info->id);
  sqlite3_bind_text(stmt, 4, info->name, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 5, info->username, -1, SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 6, info->money);
  sqlite3_bind_int(stmt, 7, info->serv);
  sqlite3_bind_int(stmt, 8, info->join);
  sqlite3_bind_int(stmt, 9, info->type);
  sqlite3_bind_text(stmt, 10, info->note, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 11, info->input_mode);
  sqlite3_bind_int(stmt, 12, info->prev_request);
  sqlite3_bind_text(stmt, 13, inet_ntoa(info->addr.sin_addr), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 14, info->port);
  sqlite3_bind_text(stmt, 15, info->version, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 16, info->id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

// 刪除玩家資訊紀錄
int delete_info(sqlite3 *db, unsigned int id) {
  char *sql = "DELETE FROM player_info WHERE id = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL 錯誤: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);
  return 0;
}
