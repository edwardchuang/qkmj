/*
 * Server  test
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "mjgps.h"
#include "mjgps_mongo_helpers.h"
#include "mongo.h"

char record_file[MAXPATHLEN];  // Kept for compatibility in main, but unused

struct player_record record;

int read_user_name(char* name) {
  bson_t* query;
  bson_t* doc;

  query = BCON_NEW("name", BCON_UTF8(name));
  doc = mongo_find_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  bson_destroy(query);

  if (doc) {
    bson_to_record(doc, &record);
    bson_destroy(doc);
    return 1;
  }
  return 0;
}

void read_user_id(unsigned int id) {
  bson_t* query;
  bson_t* doc;

  query = BCON_NEW("user_id", BCON_INT64(id));
  doc = mongo_find_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  bson_destroy(query);

  if (doc) {
    bson_to_record(doc, &record);
    bson_destroy(doc);
  } else {
    // If not found, maybe clear record?
    // Original code fseek/fread might leave garbage or fail silent if EOF.
    // We'll just zero it.
    memset(&record, 0, sizeof(record));
  }
}

void write_record() {
  bson_t* query;
  bson_t* doc;

  query = BCON_NEW("user_id", BCON_INT64(record.id));
  doc = record_to_bson(&record);

  mongo_replace_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query, doc);

  bson_destroy(query);
  bson_destroy(doc);
}

void delete_user(unsigned int id) {
  bson_t* query = BCON_NEW("user_id", BCON_INT64(id));
  mongo_delete_document(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  bson_destroy(query);
}

void print_record() {
  struct player_record tmprec;
  char time1[40], time2[40];
  int player_num;
  int i;
  int id;
  char name[40];
  long money;
  int c;

  bson_t* query = bson_new();

  printf("(1) 以 id 查看特定使用者\n");
  printf("(2) 以名稱查看特定使用者\n");
  printf("(3) 查看所有使用者\n");
  printf("(4) 查看此金額以上的使用者\n");
  printf("(5) 查看此金額以下的使用者\n");
  printf("\n請輸入你的選擇:");
  scanf("%d", &i);

  switch (i) {
    case 1:
      printf("請輸入你要查看的 id:");
      scanf("%d", &id);
      if (id < 0) {
        bson_destroy(query);
        return;
      }
      BSON_APPEND_INT64(query, "user_id", id);
      break;

    case 2:
      printf("請輸入你要查看的名稱:");
      while ((c = getchar()) != '\n' && c != EOF); /* flush stdin */
      if (fgets(name, sizeof(name), stdin)) {
        name[strcspn(name, "\n")] = 0;
      }
      BSON_APPEND_UTF8(query, "name", name);
      break;

    case 3:
      // Empty query matches all
      break;

    case 4:
      printf("請輸入金額:");
      scanf("%ld", &money);
      BSON_APPEND_INT64(query, "money", money);
      // GT operator needed?
      // Complex query construction: { money: { $gte: money } }
      bson_destroy(query);
      query = BCON_NEW("money", "{", "$gte", BCON_INT64(money), "}");
      break;

    case 5:
      printf("請輸入金額:");
      scanf("%ld", &money);
      bson_destroy(query);
      query = BCON_NEW("money", "{", "$lte", BCON_INT64(money), "}");
      break;

    default:
      bson_destroy(query);
      return;
  }

  player_num = 0;
  mongoc_cursor_t* cursor =
      mongo_find_documents(MONGO_DB_NAME, MONGO_COLLECTION_USERS, query);
  const bson_t* doc;

  if (cursor) {
    while (mongoc_cursor_next(cursor, &doc)) {
      bson_to_record(doc, &tmprec);

      printf("%d %10s %15s %ld %d %d %d  %s\n", tmprec.id, tmprec.name,
             tmprec.password, tmprec.money, tmprec.level, tmprec.login_count,
             tmprec.game_count, tmprec.last_login_from);

      strncpy(time1, ctime(&tmprec.regist_time), sizeof(time1) - 1);
      time1[sizeof(time1) - 1] = '\0';
      strncpy(time2, ctime(&tmprec.last_login_time), sizeof(time2) - 1);
      time2[sizeof(time2) - 1] = '\0';

      time1[strlen(time1) - 1] = 0;
      time2[strlen(time2) - 1] = 0;
      printf("              %s    %s\n", time1, time2);

      if (tmprec.name[0] != 0) player_num++;
    }
    mongoc_cursor_destroy(cursor);
  }

  printf("--------------------------------------------------------------\n");
  if (i == 3) printf("共 %d 人注冊\n", player_num);

  bson_destroy(query);
}

void modify_user() {
  int i;
  char name[40];
  char account[40];
  long money;
  int c;
  int res;

  printf("請輸入使用者帳號:");
  while ((c = getchar()) != '\n' && c != EOF); /* flush stdin */
  if (fgets(account, sizeof(account), stdin)) {
    account[strcspn(account, "\n")] = 0;
  }

  res = read_user_name(account);
  if (res == 0) {
    printf(" 查無此人 ");
    return;
  }

  printf("\n");
  printf("(1) 更改名稱\n");
  printf("(2) 重設密碼\n");
  printf("(3) 更改金額\n");
  printf("(4) 取消更改\n");
  printf("\n請輸入你的選擇:");
  scanf("%d", &i);
  printf("\n");

  switch (i) {
    case 1:
      printf("請輸入要更改的名稱:");
      while ((c = getchar()) != '\n' && c != EOF); /* flush stdin */
      if (fgets(name, sizeof(name), stdin)) {
        name[strcspn(name, "\n")] = 0;
      }
      strncpy(record.name, name, sizeof(record.name) - 1);
      record.name[sizeof(record.name) - 1] = '\0';
      printf("改名為 %s\n", name);
      break;

    case 2:
      record.password[0] = 0;
      printf("密碼已重設!\n");
      break;

    case 3:
      printf("請輸入要更改的金額:");
      scanf("%ld", &money);
      record.money = money;
      printf("金額更改為 %ld\n", money);
      break;

    default:
      return;
  }
  write_record();
}

int main(int argc, char** argv) {
  int i, id;
  char* mongo_uri;

  // record_file argument ignored in Mongo version but kept for arg position
  // compat Init Mongo
  mongo_uri = getenv("MONGO_URI");
  if (!mongo_uri) {
    mongo_uri = "mongodb://localhost:27017";
  }
  if (!mongo_connect(mongo_uri)) {
    fprintf(stderr, "Failed to connect to MongoDB at %s\n", mongo_uri);
    exit(1);
  }

  while (1) {
    printf("\n");
    printf("(1) 列出所有使用者資料\n");
    printf("(2) 刪除使用者\n");
    printf("(3) 更改使用者資料\n");
    printf("(4) 離開\n\n");
    printf("請輸入你的選擇:");
    scanf("%d", &i);

    switch (i) {
      case 1:
        print_record();
        break;

      case 2:
        printf("請輸入使用者代號:");
        scanf("%d", &id);

        if (id >= 0) {
          delete_user((unsigned int)id);
          printf("User %d deleted.\n", id);
        }
        break;

      case 3:
        modify_user();
        break;

      default:
        mongo_disconnect();
        return 0;
    }
  }

  mongo_disconnect();
  return 0;
}