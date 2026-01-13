#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mjdef.h"

#include "protocol.h"
#include "protocol_def.h"
#include "qkmj.h"

struct passwd* userdata;

int Check_for_data(int fd)
/* Checks the socket descriptor fd to see if any incoming data has
   arrived.  If yes, then returns 1.  If no, then returns 0.
   If an error, returns -1 and stores the error message in socket_error.
*/
{
  int status;        /* return code from Select call. */
  fd_set wait_set;   /* A set representing the connections that
                               have been established. */
  struct timeval tm; /* A timelimit of zero for polling for new
                        connections. */

  FD_ZERO(&wait_set);
  FD_SET(fd, &wait_set);

  tm.tv_sec = 0;
  tm.tv_usec = 500;
  status = select(FD_SETSIZE, &wait_set, (fd_set*)0, (fd_set*)0, &tm);

  /*  if (status < 0)
      sprintf (socket_error, "Error in select: %s", sys_errlist[errno]); */

  return (status);
}

void init_serv_socket() {
  struct sockaddr_in serv_addr;

  /* open a TCP socket for internet stream socket */
  if ((serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    err("Server: cannot open stream socket");

  /* bind our local address */
  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  do {
    serv_addr.sin_port = htons(SERV_PORT);
    SERV_PORT++;
  } while (bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <
           0);
  listen(serv_sockfd, 10);
  FD_SET(serv_sockfd, &afds);
}

void get_my_info() {
  userdata = getpwuid(getuid());
  strncpy(my_username, userdata->pw_name, sizeof(my_username) - 1);
  my_username[sizeof(my_username) - 1] = '\0';
}

int init_socket(char* host, int portnum, int* sockfd) {
  struct sockaddr_in serv_addr;

  struct hostent* hp;

  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  if ((hp = gethostbyname(host)) == NULL || hp->h_addrtype != AF_INET) {
    serv_addr.sin_addr.s_addr = inet_addr(host);
  } else {
    memmove(&serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length);
  }

  serv_addr.sin_port = htons(portnum);

  /* open a TCP socket */
  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    err("client: cannot open stream socket");

  /* connect to server */
  if (connect(*sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    return -1;
  }
  return 0;
}

void accept_new_client() {
  int alen;
  int player_id;
  char msg_buf[255];
  cJSON* payload;

  /* Find a free space */
  int i = 2;
  for (; i < MAX_PLAYER; i++)
    if (!player[i].in_table) break;
  if (i == MAX_PLAYER) err("Too many players!");
  player_id = i;
  /* NOTICE: here we don't know player[player_id].addr */
  /* it's ok! just get size. */
  alen = sizeof(player[player_id].addr);
  /* NOTICE: so here, player[player_id].sockfd must be wrong!! */
  player[player_id].sockfd =
      accept(serv_sockfd, (struct sockaddr*)&player[player_id].addr,
             (socklen_t*)&alen);
  FD_SET(player[player_id].sockfd, &afds);
  player[player_id].in_table = 1;
  player_in_table++;
  /* assign a sit to the new comer */
  if (player_in_table <= PLAYER_NUM) {
    for (int j = 1; j <= 4; j++) {
      if (!table[j]) {
        player[player_id].sit = j;
        table[j] = player_id;
        break;
      }
    }
  }
  snprintf(msg_buf, sizeof(msg_buf), "%s 加入此桌，目前人數 %d ",
           new_client_name, player_in_table);
  send_gps_line(msg_buf);
  strncpy(player[player_id].name, new_client_name,
          sizeof(player[player_id].name) - 1);
  player[player_id].name[sizeof(player[player_id].name) - 1] = '\0';
  player[player_id].id = new_client_id;
  player[player_id].money = new_client_money;

  /* 201 MSG_NEW_COMER_INFO */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player_id);
  cJSON_AddNumberToObject(payload, "sit", player[player_id].sit);
  cJSON_AddNumberToObject(payload, "count", player_in_table);
  cJSON_AddStringToObject(payload, "name", player[player_id].name);
  /* Broadcast to everyone (except 1 which is me, wait, new comer needs this
     too?) Original: broadcast_msg(1, ...). accept_new_client calls
     broadcast_msg(1, ..). broadcast_msg(id, ..) sends to everyone EXCEPT id. So
     it sends to 2..MAX_PLAYER except 1. This INCLUDES the new player
     (player_id). So new player receives 201.
  */
  for (int j = 2; j < MAX_PLAYER; j++) {
    if (player[j].in_table) {
      /* Need to clone payload for each send, or reuse? send_json deletes it. */
      send_json(player[j].sockfd, MSG_NEW_COMER_INFO,
                cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  /* 205 MSG_MY_INFO */
  /* Send to new player only */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player_id);
  cJSON_AddNumberToObject(payload, "sit", player[player_id].sit);
  cJSON_AddStringToObject(payload, "name", player[player_id].name);
  send_json(player[player_id].sockfd, MSG_MY_INFO, payload);

  /* 202 MSG_UPDATE_MONEY_P2P */
  /* Broadcast new player money to everyone */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player_id);
  cJSON_AddNumberToObject(payload, "money", new_client_money);
  cJSON_AddNumberToObject(payload, "db_id", new_client_id);
  for (int j = 2; j < MAX_PLAYER; j++) {
    if (player[j].in_table) {
      send_json(player[j].sockfd, MSG_UPDATE_MONEY_P2P,
                cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  new_client = 0;
  /* write_msg(gps_sockfd, "111"); Removed */

  /* Send all player info to the new player */
  for (int j = 1; j < MAX_PLAYER; j++) {
    if (player[j].in_table && j != player_id) {
      /* 203 MSG_OTHER_INFO */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "user_id", j);
      cJSON_AddNumberToObject(payload, "sit", player[j].sit);
      cJSON_AddStringToObject(payload, "name", player[j].name);
      send_json(player[player_id].sockfd, MSG_OTHER_INFO, payload);

      /* 202 MSG_UPDATE_MONEY_P2P */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "user_id", j);
      cJSON_AddNumberToObject(payload, "money", player[j].money);
      cJSON_AddNumberToObject(payload, "db_id", player[j].id);
      send_json(player[player_id].sockfd, MSG_UPDATE_MONEY_P2P, payload);
    }
  }
  /* Check the number of player in table */
  if (player_in_table == PLAYER_NUM) {
    /* Request match_id from GPS before starting */
    send_json(gps_sockfd, MSG_GAME_START_REQ, NULL);
    /* The game will actually start in handle_gps_message when MSG_GAME_START_REQ response arrives */
  }
}

void close_client(int player_id) {
  char msg_buf[255];
  cJSON* payload;

  if (player_in_table == 4) {
    init_global_screen();
    input_mode = TALK_MODE;
  }
  player_in_table--;

  /* 206 MSG_PLAYER_LEAVE */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(payload, "user_id", player_id);
  cJSON_AddNumberToObject(payload, "count", player_in_table);
  for (int i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != player_id) {
      send_json(player[i].sockfd, MSG_PLAYER_LEAVE,
                cJSON_Duplicate(payload, 1));
    }
  }
  cJSON_Delete(payload);

  snprintf(msg_buf, sizeof(msg_buf), "%s 離開此桌，目前人數剩下 %d 人",
           player[player_id].name, player_in_table);
  display_comment(msg_buf);
  close(player[player_id].sockfd);
  FD_CLR(player[player_id].sockfd, &afds);
  player[player_id].in_table = 0;
  table[player[player_id].sit] = 0;
}

void close_join() {
  in_join = 0;
  send_json(table_sockfd, MSG_LEAVE, NULL);
  close(table_sockfd);
  FD_CLR(table_sockfd, &afds);
  /*
    shutdown(table_sockfd,2);
  */
}

void close_serv() {
  in_serv = 0;
  for (int i = 2; i < MAX_PLAYER; i++)  // Note that i start from 2
  {
    if (player[i].in_table) {
      send_json(player[i].sockfd, MSG_LEADER_LEAVE, NULL);
      close_client(i);
      /*
            shutdown(player[i].sockfd,2);
      */
    }
  }
  FD_CLR(serv_sockfd, &afds);
}

int leave() /* the ^C trap. */
{
  send_json(gps_sockfd, MSG_LEAVE, NULL);
  /*
    shutdown(gps_sockfd,2);
  */
  close(gps_sockfd);
  if (in_join) close_join();
  if (in_serv) close_serv();
  endwin();
  exit(0);
}
