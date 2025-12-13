#include "logger.h"
#include "mongo.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

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

    // 3. Print to Console (stderr)
    // Use colors for console if possible, but keep it simple for now.
    fprintf(stderr, "[%s] [%s] [%s:%d] %s\n", 
            time_buf, level_strings[level], file, line, message);

    // 4. Save to MongoDB (WARN and above)
    if (level >= LOG_LEVEL_WARN) {
        // We use 'mongo_insert_document' from mongo.h
        // Note: We are creating a new connection potentially every log? 
        // No, mongo.c manages a global client/database handle if initialized.
        // Assuming mongo is initialized.
        
        bson_t *doc = BCON_NEW(
            "timestamp", BCON_DATE_TIME(now * 1000),
            "level", BCON_UTF8(level_strings[level]),
            "file", BCON_UTF8(file),
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

    // Save to MongoDB
    bson_t *doc = BCON_NEW(
        "level", BCON_UTF8("game"),
        "message", BCON_UTF8(json_str),
        "timestamp", BCON_DATE_TIME(now * 1000)
    );
    mongo_insert_document(MONGO_DB_NAME, MONGO_COLLECTION_LOGS, doc);
    bson_destroy(doc);
}
