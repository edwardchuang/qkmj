#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "mjdef.h"

#include <cjson/cJSON.h>

#include "ai_client.h"
#include "protocol.h"
#include "protocol_def.h"
#include "qkmj.h"

/* Prototypes for functions in this file */
void init_check_mode();
void process_make(int sit, int card);
void process_epk(int check);
void draw_epk(int id, int kind, int card1, int card2, int card3);
void draw_flower(int sit, int card);

void init_check_mode() {
  if (input_mode == TALK_MODE) {
    current_mode = CHECK_MODE;
  } else {
    input_mode = CHECK_MODE;
  }

  if (ai_is_enabled()) {
    ai_decision_t dec =
        ai_get_decision(AI_PHASE_CLAIM, current_card, card_owner);
    int ai_check_id = 0;
    if (dec.action == AI_ACTION_EAT)
      ai_check_id = EAT;
    else if (dec.action == AI_ACTION_PONG)
      ai_check_id = PONG;
    else if (dec.action == AI_ACTION_KANG)
      ai_check_id = KANG;
    else if (dec.action == AI_ACTION_WIN)
      ai_check_id = MAKE;

    if (ai_check_id != 0 && check_flag[my_sit][ai_check_id]) {
      if (ai_check_id == EAT) {
        int card1 = dec.meld_cards[0];
        int card2 = dec.meld_cards[1];
        int type = 0;
        if ((card1 == current_card + 1 || card2 == current_card + 1) &&
            (card1 == current_card + 2 || card2 == current_card + 2))
          type = 7;
        else if ((card1 == current_card - 1 || card2 == current_card - 1) &&
                 (card1 == current_card + 1 || card2 == current_card + 1))
          type = 8;
        else if ((card1 == current_card - 2 || card2 == current_card - 2) &&
                 (card1 == current_card - 1 || card2 == current_card - 1))
          type = 9;

        if (type != 0) {
          write_check(type);
          input_mode = PLAY_MODE;
          return;
        }
      } else {
        write_check(ai_check_id);
        input_mode = PLAY_MODE;
        return;
      }
    } else {
      write_check(0);
      input_mode = PLAY_MODE;
      return;
    }
  }

  current_check = 0;
  check_x = org_check_x;
  attron(A_REVERSE);
  wmvaddstr(stdscr, org_check_y + 1, org_check_x, "  ");
  wrefresh(stdscr);
  wmvaddstr(stdscr, org_check_y + 1, org_check_x, check_name[0]);
  reset_cursor();
  for (int i = 1; i < check_number; i++) {
    if (check_flag[my_sit][i]) {
      wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, "  ");
      wrefresh(stdscr);
      wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, check_name[i]);
      reset_cursor();
    }
  }
  attroff(A_REVERSE);
  beep1();
  wrefresh(stdscr);
  return_cursor();
}

