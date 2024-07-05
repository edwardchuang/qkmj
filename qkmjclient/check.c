
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncurses.h>

#include "qkmj.h"

void clear_check_flag(char sit)
{
  check_flag[sit][1] = 0;
  check_flag[sit][2] = 0;
  check_flag[sit][3] = 0;
  check_flag[sit][4] = 0;
  check_flag[sit][5] = 0;
}

int search_card(char sit, char card) {
  // 尋找牌
  if (card == 0) {
    return 0;
  }
  for (int i = 0; i < pool[sit].num; i++) {
    if (pool[sit].card[i] == card) {
      return i;
    }
  }
  return -1;
}

int check_kang(char sit, char card) {
  // 尋找牌
  int kang = 1;
  if (card == 0) {
    return 0;
  }
  // 檢查手牌
  for (int i = 0; i < pool[sit].num - 2; i++) {
    if (pool[sit].card[i] == card) {
      // 檢查是否為槓
      if (pool[sit].card[i + 1] != card || pool[sit].card[i + 2] != card) {
        kang = 0;
      }
      return kang;
    }
  }
  // 檢查出牌
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 2 && pool[sit].out_card[i][1] == card &&
        sit == card_owner) {
      return 2;
    }
  }
  return 0;
}

int check_pong(char sit, char card) {
  // 尋找牌
  if (card == 0) {
    return 0;
  }
  for (int i = 0; i < pool[sit].num - 1; i++) {
    if (pool[sit].card[i] == card && pool[sit].card[i + 1] == card) {
      return 1;
    }
  }
  return 0;
}

int check_eat(char sit, char card) {
  // 尋找牌
  int eat = 0;
  if (sit != next_turn(turn)) {
    return 0;
  }
  if (card < 1 || card > 29) {
    return 0;
  }
  int j = card % 10;
  switch (j) {
    case 1:
      if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0) {
        eat = 1;
      }
      break;
    case 2:
      if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0) {
        eat = 1;
      }
      if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0) {
        eat = 1;
      }
      break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0) {
        eat = 1;
      }
      if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0) {
        eat = 1;
      }
      if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0) {
        eat = 1;
      }
      break;
    case 8:
      if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0) {
        eat = 1;
      }
      if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0) {
        eat = 1;
      }
      break;
    case 9:
      if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0) {
        eat = 1;
      }
      break;
    default:
      break;
  }
  return eat;
}

void check_card(char sit, char card)
{
  clear_check_flag(sit);
  check_flag[sit][1]=check_eat(sit,card);
  check_flag[sit][2]=check_pong(sit,card);
  check_flag[sit][3]=check_kang(sit,card);
  check_flag[sit][4]=check_make(sit,card,0);
}

int check_begin_flower(char sit, char card, char position) {
  // 伺服器指令
  char msg_buf[80];
  if (card <= 58 && card >= 51) {
    // 廣播花牌
    snprintf(msg_buf, sizeof(msg_buf), "525%c%c", sit, pool[sit].card[position]);
    broadcast_msg(1, msg_buf);
    draw_flower(sit, card);
    // 取得下一張牌
    card = mj[card_point++];
    if (sit == my_sit) {
      // 更換牌
      change_card(position, card);
    } else {
      // 更新牌
      pool[sit].card[position] = card;
      snprintf(msg_buf, sizeof(msg_buf), "301%c%c", position, card);
      write_msg(player[table[sit]].sockfd, msg_buf);
    }
    return 1;
  }
  return 0;
}

int check_flower(char sit, char card) {
  // 檢查花牌
  char msg_buf[80];
  if (card <= 58 && card >= 51) {
    // 廣播花牌
    snprintf(msg_buf, sizeof(msg_buf), "525%c%c", sit, card);
    draw_flower(sit, card);
    if (in_join) {
      write_msg(table_sockfd, msg_buf);
    } else {
      broadcast_msg(1, msg_buf);
      // 取得下一張牌
      card = mj[card_point++];
      show_num(2, 70, 144 - card_point - 16, 2);
      card_owner = my_sit;
      // 廣播玩家資訊
      snprintf(msg_buf, sizeof(msg_buf), "305%c", (char)my_sit);
      broadcast_msg(1, msg_buf);
      snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point);
      broadcast_msg(1, msg_buf);
      play_mode = GET_CARD;
      // 顯示牌
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      return_cursor();
    }
    return 1;
  }
  return 0;
}

