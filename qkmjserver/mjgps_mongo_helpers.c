#include "mjgps_mongo_helpers.h"

#include <string.h>

#include "mjgps.h"
#include "mongo.h"

// Helper to convert MongoDB BSON to player_record struct
void bson_to_record(const bson_t* doc, struct player_record* rec) {
  bson_iter_t iter;

  if (bson_iter_init_find(&iter, doc, "user_id")) {
    rec->id = (unsigned int)bson_iter_int64(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "name")) {
    const char* str = bson_iter_utf8(&iter, NULL);
    strncpy(rec->name, str, sizeof(rec->name) - 1);
    rec->name[sizeof(rec->name) - 1] = '\0';
  }
  if (bson_iter_init_find(&iter, doc, "password")) {
    const char* str = bson_iter_utf8(&iter, NULL);
    strncpy(rec->password, str, sizeof(rec->password) - 1);
    rec->password[sizeof(rec->password) - 1] = '\0';
  }
  if (bson_iter_init_find(&iter, doc, "money")) {
    rec->money = (long)bson_iter_int64(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "level")) {
    rec->level = bson_iter_int32(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "login_count")) {
    rec->login_count = bson_iter_int32(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "game_count")) {
    rec->game_count = bson_iter_int32(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "regist_time")) {
    rec->regist_time = (long)bson_iter_int64(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "last_login_time")) {
    rec->last_login_time = (long)bson_iter_int64(&iter);
  }
  if (bson_iter_init_find(&iter, doc, "last_login_from")) {
    const char* str = bson_iter_utf8(&iter, NULL);
    strncpy(rec->last_login_from, str, sizeof(rec->last_login_from) - 1);
    rec->last_login_from[sizeof(rec->last_login_from) - 1] = '\0';
  }
}

// Helper to convert player_record struct to MongoDB BSON
bson_t* record_to_bson(const struct player_record* rec) {
  bson_t* doc = bson_new();

  // Use "user_id" as our custom ID, let Mongo manage _id
  BSON_APPEND_INT64(doc, "user_id", rec->id);
  BSON_APPEND_UTF8(doc, "name", rec->name);
  BSON_APPEND_UTF8(doc, "password", rec->password);
  BSON_APPEND_INT64(doc, "money", rec->money);
  BSON_APPEND_INT32(doc, "level", rec->level);
  BSON_APPEND_INT32(doc, "login_count", rec->login_count);
  BSON_APPEND_INT32(doc, "game_count", rec->game_count);
  BSON_APPEND_INT64(doc, "regist_time", (int64_t)rec->regist_time);
  BSON_APPEND_INT64(doc, "last_login_time", (int64_t)rec->last_login_time);
  BSON_APPEND_UTF8(doc, "last_login_from", rec->last_login_from);

  return doc;
}