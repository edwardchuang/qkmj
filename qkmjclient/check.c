#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mjdef.h"
#include "protocol.h"
#include "protocol_def.h"
#include "qkmj.h"

#define THREE_CARD 1
#define STRAIGHT_CARD 2
#define PAIR_CARD 3

void clear_check_flag(int sit) {
  for (int j = 1; j < 6; j++) {
    check_flag[sit][j] = 0;
  }
}

int search_card(int sit, int card) {
  if (card == 0) return 0;
  for (int i = 0; i < pool[sit].num; i++) {
    if (pool[sit].card[i] == card) return i;
  }
  return -1;
}

int check_kang(int sit, int card) {
  if (card == 0) return 0;
  
  // Check for concealed kang (4 cards in hand)
  // Actually logic checks if we have 3 cards in hand, plus the incoming one makes 4?
  // Or if we have 3 in hand matching 'card'.
  // The loop checks pool[sit].card[i] == card.
  // Original logic:
  // for (i = 0; i < pool[sit].num - 2; i++)
  //   if (pool[sit].card[i] == card) {
  //     if (pool[sit].card[i + 1] == card && pool[sit].card[i + 2] == card)
  //       return 1;
  //   }
  // This assumes sorted hand? Yes, likely.
  for (int i = 0; i < pool[sit].num - 2; i++) {
    if (pool[sit].card[i] == card) {
      if (pool[sit].card[i + 1] == card && pool[sit].card[i + 2] == card) {
        return 1;
      }
    }
  }

  // Check for exposed kang (adding to a pong)
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 2 && pool[sit].out_card[i][1] == card &&
        sit == card_owner) {
      return 2;
    }
  }
  return 0;
}

int check_pong(int sit, int card) {
  if (card == 0) return 0;
  for (int i = 0; i < pool[sit].num - 1; i++) {
    if (pool[sit].card[i] == card) {
      if (pool[sit].card[i + 1] == card) return 1;
    }
  }
  return 0;
}

bool check_eat(int sit, int card) {
  bool eat = false;
  int i = next_turn(turn);
  if (i != sit) return false;
  if (card < 1 || card > 29) return false;
  
  int j = card % 10;
  switch (j) {
    case 1:
      if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0)
        eat = true;
      break;
    case 2:
      if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0)
        eat = true;
      if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0)
        eat = true;
      break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0)
        eat = true;
      if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0)
        eat = true;
      if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0)
        eat = true;
      break;
    case 8:
      if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0)
        eat = true;
      if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0)
        eat = true;
      break;
    case 9:
      if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0)
        eat = true;
      break;
    default:
      break;
  }
  return eat;
}

void check_card(int sit, int card) {
  clear_check_flag(sit);
  check_flag[sit][1] = check_eat(sit, card);
  check_flag[sit][2] = check_pong(sit, card);
  check_flag[sit][3] = check_kang(sit, card);
  check_flag[sit][4] = check_make(sit, card, 0);
}

int check_begin_flower(int sit, int card, int position) {
  if (card <= 58 && card >= 51) {
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "card", card);
    cJSON_AddNumberToObject(payload, "pos", position);
    cJSON_AddNumberToObject(payload, "sit", sit);

    for (int i = 2; i < MAX_PLAYER; i++) {
      if (player[i].in_table && i != 1)
        send_json(player[i].sockfd, MSG_FLOWER, cJSON_Duplicate(payload, 1));
    }
    cJSON_Delete(payload);

    draw_flower(sit, card);
    card = mj[card_point++];
    if (sit == my_sit) {
      change_card(position, card);
    } else {
      pool[sit].card[position] = (char)card;
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "pos", position);
      cJSON_AddNumberToObject(payload, "card", card);
      send_json(player[table[sit]].sockfd, MSG_CHANGE_CARD, payload);
    }
    return 1;
  }
  return 0;
}

