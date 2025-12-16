#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdbool.h>
#include <time.h>

/* Session Status Constants */
#define SESSION_STATUS_LOBBY "LOBBY"
#define SESSION_STATUS_WAITING "WAITING"
#define SESSION_STATUS_PLAYING "PLAYING"

/**
 * Initialize the session manager (e.g., ensure indexes exist).
 * @return true on success.
 */
bool session_mgmt_init();

/**
 * Cleanup session manager resources.
 */
void session_mgmt_cleanup();

/**
 * Register a user login (create a session).
 * 
 * @param username The player's username.
 * @param server_id Unique identifier for this server process.
 * @param ip Client IP address.
 * @param money Current money in cache.
 * @return true if successful.
 */
bool session_create(const char *username, const char *server_id, const char *ip, long money);

/**
 * Remove a user session (logout).
 * 
 * @param username The player's username.
 * @return true if successful.
 */
bool session_destroy(const char *username);

/**
 * Update the user's heartbeat/last_active time to prevent timeout.
 * 
 * @param username The player's username.
 * @return true if successful.
 */
bool session_heartbeat(const char *username);

/**
 * Update the user's current status and table.
 * 
 * @param username The player's username.
 * @param status The new status (e.g., "PLAYING").
 * @param table_leader The username of the table leader (or NULL if in lobby).
 * @return true if successful.
 */
bool session_update_status(const char *username, const char *status, const char *table_leader);

/**
 * Check if a user is currently online (has an active session).
 * 
 * @param username The player's username.
 * @return true if online.
 */
bool session_is_online(const char *username);

#endif // SESSION_MANAGER_H
