#include "session_manager.h"
#include "mongo.h"
#include "logger.h"
#include <bson/bson.h>
#include <stdio.h>

#define COL_SESSIONS "active_sessions"

bool session_mgmt_init() {
    // Ideally, we would ensure indexes here (e.g., unique username).
    // For now, we assume the database is reachable.
    return true;
}

void session_mgmt_cleanup() {
    // No specific cleanup needed as we rely on the global mongo connection
}

bool session_create(const char *username, const char *server_id, const char *ip, long money) {
    if (!username || !server_id) return false;

    bson_t *doc = bson_new();
    BSON_APPEND_UTF8(doc, "username", username);
    BSON_APPEND_UTF8(doc, "server_id", server_id);
    BSON_APPEND_UTF8(doc, "ip", ip ? ip : "");
    BSON_APPEND_INT64(doc, "money", money);
    BSON_APPEND_DATE_TIME(doc, "login_time", time(NULL) * 1000);
    BSON_APPEND_DATE_TIME(doc, "last_active", time(NULL) * 1000);
    BSON_APPEND_UTF8(doc, "status", SESSION_STATUS_LOBBY);
    // 'current_table' is null/missing initially

    // We use "replace" logic: Upsert based on username
    bson_t *query = BCON_NEW("username", BCON_UTF8(username));
    
    // Using mongo_replace_document or mongo_update_document with upsert?
    // mongo_replace_document replaces the whole doc.
    // We want to ensure one session per user.
    
    bool success = mongo_replace_document(MONGO_DB_NAME, COL_SESSIONS, query, doc);
    
    bson_destroy(query);
    bson_destroy(doc);
    
    if (success) {
        LOG_INFO("Session created for user: %s", username);
    } else {
        LOG_ERROR("Failed to create session for user: %s", username);
    }
    
    return success;
}

bool session_destroy(const char *username) {
    if (!username) return false;

    bson_t *query = BCON_NEW("username", BCON_UTF8(username));
    bool success = mongo_delete_document(MONGO_DB_NAME, COL_SESSIONS, query);
    bson_destroy(query);
    
    if (success) {
        LOG_INFO("Session destroyed for user: %s", username);
    }
    
    return success;
}

bool session_heartbeat(const char *username) {
    if (!username) return false;

    bson_t *query = BCON_NEW("username", BCON_UTF8(username));
    bson_t *update = BCON_NEW("$set", "{", "last_active", BCON_DATE_TIME(time(NULL) * 1000), "}");
    
    bool success = mongo_update_document(MONGO_DB_NAME, COL_SESSIONS, query, update);
    
    bson_destroy(query);
    bson_destroy(update);
    
    return success;
}

bool session_update_status(const char *username, const char *status, const char *table_leader) {
    if (!username || !status) return false;

    bson_t *query = BCON_NEW("username", BCON_UTF8(username));
    bson_t *update;
    
    if (table_leader) {
        update = BCON_NEW("$set", "{", 
                          "status", BCON_UTF8(status),
                          "current_table", BCON_UTF8(table_leader),
                          "last_active", BCON_DATE_TIME(time(NULL) * 1000),
                          "}");
    } else {
         update = BCON_NEW("$set", "{", 
                          "status", BCON_UTF8(status),
                          "current_table", BCON_NULL,
                          "last_active", BCON_DATE_TIME(time(NULL) * 1000),
                          "}");
    }

    bool success = mongo_update_document(MONGO_DB_NAME, COL_SESSIONS, query, update);
    
    bson_destroy(query);
    bson_destroy(update);
    
    return success;
}

bool session_is_online(const char *username) {
    if (!username) return false;

    bson_t *query = BCON_NEW("username", BCON_UTF8(username));
    int64_t count = mongo_count_documents(MONGO_DB_NAME, COL_SESSIONS, query);
    bson_destroy(query);
    
    return (count > 0);
}
