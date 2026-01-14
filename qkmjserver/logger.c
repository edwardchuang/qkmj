#include "logger.h"
#include "mongo.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_CJSON_CJSON_H
#include <cjson/cJSON.h>
#elif defined(HAVE_CJSON_H)
#include <cJSON.h>
#else
#include <cJSON.h> /* Fallback */
#endif

static const char* level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
    // 1. Format the message
    char message[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // 2. Get timestamp
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", local);

    // 3. Shorten filename
    const char *short_file = strrchr(file, '/');
    if (short_file) {
        short_file++; // Skip the slash
    } else {
        short_file = file;
    }

    // 4. Print to Console (stderr) in JSON format
    cJSON *log_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(log_obj, "timestamp", time_buf);
    cJSON_AddStringToObject(log_obj, "level", level_strings[level]);
    cJSON_AddStringToObject(log_obj, "file", short_file);
    cJSON_AddNumberToObject(log_obj, "line", line);
    cJSON_AddStringToObject(log_obj, "message", message);
    
    char *json_str = cJSON_PrintUnformatted(log_obj);
    if (json_str) {
        fprintf(stderr, "%s\n", json_str);
        free(json_str);
    }
    cJSON_Delete(log_obj);

    // 5. Save to MongoDB
    // Determine minimum log level from environment, defaulting to INFO
    const char *env_level_str = getenv("MONGO_LOG_LEVEL");
    LogLevel min_mongo_level = LOG_LEVEL_INFO; // Default to INFO

    if (env_level_str) {
        if (strcmp(env_level_str, "DEBUG") == 0) min_mongo_level = LOG_LEVEL_DEBUG;
        else if (strcmp(env_level_str, "INFO") == 0) min_mongo_level = LOG_LEVEL_INFO;
        else if (strcmp(env_level_str, "WARN") == 0) min_mongo_level = LOG_LEVEL_WARN;
        else if (strcmp(env_level_str, "ERROR") == 0) min_mongo_level = LOG_LEVEL_ERROR;
        else if (strcmp(env_level_str, "FATAL") == 0) min_mongo_level = LOG_LEVEL_FATAL;
    }

    if (level >= min_mongo_level) {
        // We use 'mongo_insert_document' from mongo.h
        
        bson_t *doc = BCON_NEW(
            "timestamp", BCON_DATE_TIME(now * 1000),
            "level", BCON_UTF8(level_strings[level]),
            "file", BCON_UTF8(short_file),
            "line", BCON_INT32(line),
            "message", BCON_UTF8(message)
        );
        mongo_insert_document(MONGO_DB_NAME, MONGO_COLLECTION_LOGS, doc);
        bson_destroy(doc);
    }
}

void log_game_record(const char *json_str) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", local);

    // Print to console for verification
    fprintf(stderr, "[%s] [GAME] %s\n", time_buf, json_str);

    // Parse JSON to BSON for structured storage
    bson_error_t error;
    bson_t *data_bson = bson_new_from_json((const uint8_t *)json_str, -1, &error);
    
    if (data_bson) {
        bson_t *doc = bson_new();
        BSON_APPEND_UTF8(doc, "level", "game_record");
        BSON_APPEND_DATE_TIME(doc, "timestamp", now * 1000);
        BSON_APPEND_DOCUMENT(doc, "data", data_bson);
        
        // Also extract match_id to top level if present for easier indexing
        bson_iter_t iter;
        if (bson_iter_init_find(&iter, data_bson, "match_id")) {
            BSON_APPEND_UTF8(doc, "match_id", bson_iter_utf8(&iter, NULL));
        }

        mongo_insert_document(MONGO_DB_NAME, MONGO_COLLECTION_LOGS, doc);
        bson_destroy(doc);
        bson_destroy(data_bson);
    } else {
        // Fallback to old format if JSON is invalid
        bson_t *doc = BCON_NEW(
            "level", BCON_UTF8("game_record"),
            "message", BCON_UTF8(json_str),
            "timestamp", BCON_DATE_TIME(now * 1000)
        );
        mongo_insert_document(MONGO_DB_NAME, MONGO_COLLECTION_LOGS, doc);
        bson_destroy(doc);
    }
}