void process_make(int sit, int card) {
  char msg_buf[80];
  long change_money[5];
  cJSON *root, *cards, *player_node, *card_array, *out_card_array,
      *out_card_group, *moneys_array, *money_node;
  cJSON* payload;
  int sendlog = 1;
  char tai_buf[1024];
  tai_buf[0] = 0;

  play_mode = WAIT_CARD;
  for (int i = 0; i <= 4; i++) {
    change_money[i] = 0;
  }
  current_card = card;
  check_make(sit, card, 1);
  set_color(37, 40);
  clear_screen_area(THROW_Y, THROW_X, 8, 34);
  int max_index = 0;
  int max_sum = card_comb[0].tai_sum;
  for (int i = 0; i < comb_num; i++) {
    if (card_comb[i].tai_sum > max_sum) {
      max_index = i;
      max_sum = card_comb[i].tai_sum;
    }
  }
  set_color(32, 40);
  set_mode(1);
  wmove(stdscr, THROW_Y, THROW_X);
  wprintw(stdscr, "%s ", player[table[sit]].name);

  if (sit == card_owner) {
    wprintw(stdscr, "自摸");
  } else {
    wprintw(stdscr, "胡牌");
    wmove(stdscr, THROW_Y, THROW_X + 19);
    wprintw(stdscr, "%s ", player[table[card_owner]].name);
    wprintw(stdscr, "放槍");
  }
  set_color(33, 40);
  int j = 1;
  int k = 0;
  for (int i = 0; i <= 51; i++) {
    if (card_comb[max_index].tai_score[i]) {
      wmove(stdscr, THROW_Y + j, THROW_X + k);
      wprintw(stdscr, "%-10s%2d 台", tai[i].name,
              card_comb[max_index].tai_score[i]);
      /* Record tai for log */
      if (in_serv && sendlog == 1) {
        char tmp_tai[100];
        snprintf(tmp_tai, sizeof(tmp_tai), "%s::%d;;", tai[i].name,
                 card_comb[max_index].tai_score[i]);
        strncat(tai_buf, tmp_tai, sizeof(tai_buf) - strlen(tai_buf) - 1);
      }
      j++;
      if (j == 7) {
        j = 2;
        k = 18;
      }
    }
  }

  if (card_comb[max_index].tai_score[52]) /* 連莊 */
  {
    wmove(stdscr, THROW_Y + j, THROW_X + k);
    if (info.cont_dealer < 10)
      wprintw(stdscr, "連%s拉%s  %2d 台", number_item[info.cont_dealer],
              number_item[info.cont_dealer], info.cont_dealer * 2);
    else
      wprintw(stdscr, "連%2d拉%2d  %2d 台", info.cont_dealer, info.cont_dealer,
              info.cont_dealer * 2);
  }
  set_color(31, 40);
  wmove(stdscr, THROW_Y + 6, THROW_X + 26);
  wprintw(stdscr, "共 %2d 台", card_comb[max_index].tai_sum);

  if ((sit == card_owner && sit != info.dealer) ||
      (sit != card_owner && card_owner == info.dealer)) {
    if (info.cont_dealer > 0) {
      wmove(stdscr, THROW_Y + 7, THROW_X + 15);
      if (info.cont_dealer < 10)
        wprintw(stdscr, "莊家 連%s拉%s %2d 台", number_item[info.cont_dealer],
                number_item[info.cont_dealer],
                card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2);
      else
        wprintw(stdscr, "莊家 連%2d拉%2d %2d 台", info.cont_dealer,
                info.cont_dealer,
                card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2);
    } else {
      wmove(stdscr, THROW_Y + 7, THROW_X + 24);
      wprintw(stdscr, "莊家 %2d 台", card_comb[max_index].tai_sum + 1);
    }
  }
  wrefresh(stdscr);
  set_color(37, 40);
  set_mode(0);
  for (int i = 1; i <= 4; i++) {
    if (table[i] && i != my_sit) {
      show_allcard(i);
      show_kang(i);
    }
  }
  show_newcard(sit, 4);
  return_cursor();

  /* Process money */
  if (sit == card_owner) /* 自摸 */
  {
    for (int i = 1; i <= 4; i++) {
      if (i != sit) {
        if (i == info.dealer) {
          change_money[i] = -(info.base_value + (card_comb[max_index].tai_sum +
                                                 1 + info.cont_dealer * 2) *
                                                    info.tai_value);
        } else {
          change_money[i] = -(info.base_value +
                              card_comb[max_index].tai_sum * info.tai_value);
        }
        change_money[sit] += -change_money[i];
      }
    }
  } else /* 別人放槍 */
  {
    if (card_owner == info.dealer) {
      change_money[card_owner] =
          -(info.base_value +
            (card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2) *
                info.tai_value);
    } else {
      change_money[card_owner] =
          -(info.base_value + card_comb[max_index].tai_sum * info.tai_value);
    }
    change_money[sit] += -change_money[card_owner];
  }

  /* Generate JSON Log */
  if (in_serv && sendlog == 1) {
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "card_owner", player[table[card_owner]].name);
    cJSON_AddStringToObject(root, "winer", player[table[sit]].name);

    cards = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "cards", cards);

    for (int sitInd = 1; sitInd <= 4; ++sitInd) {
      player_node = cJSON_CreateObject();
      cJSON_AddItemToObject(cards, player[table[sitInd]].name, player_node);

      snprintf(msg_buf, sizeof(msg_buf), "%d", sitInd);
      cJSON_AddStringToObject(player_node, "ind",
                              msg_buf);  // Keep string to match original

      // Card array
      card_array = cJSON_CreateArray();
      for (int i = 0; i < pool[sitInd].num; i++) {
        cJSON_AddItemToArray(card_array,
                             cJSON_CreateNumber(pool[sitInd].card[i]));
      }
      cJSON_AddItemToObject(player_node, "card", card_array);

      // Out card array
      out_card_array = cJSON_CreateArray();
      for (int i = 0; i < pool[sitInd].out_card_index; i++) {
        out_card_group = cJSON_CreateArray();
        for (int m = 1; m < 6; m++) {
          cJSON_AddItemToArray(out_card_group,
                               cJSON_CreateNumber(pool[sitInd].out_card[i][m]));
        }
        cJSON_AddItemToArray(out_card_array, out_card_group);
      }
      cJSON_AddItemToObject(player_node, "out_card", out_card_array);
    }

    cJSON_AddStringToObject(root, "tais", tai_buf);
    cJSON_AddNumberToObject(root, "cont_win", info.cont_dealer);
    cJSON_AddNumberToObject(root, "cont_tai", info.cont_dealer * 2);
    cJSON_AddNumberToObject(root, "count_win",
                            info.cont_dealer);  // Original had both?
    cJSON_AddNumberToObject(root, "count_tai", info.cont_dealer * 2);

    if ((sit == card_owner && sit != info.dealer) ||
        (sit != card_owner && card_owner == info.dealer)) {
      cJSON_AddNumberToObject(root, "is_dealer", 1);
      cJSON_AddStringToObject(root, "dealer", player[table[info.dealer]].name);
    }

    cJSON_AddNumberToObject(root, "base_value", info.base_value);
    cJSON_AddNumberToObject(root, "tai_value", info.tai_value);

    moneys_array = cJSON_CreateArray();
    for (int i = 1; i <= 4; i++) {
      if (table[i]) {
        money_node = cJSON_CreateObject();
        cJSON_AddStringToObject(money_node, "name", player[table[i]].name);
        cJSON_AddNumberToObject(money_node, "now_money",
                                player[table[i]].money);
        cJSON_AddNumberToObject(money_node, "change_money", change_money[i]);
        cJSON_AddItemToArray(moneys_array, money_node);
      }
    }
    cJSON_AddItemToObject(root, "moneys", moneys_array);
    cJSON_AddNumberToObject(root, "time", (double)time(NULL) * 1000.0);
    if (current_match_id[0] != '\0') {
      cJSON_AddStringToObject(root, "match_id", current_match_id);
    }

    send_json(gps_sockfd, MSG_GAME_RECORD, root);
    /* Do not cJSON_Delete(root) as send_json takes ownership */
  }

  /* Send money info to GPS (Update money) */
  if (in_serv) {
    for (int i = 1; i <= 4; i++) {
      if (table[i]) {
        payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(payload, "winner_id", player[table[i]].id);
        cJSON_AddNumberToObject(payload, "money",
                                player[table[i]].money + change_money[i]);
        send_json(gps_sockfd, MSG_WIN_GAME, payload);
      }
    }
  }

  wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
  set_color(37, 40);
  clear_screen_area(THROW_Y, THROW_X, 8, 34);
  attron(A_BOLD);
  wmvaddstr(stdscr, THROW_Y + 1, THROW_X + 12, "金 額 統 計");
  for (int i = 1; i <= 4; i++) {
    wmove(stdscr, THROW_Y + 2 + i, THROW_X);
    wprintw(stdscr, "%s家：%7ld %c %7ld = %7ld", sit_name[i],
            player[table[i]].money, (change_money[i] < 0) ? '-' : '+',
            (change_money[i] < 0) ? -change_money[i] : change_money[i],
            player[table[i]].money + change_money[i]);
    player[table[i]].money += change_money[i];
  }
  my_money = player[my_id].money;
  if (in_serv) waiting = 1;
  return_cursor();
  attroff(A_BOLD);
  if (sit != info.dealer) {
    info.dealer++;
    info.cont_dealer = 0;
    if (info.dealer == 5) {
      info.dealer = 1;
      info.wind++;
      if (info.wind == 5) info.wind = 1;
    }
  } else {
    info.cont_dealer++;
  }
  wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
  if (in_serv) {
    wait_hit[my_sit] = 1;
  } else {
    send_json(table_sockfd, MSG_WAIT_HIT, NULL);
  }
}