void write_check(char check) {
  // 完成檢查
  char msg_buf[80];
  if (in_join) {
    snprintf(msg_buf, sizeof(msg_buf), "510%c", check + '0');
    write_msg(table_sockfd, msg_buf);
  } else {
    in_check[player[1].sit] = 0;
    check_for[player[1].sit] = check;
  }
}

void send_pool_card() {
  // 公布四家的牌
  char msg_buf[80];
  // 0-2: 512 公布四家的牌
  // 3-18: 東家的牌
  // 19-34: 南家的牌
  // 35-50: 西家的牌
  // 51-66: 北家的牌
  // 67: 0
  sprintf(msg_buf, "521");
  for (int j = 0; j < 16; j++) {
    msg_buf[3 + j] = pool[1].card[j];
  }
  for (int j = 0; j < 16; j++) {
    msg_buf[19 + j] = pool[2].card[j];
  }
  for (int j = 0; j < 16; j++) {
    msg_buf[35 + j] = pool[3].card[j];
  }
  for (int j = 0; j < 16; j++) {
    msg_buf[51 + j] = pool[4].card[j];
  }
  msg_buf[67] = 0;
  broadcast_msg(1, msg_buf);
}

void compare_check() {
  // 檢查牌
  int i, j;
  char msg_buf[80];

  next_player_request = 0;
  // 檢查是否有人要胡牌
  i = card_owner;
  for (j = 1; j <= 4; j++) {
    i = next_turn(i);
    if (check_for[i] == MAKE) {
      send_pool_card();
      snprintf(msg_buf, sizeof(msg_buf), "522%c%c", i, current_card);
      broadcast_msg(1, msg_buf);
      process_make(i, current_card);
      return;
    }
  }
  // 檢查是否有人要槓牌
  for (i = 1; i <= 4; i++) {
    if (check_for[i] == KANG) {
      card_owner = i;
      if (table[i] == 1) { // 伺服器要處理槓牌
        if (search_card(i, current_card) >= 0) {
          if (turn == i) { // 自己摸牌
            process_epk(11);
          } else {
            process_epk(KANG);
          }
        } else {
          process_epk(12);
        }
      } else {
        if (search_card(i, current_card) >= 0) {
          if (turn == i) {
            snprintf(msg_buf, sizeof(msg_buf), "520%c", 11);
          } else {
            snprintf(msg_buf, sizeof(msg_buf), "520%c", KANG);
          }
        } else {
          snprintf(msg_buf, sizeof(msg_buf), "520%c", 12);
        }
        write_msg(player[table[i]].sockfd, msg_buf);
        show_newcard(turn, 1);
        return_cursor();
      }
      turn = i;
      snprintf(msg_buf, sizeof(msg_buf), "314%c%c", i, 1);
      broadcast_msg(table[i], msg_buf);
      return;
    }
  }
  // 檢查是否有人要碰牌
  for (i = 1; i <= 4; i++) {
    if (check_for[i] == PONG) {
      card_owner = i;
      if (table[i] == 1) {
        process_epk(PONG);
      } else {
        snprintf(msg_buf, sizeof(msg_buf), "520%c", PONG);
        write_msg(player[table[i]].sockfd, msg_buf);
        show_newcard(i, 2);
        return_cursor();
      }
      turn = i;
      snprintf(msg_buf, sizeof(msg_buf), "314%c%c", i, 2);
      broadcast_msg(table[i], msg_buf);
      return;
    }
  }
  // 檢查是否有人要吃牌
  for (i = 1; i <= 4; i++) {
    if (check_for[i] >= 7 && check_for[i] <= 9) {
      card_owner = i;
      if (table[i] == 1) {
        process_epk(check_for[i]);
      } else {
        snprintf(msg_buf, sizeof(msg_buf), "520%c", check_for[i]);
        write_msg(player[table[i]].sockfd, msg_buf);
        show_newcard(i, 2);
        return_cursor();
      }
      turn = i;
      snprintf(msg_buf, sizeof(msg_buf), "314%c%c", i, 2);
      broadcast_msg(table[i], msg_buf);
      return;
    }
  }
  if (!getting_card) {
    next_player_request = 1;
  }
  // 清除檢查標記
  for (i = 1; i <= 4; i++) {
    check_for[i] = 0;
  }
  getting_card = 0;
}
