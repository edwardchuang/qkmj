#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mjdef.h"

#include "ai_client.h"
#include "misc.h"
#include "protocol.h"
#include "protocol_def.h"
#include "qkmj.h"

/* Helper functions for JSON extraction */
static const char* j_str(cJSON* json, const char* name) {
  cJSON* item = cJSON_GetObjectItem(json, name);
  return item ? cJSON_GetStringValue(item) : "";
}

static int j_int(cJSON* json, const char* name) {
  cJSON* item = cJSON_GetObjectItem(json, name);
  return item ? item->valueint : 0;
}

static double j_double(cJSON* json, const char* name) {
  cJSON* item = cJSON_GetObjectItem(json, name);
  return item ? item->valuedouble : 0.0;
}

/*
 * 處理 GPS 伺服器訊息
 */
void handle_gps_message(int msg_id, cJSON* data) {
  char msg_buf[255];
  char ans_buf[255];
  char ans_buf1[255];
  cJSON* payload = NULL;

  switch (msg_id) {
    case MSG_LOGIN_OK: /* 2 */
      if (my_pass[0] != 0)
        strncpy(ans_buf, (char*)my_pass, sizeof(ans_buf) - 1);
      else {
        ans_buf[0] = 0;
        ask_question("請輸入你的密碼：", ans_buf, 8, 0);
        ans_buf[8] = 0;
      }
      ans_buf[sizeof(ans_buf) - 1] = '\0';

      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "password", ans_buf);
      send_json(gps_sockfd, MSG_CHECK_PASSWORD, payload);

      strncpy((char*)my_pass, ans_buf, sizeof(my_pass) - 1);
      my_pass[sizeof(my_pass) - 1] = '\0';
      break;

    case MSG_WELCOME: /* 3 */
      pass_login = 1;
      input_mode = TALK_MODE;
      display_comment("請打 /HELP 查看簡單指令說明，/Exit 離開");

      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "note", (char*)my_note);
      send_json(gps_sockfd, MSG_SET_NOTE, payload);
      break;

    case MSG_PASSWORD_FAIL: /* 4 */
      pass_count++;
      my_pass[0] = 0;
      if (pass_count == 3) leave();
      do {
        ans_buf[0] = 0;
        ask_question("密碼錯誤! 請重新輸入你的名稱：", ans_buf, 10, 1);
      } while (ans_buf[0] == 0);
      ans_buf[10] = 0;

      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "name", ans_buf);
      send_json(gps_sockfd, MSG_LOGIN, payload);

      strncpy((char*)my_name, ans_buf, sizeof(my_name) - 1);
      my_name[sizeof(my_name) - 1] = '\0';
      break;

    case MSG_NEW_USER: /* 5 */
      ans_buf[0] = 0;
      ask_question("看來你是個新朋友, 你要使用這個名稱嗎？(y/N)：", ans_buf, 1,
                   1);
      if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {
        ans_buf[0] = 0;
        ask_question("請輸入你的密碼：", ans_buf, 8, 0);
        ans_buf1[0] = 0;
        ask_question("請再輸入一次確認：", ans_buf1, 8, 0);
        ans_buf[8] = 0;
        ans_buf1[8] = 0;
        while (1) {
          if (strcmp(ans_buf, ans_buf1) == 0) {
            payload = cJSON_CreateObject();
            cJSON_AddStringToObject(payload, "password", ans_buf);
            send_json(gps_sockfd, MSG_CREATE_ACCOUNT, payload);

            strncpy((char*)my_pass, ans_buf, sizeof(my_pass) - 1);
            my_pass[sizeof(my_pass) - 1] = '\0';
            break;
          } else {
            ans_buf[0] = 0;
            ask_question("兩次密碼不同! 請重新輸入你的密碼：", ans_buf, 8, 0);
            ans_buf1[0] = 0;
            ask_question("請再輸入一次確認：", ans_buf1, 8, 0);
            ans_buf[8] = 0;
            ans_buf1[8] = 0;
          }
        }
      } else {
        do {
          ans_buf[0] = 0;
          ask_question("請重新輸入你的名稱：", ans_buf, 10, 1);
        } while (ans_buf[0] == 0);
        ans_buf[10] = 0;

        payload = cJSON_CreateObject();
        cJSON_AddStringToObject(payload, "name", ans_buf);
        send_json(gps_sockfd, MSG_LOGIN, payload);

        strncpy((char*)my_name, ans_buf, sizeof(my_name) - 1);
        my_name[sizeof(my_name) - 1] = '\0';
      }
      break;

    case MSG_DUPLICATE_LOGIN: /* 6 */
      ans_buf[0] = 0;
      ask_question("重覆進入! 你要殺掉另一個帳號嗎? (y/N)：", ans_buf, 1, 1);
      if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {
        send_json(gps_sockfd, MSG_KILL_DUPLICATE, NULL);
      } else
        leave();
      break;

    case MSG_VERSION_ERROR: /* 10 */
      leave();
      break;

    case MSG_JOIN_INFO: /* 11 */ {
      int status = j_int(data, "status");
      if (status == 0) {
        const char* ip = j_str(data, "ip");
        int port = j_int(data, "port");

        send_gps_line("與該桌連線中...");

        init_socket((char*)ip, port, &table_sockfd);
        FD_SET(table_sockfd, &afds);
        in_join = 1;
      } else if (status == 1) {
        send_gps_line("查無此桌，請重新再試。（可用 /FREE 查詢空桌）");
      } else {
        send_gps_line("無法連線");
      }
    } break;

    case MSG_OPEN_OK: /* 12 */
      init_serv_socket();
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "port", SERV_PORT - 1);
      send_json(gps_sockfd, MSG_OPEN_TABLE, payload);

      my_id = 1;
      in_serv = 1;
      on_seat = 0;
      player_in_table = 1;
      player[1].sit = 1;
      player[1].money = my_money;
      player[1].id = my_gps_id;
      strncpy(player[1].name, (char*)my_name, sizeof(player[1].name) - 1);
      player[1].name[sizeof(player[1].name) - 1] = '\0';
      my_sit = 1;
      for (int i = 0; i <= 4; i++) table[i] = 0;
      table[1] = 1;
      if (player_in_table == PLAYER_NUM) {
        init_playing_screen();
        opening();
        open_deal();
      }
      strncpy(player[1].name, (char*)my_name, sizeof(player[1].name) - 1);
      player[1].name[sizeof(player[1].name) - 1] = '\0';
      player[1].in_table = 1;
      send_gps_line("您已建立新桌，目前人數1人，可使用 /who 查詢本桌清單");
      send_gps_line("如要關桌請輸入 /Leave (/L) 踢除使用者請用 /Kick ");
      send_gps_line("請用 /Note <附註> 設定附註，其他人查詢空桌時將可參考。");
      break;

    case MSG_TEXT_MESSAGE: /* 101 */
      send_gps_line((char*)j_str(data, "text"));
      break;

    /* case 102 (Display News) - Assuming we add MSG_DISPLAY_NEWS */
    case 102:
      display_news(gps_sockfd);
      break;

    case MSG_UPDATE_MONEY: /* 120 */
      new_client_id = (unsigned int)j_int(data, "user_id");
      new_client_money = (long)j_double(data, "money");
      if (!in_serv) {
        my_gps_id = new_client_id;
        my_money = new_client_money;
      }
      break;

    case MSG_LEAVE: /* 200 */
      close(gps_sockfd);
      endwin();
      exit(0);
      break;

    case MSG_JOIN_NOTIFY: /* 211 */
      strncpy(new_client_name, j_str(data, "name"),
              sizeof(new_client_name) - 1);
      new_client_name[sizeof(new_client_name) - 1] = '\0';
      new_client = 1;
      break;

    case MSG_GAME_START_REQ: /* 15 response from GPS */
      strncpy(current_match_id, j_str(data, "match_id"), sizeof(current_match_id) - 1);
      current_match_id[sizeof(current_match_id) - 1] = '\0';
      move_serial = 0;
      
      /* Broadcast match_id to all other players */
      payload = cJSON_CreateObject();
      cJSON_AddStringToObject(payload, "match_id", current_match_id);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table) {
          send_json(player[i].sockfd, MSG_MATCH_ID, cJSON_Duplicate(payload, 1));
        }
      }
      cJSON_Delete(payload);

      /* Start the game */
      init_playing_screen();
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table) {
          send_json(player[i].sockfd, MSG_INIT_SCREEN, NULL);
        }
      }
      opening();
      open_deal();
      break;

    default:
      snprintf(msg_buf, sizeof(msg_buf), "Unknown msg_id=%d", msg_id);
      display_comment(msg_buf);
  }
}