void process_epk(int check) {
  char card1, card2, card3;
  cJSON *payload, *cards;

  switch (check) {
    case 2:
      play_mode = THROW_CARD;
      for (int i = 0; i < pool[my_sit].num; i++) {
        if (pool[my_sit].card[i] == current_card) {
          pool[my_sit].card[i] = pool[my_sit].card[pool[my_sit].num - 1];
          pool[my_sit].card[i + 1] = pool[my_sit].card[pool[my_sit].num - 2];
          break;
        }
      }
      card1 = card2 = card3 = (char)current_card;
      draw_epk(my_id, check, card1, card2, card3);
      pool[my_sit].num -= 3;
      sort_card(1);
      break;
    case 3:
    case 11:
      for (int i = 0; i < pool[my_sit].num; i++) {
        if (pool[my_sit].card[i] == current_card) {
          pool[my_sit].card[i] = pool[my_sit].card[pool[my_sit].num - 1];
          pool[my_sit].card[i + 1] = pool[my_sit].card[pool[my_sit].num - 2];
          pool[my_sit].card[i + 2] = pool[my_sit].card[pool[my_sit].num - 3];
        }
      }
      card1 = (char)current_card;
      card2 = (char)current_card;
      card3 = (char)current_card;
      draw_epk(my_id, check, card1, card2, card3);
      pool[my_sit].num -= 3;
      sort_card(0);
      play_mode = GET_CARD;
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      break;
    case 12: {
      int i;
      for (i = 0; i < pool[my_sit].out_card_index; i++) {
        if (pool[my_sit].out_card[i][1] == current_card ||
            pool[my_sit].out_card[i][2] == current_card)
          break;
      }
      pool[my_sit].out_card[i][0] = 12;
      if (i == pool[my_sit].out_card_index) break;
      card1 = card2 = card3 = (char)current_card;
      draw_epk(my_id, check, card1, card2, card3);
      play_mode = GET_CARD;
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      break;
    }
    case 7:
      play_mode = THROW_CARD;
      pool[my_sit].card[search_card(my_sit, current_card + 1)] =
          pool[my_sit].card[pool[my_sit].num - 1];
      pool[my_sit].card[search_card(my_sit, current_card + 2)] =
          pool[my_sit].card[pool[my_sit].num - 2];
      card1 = (char)(current_card + 1);
      card2 = (char)current_card;
      card3 = (char)(current_card + 2);
      draw_epk(my_id, check, card1, card2, card3);
      pool[my_sit].num -= 3;
      sort_card(1);
      break;
    case 8:
      play_mode = THROW_CARD;
      pool[my_sit].card[search_card(my_sit, current_card - 1)] =
          pool[my_sit].card[pool[my_sit].num - 1];
      pool[my_sit].card[search_card(my_sit, current_card + 1)] =
          pool[my_sit].card[pool[my_sit].num - 2];
      card1 = (char)(current_card - 1);
      card2 = (char)current_card;
      card3 = (char)(current_card + 1);
      draw_epk(my_id, check, card1, card2, card3);
      pool[my_sit].num -= 3;
      sort_card(1);
      break;
    case 9:
      play_mode = THROW_CARD;
      pool[my_sit].card[search_card(my_sit, current_card - 1)] =
          pool[my_sit].card[pool[my_sit].num - 1];
      pool[my_sit].card[search_card(my_sit, current_card - 2)] =
          pool[my_sit].card[pool[my_sit].num - 2];
      card1 = (char)(current_card - 2);
      card2 = (char)current_card;
      card3 = (char)(current_card - 1);
      draw_epk(my_id, check, card1, card2, card3);
      pool[my_sit].num -= 3;
      sort_card(1);
      break;
  }
  if (check != 12) {
    pool[my_sit].out_card[pool[my_sit].out_card_index][0] = (char)check;
    pool[my_sit].out_card[pool[my_sit].out_card_index][1] = card1;
    pool[my_sit].out_card[pool[my_sit].out_card_index][2] = card2;
    pool[my_sit].out_card[pool[my_sit].out_card_index][3] = card3;
    if (check == 3 || check == 11) {
      pool[my_sit].out_card[pool[my_sit].out_card_index][4] = card3;
      pool[my_sit].out_card[pool[my_sit].out_card_index][5] = 0;
    } else {
      pool[my_sit].out_card[pool[my_sit].out_card_index][4] = 0;
    }
    pool[my_sit].out_card_index++;
  }

  /* Construct JSON payload for 530 MSG_SHOW_EPK */
  payload = cJSON_CreateObject();
  cJSON_AddNumberToObject(
      payload, "sit",
      my_id); /* "sit" field, but sending my_id (player index) */
  cJSON_AddNumberToObject(payload, "type", check);
  cards = cJSON_CreateArray();
  cJSON_AddItemToArray(cards, cJSON_CreateNumber(card1));
  cJSON_AddItemToArray(cards, cJSON_CreateNumber(card2));
  cJSON_AddItemToArray(cards, cJSON_CreateNumber(card3));
  cJSON_AddItemToObject(payload, "cards", cards);

  turn = my_sit;
  card_owner = my_sit;
  if (in_serv) {
    gettimeofday(&before, (struct timezone*)0);
    /* Broadcast to all (except me) */
    for (int i = 2; i < MAX_PLAYER; i++) {
      if (player[i].in_table && i != 1) { /* 1 is server (me) */
        send_json(player[i].sockfd, MSG_SHOW_EPK, cJSON_Duplicate(payload, 1));
      }
    }
    cJSON_Delete(payload);
  } else {
    send_json(table_sockfd, MSG_SHOW_EPK, payload);
  }

  current_item = pool[my_sit].num;
  pos_x = INDEX_X + 16 * 2 + 1;
  draw_index(pool[my_sit].num + 1);
  display_point(my_sit);
  show_num(2, 70, 144 - card_point - 16, 2);
  return_cursor();
}

