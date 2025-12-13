#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "cJSON.h"

/*
 * Sends a JSON packet with the specified ID and data.
 * Format: {"id": msg_id, "data": data}
 * 
 * @param fd The file descriptor to write to.
 * @param msg_id The command ID (from protocol_def.h).
 * @param data The JSON object containing the payload. 
 *             NOTE: Ownership of 'data' is TAKEN by this function. 
 *             It will be freed when the wrapper object is freed.
 *             Pass NULL if no data payload is needed.
 * @return 1 on success, 0 on failure.
 */
int send_json(int fd, int msg_id, cJSON *data);

/*
 * Receives a JSON packet.
 * Reads from fd until a null terminator is found.
 * 
 * @param fd The file descriptor to read from.
 * @param msg_id Pointer to store the received command ID.
 * @param data Pointer to a cJSON pointer. Will be set to the "data" object 
 *             extracted from the received JSON.
 *             NOTE: The caller assumes ownership of this cJSON object 
 *             and must free it with cJSON_Delete().
 *             This may be NULL if no data field exists or on error.
 * @return 1 on success, 0 on failure/timeout/disconnect.
 */
int recv_json(int fd, int *msg_id, cJSON **data);

#endif /* PROTOCOL_H */