/*
 * 處理其他客戶端訊息 (P2P)
 */
void handle_client_message(int player_id, int msg_id, cJSON* data) {
  char msg_buf[255];
  int i, sit;
  cJSON* payload = NULL;

  switch (msg_id) {
    case MSG_TEXT_MESSAGE: /* 101 */
      send_gps_line((char*)j_str(data, "text"));
      {
        // Re-broadcast
        for (int i = 2; i < MAX_PLAYER; i++) {
          if (player[i].in_table && i != player_id) {
            send_json(player[i].sockfd, msg_id, cJSON_Duplicate(data, 1));
          }
        }
      }
      break;

    case MSG_ACTION_CHAT: /* 102 */
      display_comment((char*)j_str(data, "text"));
      /* Broadcast */
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id) {
          send_json(player[i].sockfd, msg_id, cJSON_Duplicate(data, 1));
        }
      }
      break;

    case MSG_AI_MODE: /* 130 */
      player[player_id].is_ai = j_int(data, "enabled");
      /* Broadcast */
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id) {
          send_json(player[i].sockfd, msg_id, cJSON_Duplicate(data, 1));
        }
      }
      break;

    case MSG_LEAVE: /* 200 */
      close_client(player_id);
      break;

    case MSG_NEW_ROUND: /* 290 */
      /* Broadcast */
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id) {
          send_json(player[i].sockfd, msg_id, cJSON_Duplicate(data, 1));
        }
      }
      opening();
      open_deal();
      break;

    case MSG_REQUEST_CARD: /* 313 */
      send_card_request = 1;
      break;

    case MSG_FINISH: /* 315 */
      break;

    case MSG_THROW_CARD: /* 401 */
      pool[player[player_id].sit].time += thinktime();
      display_time(player[player_id].sit);

      /* Broadcast Time 312 */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", player[player_id].sit);
      cJSON_AddNumberToObject(payload, "time",
                              pool[player[player_id].sit].time);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != 1) {
          send_json(player[i].sockfd, MSG_TIME, cJSON_Duplicate(payload, 1));
        }
      }
      cJSON_Delete(payload);

      pool[player[player_id].sit].first_round = 0;
      in_kang = 0;
      show_newcard(player[player_id].sit, 3);

      /* Broadcast Show New Card 314 */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", player[player_id].sit);
      cJSON_AddNumberToObject(payload, "mode", 3);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id)
          send_json(player[i].sockfd, MSG_SHOW_NEW_CARD,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      int card = j_int(data, "card");

      /* Broadcast Card Thrown 402 */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "user_id", player_id);
      cJSON_AddNumberToObject(payload, "card", card);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id)
          send_json(player[i].sockfd, MSG_CARD_THROWN,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      current_id = player_id;
      current_card = card;
      getting_card = 0;
      show_cardmsg(player[player_id].sit, card);
      throw_card(card);
      return_cursor();
      sit = player[player_id].sit;
      for (int i = 0; i < pool[sit].num; i++)
        if (pool[sit].card[i] == current_card) break;
      pool[sit].card[i] = pool[sit].card[pool[sit].num];
      sort_pool(sit);
      check_on = 1;
      send_card_on = 0;
      next_player_on = 0;

      /* check_card */
      for (int i = 1; i <= 4; i++) {
        if (table[i] > 0 && table[i] != player_id) check_card(i, card);
      }
      for (int i = 1; i <= 4; i++) {
        if (table[i] != 1 && i != turn)
          for (int j = 1; j < check_number; j++) {
            if (check_flag[i][j]) {
              /* Send 501 Check Card */
              payload = cJSON_CreateObject();
              cJSON_AddNumberToObject(payload, "eat", check_flag[i][1]);
              cJSON_AddNumberToObject(payload, "pong", check_flag[i][2]);
              cJSON_AddNumberToObject(payload, "kang", check_flag[i][3]);
              cJSON_AddNumberToObject(payload, "win", check_flag[i][4]);
              send_json(player[table[i]].sockfd, MSG_CHECK_CARD, payload);

              in_check[i] = 1;
              break;
            } else
              in_check[i] = 0;
          }
      }
      for (int j = 1; j < check_number; j++)
        if (check_flag[my_sit][j]) {
          init_check_mode();
          in_check[my_sit] = 1;
          goto in_check_now1;
        }
      in_check[my_sit] = 0;
    in_check_now1:;
      break;

    case MSG_WAIT_HIT: /* 450 */
      wait_hit[player[player_id].sit] = 1;
      break;

    case MSG_CHECK_CARD: /* 501 */
      check_flag[my_sit][1] = j_int(data, "eat");
      check_flag[my_sit][2] = j_int(data, "pong");
      check_flag[my_sit][3] = j_int(data, "kang");
      check_flag[my_sit][4] = j_int(data, "win");
      init_check_mode();
      break;

    case MSG_CHECK_RESULT: /* 510 */
      in_check[player[player_id].sit] = 0;
      check_for[player[player_id].sit] = j_int(data, "action");
      break;

    case MSG_NEXT_REQUEST: /* 515 */
      next_player_request = 0;
      turn = player[player_id].sit;

      /* Broadcast 310 */
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", turn);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id) {
          send_json(player[i].sockfd, MSG_PLAYER_POINTER,
                    cJSON_Duplicate(payload, 1));
        }
      }
      cJSON_Delete(payload);

      display_point(turn);
      return_cursor();
      break;

    case MSG_FLOWER: /* 525 */
      send_card_request = 1;
      draw_flower(j_int(data, "card"), j_int(data, "pos"));
      /* Broadcast */
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id) {
          send_json(player[i].sockfd, msg_id, cJSON_Duplicate(data, 1));
        }
      }
      break;

    case MSG_SHOW_EPK: /* 530 */
      gettimeofday(&before, (struct timezone*)0);
      int sit = j_int(data, "sit");
      int type = j_int(data, "type");
      cJSON* cards = cJSON_GetObjectItem(data, "cards");
      int c[5] = {0};  // c[0] is card1 (buf[5]), c[1] is card2 (buf[6]), c[2]
                       // is card3 (buf[7])
      /* Note: original buf mapping:
         buf[3] -> sit
         buf[4] -> type
         buf[5] -> card1
         buf[6] -> card2
         buf[7] -> card3
      */
      if (cards && cJSON_IsArray(cards)) {
        for (int k = 0; k < 3; k++) {
          cJSON* item = cJSON_GetArrayItem(cards, k);
          if (item) c[k] = item->valueint;
        }
      }

      turn = player[sit].sit;
      card_owner = turn;
      display_point(turn);

      if (type == 12) {  // Flower
        for (i = 0; i < pool[turn].out_card_index; i++)
          if (pool[turn].out_card[i][0] == 2 &&
              pool[turn].out_card[i][1] == c[0]) {
            pool[turn].out_card[i][0] = 12;
            pool[turn].out_card[i][4] = c[0];
            pool[turn].out_card[i][5] = 0;
            break;
          }
      } else {
        for (i = 0; i <= 3; i++)
          pool[turn].out_card[pool[turn].out_card_index][i] =
              0;  // Clear or what?
        // Original: pool[turn].out_card[pool[turn].out_card_index][i] =
        // buf[i+4];
        // buf[4]=type, buf[5]=c1, buf[6]=c2, buf[7]=c3

        pool[turn].out_card[pool[turn].out_card_index][0] = type;
        pool[turn].out_card[pool[turn].out_card_index][1] = c[0];
        pool[turn].out_card[pool[turn].out_card_index][2] = c[1];
        pool[turn].out_card[pool[turn].out_card_index][3] = c[2];

        if (type == 3 || type == 11) /* 槓牌 */
        {
          pool[turn].out_card[pool[turn].out_card_index][4] = c[2];  // buf[7]
          pool[turn].out_card[pool[turn].out_card_index][5] = 0;
        } else
          pool[turn].out_card[pool[turn].out_card_index][4] = 0;
        pool[turn].out_card_index++;
      }

      draw_epk(sit, type, c[0], c[1], c[2]);

      /* Broadcast */
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != player_id) {
          send_json(player[i].sockfd, msg_id, cJSON_Duplicate(data, 1));
        }
      }
      return_cursor();

      /* Logic from original */
      switch (type) {
        case 2:
          for (i = 0; i < pool[turn].num; i++)
            if (pool[turn].card[i] == c[1]) {
              pool[turn].card[i] = pool[turn].card[pool[turn].num - 1];
              pool[turn].card[i + 1] = pool[turn].card[pool[turn].num - 2];
              break;
            }
          pool[turn].num -= 3;
          sort_pool(turn);
          break;
        case 3:
        case 11:
          for (i = 0; i < pool[turn].num; i++)
            if (pool[turn].card[i] == c[1]) {
              pool[turn].card[i] = pool[turn].card[pool[turn].num - 1];
              pool[turn].card[i + 1] = pool[turn].card[pool[turn].num - 2];
              pool[turn].card[i + 2] = pool[turn].card[pool[turn].num - 3];
              break;
            }
          pool[turn].num -= 3;
          sort_pool(turn);
          break;
        case 7:
        case 8:
        case 9:
          pool[turn].card[search_card(turn, c[0])] =
              pool[turn].card[pool[turn].num - 1];
          pool[turn].card[search_card(turn, c[2])] =
              pool[turn].card[pool[turn].num - 2];
          pool[turn].num -= 3;
          sort_pool(turn);
          break;
      }
      break;

    default:
      break;
  }
}