int check_flower(int sit, int card) {
  if (card <= 58 && card >= 51) {
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "sit", sit);
    cJSON_AddNumberToObject(payload, "card", card);

    draw_flower(sit, card);
    if (in_join) {
      send_json(table_sockfd, MSG_FLOWER, payload);
    } else {
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != 1)
          send_json(player[i].sockfd, MSG_FLOWER, cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      card = mj[card_point++];
      show_num(2, 70, 144 - card_point - 16, 2);
      card_owner = my_sit;

      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", my_sit);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != 1)
          send_json(player[i].sockfd, MSG_CARD_OWNER,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "count", card_point);
      for (int i = 2; i < MAX_PLAYER; i++) {
        if (player[i].in_table && i != 1)
          send_json(player[i].sockfd, MSG_CARD_POINT,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      play_mode = GET_CARD;
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      return_cursor();
    }
    return 1;
  }
  return 0;
}

void send_pool_card() {
  cJSON* payload = cJSON_CreateObject();
  cJSON* p_array = cJSON_CreateArray();
  cJSON_AddItemToObject(payload, "pool", p_array);

  for (int i = 1; i <= 4; i++) {
    cJSON* hand = cJSON_CreateArray();
    for (int j = 0; j < 16; j++) {
      cJSON_AddItemToArray(hand, cJSON_CreateNumber(pool[i].card[j]));
    }
    cJSON_AddItemToArray(p_array, hand);
  }

  for (int i = 2; i < MAX_PLAYER; i++) {
    if (player[i].in_table && i != 1)
      send_json(player[i].sockfd, MSG_POOL_INFO, cJSON_Duplicate(payload, 1));
  }
  cJSON_Delete(payload);
}

void compare_check() {
  cJSON* payload;
  next_player_request = 0;

  // Check for make
  int i = card_owner;
  for (int j = 1; j <= 4; j++) {
    i = next_turn(i);
    if (check_for[i] == MAKE) {
      send_pool_card();
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "user_id", i);
      cJSON_AddNumberToObject(payload, "type", current_card);
      for (int k = 2; k < MAX_PLAYER; k++) {
        if (player[k].in_table && k != 1)
          send_json(player[k].sockfd, MSG_MAKE, cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      process_make(i, current_card);
      goto finish;
    }
  }

  // Check for kang
  for (int i = 1; i <= 4; i++) {
    if (check_for[i] == KANG) {
      card_owner = i;
      if (table[i] == 1) {
        // Server wants to process_epk
        if (search_card(i, current_card) >= 0) {
          if (turn == i)  // Self draw
            process_epk(11);
          else
            process_epk(KANG);
        } else {
          process_epk(12);
        }
      } else {
        if (search_card(i, current_card) >= 0) {
          if (turn == i) {
            payload = cJSON_CreateObject();
            cJSON_AddNumberToObject(payload, "card", 11);
            send_json(player[table[i]].sockfd, MSG_EPK, payload);
          } else {
            payload = cJSON_CreateObject();
            cJSON_AddNumberToObject(payload, "card", KANG);
            send_json(player[table[i]].sockfd, MSG_EPK, payload);
          }
        } else {
          payload = cJSON_CreateObject();
          cJSON_AddNumberToObject(payload, "card", 12);
          send_json(player[table[i]].sockfd, MSG_EPK, payload);
        }
        show_newcard(turn, 1);
        return_cursor();
      }
      turn = i;
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", i);
      cJSON_AddNumberToObject(payload, "mode", 1);
      for (int k = 2; k < MAX_PLAYER; k++) {
        if (player[k].in_table && k != table[i])
          send_json(player[k].sockfd, MSG_SHOW_NEW_CARD,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      goto finish;
    }
  }

  // Check for pong
  for (int i = 1; i <= 4; i++) {
    if (check_for[i] == PONG) {
      card_owner = i;
      if (table[i] == 1)
        process_epk(PONG);
      else {
        payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(payload, "card", PONG);
        send_json(player[table[i]].sockfd, MSG_EPK, payload);
        show_newcard(i, 2);
        return_cursor();
      }
      turn = i;
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", i);
      cJSON_AddNumberToObject(payload, "mode", 2);
      for (int k = 2; k < MAX_PLAYER; k++) {
        if (player[k].in_table && k != table[i])
          send_json(player[k].sockfd, MSG_SHOW_NEW_CARD,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);

      goto finish;
    }
  }

  // Check for eat
  for (int i = 1; i <= 4; i++) {
    if (check_for[i] >= 7 && check_for[i] <= 9) {
      card_owner = i;
      if (table[i] == 1)
        process_epk(check_for[i]);
      else {
        payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(payload, "card", check_for[i]);
        send_json(player[table[i]].sockfd, MSG_EPK, payload);
        show_newcard(i, 2);
        return_cursor();
      }
      turn = i;
      payload = cJSON_CreateObject();
      cJSON_AddNumberToObject(payload, "sit", i);
      cJSON_AddNumberToObject(payload, "mode", 2);
      for (int k = 2; k < MAX_PLAYER; k++) {
        if (player[k].in_table && k != table[i])
          send_json(player[k].sockfd, MSG_SHOW_NEW_CARD,
                    cJSON_Duplicate(payload, 1));
      }
      cJSON_Delete(payload);
      goto finish;
    }
  }
  if (!getting_card) next_player_request = 1;

finish:;
  for (int i = 1; i <= 4; i++) check_for[i] = 0;
  getting_card = 0;
}
