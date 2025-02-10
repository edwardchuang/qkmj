#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include "qkmj.h"

// 常數定義，避免 Magic Number
#define MAX_EAT_OPTIONS 5
#define TALK_BUFFER_SIZE (talk_right - talk_left) //輸入框長度
#define MAX_CARD_INDEX 143
#define MIN_CARD_INDEX 0

// 靜態變數宣告，加上 static 限制作用域
static int m, n, current_eat;
static int eat_pool[MAX_EAT_OPTIONS];

// 處理按鍵事件的主函式
void process_key() {
    int key;

    // 迴圈處理可用的輸入資料
    while (check_for_data(0)) {
        key = my_getch();

        // 根據目前輸入模式，呼叫對應的處理函式
        switch (input_mode) {
            case PLAY_MODE:
                handle_play_mode_key(key);
                break;
            case CHECK_MODE:
                handle_check_mode_key(key);
                break;
            case EAT_MODE:
                handle_eat_mode_key(key);
                break;
            case TALK_MODE:
                handle_talk_mode_key(key);
                break;
            default:
                // 錯誤處理：未知的輸入模式
                err("未知的輸入模式");
                break;
        }
    }
}

// 處理遊戲模式的按鍵事件
static void handle_play_mode_key(int key) {
  int i;
  char msg_buf[255]; //指令緩衝區
  char card;

    // 如果輸入的是 'a' 到 'a' + 手牌數量 之間的字元
    if (key >= 'a' && key <= 'a' + pool[my_sit].num) {
        // 如果目前是丟牌模式
        if (play_mode == THROW_CARD) {
            // 設定選擇的牌的索引
            current_item = key - 'a';
            return_cursor();
            // 丟牌處理移到這裡
            play_mode = WAIT_CARD;
            in_kang = 0;
            pool[my_sit].first_round = 0;

            if (in_join) {
                // 加入牌局：傳送丟牌訊息給伺服器
                sprintf(msg_buf, "401%c", pool[my_sit].card[current_item]);
                write_msg(table_sockfd, msg_buf);
                current_id = my_id;
                current_card = pool[my_sit].card[current_item];
            } else if (in_serv) {
                // 開設牌局：廣播丟牌訊息
                pool[my_sit].time += thinktime(before);
                display_time(my_sit);
                sprintf(msg_buf, "312%c%f", my_sit, pool[my_sit].time);
                broadcast_msg(1, msg_buf);
                sprintf(msg_buf, "314%c%c", my_sit, 3);
                broadcast_msg(1, msg_buf);
                sprintf(msg_buf, "402%c%c", 1, pool[my_sit].card[current_item]);
                broadcast_msg(1, msg_buf);
                current_card = pool[my_sit].card[current_item];

                // 檢查其他玩家是否可以吃、碰、槓
                for (i = 1; i <= 4; i++) {
                    if (table[i] > 1) { // clients
                        check_card(i, current_card);
                    }
                }

                // 設定檢查標誌
                for (i = 1; i <= 4; i++) {
                    if (table[i] > 1) {
                        for (int j = 1; j < check_number; j++) {
                            if (check_flag[i][j]) {
                                sprintf(msg_buf, "501%c%c%c%c", check_flag[i][1] + '0',
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
            // 更新畫面
            throw_card(pool[my_sit].card[current_item]);
            show_cardmsg(my_sit, pool[my_sit].card[current_item]);
            pool[my_sit].card[current_item] = pool[my_sit].card[pool[my_sit].num];
            current_item = pool[my_sit].num;
            pos_x = INDEX_X + 16 * 2 + 1;
            play_mode = WAIT_CARD;
            show_card(20, pos_x, INDEX_Y + 1, 1);
            sort_card(0);
            wrefresh(stdscr);
            return_cursor();
        }
    } else {
        // 處理其他按鍵
        switch (key) {
            case TAB:
                // 切換到聊天模式
                input_mode = TALK_MODE;
                current_mode = PLAY_MODE;
                return_cursor();
                break;
            case CTRL_L:
                // 重繪畫面
                redraw_screen();
                break;
            case KEY_ENTER:
            case ENTER:
            case SPACE:
                if (play_mode == GET_CARD) {
                    // 抽牌模式：從牌堆抽一張牌
                    play_mode = WAIT_CARD;
                    if (in_join) {
                        // 加入牌局：傳送訊息給伺服器，要求發牌
                        write_msg(table_sockfd, "313");
                    } else {
                        // 開設牌局：從牌堆抽一張牌
                        card = mj[card_point++];
                        current_card = card;
                        show_num(2, 70, MAX_CARD_INDEX - card_point - 15, 2);

                        card_owner = my_sit;
                        sprintf(msg_buf, "305%c", (char)my_sit);
                        broadcast_msg(1, msg_buf);

                        sprintf(msg_buf, "314%c%c", my_sit, 2);
                        broadcast_msg(1, msg_buf);

                        sprintf(msg_buf, "306%c", card_point);
                        broadcast_msg(1, msg_buf);

                        // 處理抽到的新牌
                        process_new_card(my_sit, card);
                        clear_check_flag(my_sit);
                        check_flag[my_sit][3] = check_kang(my_sit, card);
                        check_flag[my_sit][4] = check_make(my_sit, card, 0);
                        in_check[1] = 0;

                        // 檢查是否可以吃、碰、槓、胡
                        for (i = 1; i < check_number; i++) {
                            if (check_flag[my_sit][i]) {
                                getting_card = 1;
                                init_check_mode();
                                in_check[1] = 1;
                                check_on = 1;
                            }
                        }

                        gettimeofday(&before, (struct timezone*)0);
                    }
                }
                break;
            case ',':
            case KEY_LEFT:
                //向左選擇手牌
                if (play_mode != THROW_CARD)
                    break;
                current_item--;
                if (current_item == -1)
                    current_item = pool[my_sit].num;
                pos_x = INDEX_X + (16 - pool[my_sit].num + current_item) * 2;
                if (current_item == pool[my_sit].num)
                    pos_x++;
                return_cursor();
                break;
            case '.':
            case KEY_RIGHT:
                //向右選擇手牌
                if (play_mode != THROW_CARD)
                    break;
                current_item++;
                if (current_item == pool[my_sit].num + 1)
                    current_item = 0;
                pos_x = INDEX_X + (16 - pool[my_sit].num + current_item) * 2;
                if (current_item == pool[my_sit].num)
                    pos_x++;
                return_cursor();
                break;
            case '`':
                //作弊模式：切換手牌
                if (!cheat_mode)
                    break;
                if (play_mode == THROW_CARD) {
                    if (in_join) {
                        //Do Nothing
                    } else if (in_serv) {
                        card_index--;
                        if (card_index < card_point)
                            card_index = MAX_CARD_INDEX;
                        card = mj[card_index];
                        change_card(current_item, card);
                        mj[card_index] = pool[my_sit].card[current_item];
                        return_cursor();
                    }
                }
                break;
            default:
                // 其他按鍵：不做處理
                break;
        }
    }
}

// 處理檢查模式的按鍵事件
static void handle_check_mode_key(int key) {
  int i;
  char msg_buf[255]; //指令緩衝區

    // 如果輸入的是 '0' 到 '0' + 檢查選項數量 之間的字元
    if (key >= '0' && key <= '0' + check_number - 1) {
        current_check = key - '0';
        check_x = org_check_x + current_check * 3;
        return_cursor();

    } else {
        // 處理其他按鍵
        switch (key) {
            case TAB:
                // 切換到聊天模式
                input_mode = TALK_MODE;
                current_mode = CHECK_MODE;
                return_cursor();
                break;
            case CTRL_L:
                // 重繪畫面
                redraw_screen();
                break;
            case ',':
            case KEY_LEFT:
                //向上選擇 check item
                current_check--;
                if (current_check == -1)
                    current_check = check_number - 1;
                check_x = org_check_x + current_check * 3;
                return_cursor();
                break;
            case '.':
            case KEY_RIGHT:
                //向下選擇 check item
                current_check++;
                if (current_check == check_number)
                    current_check = 0;
                check_x = org_check_x + current_check * 3;
                return_cursor();
                break;
            case KEY_ENTER:
            case ENTER:
            case SPACE:
                // 選擇檢查選項
                if (current_check && !check_flag[my_sit][current_check])
                    break;

                // 清除先前的檢查選項顯示
                for (i = 0; i < check_number; i++) {
                    wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, "  ");
                    wrefresh(stdscr);
                    wmvaddstr(stdscr, org_check_y + 1, org_check_x + i * 3, check_name[i]);
                    wrefresh(stdscr);
                }
                show_cardmsg(0, 0);

                if (current_check == EAT) {
                    // 吃牌：判斷可以吃的牌
                    m = 0;
                    eat_pool[m] = 0;

                    if (current_card % 10 >= 3)
                        if (search_card(my_sit, current_card - 2) >= 0 &&
                            search_card(my_sit, current_card - 1) >= 0) {
                            eat_pool[m] = current_card - 2;
                            m++;
                        }
                    if (current_card % 10 >= 2 && current_card % 10 <= 8)
                        if (search_card(my_sit, current_card - 1) >= 0 &&
                            search_card(my_sit, current_card + 1) >= 0) {
                            eat_pool[m] = current_card - 1;
                            m++;
                        }
                    if (current_card % 10 <= 7)
                        if (search_card(my_sit, current_card + 1) >= 0 &&
                            search_card(my_sit, current_card + 2) >= 0) {
                            eat_pool[m] = current_card;
                            m++;
                        }
                    eat_pool[m] = 0;

                    if (m > 1) {
                        // 如果有多種吃法，切換到吃牌模式
                        input_mode = EAT_MODE;
                        // 顯示不同的吃牌選擇
                        wmvaddstr(stdscr, 15, 58, "┌───────┐");
                        wmvaddstr(stdscr, 16, 58, "│              │");
                        wmvaddstr(stdscr, 17, 58, "└───────┘");
                        for (i = 0; i < m; i++) {
                            if (eat_pool[i]) {
                                sprintf(msg_buf, "%d%d%d", eat_pool[i] % 10, eat_pool[i] % 10 + 1,
                                        eat_pool[i] % 10 + 2);
                                wmvaddstr(stdscr, org_eat_y, org_eat_x + i * 4, msg_buf);
                            }
                        }
                        current_eat = 0;
                        eat_x = org_eat_x;
                        eat_y = org_eat_y;
                        return_cursor();
                    } else {
                        // 如果只有一種吃法，直接執行
                        int i = current_card - eat_pool[m - 1] + 7;
                        write_check(i);
                        input_mode = PLAY_MODE;
                        return_cursor();
                    }
                } else {
                    // 執行其他檢查選項
                    write_check(current_check);
                    input_mode = PLAY_MODE;
                    return_cursor();
                }
                break;
            default:
                // 其他按鍵：不做處理
                break;
        }
    }
}

// 處理吃牌模式的按鍵事件
static void handle_eat_mode_key(int key) {
  int i;
  // 檢查輸入是否為數字鍵，且對應可選擇的吃牌組合
  for (i = 0; i < m; i++) {
      if (key == '0' + eat_pool[i] % 10) {
          current_eat = i;
          //快速選擇吃牌
          input_mode = PLAY_MODE;
          i = current_card - eat_pool[current_eat] + 7;
          write_check(i);
          show_cardmsg(0, 0);
          break;
      }
  }
  switch (key) {
      case TAB:
          input_mode = TALK_MODE;
          current_mode = EAT_MODE;
          return_cursor();
          break;
      case ',':
      case KEY_LEFT:
          //選擇左邊的吃牌組合
          if (current_eat <= 0)
              break;
          current_eat--;
          eat_x -= 4;
          return_cursor();
          break;
      case '.':
      case KEY_RIGHT:
          //選擇右邊的吃牌組合
          if (current_eat >= m - 1)
              break;
          current_eat++;
          eat_x += 4;
          return_cursor();
          break;
      case CTRL_L:
          //重繪畫面
          redraw_screen();
          break;
      case KEY_ENTER:
      case ENTER:
      case SPACE:
          //確認選擇的吃牌組合
          input_mode = PLAY_MODE;
          i = current_card - eat_pool[current_eat] + 7;
          write_check(i);
          show_cardmsg(0, 0);
          break;
      default:
          break;
  }
}

// 處理聊天模式的按鍵事件
static void handle_talk_mode_key(int key) {
  int i;
  char msg_buf[255]; // 指令緩衝區

  switch (key) {
      case 0:
          // 空按鍵：不做處理
          break;
      case TAB:
          // 切換回先前的輸入模式
          if (screen_mode == PLAYING_SCREEN_MODE) {
              input_mode = current_mode;
              return_cursor();
          }
          break;
      case KEY_LEFT:
          // 向左移動游標
          if (talk_x == 0)
              break;
          talk_x--;
          return_cursor();
          break;
      case KEY_RIGHT:
          // 向右移動游標
          if (talk_x == talk_buf_count)
              break;
          talk_x++;
          return_cursor();
          break;
      case CTRL_P:
      case KEY_UP:
          // 瀏覽上一條歷史訊息
          if (h_point == (h_head + 1) % HISTORY_SIZE || h_point == h_head)
              break;
          else
              h_point = (h_point - 1 + HISTORY_SIZE) % HISTORY_SIZE;
          werase(inputwin);
          mvwaddstr(inputwin, 0, 0, history[h_point]);
          wrefresh(inputwin);
          strcpy(talk_buf, history[h_point]);
          talk_buf[TALK_BUFFER_SIZE - 1] = 0;
          talk_buf_count = strlen(talk_buf);
          talk_x = talk_buf_count;
          break;
      case CTRL_N:
      case KEY_DOWN:
          // 瀏覽下一條歷史訊息
          if (h_point == h_tail)
              break;
          else
              h_point = (h_point + 1) % HISTORY_SIZE;
          werase(inputwin);
          mvwaddstr(inputwin, 0, 0, history[h_point]);
          wrefresh(inputwin);
          strcpy(talk_buf, history[h_point]);
          talk_buf[TALK_BUFFER_SIZE - 1] = 0;
          talk_buf_count = strlen(talk_buf);
          talk_x = talk_buf_count;
          break;
      case CTRL_A:
          // 游標移動到行首
          talk_x = 0;
          wmove(inputwin, talk_y, talk_x);
          wrefresh(inputwin);
          break;
      case CTRL_E:
          // 游標移動到行尾
          talk_x = strlen(talk_buf);
          wmove(inputwin, talk_y, talk_x);
          wrefresh(inputwin);
          break;
      case CTRL_D:
          // 向前刪除字元
          talk_x++;
      case CTRL_H:
      case BACKSPACE:
      case KEY_BACKSPACE: // ncurses
          // 刪除字元
          if (talk_x == 0)
              break;
          talk_x--;
          for (i = talk_x; i < talk_buf_count; i++)
              talk_buf[i] = talk_buf[i + 1];
          talk_buf[talk_buf_count--] = '\0';
          strcpy(history[h_tail], talk_buf);
          mvprintstr(inputwin, talk_y, talk_x, talk_buf + talk_x);
          printch(inputwin, ' ');
          return_cursor();
          break;
      case KEY_ENTER:
      case ENTER: // TALK TO Others
          // 處理 Enter 鍵，傳送訊息或執行指令
          switch (input_mode) {
              case TALK_MODE:
                  if (talk_buf_count == 0)
                      break;
                  strcpy(history[h_tail], talk_buf);
                  command_parser(talk_buf);
                  h_tail = (h_tail + 1) % HISTORY_SIZE;
                  history[h_tail][0] = 0;
                  if (h_tail == h_head)
                      h_head = (h_head + 1) % HISTORY_SIZE;
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
                  err("未知的輸入模式");
                  break;
          }
          break;
      case CTRL_U:
          // 清除輸入行
          clear_input_line();
          break;
      case CTRL_J:
          break;
      case CTRL_K:
          break;
      case CTRL_L:
          // 重繪畫面
          redraw_screen();
          break;
      case CTRL_G:
          break;
      default:
          // 處理一般字元輸入
          if (talk_x == TALK_BUFFER_SIZE)
              break;
          /* Cursor is in the right most side of characters */
          if (talk_buf_count == talk_x) {
              talk_buf[talk_buf_count] = key;
              talk_buf_count++;
              talk_buf[talk_buf_count] = '\0';
              mvprintch(inputwin, talk_y, talk_x, key);
              talk_x++;
              wrefresh(inputwin);
          } else {
              if (talk_buf_count < TALK_BUFFER_SIZE)
                  talk_buf[++talk_buf_count] = '\0';
              else
                  talk_buf[talk_buf_count - 1] = '\0';
              for (i = talk_buf_count; i > talk_x; i--)
                  talk_buf[i] = talk_buf[i - 1];
              talk_buf[talk_x] = key;
              mvprintstr(inputwin, talk_y, talk_x, talk_buf + talk_x);
              talk_x++;
              return_cursor();
          }
          strcpy(history[h_tail], talk_buf);
          break;
  }
}

int my_getch() {
  int i = getch(); // 從 curses 取得字元

  // 檢查 getch() 是否出錯，如果出錯就離開程式
  if (i == ERR) { // ERR 是 curses 中定義的錯誤碼
    leave(); // 呼叫離開函式
    return i; // 為了更明確, 還是回傳錯誤
  }
  
  // 處理換行字元和 Enter 鍵的組合，避免重複觸發
  static int last_char = 0; // 儲存前一個字元
  if ((i == 10 && last_char == ENTER) || (i == ENTER && last_char == 10)) {
    last_char = 0; // 重置前一個字元
    return my_getch(); // 遞迴呼叫，讀取下一個字元
  }

  // 處理單個換行或 Enter 鍵
  if (i == 10 || i == ENTER) {
    last_char = i; // 儲存目前的字元
    return ENTER; // 回傳 ENTER 常數
  }

  last_char = 0; // 重置前一個字元

  // 處理 Telnet Escape 序列
  if (i == ESCAPE) { // ESCAPE 是跳脫字元 (通常是 \033)
    int next1 = getch(); // 讀取跳脫字元後的字元
    if (next1 == ERR) { // 檢查 getch() 是否出錯
      leave(); // 呼叫離開函式
      return i; //回傳跳脫字元.
    }

    int next2;
    switch (next1) {
      case '[': // 箭頭鍵的跳脫序列
        next2 = getch(); // 讀取箭頭鍵的編碼
        if (next2 == ERR) { // 檢查 getch() 是否出錯
          leave(); // 呼叫離開函式
          return i; // 回傳跳脫字元
        }
        switch (next2) {
          case 'A': return KEY_UP;   // 上箭頭
          case 'B': return KEY_DOWN; // 下箭頭
          case 'C': return KEY_RIGHT;  // 右箭頭
          case 'D': return KEY_LEFT;  // 左箭頭
          default: return i; // 未知的跳脫序列，回傳原本的字元
        }
      default:
        return i; // 不是箭頭鍵，回傳原本的字元
    }
  }

  return i; // 回傳讀取的字元
}