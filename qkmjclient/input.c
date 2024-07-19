#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#if defined(HAVE_LIBNCURSES)
  #include  <ncurses.h>
#endif

#include "qkmj.h"

void process_key()
{
  int i, j, key;
  static int m, n, current_eat;
  static int eat_pool[5];
  char msg_buf[255];
  char card, card1;

  while(Check_for_data(0))
  {
    key=my_getch();
    /* ---------  PLAY_MODE  ------------ */
    if (input_mode == PLAY_MODE) {
      // 處理玩家手牌選擇
      if (key >= 'a' && key <= 'a' + pool[my_sit].num) {
        if (play_mode == THROW_CARD) {
          // 選擇要丟棄的牌
          current_item = key - 'a';
          return_cursor();
          // 執行丟棄牌的動作
          play_mode = WAIT_CARD;
          in_kang = 0;
          pool[my_sit].first_round = 0;
          if (in_join) {
            snprintf(msg_buf, sizeof(msg_buf), "401%c", pool[my_sit].card[current_item]);
            write_msg(table_sockfd, msg_buf);
            current_id = my_id;
            current_card = pool[my_sit].card[current_item];
          } else if (in_serv) { // 不需要檢查自己的牌
            pool[my_sit].time += thinktime();
            display_time(my_sit);
            snprintf(msg_buf, sizeof(msg_buf), "312%c%f", my_sit, pool[my_sit].time);
            broadcast_msg(1, msg_buf);
            snprintf(msg_buf, sizeof(msg_buf), "314%c%c", my_sit, 3);
            broadcast_msg(1, msg_buf);
            snprintf(msg_buf, sizeof(msg_buf), "402%c%c", 1, pool[my_sit].card[current_item]);
            broadcast_msg(1, msg_buf);
            current_card = pool[my_sit].card[current_item];
            // 檢查其他玩家是否可以吃碰槓
            for (i = 1; i <= 4; i++) {
              if (table[i] > 1) { // 檢查其他玩家
                check_card(i, current_card);
              }
            }
            // 通知其他玩家可以檢查
            for (i = 1; i <= 4; i++) {
              if (table[i] > 1) {
                for (j = 1; j < check_number; j++) {
                  if (check_flag[i][j]) {
                    snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c", check_flag[i][1] + '0',
                              check_flag[i][2] + '0', check_flag[i][3] + '0',
                              check_flag[i][4] + '0');
                    write_msg(player[table[i]].sockfd, msg_buf);
                    in_check[i] = 1;
                    break;
                  } else {
                    in_check[i] = 0;
                  }
                }
              }
            }
            in_check[1] = 0;
            check_on = 1;
            current_id = 1;
            send_card_on = 0;
            next_player_request = 1;
            next_player_on = 0;
          }
          // 丟棄牌
          throw_card(pool[my_sit].card[current_item]);
          show_cardmsg(my_sit, pool[my_sit].card[current_item]);
          // 將丟棄的牌移到最後
          pool[my_sit].card[current_item] = pool[my_sit].card[pool[my_sit].num];
          current_item = pool[my_sit].num;
          pos_x = INDEX_X + 16 * 2 + 1;
          play_mode = WAIT_CARD;
          show_card(20, pos_x, INDEX_Y + 1, 1);
          sort_card(0);
          wrefresh(stdscr);
          return_cursor();
        }
      }
      switch (key) {
        case 0:
          break;
        case TAB:
          // 切換到聊天模式
          input_mode = TALK_MODE;
          current_mode = PLAY_MODE;
          return_cursor();
          break;
        case '3':
          break;
        case ',':
        case KEY_LEFT:
          // 處理向左移動選擇
          if (play_mode != THROW_CARD) {
            break;
          }
          current_item--;
          if (current_item == -1) {
            current_item = pool[my_sit].num;
          }
          pos_x = INDEX_X + (16 - pool[my_sit].num + current_item) * 2;
          if (current_item == pool[my_sit].num) {
            pos_x++;
          }
          return_cursor();
          break;
        case '.':
        case KEY_RIGHT:
          // 處理向右移動選擇
          if (play_mode != THROW_CARD) {
            break;
          }
          current_item++;
          if (current_item == pool[my_sit].num + 1) {
            current_item = 0;
          }
          pos_x = INDEX_X + (16 - pool[my_sit].num + current_item) * 2;
          if (current_item == pool[my_sit].num) {
            pos_x++;
          }
          return_cursor();
          break;
        case '`':
          // 處理作弊模式
          if (!cheat_mode) {
            break;
          }
          if (play_mode == THROW_CARD) {
            if (in_join) {
              // 處理加入房間的作弊
            } else if (in_serv) {
              // 處理遊戲中的作弊
              card_index--;
              if (card_index < card_point) {
                card_index = 143;
              }
              card = mj[card_index];
              change_card(current_item, card);
              mj[card_index] = pool[my_sit].card[current_item];
              return_cursor();
            }
          }
          break;
        case CTRL_L:
          // 重新繪製畫面
          redraw_screen();
          break;
        case KEY_ENTER:
        case ENTER:
        case SPACE:
          // 處理 ENTER 或 SPACE 按鍵
          if (play_mode == GET_CARD) {
            // 處理取得新牌
            play_mode = WAIT_CARD;
            if (in_join) {
              write_msg(table_sockfd, "313");
              break;
            } else {
              card = mj[card_point++];
              current_card = card;
              show_num(2, 70, 144 - card_point - 16, 2);
              // 切換回合
              card_owner = my_sit;
              snprintf(msg_buf, sizeof(msg_buf), "305%c", (char)my_sit);
              broadcast_msg(1, msg_buf);
              // 顯示新牌背
              snprintf(msg_buf, sizeof(msg_buf), "314%c%c", my_sit, 2);
              broadcast_msg(1, msg_buf);
              // 更新牌數
              snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point);
              broadcast_msg(1, msg_buf);
              // 處理新牌
              process_new_card(my_sit, card);
              // 清除檢查標記
              clear_check_flag(my_sit);
              // 檢查槓和胡
              check_flag[my_sit][3] = check_kang(my_sit, card);
              check_flag[my_sit][4] = check_make(my_sit, card, 0);
              in_check[1] = 0;
              // 檢查是否有可以執行的動作
              for (i = 1; i < check_number; i++) {
                if (check_flag[my_sit][i]) {
                  getting_card = 1;
                  init_check_mode();
                  in_check[1] = 1;
                  check_on = 1;
                }
              }
              gettimeofday(&before, (struct timezone *)0);
            }
            break;
          } else if (play_mode == THROW_CARD) {
            // 處理丟棄牌
            play_mode = WAIT_CARD;
            in_kang = 0;
            pool[my_sit].first_round = 0;
            if (in_join) {
              snprintf(msg_buf, sizeof(msg_buf), "401%c", pool[my_sit].card[current_item]);
              write_msg(table_sockfd, msg_buf);
              current_id = my_id;
              current_card = pool[my_sit].card[current_item];
            } else if (in_serv) { // 不需要檢查自己的牌
              pool[my_sit].time += thinktime();
              display_time(my_sit);
              snprintf(msg_buf, sizeof(msg_buf), "312%c%f", my_sit, pool[my_sit].time);
              broadcast_msg(1, msg_buf);
              snprintf(msg_buf, sizeof(msg_buf), "314%c%c", my_sit, 3);
              broadcast_msg(1, msg_buf);
              snprintf(msg_buf, sizeof(msg_buf), "402%c%c", 1, pool[my_sit].card[current_item]);
              broadcast_msg(1, msg_buf);
              current_card = pool[my_sit].card[current_item];
              // 檢查其他玩家是否可以吃碰槓
              for (i = 1; i <= 4; i++) {
                if (table[i] > 1) { // 檢查其他玩家
                  check_card(i, current_card);
                }
              }
              // 通知其他玩家可以檢查
              for (i = 1; i <= 4; i++) {
                if (table[i] > 1) {
                  for (j = 1; j < check_number; j++) {
                    if (check_flag[i][j]) {
                      snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c", check_flag[i][1] + '0',
                                check_flag[i][2] + '0', check_flag[i][3] + '0',
                                check_flag[i][4] + '0');
                      write_msg(player[table[i]].sockfd, msg_buf);
                      in_check[i] = 1;
                      break;
                    } else {
                      in_check[i] = 0;
                    }
                  }
                }
              }
              in_check[1] = 0;
              check_on = 1;
              current_id = 1;
              send_card_on = 0;
              next_player_request = 1;
              next_player_on = 0;
            }
            // 丟棄牌
            throw_card(pool[my_sit].card[current_item]);
            show_cardmsg(my_sit, pool[my_sit].card[current_item]);
            // 將丟棄的牌移到最後
            pool[my_sit].card[current_item] = pool[my_sit].card[pool[my_sit].num];
            current_item = pool[my_sit].num;
            pos_x = INDEX_X + 16 * 2 + 1;
            play_mode = WAIT_CARD;
            show_card(20, pos_x, INDEX_Y + 1, 1);
            sort_card(0);
            wrefresh(stdscr);
            return_cursor();
          } else {
            break;
          }
          break;
        default:
          break;
      }
    }
    else if (input_mode == CHECK_MODE) {
      // 處理檢查模式的輸入
      if (key >= '0' && key <= '0' + check_number - 1) {
        // 選擇檢查選項
        current_check = key - '0';
        check_x = org_check_x + current_check * 3;
        return_cursor();
        // 執行選擇檢查選項的動作
        if (current_check && !check_flag[my_sit][current_check]) {
          break;
        }
        // 顯示檢查選項
        for (i = 0; i < check_number; i++) {
          wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, "  ");
          wrefresh(stdscr);
          wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, check_name[i]);
          wrefresh(stdscr);
        }
        show_cardmsg(0, 0);
        // 處理吃牌
        if (current_check == EAT) {
          m = 0;
          eat_pool[m] = 0;
          // 檢查是否可以吃牌
          if (current_card % 10 >= 3) {
            if (search_card(my_sit, current_card - 2) >= 0 &&
                search_card(my_sit, current_card - 1) >= 0) {
              eat_pool[m] = current_card - 2;
              m++;
            }
          }
          if (current_card % 10 >= 2 && current_card % 10 <= 8) {
            if (search_card(my_sit, current_card - 1) >= 0 &&
                search_card(my_sit, current_card + 1) >= 0) {
              eat_pool[m] = current_card - 1;
              m++;
            }
          }
          if (current_card % 10 <= 7) {
            if (search_card(my_sit, current_card + 1) >= 0 &&
                search_card(my_sit, current_card + 2) >= 0) {
              eat_pool[m] = current_card;
              m++;
            }
          }
          eat_pool[m] = 0;
          // 顯示吃牌選項
          if (m > 1) {
            input_mode = EAT_MODE;
            wmvaddstr(stdscr, 15, 58, "┌───────┐");
            wmvaddstr(stdscr, 16, 58, "│              │");
            wmvaddstr(stdscr, 17, 58, "└───────┘");
            for (i = 0; i < m; i++) {
              if (eat_pool[i]) {
                snprintf(msg_buf, sizeof(msg_buf), "%d%d%d", eat_pool[i] % 10, eat_pool[i] % 10 + 1,
                          eat_pool[i] % 10 + 2);
                wmvaddstr(stdscr, org_eat_y, org_eat_x + i * 4, msg_buf);
              }
            }
            current_eat = 0;
            eat_x = org_eat_x;
            eat_y = org_eat_y;
            return_cursor();
          } else {
            // 執行吃牌動作
            i = current_card - eat_pool[m - 1] + 7;
            write_check(i);
            input_mode = PLAY_MODE;
            return_cursor();
          }
        } else {
          // 執行其他檢查動作
          write_check(current_check);
          input_mode = PLAY_MODE;
          return_cursor();
        }
      } else {
        switch (key) {
          case 0:
            break;
          case TAB:
            // 切換到聊天模式
            input_mode = TALK_MODE;
            current_mode = CHECK_MODE;
            return_cursor();
            break;
          case CTRL_L:
            // 重新繪製畫面
            redraw_screen();
            break;
          case ',':
          case KEY_LEFT:
            // 處理向左移動選擇
            current_check--;
            if (current_check == -1) {
              current_check = check_number - 1;
            }
            check_x = org_check_x + current_check * 3;
            return_cursor();
            break;
          case '.':
          case KEY_RIGHT:
            // 處理向右移動選擇
            current_check++;
            if (current_check == check_number) {
              current_check = 0;
            }
            check_x = org_check_x + current_check * 3;
            return_cursor();
            break;
          case KEY_ENTER:
          case ENTER:
          case SPACE:
            // 執行選擇檢查選項的動作
            if (current_check && !check_flag[my_sit][current_check]) {
              break;
            }
            // 顯示檢查選項
            for (i = 0; i < check_number; i++) {
              wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, "  ");
              wrefresh(stdscr);
              wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, check_name[i]);
              wrefresh(stdscr);
            }
            show_cardmsg(0, 0);
            // 處理吃牌
            if (current_check == EAT) {
              m = 0;
              eat_pool[m] = 0;
              // 檢查是否可以吃牌
              if (current_card % 10 >= 3) {
                if (search_card(my_sit, current_card - 2) >= 0 &&
                    search_card(my_sit, current_card - 1) >= 0) {
                  eat_pool[m] = current_card - 2;
                  m++;
                }
              }
              if (current_card % 10 >= 2 && current_card % 10 <= 8) {
                if (search_card(my_sit, current_card - 1) >= 0 &&
                    search_card(my_sit, current_card + 1) >= 0) {
                  eat_pool[m] = current_card - 1;
                  m++;
                }
              }
              if (current_card % 10 <= 7) {
                if (search_card(my_sit, current_card + 1) >= 0 &&
                    search_card(my_sit, current_card + 2) >= 0) {
                  eat_pool[m] = current_card;
                  m++;
                }
              }
              eat_pool[m] = 0;
              // 顯示吃牌選項
              if (m > 1) {
                input_mode = EAT_MODE;
                wmvaddstr(stdscr, 15, 58, "┌───────┐");
                wmvaddstr(stdscr, 16, 58, "│              │");
                wmvaddstr(stdscr, 17, 58, "└───────┘");
                for (i = 0; i < m; i++) {
                  if (eat_pool[i]) {
                    snprintf(msg_buf, sizeof(msg_buf), "%d%d%d", eat_pool[i] % 10, eat_pool[i] % 10 + 1,
                              eat_pool[i] % 10 + 2);
                    wmvaddstr(stdscr, org_eat_y, org_eat_x + i * 4, msg_buf);
                  }
                }
                current_eat = 0;
                eat_x = org_eat_x;
                eat_y = org_eat_y;
                return_cursor();
              } else {
                // 執行吃牌動作
                i = current_card - eat_pool[m - 1] + 7;
                write_check(i);
                input_mode = PLAY_MODE;
                return_cursor();
              }
            } else {
              // 執行其他檢查動作
              write_check(current_check);
              input_mode = PLAY_MODE;
              return_cursor();
            }
            break;
          default:
            break;
        }
      }
    }
    /* --------- CHOOSE_EAT  ------------ */
    else if (input_mode == EAT_MODE) {
      // 處理吃牌模式的輸入
      for (i = 0; i < m; i++) {
        if (key == '0' + eat_pool[i] % 10) {
          // 選擇吃牌選項
          current_eat = i;
          // 執行吃牌動作
          input_mode = PLAY_MODE;
          i = current_card - eat_pool[current_eat] + 7;
          write_check(i);
          show_cardmsg(0, 0);
          break;
        }
      }
      switch (key) {
        case TAB:
          // 切換到聊天模式
          input_mode = TALK_MODE;
          current_mode = EAT_MODE;
          return_cursor();
          break;
        case ',':
        case KEY_LEFT:
          // 處理向左移動選擇
          if (current_eat <= 0) {
            break;
          }
          current_eat--;
          eat_x -= 4;
          return_cursor();
          break;
        case '.':
        case KEY_RIGHT:
          // 處理向右移動選擇
          if (current_eat >= m - 1) {
            break;
          }
          current_eat++;
          eat_x += 4;
          return_cursor();
          break;
        case CTRL_L:
          // 重新繪製畫面
          redraw_screen();
          break;
        case KEY_ENTER:
        case ENTER:
        case SPACE:
          // 執行選擇吃牌選項的動作
          if (current_eat >= 0 && current_eat < m) {
            input_mode = PLAY_MODE;
            i = current_card - eat_pool[current_eat] + 7;
            write_check(i);
            show_cardmsg(0, 0);
          }
          break;
        default:
          break;
      }
    }
    /* ---------  TALK_MODE  ------------ */
    else if (input_mode == TALK_MODE) {
      switch (key) {
        case 0:
          break;
        case TAB:
          // 切換到上一個模式
          if (screen_mode == PLAYING_SCREEN_MODE) {
            input_mode = current_mode;
            return_cursor();
          }
          break;
        case KEY_LEFT:
          // 處理向左移動游標
          if (talk_x == 0) {
            break;
          }
          talk_x--;
          return_cursor();
          break;
        case KEY_RIGHT:
          // 處理向右移動游標
          if (talk_x == talk_buf_count) {
            break;
          }
          talk_x++;
          return_cursor();
          break;
        case CTRL_P:
        case KEY_UP:
          // 處理向上翻頁
          if (h_point == (h_head + 1) % HISTORY_SIZE || h_point == h_head) {
            break;
          } else {
            h_point = (h_point - 1 + HISTORY_SIZE) % HISTORY_SIZE;
          }
          werase(inputwin);
          mvwaddstr(inputwin, 0, 0, history[h_point]);
          wrefresh(inputwin);
          strncpy(talk_buf, history[h_point], sizeof(talk_buf));
          talk_buf[talk_right - talk_left - 1] = 0;
          talk_buf_count = strlen(talk_buf);
          talk_x = talk_buf_count;
          break;
        case CTRL_N:
        case KEY_DOWN:
          // 處理向下翻頁
          if (h_point == h_tail) {
            break;
          } else {
            h_point = (h_point + 1) % HISTORY_SIZE;
          }
          werase(inputwin);
          mvwaddstr(inputwin, 0, 0, history[h_point]);
          wrefresh(inputwin);
          strncpy(talk_buf, history[h_point], sizeof(talk_buf));
          talk_buf[talk_right - talk_left - 1] = 0;
          talk_buf_count = strlen(talk_buf);
          talk_x = talk_buf_count;
          break;
        case CTRL_A:
          // 將游標移到輸入框的開頭
          talk_x = 0;
          wmove(inputwin, talk_y, talk_x);
          wrefresh(inputwin);
          break;
        case CTRL_E:
          // 將游標移到輸入框的結尾
          talk_x = strlen(talk_buf);
          wmove(inputwin, talk_y, talk_x);
          wrefresh(inputwin);
          break;
        case CTRL_D:
          talk_x++;
        case CTRL_H:
        case BACKSPACE:
        case KEY_BACKSPACE: // ncurses
          // 處理退格鍵
          if (talk_x == 0) {
            break;
          }
          talk_x--;
          for (i = talk_x; i < talk_buf_count; i++) {
            talk_buf[i] = talk_buf[i + 1];
          }
          talk_buf[talk_buf_count--] = '\0';
          strncpy(history[h_tail], talk_buf, sizeof(history[h_tail]));
          mvprintstr(inputwin, talk_y, talk_x, talk_buf + talk_x);
          printch(inputwin, ' ');
          return_cursor();
          break;
        case KEY_ENTER:
        case ENTER: //TALK TO Others
          // 處理 ENTER 鍵
          switch (input_mode) {
            case TALK_MODE:
              if (talk_buf_count == 0) {
                break;
              }
              strncpy(history[h_tail], talk_buf, sizeof(history[h_tail]));
              command_parser(talk_buf);
              h_tail = (h_tail + 1) % HISTORY_SIZE;
              history[h_tail][0] = 0;
              if (h_tail == h_head) {
                h_head = (h_head + 1) % HISTORY_SIZE;
              }
              h_point = h_tail;
              clear_input_line();
              talk_x = 0;
              return_cursor();
              if (input_mode == PLAY_MODE) {
                return_cursor();
              }
              break;
            case PLAY_MODE:
              break;
            default:
              err("Unknow input mode");
              break;
          }
          break;
        case CTRL_U:
          // 清除輸入框
          clear_input_line();
          break;
        case CTRL_J:
          break;
        case CTRL_K:
          break;
        case CTRL_L:
          // 重新繪製畫面
          redraw_screen();
          break;
        case CTRL_G:
          break;
        default:
          // 處理其他鍵盤輸入
          if (talk_x == talk_right - talk_left) {
            break;
          }
          // 游標在最右側
          if (talk_buf_count == talk_x) {
            talk_buf[talk_buf_count] = key;
            talk_buf_count++;
            talk_buf[talk_buf_count] = '\0';
            mvprintch(inputwin, talk_y, talk_x, key);
            talk_x++;
            wrefresh(inputwin);
          } else {
            if (talk_buf_count < talk_right - talk_left) {
              talk_buf[++talk_buf_count] = '\0';
            } else {
              talk_buf[talk_buf_count - 1] = '\0';
            }
            for (i = talk_buf_count; i > talk_x; i--) {
              talk_buf[i] = talk_buf[i - 1];
            }
            talk_buf[talk_x] = key;
            mvprintstr(inputwin, talk_y, talk_x, talk_buf + talk_x);
            talk_x++;
            return_cursor();
          }
          strncpy(history[h_tail], talk_buf, sizeof(history[h_tail]));
          break;
      }
    }
  }
}

int my_getch() {
  int i;
  static int l = 0;
  // 處理重複的 ENTER 或 10
  while ((i = getch()) == 10 || i == ENTER) {
    if (l == ENTER || l == 10) {
      l = 0;
      continue;
    }
    l = i;
    return ENTER;
  }
  l = 0;
  // 處理 telnet ESCAPE(\033)
  if (i == ESCAPE) {
    // 忽略 '['
    getch();
    switch (getch()) {
      case 'A':
        i = KEY_UP;
        break;
      case 'B':
        i = KEY_DOWN;
        break;
      case 'C':
        i = KEY_RIGHT;
        break;
      case 'D':
        i = KEY_LEFT;
        break;
    }
  }
  // 處理 leave()
  if (i == -1) {
    leave();
  }
  return i;
}