/*  Draw eat,pong or kang */
void draw_epk(int id, int kind, int card1, int card2, int card3) {
  int sit = player[id].sit;
  beep1();

  if (kind == KANG || kind == 11 || kind == 12) in_kang = 1;
  if (kind != 11 && kind != 12) throw_card(20);
  if (kind == 12) {
    int i;
    for (i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][1] == card1 &&
          pool[sit].out_card[i][2] == card2)
        break;
    }
    attron(A_REVERSE);
    switch ((sit - my_sit + 4) % 4) {
      case 0:
        wmvaddstr(stdscr, INDEX_Y, INDEX_X + i * 6, "槓");
        break;
      case 1:
        wmvaddstr(stdscr, INDEX_Y1 - i * 3 - 1, INDEX_X1 - 2, "槓");
        break;
      case 2:
        wmvaddstr(stdscr, INDEX_Y2 + 2, INDEX_X2 - i * 6 - 2, "槓");
        break;
      case 3:
        wmvaddstr(stdscr, INDEX_Y3 + i * 3 + 1, INDEX_X3 + 4, "槓");
        break;
    }
    attroff(A_REVERSE);
    return_cursor();
  } else {
    switch ((sit - my_sit + 4) % 4) {
      case 0:
        show_card(20, INDEX_X + (16 - pool[my_sit].num) * 2 + 4, INDEX_Y + 1,
                  1);
        wrefresh(stdscr);
        wmvaddstr(stdscr, INDEX_Y, INDEX_X + (16 - pool[my_sit].num) * 2 - 2,
                  "┌");
        if (kind == KANG || kind == 11) {
          attron(A_REVERSE);
          wmvaddstr(stdscr, INDEX_Y, INDEX_X + (16 - pool[my_sit].num) * 2,
                    "槓");
          attroff(A_REVERSE);
        } else {
          wmvaddstr(stdscr, INDEX_Y, INDEX_X + (16 - pool[my_sit].num) * 2,
                    "─");
        }
        wmvaddstr(stdscr, INDEX_Y, INDEX_X + (16 - pool[my_sit].num) * 2 + 2,
                  "┐  ");
        wrefresh(stdscr);
        if (kind == 11) card1 = card3 = 30;
        show_card(card1, INDEX_X + (16 - pool[my_sit].num) * 2 - 2, INDEX_Y + 1,
                  1);
        show_card(card2, INDEX_X + (16 - pool[my_sit].num) * 2, INDEX_Y + 1, 1);
        show_card(card3, INDEX_X + (16 - pool[my_sit].num) * 2 + 2, INDEX_Y + 1,
                  1);
        break;
      case 1:
        wmvaddstr(stdscr, INDEX_Y1 - (16 - pool[sit].num) - 2, INDEX_X1 - 2,
                  "┌");
        if (kind == KANG || kind == 11) {
          attron(A_REVERSE);
          wmvaddstr(stdscr, INDEX_Y1 - (16 - pool[sit].num) - 1, INDEX_X1 - 2,
                    "槓");
          attroff(A_REVERSE);
        } else {
          wmvaddstr(stdscr, INDEX_Y1 - (16 - pool[sit].num) - 1, INDEX_X1 - 2,
                    "│");
        }
        wmvaddstr(stdscr, INDEX_Y1 - (16 - pool[sit].num), INDEX_X1 - 2, "└");
        wrefresh(stdscr);
        if (kind == 11) card1 = card2 = card3 = 40;
        show_card(card1, INDEX_X1, INDEX_Y1 - (16 - pool[sit].num), 0);
        show_card(card2, INDEX_X1, INDEX_Y1 - (16 - pool[sit].num) - 1, 0);
        show_card(card3, INDEX_X1, INDEX_Y1 - (16 - pool[sit].num) - 2, 0);
        break;
      case 2:
        wmvaddstr(stdscr, INDEX_Y2 + 2, INDEX_X2 - (16 - pool[sit].num) * 2 - 2,
                  "└");
        if (kind == KANG || kind == 11) {
          attron(A_REVERSE);
          wmvaddstr(stdscr, INDEX_Y2 + 2, INDEX_X2 - (16 - pool[sit].num) * 2,
                    "槓");
          attroff(A_REVERSE);
        } else {
          wmvaddstr(stdscr, INDEX_Y2 + 2, INDEX_X2 - (16 - pool[sit].num) * 2,
                    "─");
        }
        if (kind == 11) card1 = card2 = card3 = 30;
        wmvaddstr(stdscr, INDEX_Y2 + 2, INDEX_X2 - (16 - pool[sit].num) * 2 + 2,
                  "┘");
        wrefresh(stdscr);
        show_card(20, INDEX_X2 - (16 - pool[sit].num) * 2 - 4, INDEX_Y2, 1);
        show_card(card1, INDEX_X2 - (16 - pool[sit].num) * 2 + 2, INDEX_Y2, 1);
        show_card(card2, INDEX_X2 - (16 - pool[sit].num) * 2, INDEX_Y2, 1);
        show_card(card3, INDEX_X2 - (16 - pool[sit].num) * 2 - 2, INDEX_Y2, 1);
        break;
      case 3:
        wmvaddstr(stdscr, INDEX_Y3 + (16 - pool[sit].num), INDEX_X3 + 4, "┐");
        if (kind == KANG || kind == 11) {
          attron(A_REVERSE);
          wmvaddstr(stdscr, INDEX_Y3 + (16 - pool[sit].num) + 1, INDEX_X3 + 4,
                    "槓");
          attroff(A_REVERSE);
        } else {
          wmvaddstr(stdscr, INDEX_Y3 + (16 - pool[sit].num) + 1, INDEX_X3 + 4,
                    "│");
        }
        if (kind == 11) card1 = card2 = card3 = 40;
        wmvaddstr(stdscr, INDEX_Y3 + (16 - pool[sit].num) + 2, INDEX_X3 + 4,
                  "┘");
        wrefresh(stdscr);
        show_card(card1, INDEX_X3, INDEX_Y3 + (16 - pool[sit].num), 0);
        show_card(card2, INDEX_X3, INDEX_Y3 + (16 - pool[sit].num) + 1, 0);
        show_card(card3, INDEX_X3, INDEX_Y3 + (16 - pool[sit].num) + 2, 0);
        break;
    }
  }
}

void draw_flower(int sit, int card) {
  char msg_buf[80];

  attron(A_BOLD);
  in_kang = 1;
  pool[sit].flower[card - 51] = 1;
  strncpy(msg_buf, mj_item[card], sizeof(msg_buf) - 1);
  msg_buf[sizeof(msg_buf) - 1] = '\0';
  msg_buf[3] = 0;
  reset_cursor();
  switch ((sit - my_sit + 4) % 4) {
    case 0:
      wmvaddstr(stdscr, FLOWER_Y, FLOWER_X + (card - 51) * 2, msg_buf);
      break;
    case 1:
      wmvaddstr(stdscr, FLOWER_Y1 - (card - 51), FLOWER_X1, msg_buf);
      break;
    case 2:
      wmvaddstr(stdscr, FLOWER_Y2, FLOWER_X2 - (card - 51) * 2, msg_buf);
      break;
    case 3:
      wmvaddstr(stdscr, FLOWER_Y3 + (card - 51), FLOWER_X3, msg_buf);
      break;
  }
  return_cursor();
  attroff(A_BOLD);
}
