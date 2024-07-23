#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

#include "qkmj.h"

void init_check_mode() {
  // 初始化檢查模式
  int i;

  if (input_mode == TALK_MODE) {
    current_mode = CHECK_MODE;
  } else {
    input_mode = CHECK_MODE;
  }
  current_check = 0;
  check_x = org_check_x;
  attron(A_REVERSE);
  // 顯示檢查選項
  mvaddstr(org_check_y + 1, org_check_x, "  ");
  refresh();
  mvaddstr(org_check_y + 1, org_check_x, check_name[0]);
  reset_cursor();
  for (i = 1; i < check_number; i++) {
    if (check_flag[my_sit][i]) {
      mvaddstr(org_check_y + 1, org_check_x + i * 3, "  ");
      refresh();
      mvaddstr(org_check_y + 1, org_check_x + i * 3, check_name[i]);
      reset_cursor();
    }
  }
  attroff(A_REVERSE);
  beep1();
  refresh();
  return_cursor();
}

void process_make(char sit, char card) {
  // 處理胡牌
  int i, j, k, max_sum, max_index, sitInd;
  char msg_buf[80];
  char result_record_buf[2000] = {0};
  char result_buf[2000];
  long change_money[5] = {0};
  static int sendlog = 1;

  // 設定遊戲模式為等待摸牌
  play_mode = WAIT_CARD;
  current_card = card;
  // 檢查胡牌
  check_make(sit, card, 1);
  // 設定顏色
  set_color(37, 40);
  // 清除畫面區域
  clear_screen_area(THROW_Y, THROW_X, 8, 34);
  // 找出最大台數的胡牌組合
  max_index = 0;
  max_sum = card_comb[0].tai_sum;
  for (i = 0; i < comb_num; i++) {
    if (card_comb[i].tai_sum > max_sum) {
      max_index = i;
      max_sum = card_comb[i].tai_sum;
    }
  }
  // 設定顏色
  set_color(32, 40);
  // 設定模式
  set_mode(1);
  // 移動游標
  wmove(stdscr, THROW_Y, THROW_X);
  // 顯示胡牌玩家姓名
  wprintw(stdscr, "%s ", player[table[sit]].name);

  // 紀錄遊戲結果
  if (in_serv && sendlog == 1) {
    // 紀錄遊戲結果
    snprintf(result_record_buf, sizeof(result_record_buf),
             "900{card_owner:\"%s\",winer:\"%s\",cards:{",
             player[table[card_owner]].name, player[table[sit]].name);
    for (sitInd = 1; sitInd <= 4; ++sitInd) {
      // 紀錄每個玩家的牌
      snprintf(result_buf, sizeof(result_buf),
               "\"%s\":{ind:\"%d\",card:[", player[table[sitInd]].name,
               sitInd);
      // 使用 memmove 複製字串
      memmove(result_record_buf + strlen(result_record_buf), result_buf,
             strlen(result_buf));
      for (i = 0; i < pool[sitInd].num; i++) {
        // 紀錄玩家手牌
        snprintf(result_buf, sizeof(result_buf), "%d,",
                 pool[sitInd].card[i]);
        // 使用 memmove 複製字串
        memmove(result_record_buf + strlen(result_record_buf), result_buf,
               strlen(result_buf));
      }
      // 紀錄玩家出牌
      memmove(result_record_buf + strlen(result_record_buf), "],out_card:[", 11);
      for (i = 0; i < pool[sitInd].out_card_index; i++) {
        // 紀錄每個出牌組
        memmove(result_record_buf + strlen(result_record_buf), "[", 1);
        for (j = 1; j < 6; j++) {
          // 紀錄出牌組的牌
          snprintf(result_buf, sizeof(result_buf), "%d,",
                   pool[sitInd].out_card[i][j]);
          // 使用 memmove 複製字串
          memmove(result_record_buf + strlen(result_record_buf), result_buf,
                 strlen(result_buf));
        }
        // 紀錄出牌組的結尾
        memmove(result_record_buf + strlen(result_record_buf), "]", 1);
      }
      // 紀錄出牌的結尾
      memmove(result_record_buf + strlen(result_record_buf), "]", 1);
      if (sitInd != 4) {
        // 紀錄玩家的結尾
        memmove(result_record_buf + strlen(result_record_buf), "},", 2);
      } else {
        // 紀錄所有玩家的結尾
        memmove(result_record_buf + strlen(result_record_buf), "}", 1);
      }
    }
    // 紀錄台數
    memmove(result_record_buf + strlen(result_record_buf), "},tais:\"", 8);
  }

  // 判斷胡牌類型
  if (sit == card_owner) {
    wprintw(stdscr, "自摸");
  } else {
    wprintw(stdscr, "胡牌");
    wmove(stdscr, THROW_Y, THROW_X + 19);
    wprintw(stdscr, "%s ", player[table[card_owner]].name);
    wprintw(stdscr, "放槍");
  }
  // 設定顏色
  set_color(33, 40);
  j = 1;
  k = 0;
  // 顯示胡牌台數
  for (i = 0; i <= 51; i++) {
    if (card_comb[max_index].tai_score[i]) {
      wmove(stdscr, THROW_Y + j, THROW_X + k);
      wprintw(stdscr, "%-10s%2d 台", tai[i].name,
              card_comb[max_index].tai_score[i]);
      // 紀錄台數
      if (in_serv && sendlog == 1) {
        char buf[3000];
        snprintf(buf, sizeof(buf), "%20s::%d;;", tai[i].name,
                 card_comb[max_index].tai_score[i]);
        // 使用 memmove 複製字串
        memmove(result_record_buf + strlen(result_record_buf), buf,
               strlen(buf));
      }
      j++;
      if (j == 7) {
        j = 2;
        k = 18;
      }
    }
  }
  // 紀錄台數
  memmove(result_record_buf + strlen(result_record_buf), "\",", 2);

  // 顯示連莊台數
  if (card_comb[max_index].tai_score[52]) {
    // 紀錄連莊台數
    if (in_serv && sendlog == 1) {
      snprintf(result_buf, sizeof(result_buf), "cont_win:%d,cont_tai:%d,",
               info.cont_dealer, info.cont_dealer * 2);
      // 使用 memmove 複製字串
      memmove(result_record_buf + strlen(result_record_buf), result_buf,
             strlen(result_buf));
    }
    wmove(stdscr, THROW_Y + j, THROW_X + k);
    if (info.cont_dealer < 10) {
      wprintw(stdscr, "連%s拉%s  %2d 台", number_item[info.cont_dealer],
              number_item[info.cont_dealer], info.cont_dealer * 2);
    } else {
      wprintw(stdscr, "連%2d拉%2d  %2d 台", info.cont_dealer,
              info.cont_dealer, info.cont_dealer * 2);
    }
  }
  // 設定顏色
  set_color(31, 40);
  // 顯示總台數
  wmove(stdscr, THROW_Y + 6, THROW_X + 26);
  wprintw(stdscr, "共 %2d 台", card_comb[max_index].tai_sum);

  // 紀錄總台數
  if (in_serv && sendlog == 1) {
    snprintf(result_buf, sizeof(result_buf), "count_win:%d,count_tai:%d,",
             info.cont_dealer, info.cont_dealer * 2);
    // 使用 memmove 複製字串
    memmove(result_record_buf + strlen(result_record_buf), result_buf,
           strlen(result_buf));
  }
  // 顯示莊家台數
  if ((sit == card_owner && sit != info.dealer) ||
      (sit != card_owner && card_owner == info.dealer)) {
    // 紀錄莊家台數
    if (in_serv && sendlog == 1) {
      snprintf(result_buf, sizeof(result_buf), "is_dealer:1,dealer:\"%s\",",
               player[table[info.dealer]].name);
      // 使用 memmove 複製字串
      memmove(result_record_buf + strlen(result_record_buf), result_buf,
             strlen(result_buf));
    }
    wmove(stdscr, THROW_Y + 7, THROW_X + 15);
    if (info.cont_dealer > 0) {
      if (info.cont_dealer < 10) {
        wprintw(stdscr, "莊家 連%s拉%s %2d 台",
                number_item[info.cont_dealer],
                number_item[info.cont_dealer],
                card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2);
      } else {
        wprintw(stdscr, "莊家 連%2d拉%2d %2d 台", info.cont_dealer,
                info.cont_dealer,
                card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2);
      }
    } else {
      wmove(stdscr, THROW_Y + 7, THROW_X + 24);
      wprintw(stdscr, "莊家 %2d 台", card_comb[max_index].tai_sum + 1);
    }
  }
  wrefresh(stdscr);
  // 設定顏色
  set_color(37, 40);
  // 設定模式
  set_mode(0);
  // 顯示其他玩家的牌
  for (i = 1; i <= 4; i++) {
    if (table[i] && i != my_sit) {
      show_allcard(i);
      show_kang(i);
    }
  }
  // 顯示新摸的牌
  show_newcard(sit, 4);
  return_cursor();

  // 紀錄基本分和台分
  if (in_serv && sendlog == 1) {
    snprintf(result_buf, sizeof(result_buf), "base_value:%d,",
             info.base_value);
    // 使用 memmove 複製字串
    memmove(result_record_buf + strlen(result_record_buf), result_buf,
           strlen(result_buf));
    snprintf(result_buf, sizeof(result_buf), "tai_value:%d,",
             info.tai_value);
    // 使用 memmove 複製字串
    memmove(result_record_buf + strlen(result_record_buf), result_buf,
           strlen(result_buf));
    // 紀錄金錢
    memmove(result_record_buf + strlen(result_record_buf), "moneys:", 7);
  }

  // 計算金錢
  if (sit == card_owner) {
    // 自摸
    for (i = 1; i <= 4; i++) {
      if (i != sit) {
        if (i == info.dealer) {
          change_money[i] =
              -(info.base_value +
                (card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2) *
                info.tai_value);
        } else {
          change_money[i] =
              -(info.base_value + card_comb[max_index].tai_sum * info.tai_value);
        }
        change_money[sit] += -change_money[i];
      }
    }
  } else {
    // 放槍
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

  // 傳送金錢資訊到 GPS
  if (in_serv) {
    // 紀錄金錢資訊
    memmove(result_record_buf + strlen(result_record_buf), "[", 1);
    for (i = 1; i <= 4; i++) {
      if (table[i]) {
        // 紀錄金錢資訊
        if (sendlog == 1) {
          snprintf(result_buf, sizeof(result_buf),
                   "{name:\"%s\",now_money:%ld,change_money:%ld}",
                   player[table[i]].name, player[table[i]].money,
                   change_money[i]);
          // 使用 memmove 複製字串
          memmove(result_record_buf + strlen(result_record_buf), result_buf,
                 strlen(result_buf));
          if (i != 4) {
            // 紀錄金錢資訊的結尾
            memmove(result_record_buf + strlen(result_record_buf), ",", 1);
          }
        }
        // 傳送金錢資訊
        snprintf(msg_buf, sizeof(msg_buf), "020%5d%ld", player[table[i]].id,
                 player[table[i]].money + change_money[i]);
        write_msg(gps_sockfd, msg_buf);
      }
    }
    // 紀錄時間
    if (sendlog == 1) {
      long current_time = 0;
      time(&current_time);
      snprintf(result_buf, sizeof(result_buf), "],time:%ld000}",
               current_time);
      // 使用 memmove 複製字串
      memmove(result_record_buf + strlen(result_record_buf), result_buf,
             strlen(result_buf));
      // 傳送金錢資訊
      write_msg(gps_sockfd, result_record_buf);
    }
  }

  // 等待使用者按下任意鍵
  wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
  // 設定顏色
  set_color(37, 40);
  // 清除畫面區域
  clear_screen_area(THROW_Y, THROW_X, 8, 34);
  // 設定粗體字
  attron(A_BOLD);
  // 顯示金錢統計
  wmvaddstr(stdscr, THROW_Y + 1, THROW_X + 12, "金 額 統 計");
  // 顯示每個玩家的金錢變化
  for (i = 1; i <= 4; i++) {
    wmove(stdscr, THROW_Y + 2 + i, THROW_X);
    wprintw(stdscr, "%s家：%7ld %c %7ld = %7ld", sit_name[i],
            player[table[i]].money, (change_money[i] < 0) ? '-' : '+',
            (change_money[i] < 0) ? -change_money[i] : change_money[i],
            player[table[i]].money + change_money[i]);
    player[table[i]].money += change_money[i];
  }
  // 更新玩家金錢
  my_money = player[my_id].money;
  // 設定等待狀態
  if (in_serv) {
    waiting = 1;
  }
  return_cursor();
  // 關閉粗體字
  attroff(A_BOLD);
  // 更新莊家
  if (sit != info.dealer) {
    info.dealer++;
    info.cont_dealer = 0;
    if (info.dealer == 5) {
      info.dealer = 1;
      info.wind++;
      if (info.wind == 5) {
        info.wind = 1;
      }
    }
  } else {
    info.cont_dealer++;
  }
  // 等待使用者按下任意鍵
  wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);
  // 傳送訊息
  if (in_serv) {
    wait_hit[my_sit] = 1;
  } else {
    write_msg(table_sockfd, "450");
  }
}