/*
 * 處理牌桌伺服器訊息
 */
void handle_serv_message(int msg_id, cJSON* data) {
  char msg_buf[255];
  int i;
  cJSON* payload = NULL;

  switch (msg_id) {
    case MSG_TEXT_MESSAGE: /* 101 */
      send_gps_line((char*)j_str(data, "text"));
      break;

    case MSG_ACTION_CHAT: /* 102 */
      display_comment((char*)j_str(data, "text"));
      break;

    case MSG_AI_MODE: /* 130 */ {
      int sit = j_int(data, "sit");
      int enabled = j_int(data, "enabled");
      if (table[sit]) {
        int pid = table[sit];
        player[pid].is_ai = enabled;
        if (in_serv) {
          /* Broadcast to self? No, this is FROM_SERV, so I am client. */
        }
      }
    } break;

    case MSG_LEADER_LEAVE: /* 199 */
    case MSG_LEAVE:        /* 200 */
      send_json(gps_sockfd, MSG_LEAVE_TABLE_GPS, NULL);
      close(table_sockfd);
      FD_CLR(table_sockfd, &afds);
      in_join = 0;
      input_mode = TALK_MODE;
      init_global_screen();
      if (msg_id == MSG_LEADER_LEAVE) display_comment("開桌者已離開牌桌");
      send_json(gps_sockfd, MSG_STATUS, NULL);
      break;

    case MSG_NEW_COMER_INFO: /* 201 */ {
      int user_id = j_int(data, "user_id");
      int sit = j_int(data, "sit");
      int count = j_int(data, "count");
      const char* name = j_str(data, "name");

      strncpy(player[user_id].name, name, sizeof(player[user_id].name) - 1);
      player[user_id].name[sizeof(player[user_id].name) - 1] = '\0';
      player[user_id].in_table = 1;
      player[user_id].sit = sit;
      player_in_table = count;

      if (strcmp((char*)my_name, player[user_id].name) == 0) {
        snprintf(msg_buf, sizeof(msg_buf),
                 "您已加入此桌，目前人數 %d ", player_in_table);
      } else {
        snprintf(msg_buf, sizeof(msg_buf),
                 "%s 加入此桌，目前人數 %d ", player[user_id].name,
                 player_in_table);
      }
      send_gps_line(msg_buf);
      if (player[user_id].sit) table[player[user_id].sit] = user_id;
    } break;

    case MSG_UPDATE_MONEY_P2P: /* 202 */ {
      int user_id = j_int(data, "user_id");
      long money = (long)j_double(data, "money");
      player[user_id].id = j_int(data, "id");
      player[user_id].id = (unsigned int)j_int(data, "db_id");
      player[user_id].money = money;
    } break;

    case MSG_OTHER_INFO: /* 203 */ {
      int user_id = j_int(data, "user_id");
      int sit = j_int(data, "sit");
      const char* name = j_str(data, "name");
      strncpy(player[user_id].name, name, sizeof(player[user_id].name) - 1);
      player[user_id].name[sizeof(player[user_id].name) - 1] = '\0';
      player[user_id].sit = sit;
      player[user_id].in_table = 1;
      table[sit] = user_id;
    } break;

    case MSG_TABLE_INFO: /* 204 */ {
      cJSON* tbl = cJSON_GetObjectItem(data, "table");
      if (cJSON_IsArray(tbl)) {
        for (i = 1; i <= 4; i++) {
          cJSON* item = cJSON_GetArrayItem(tbl, i - 1);
          if (item) {
            table[i] = item->valueint;
            if (table[i]) {
              player[table[i]].sit = i;
              on_seat++;
            }
          }
        }
      }
    } break;

    case MSG_MY_INFO: /* 205 */
      my_id = j_int(data, "user_id");
      my_sit = j_int(data, "sit");

      strncpy(player[my_id].name, j_str(data, "name"),
              sizeof(player[my_id].name) - 1);
      player[my_id].name[sizeof(player[my_id].name) - 1] = '\0';

      player[my_id].sit = my_sit;
      player[my_id].in_table = 1;
      player[my_id].id = my_gps_id;
      player[my_id].money = my_money;
      table[my_sit] = my_id;
      break;

    case MSG_PLAYER_LEAVE: /* 206 */ {
      int user_id = j_int(data, "user_id");
      int count = j_int(data, "count");

      if (player_in_table == 4) {
        init_global_screen();
        input_mode = TALK_MODE;
      }
      player_in_table = count;
      snprintf(msg_buf, sizeof(msg_buf),
               "%s 離開此桌，目前人數剩下 %d 人", player[user_id].name,
               player_in_table);
      display_comment(msg_buf);
      player[user_id].in_table = 0;
    } break;

    case MSG_NEW_ROUND: /* 290 */
      opening();
      break;

    case MSG_MATCH_ID: /* 320 */
      strncpy(current_match_id, j_str(data, "match_id"), sizeof(current_match_id) - 1);
      current_match_id[sizeof(current_match_id) - 1] = '\0';
      move_serial = 0;
      if (screen_mode == PLAYING_SCREEN_MODE) {
        display_info();
      }
      break;

    case MSG_INIT_SCREEN: /* 300 */
      if (screen_mode == GLOBAL_SCREEN_MODE) {
        init_playing_screen();
      }
      opening();
      break;

    case MSG_CHANGE_CARD: /* 301 */
      change_card(j_int(data, "pos"), j_int(data, "card"));
      break;

    case MSG_DEAL_CARDS: /* 302 */ {
      cJSON* cards = cJSON_GetObjectItem(data, "cards");
      if (cJSON_IsArray(cards)) {
        for (i = 0; i < 16; i++) {
          cJSON* item = cJSON_GetArrayItem(cards, i);
          if (item) pool[my_sit].card[i] = item->valueint;
        }
        sort_card(0);
      }
    } break;

    case MSG_CAN_GET: /* 303 */
      play_mode = GET_CARD;
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      wrefresh(stdscr);
      beep1();
      return_cursor();
      break;

    case MSG_GET_CARD: /* 304 */
      process_new_card(my_sit, j_int(data, "card"));
      break;

    case MSG_CARD_OWNER: /* 305 */
      card_owner = j_int(data, "sit");
      if (card_owner != my_sit) show_cardmsg(card_owner, 0);
      break;

    case MSG_CARD_POINT: /* 306 */
      card_point = j_int(data, "count");
      show_num(2, 70, 144 - card_point - 16, 2);
      return_cursor();
      break;

    case MSG_SORT_CARD: /* 308 */
      sort_card(j_int(data, "mode"));
      break;

    case MSG_PLAYER_POINTER: /* 310 */
      turn = j_int(data, "sit");
      display_point(turn);
      return_cursor();
      break;

    case MSG_TIME: /* 312 */
      pool[j_int(data, "sit")].time = j_double(data, "time");
      display_time(j_int(data, "sit"));
      break;

    case MSG_SHOW_NEW_CARD: /* 314 */
      show_newcard(j_int(data, "sit"), j_int(data, "mode"));
      return_cursor();
      break;

    case MSG_SEA_BOTTOM: /* 330 */
      for (i = 1; i <= 4; i++) {
        if (table[i] && i != my_sit) {
          show_allcard(i);
          show_kang(i);
        }
      }
      info.cont_dealer++;
      clear_screen_area(THROW_Y, THROW_X, 8, 34);
      wmvaddstr(stdscr, THROW_Y + 3, THROW_X + 12, "海 底 流 局");
      return_cursor();
      wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
      break;

    case MSG_CARD_THROWN: /* 402 */
      in_kang = 0;
      {
        int uid = j_int(data, "user_id");
        int card = j_int(data, "card");

        /* Need to check bounds/validity */
        if (uid < 0 || uid >= MAX_PLAYER) break;

        pool[player[uid].sit].first_round = 0;
        show_cardmsg(player[uid].sit, card);
        current_card = card;
        throw_card(card);
        return_cursor();
        current_id = uid;
      }
      break;

    case MSG_CHECK_CARD: /* 501 */
      go_to_check = 0;
      check_flag[my_sit][1] = j_int(data, "eat");
      check_flag[my_sit][2] = j_int(data, "pong");
      check_flag[my_sit][3] = j_int(data, "kang");
      check_flag[my_sit][4] = j_int(data, "win");

      for (i = 1; i <= 4; i++) {
        if (check_flag[my_sit][i]) go_to_check = 1;
      }

      if (go_to_check) init_check_mode();
      go_to_check = 0;
      break;

    case MSG_WIND_INFO: /* 518 */ {
      cJSON* winds = cJSON_GetObjectItem(data, "winds");
      if (cJSON_IsArray(winds)) {
        for (i = 1; i <= 4; i++) {
          cJSON* item = cJSON_GetArrayItem(winds, i - 1);
          if (item) pool[i].door_wind = item->valueint;
        }
        wmvaddstr(stdscr, 2, 64, sit_name[pool[my_sit].door_wind]);
        return_cursor();
      }
    } break;

    case MSG_EPK: /* 520 */
      process_epk(j_int(data, "card"));
      break;

    case MSG_POOL_INFO: /* 521 */ {
      cJSON* p = cJSON_GetObjectItem(data, "pool");
      if (cJSON_IsArray(p)) {
        for (int k = 0; k < 4; k++) {
          cJSON* hand = cJSON_GetArrayItem(p, k);
          if (cJSON_IsArray(hand)) {
            for (int m = 0; m < pool[k + 1].num; m++) {
              cJSON* c = cJSON_GetArrayItem(hand, m);
              if (c) pool[k + 1].card[m] = c->valueint;
            }
          }
        }
      }
    } break;

    case MSG_MAKE: /* 522 */
      process_make(j_int(data, "user_id"), j_int(data, "type"));
      break;

    case MSG_FLOWER: /* 525 */
      draw_flower(j_int(data, "card"), j_int(data, "pos"));
      break;

    case MSG_SHOW_EPK: /* 530 */ {
      int sit = j_int(data, "sit");
      int type = j_int(data, "type");
      cJSON* cards = cJSON_GetObjectItem(data, "cards");
      int c[5] = {0};
      if (cards) {
        for (int k = 0; k < 3; k++) {
          cJSON* item = cJSON_GetArrayItem(cards, k);
          if (item) c[k] = item->valueint;
        }
      }

      turn = player[sit].sit;
      card_owner = turn;
      display_point(turn);

      if (type == 12) {
        for (i = 0; i < pool[turn].out_card_index; i++)
          if (pool[turn].out_card[i][1] == c[0] &&
              pool[turn].out_card[i][2] == c[1]) {
            pool[turn].out_card[i][0] = 12;
            pool[turn].out_card[i][4] = c[0];
            pool[turn].out_card[i][5] = 0;
            break;
          }
      } else {
        // ... Logic similar to client msg 530 ...
        for (i = 0; i <= 3; i++)
          pool[turn].out_card[pool[turn].out_card_index][i] = 0;
        pool[turn].out_card[pool[turn].out_card_index][0] = type;
        pool[turn].out_card[pool[turn].out_card_index][1] = c[0];
        pool[turn].out_card[pool[turn].out_card_index][2] = c[1];
        pool[turn].out_card[pool[turn].out_card_index][3] = c[2];

        if (type == 3 || type == 11) {
          pool[turn].out_card[pool[turn].out_card_index][4] = c[2];
          pool[turn].out_card[pool[turn].out_card_index][5] = 0;
        } else
          pool[turn].out_card[pool[turn].out_card_index][4] = 0;

        draw_epk(sit, type, c[0], c[1], c[2]);
        pool[turn].out_card_index++;
        pool[turn].num -= 3;
      }
      return_cursor();
    } break;

    default:
      break;
  }
}

/*
 * 處理訊息分發 (Message Dispatcher)
 */
void process_msg(int player_id, int msg_id, cJSON* data, int msg_type) {
  switch (msg_type) {
    case (FROM_GPS):
      handle_gps_message(msg_id, data);
      break;
    case (FROM_CLIENT):
      handle_client_message(player_id, msg_id, data);
      break;
    case (FROM_SERV):
      handle_serv_message(msg_id, data);
      break;
    default:
      break;
  }
}