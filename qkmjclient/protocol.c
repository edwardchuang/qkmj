#include "protocol.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* Max buffer size for receiving JSON packets */
#define MAX_JSON_PACKET_SIZE 65536

/*
 * Helper to check if data is available on socket
 * Returns 1 if data available, 0 if not, -1 on error
 */
static int check_socket_read(int fd, int timeout_sec, int timeout_usec) {
  fd_set rfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  tv.tv_sec = timeout_sec;
  tv.tv_usec = timeout_usec;

  retval = select(fd + 1, &rfds, NULL, NULL, &tv);
  return retval;
}

int send_json(int fd, int msg_id, cJSON* data) {
  cJSON* root = cJSON_CreateObject();
  if (!root) {
    return 0;
  }

  cJSON_AddNumberToObject(root, "id", msg_id);
  if (data) {
    cJSON_AddItemToObject(root, "data", data);
  }

  char* json_str = cJSON_PrintUnformatted(root);
  if (!json_str) {
    cJSON_Delete(root);
    return 0;
  }

  size_t len = strlen(json_str);
  if (len >= MAX_JSON_PACKET_SIZE - 1) {
    fprintf(stderr, "Error: JSON packet too large (%zu bytes, max %d)\n", len, MAX_JSON_PACKET_SIZE - 1);
    free(json_str);
    cJSON_Delete(root);
    return 0;
  }

  ssize_t written = 0;
  ssize_t n;

  /* Write the string */
  while (written < len) {
    /* Safety: Check if socket is writable with 2s timeout */
    fd_set wfds;
    struct timeval tv = {2, 0};
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    if (select(fd + 1, NULL, &wfds, NULL, &tv) <= 0) {
        fprintf(stderr, "Error: send_json write timeout on fd %d\n", fd);
        free(json_str);
        cJSON_Delete(root);
        return 0;
    }

    n = write(fd, json_str + written, len - written);
    if (n < 0) {
      if (errno == EINTR) continue;
      free(json_str);
      cJSON_Delete(root);
      return 0;
    }
    written += n;
  }

  /* Write the null terminator */
  char terminator = '\0';
  if (write(fd, &terminator, 1) < 0) {
    free(json_str);
    cJSON_Delete(root);
    return 0;
  }

  free(json_str);
  cJSON_Delete(root);
  return 1;
}

#define PEEK_SIZE 4096

int recv_json(int fd, int* msg_id, cJSON** data) {
  char buf[MAX_JSON_PACKET_SIZE];
  int n = 0;
  ssize_t read_count;
  ssize_t peek_count;
  char peek_buf[PEEK_SIZE];
  int found_null;
  int i;

  if (data) *data = NULL;
  if (msg_id) *msg_id = 0;

  /* Wait for data with timeout */
  if (check_socket_read(fd, 5, 0) <= 0) {
    return 0;
  }

  /* Read until null terminator */
  while (n < MAX_JSON_PACKET_SIZE - 1) {
    /* Optimization: Peek ahead to find null terminator */
    peek_count = recv(fd, peek_buf, sizeof(peek_buf), MSG_PEEK);

    if (peek_count < 0) {
      if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
        /* Wait a bit if non-blocking */
        if (check_socket_read(fd, 1, 0) <= 0) return 0;
        continue;
      }
      return 0;
    } else if (peek_count == 0) {
      /* Connection closed */
      return 0;
    }

    /* Scan for null terminator in the peeked data */
    found_null = -1;
    for (i = 0; i < peek_count; i++) {
        if (peek_buf[i] == '\0') {
            found_null = i;
            break;
        }
    }

    if (found_null != -1) {
        /* We found the end of the message. Read exactly up to it. */
        /* Check if it fits in our buffer */
        if (n + found_null + 1 >= MAX_JSON_PACKET_SIZE) {
            /* Packet too large. We could drain and fail, but for now just fail. */
            return 0;
        }
        
        read_count = read(fd, &buf[n], found_null + 1);
        if (read_count != found_null + 1) {
             /* Should not happen if peek said it was there */
             return 0;
        }
        n += read_count;
        break; /* We are done */
    } else {
        /* No null found, consume all peeked data and continue */
        if (n + peek_count >= MAX_JSON_PACKET_SIZE) {
             return 0; /* Packet too large */
        }
        read_count = read(fd, &buf[n], peek_count);
        n += read_count;
    }
  }
  buf[n] = '\0'; /* Ensure null termination */

  if (n == 0) return 0; /* Empty packet */

  /* Parse JSON */
  cJSON* root = cJSON_Parse(buf);
  if (!root) {
    /* Failed to parse */
    return 0;
  }

  /* Extract ID */
  cJSON* id_item = cJSON_GetObjectItem(root, "id");
  if (!cJSON_IsNumber(id_item)) {
    cJSON_Delete(root);
    return 0;
  }
  *msg_id = id_item->valueint;

  /* Extract Data */
  cJSON* data_item = cJSON_GetObjectItem(root, "data");
  if (data_item) {
    /* Detach the item so we can return it without it being deleted with root */
    *data = cJSON_DetachItemViaPointer(root, data_item);
  } else {
    *data = NULL;
  }

  cJSON_Delete(root);
  return 1;
}