void process_epk(char check) {
  // 處理吃碰槓
  char card1, card2, card3;
  int i;
  char msg_buf[80];

  switch (check) {
    case 2:
      // 碰牌
      play_mode = THROW_CARD;
      // 找到要碰的牌
      for (i = 0; i < pool[my_sit].num; i++) {
        if (pool[my_sit].card[i] == current_card) {
          // 將要碰的牌移到牌組的最後面
          memmove(&pool[my_sit].card[i], &pool[my_sit].card[i + 1],
                 (pool[my_sit].num - i - 1) * sizeof(char));
          break;
        }
      }
      // 設定碰牌的牌
      card1 = card2 = card3 = current_card;
      // 顯示碰牌
      draw_epk(my_id, check, card1, card2, card3);
      // 移除碰牌
      pool[my_sit].num -= 3;
      // 排序牌組
      sort_card(1);
      break;
    case 3:
    case 11:
      // 明槓或暗槓
      // 找到要槓的牌
      for (i = 0; i < pool[my_sit].num; i++) {
        if (pool[my_sit].card[i] == current_card) {
          // 將要槓的牌移到牌組的最後面
          memmove(&pool[my_sit].card[i], &pool[my_sit].card[i + 1],
                 (pool[my_sit].num - i - 1) * sizeof(char));
          break;
        }
      }
      // 設定槓牌的牌
      card1 = current_card;
      card2 = current_card;
      card3 = current_card;
      // 顯示槓牌
      draw_epk(my_id, check, card1, card2, card3);
      // 移除槓牌
      pool[my_sit].num -= 3;
      // 排序牌組
      sort_card(0);
      // 設定遊戲模式為摸牌
      play_mode = GET_CARD;
      // 顯示摸牌指示
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      break;
    case 12:
      // 出牌槓
      // 找到要槓的牌組
      for (i = 0; i < pool[my_sit].out_card_index; i++) {
        if (pool[my_sit].out_card[i][1] == current_card ||
            pool[my_sit].out_card[i][2] == current_card) {
          break;
        }
      }
      // 設定槓牌類型
      pool[my_sit].out_card[i][0] = 12;
      // 檢查是否找到要槓的牌組
      if (i == pool[my_sit].out_card_index) {
        break;
      }
      // 設定槓牌的牌
      card1 = card2 = card3 = current_card;
      // 顯示槓牌
      draw_epk(my_id, check, card1, card2, card3);
      // 設定遊戲模式為摸牌
      play_mode = GET_CARD;
      // 顯示摸牌指示
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      break;
    case 7:
      // 吃牌 (右邊)
      play_mode = THROW_CARD;
      // 找到要吃的牌
      pool[my_sit].card[search_card(my_sit, current_card + 1)] =
          pool[my_sit].card[pool[my_sit].num - 1];
      pool[my_sit].card[search_card(my_sit, current_card + 2)] =
          pool[my_sit].card[pool[my_sit].num - 2];
      // 設定吃的牌
      card1 = current_card + 1;
      card2 = current_card;
      card3 = current_card + 2;
      // 顯示吃牌
      draw_epk(my_id, check, card1, card2, card3);
      // 移除吃的牌
      pool[my_sit].num -= 3;
      // 排序牌組
      sort_card(1);
      break;
    case 8:
      // 吃牌 (左右)
      play_mode = THROW_CARD;
      // 找到要吃的牌
      pool[my_sit].card[search_card(my_sit, current_card - 1)] =
          pool[my_sit].card[pool[my_sit].num - 1];
      pool[my_sit].card[search_card(my_sit, current_card + 1)] =
          pool[my_sit].card[pool[my_sit].num - 2];
      // 設定吃的牌
      card1 = current_card - 1;
      card2 = current_card;
      card3 = current_card + 1;
      // 顯示吃牌
      draw_epk(my_id, check, card1, card2, card3);
      // 移除吃的牌
      pool[my_sit].num -= 3;
      // 排序牌組
      sort_card(1);
      break;
    case 9:
      // 吃牌 (左邊)
      play_mode = THROW_CARD;
      // 找到要吃的牌
      pool[my_sit].card[search_card(my_sit, current_card - 1)] =
          pool[my_sit].card[pool[my_sit].num - 1];
      pool[my_sit].card[search_card(my_sit, current_card - 2)] =
          pool[my_sit].card[pool[my_sit].num - 2];
      // 設定吃的牌
      card1 = current_card - 2;
      card2 = current_card;
      card3 = current_card - 1;
      // 顯示吃牌
      draw_epk(my_id, check, card1, card2, card3);
      // 移除吃的牌
      pool[my_sit].num -= 3;
      // 排序牌組
      sort_card(1);
      break;
  }
  if (check != 12) {
    // 處理吃碰
    pool[my_sit].out_card[pool[my_sit].out_card_index][0] = check;
    pool[my_sit].out_card[pool[my_sit].out_card_index][1] = card1;
    pool[my_sit].out_card[pool[my_sit].out_card_index][2] = card2;
    pool[my_sit].out_card[pool[my_sit].out_card_index][3] = card3;
    if (check == 3 || check == 11) {
      // 明槓或暗槓
      pool[my_sit].out_card[pool[my_sit].out_card_index][4] = card3;
      pool[my_sit].out_card[pool[my_sit].out_card_index][5] = 0;
    } else {
      // 碰牌
      pool[my_sit].out_card[pool[my_sit].out_card_index][4] = 0;
    }
    pool[my_sit].out_card_index++;
  }
  // 使用 snprintf 避免緩衝區溢出
  snprintf(msg_buf, sizeof(msg_buf), "530%c%c%c%c%c", my_id, check, card1, card2,
           card3);
  turn = my_sit;
  card_owner = my_sit;
  if (in_serv) {
    gettimeofday(&before, (struct timezone *)0);
    broadcast_msg(1, msg_buf);
  } else {
    write_msg(table_sockfd, msg_buf);
  }
  current_item = pool[my_sit].num;
  pos_x = INDEX_X + 16 * 2 + 1;
  draw_index(pool[my_sit].num + 1);
  display_point(my_sit);
  show_num(2, 70, 144 - card_point - 16, 2);
  return_cursor();
}

/*  Draw eat,pong or kang */
void draw_epk(char id, char kind, char card1, char card2, char card3) {
  // 顯示吃碰槓
  int sit;
  int i;
  char msg_buf[80];

  beep1();
  sit = player[id].sit;
  // 判斷是否為槓牌
  if (kind == KANG || kind == 11 || kind == 12) {
    in_kang = 1;
  }
  // 判斷是否為明槓或暗槓
  if (kind != 11 && kind != 12) {
    throw_card(20);
  }
  // 判斷是否為出牌槓
  if (kind == 12) {
    // 尋找出牌槓的牌組
    for (i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][1] == card1 &&
          pool[sit].out_card[i][2] == card2) {
        break;
      }
    }
    // 顯示出牌槓
    attron(A_REVERSE);
    switch ((sit - my_sit + 4) % 4) {
    case 0:
      // 顯示玩家自己的出牌槓
      wmvaddstr(stdscr, INDEX_Y, INDEX_X + i * 6, "槓");
      break;
    case 1:
      // 顯示玩家右邊的出牌槓
      wmvaddstr(stdscr, INDEX_Y1 - i * 3 - 1, INDEX_X1 - 2, "槓");
      break;
    case 2:
      // 顯示玩家對面的出牌槓
      wmvaddstr(stdscr, INDEX_Y2 + 2, INDEX_X2 - i * 6 - 2, "槓");
      break;
    case 3:
      // 顯示玩家左邊的出牌槓
      wmvaddstr(stdscr, INDEX_Y3 + i * 3 + 1, INDEX_X3 + 4, "槓");
      break;
    }
    attroff(A_REVERSE);
    return_cursor();
  } else {
    // 顯示吃碰
    switch ((sit - my_sit + 4) % 4) {
    case 0:
      // 顯示玩家自己的吃碰
      show_card(20, INDEX_X + (16 - pool[my_sit].num) * 2 + 4, INDEX_Y + 1, 1);
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
      if (kind == 11) {
        card1 = card3 = 30;
      }
      show_card(card1, INDEX_X + (16 - pool[my_sit].num) * 2 - 2,
                INDEX_Y + 1, 1);
      show_card(card2, INDEX_X + (16 - pool[my_sit].num) * 2, INDEX_Y + 1, 1);
      show_card(card3, INDEX_X + (16 - pool[my_sit].num) * 2 + 2,
                INDEX_Y + 1, 1);
      break;
    case 1:
      // 顯示玩家右邊的吃碰
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
      if (kind == 11) {
        card1 = card2 = card3 = 40;
      }
      show_card(card1, INDEX_X1, INDEX_Y1 - (16 - pool[sit].num), 0);
      show_card(card2, INDEX_X1, INDEX_Y1 - (16 - pool[sit].num) - 1, 0);
      show_card(card3, INDEX_X1, INDEX_Y1 - (16 - pool[sit].num) - 2, 0);
      break;
    case 2:
      // 顯示玩家對面的吃碰
      wmvaddstr(stdscr, INDEX_Y2 + 2,
                INDEX_X2 - (16 - pool[sit].num) * 2 - 2, "└");
      if (kind == KANG || kind == 11) {
        attron(A_REVERSE);
        wmvaddstr(stdscr, INDEX_Y2 + 2,
                  INDEX_X2 - (16 - pool[sit].num) * 2, "槓");
        attroff(A_REVERSE);
      } else {
        wmvaddstr(stdscr, INDEX_Y2 + 2,
                  INDEX_X2 - (16 - pool[sit].num) * 2, "─");
      }
      if (kind == 11) {
        card1 = card2 = card3 = 30;
      }
      wmvaddstr(stdscr, INDEX_Y2 + 2,
                INDEX_X2 - (16 - pool[sit].num) * 2 + 2, "┘");
      wrefresh(stdscr);
      show_card(20, INDEX_X2 - (16 - pool[sit].num) * 2 - 4, INDEX_Y2, 1);
      show_card(card1, INDEX_X2 - (16 - pool[sit].num) * 2 + 2, INDEX_Y2, 1);
      show_card(card2, INDEX_X2 - (16 - pool[sit].num) * 2, INDEX_Y2, 1);
      show_card(card3, INDEX_X2 - (16 - pool[sit].num) * 2 - 2, INDEX_Y2, 1);
      break;
    case 3:
      // 顯示玩家左邊的吃碰
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
      if (kind == 11) {
        card1 = card2 = card3 = 40;
      }
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

void draw_flower(char sit, char card) {
  // 顯示花牌
  char msg_buf[80];

  // 啟用粗體字
  attron(A_BOLD);
  // 設定為聽牌狀態
  in_kang = 1;
  // 標記花牌已存在
  pool[sit].flower[card - 51] = 1;
  // 使用 strncpy 避免緩衝區溢出
  strncpy(msg_buf, mj_item[card], sizeof(msg_buf) - 1);
  // 確保字串結尾
  msg_buf[sizeof(msg_buf) - 1] = '\0';
  // 重置游標位置
  reset_cursor();
  // 根據玩家位置顯示花牌
  switch ((sit - my_sit + 4) % 4) {
  case 0:
    // 顯示玩家自己的花牌
    wmvaddstr(stdscr, FLOWER_Y, FLOWER_X + (card - 51) * 2, msg_buf);
    break;
  case 1:
    // 顯示玩家右邊的花牌
    wmvaddstr(stdscr, FLOWER_Y1 - (card - 51), FLOWER_X1, msg_buf);
    break;
  case 2:
    // 顯示玩家對面的花牌
    wmvaddstr(stdscr, FLOWER_Y2, FLOWER_X2 - (card - 51) * 2, msg_buf);
    break;
  case 3:
    // 顯示玩家左邊的花牌
    wmvaddstr(stdscr, FLOWER_Y3 + (card - 51), FLOWER_X3, msg_buf);
    break;
  }
  // 回復游標位置
  return_cursor();
  // 關閉粗體字
  attroff(A_BOLD);
}