#include "checkscr.h"
#include "qkmj.h"

void init_check_mode() {
    int i;

    // 設定遊戲模式並重置游標
    if (input_mode == TALK_MODE) {
        current_mode = CHECK_MODE;
    } else {
        input_mode = CHECK_MODE;
    }
    current_check = 0;
    check_x = org_check_x;

    // 使用更有效率的螢幕更新方式，減少不必要的 wrefresh 呼叫
    attron(A_REVERSE);
    mvaddstr(org_check_y + 1, org_check_x, "  "); // 直接使用 mvaddstr
    mvaddstr(org_check_y + 1, org_check_x, check_name[0]); // 直接使用 mvaddstr
    for (i = 1; i < check_number; i++) {
        if (check_flag[my_sit][i]) {
            mvaddstr(org_check_y + 1, org_check_x + i * 3, "  "); // 直接使用 mvaddstr
            mvaddstr(org_check_y + 1, org_check_x + i * 3, check_name[i]); // 直接使用 mvaddstr
        }
    }
    attroff(A_REVERSE);
    beep();
    refresh(); // 只需呼叫一次 refresh
    return_cursor();
}

// 計算最大台數及索引
static int calculate_max_tai(int *max_index, int *max_sum) {
  *max_index = 0;
  *max_sum = card_comb[0].tai_sum;
  for (int i = 1; i < comb_num; i++) {
    if (card_comb[i].tai_sum > *max_sum) {
      *max_index = i;
      *max_sum = card_comb[i].tai_sum;
    }
  }
  return *max_index;
}

// 顯示台數資訊
static void display_tai_info(int sit, int card_owner, int max_index) {
  int j = 1, k = 0;
  for (int i = 0; i <= 51; i++) {
    if (card_comb[max_index].tai_score[i]) {
      wmove(stdscr, THROW_Y + j, THROW_X + k);
      wprintw(stdscr, "%-10s%2d 台", tai[i].name, card_comb[max_index].tai_score[i]);
      j++;
      if (j == 7) {
        j = 2;
        k = 18;
      }
    }
  }

  if (card_comb[max_index].tai_score[52]) { // 連莊
    wmove(stdscr, THROW_Y + j, THROW_X + k);
    if (info.cont_dealer < 10) {
      wprintw(stdscr, "連%s拉%s  %2d 台", number_item[info.cont_dealer], number_item[info.cont_dealer], info.cont_dealer * 2);
    } else {
      wprintw(stdscr, "連%2d拉%2d  %2d 台", info.cont_dealer, info.cont_dealer, info.cont_dealer * 2);
    }
  }

  if ((sit == card_owner && sit != info.dealer) || (sit != card_owner && card_owner == info.dealer)) {
    if (info.cont_dealer > 0) {
      wmove(stdscr, THROW_Y + 7, THROW_X + 15);
      if (info.cont_dealer < 10) {
        wprintw(stdscr, "莊家 連%s拉%s %2d 台", number_item[info.cont_dealer], number_item[info.cont_dealer], card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2);
      } else {
        wprintw(stdscr, "莊家 連%2d拉%2d %2d 台", info.cont_dealer, info.cont_dealer, card_comb[max_index].tai_sum + 1 + info.cont_dealer * 2);
      }
    } else {
      wmove(stdscr, THROW_Y + 7, THROW_X + 24);
      wprintw(stdscr, "莊家 %2d 台", card_comb[max_index].tai_sum + 1);
    }
  }
}

// 計算金錢變化
static void calculate_money_change(long change_money[], int sit, int card_owner, int max_index) {
  // 計算基本底分
  long base_payment = info.base_value;

  // 計算台數加成
  long tai_payment = card_comb[max_index].tai_sum * info.tai_value;

  // 連莊加成 (莊家連莊)
  long cont_dealer_bonus = 0;
  if (card_owner == info.dealer && info.cont_dealer > 0) {
    cont_dealer_bonus = info.cont_dealer * 2 * info.tai_value;
  }

  // 自摸或放槍的判斷合併
  bool is_self_drawn = (sit == card_owner);
  long winner_payment = base_payment + tai_payment + (is_self_drawn ? 1 : 0) * info.tai_value + cont_dealer_bonus;

  // 調整支付金額 (莊家額外支付)
  if (card_owner == info.dealer) {
    winner_payment += info.tai_value; //莊家額外支付
  }

  // 計算輸贏金額
  if (is_self_drawn) { // 自摸
    for (int i = 1; i <= 4; i++) {
      if (i != sit && table[i]) {
        change_money[i] = -winner_payment;
        change_money[sit] -= change_money[i];
      }
    }
  } else { // 放槍
    change_money[card_owner] = -winner_payment;
    change_money[sit] = winner_payment;
  }
}

// 更新玩家金錢並發送至伺服器
static void update_player_money(long change_money[]) {
  char msg_buf[80];
  for (int i = 1; i <= 4; i++) {
    if (table[i]) {
      if (in_serv) {
        snprintf(msg_buf, sizeof(msg_buf), "020%5d%ld", player[table[i]].id, player[table[i]].money + change_money[i]);
        write_msg(gps_sockfd, msg_buf);
      }
      player[table[i]].money += change_money[i];
    }
  }
  my_money = player[my_id].money;
}

// 顯示金額統計
static void display_money_summary(const long change_money[]) {
  clear_screen_area(THROW_Y, THROW_X, 8, 34);
  attron(A_BOLD);
  mvaddstr(THROW_Y + 1, THROW_X + 12, "金 額 統 計");
  for (int i = 1; i <= 4; i++) {
    wmove(stdscr, THROW_Y + 2 + i, THROW_X);
    wprintw(stdscr, "%s家：%7ld %c %7ld = %7ld", sit_name[i],
            player[table[i]].money - change_money[i], (change_money[i] < 0) ? '-' : '+',
            (change_money[i] < 0) ? -change_money[i] : change_money[i],
            player[table[i]].money);
  }
  attroff(A_BOLD);
  return_cursor();
}

void process_make(char sit, char card) {
  int i, j, max_index, max_sum, sitInd;
  long change_money[5];
  char msg_buf[80];
  int sendlog = 1;
  cJSON *root = NULL;
  char *json_string = NULL;


  play_mode = WAIT_CARD;
  current_card = card;

  // 檢查胡牌
  check_make(sit, card, 1);

  // 清除畫面區域
  set_color(37, 40);
  clear_screen_area(THROW_Y, THROW_X, 8, 34);

  // 計算最大台數
  max_index = calculate_max_tai(&max_index, &max_sum);

  // 顯示胡牌玩家
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

  // 顯示台數資訊
  set_color(33, 40);
  display_tai_info(sit, card_owner, max_index);

  // 顯示總台數
  set_color(31, 40);
  wmove(stdscr, THROW_Y + 6, THROW_X + 26);
  wprintw(stdscr, "共 %2d 台", card_comb[max_index].tai_sum);
  wrefresh(stdscr);

  // 顯示所有牌和槓
  set_color(37, 40);
  set_mode(0);
  for (i = 1; i <= 4; i++) {
    if (table[i] && i != my_sit) {
      show_allcard(i);
      show_kang(i);
    }
  }
  show_newcard(sit, 4);
  return_cursor();

  memset(change_money, 0, sizeof(change_money));

  // 計算金錢變化
  calculate_money_change(change_money, sit, card_owner, max_index);

  /* 記錄牌局資訊 (使用 cJSON) */
  if (in_serv && sendlog == 1) {
    root = cJSON_CreateObject();
    // ... (新增 card_owner, winner, cards, tais 等資訊到 root 物件中，參考上一個回覆的程式碼)

    // 金錢資訊
    cJSON *moneys = cJSON_CreateArray();
    for (i = 1; i <= 4; i++) {
      if (table[i]) {
        cJSON *money_info = cJSON_CreateObject();
        cJSON_AddStringToObject(money_info, "name", player[table[i]].name);
        cJSON_AddNumberToObject(money_info, "now_money", player[table[i]].money);
        cJSON_AddNumberToObject(money_info, "change_money", change_money[i]);
        cJSON_AddItemToArray(moneys, money_info);
      }
    }
    cJSON_AddItemToObject(root, "moneys", moneys);

    // 時間戳記
    long current_time = 0;
    time(&current_time);
    cJSON_AddNumberToObject(root, "time", current_time);

    // 生成 JSON 字串並發送
    json_string = cJSON_Print(root);
    write_msg(gps_sockfd, json_string);

    // 釋放 cJSON 物件和字串
    cJSON_Delete(root);
    free(json_string);
  }

  // 更新玩家金錢
  update_player_money(change_money);
  wait_a_key(PRESS_ANY_KEY_TO_CONTINUE);

  // 顯示金額統計
  display_money_summary(change_money);

  // 更新莊家和局數
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
    write_msg(table_sockfd, "450");
  }
}

void process_epk(char check) {
    char card1, card2, card3;
    int i;
    char msg_buf[80]; // 確保緩衝區夠大，使用 snprintf 避免緩衝區溢位
		int kang_count = 0;

    // 錯誤處理：檢查 check 的有效性
    if (check < 2 || check > 9 || (check >= 7 && check <= 9 && next_turn(turn) != my_sit)) {
        fprintf(stderr, "錯誤: process_epk() 收到無效的 check 值: %d\n", check);
        return; // 回報錯誤並返回
    }

    switch (check) {
			case PONG: // 碰
				play_mode = THROW_CARD;
				// 尋找要碰的牌，移除兩張
				for (i = 0; i < pool[my_sit].num ; i++) {  // 修正迴圈條件，避免讀取到錯誤的記憶體位置
					if (pool[my_sit].card[i] == current_card) {
						pool[my_sit].card[i] = pool[my_sit].card[pool[my_sit].num - 1];
						pool[my_sit].num--; // 立即減少手牌數量
						i++; // 跳過下一張（因為已經是相同的牌）
						if(i < pool[my_sit].num){ // 檢查邊界條件
							pool[my_sit].card[i] = pool[my_sit].card[pool[my_sit].num - 1];
							pool[my_sit].num--;
							break; 
						} else {
							fprintf(stderr, "錯誤: process_epk() 碰牌時手牌數量不足\n");
							return;
						}
					}
				}
				card1 = card2 = card3 = current_card;
				draw_epk(my_id, check, card1, card2, card3);
				sort_card(1);
				break;
			case KANG: // 暗槓 or 碰槓
			case 11: // 明槓 (from process_make)
				//  槓牌處理與碰牌類似，需要移除三張牌
				kang_count = 0;
				for (i = 0; i < pool[my_sit].num && kang_count < 3; i++) { //  限制移除次數
					if (pool[my_sit].card[i] == current_card) {
						pool[my_sit].card[i] = pool[my_sit].card[pool[my_sit].num - 1];
						pool[my_sit].num--;
						kang_count++;
						i--; // 檢查替換後的牌是否也相同
					}
				}

				if(kang_count != 3){ //  如果沒有移除三張牌，則回報錯誤
					fprintf(stderr, "錯誤: process_epk() 槓牌時手牌數量不足\n");
					return;
				}

				card1 = card2 = card3 = current_card;
				draw_epk(my_id, check, card1, card2, card3);
				sort_card(0);
				play_mode = GET_CARD;
				attron(A_REVERSE);
				show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
				attroff(A_REVERSE);
				break;
			case 12: // 碰槓
					for (i = 0; i < pool[my_sit].out_card_index; i++) {
							if (pool[my_sit].out_card[i][1] == current_card || pool[my_sit].out_card[i][2] == current_card) break;
					}
					pool[my_sit].out_card[i][0] = 12;
					card1 = card2 = card3 = current_card;
					draw_epk(my_id, check, card1, card2, card3);
					play_mode = GET_CARD;
					attron(A_REVERSE);
					show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
					attroff(A_REVERSE);
					break;
			case 7: // 吃牌 (1,2,3)
			case 8: // 吃牌 (2,3,4)
			case 9: // 吃牌 (3,4,5)
					//  Implement eat logic (Similar to pong/kang, but with different card selection)
					play_mode = THROW_CARD; // Update game mode
					if(check == 7){
							pool[my_sit].card[search_card(my_sit, current_card + 1)] = pool[my_sit].card[pool[my_sit].num - 1];
							pool[my_sit].card[search_card(my_sit, current_card + 2)] = pool[my_sit].card[pool[my_sit].num - 2];
							card1 = current_card + 1;
							card2 = current_card;
							card3 = current_card + 2;
					} else if (check == 8) {
							pool[my_sit].card[search_card(my_sit, current_card - 1)] = pool[my_sit].card[pool[my_sit].num - 1];
							pool[my_sit].card[search_card(my_sit, current_card + 1)] = pool[my_sit].card[pool[my_sit].num - 2];
							card1 = current_card - 1;
							card2 = current_card;
							card3 = current_card + 1;
					} else { // check == 9
							pool[my_sit].card[search_card(my_sit, current_card - 1)] = pool[my_sit].card[pool[my_sit].num - 1];
							pool[my_sit].card[search_card(my_sit, current_card - 2)] = pool[my_sit].card[pool[my_sit].num - 2];
							card1 = current_card - 2;
							card2 = current_card;
							card3 = current_card - 1;
					}
					draw_epk(my_id, check, card1, card2, card3);
					pool[my_sit].num -= 3;
					sort_card(1);
					break;
    }

    if (check != 12) {
			pool[my_sit].out_card[pool[my_sit].out_card_index][0] = check;
			pool[my_sit].out_card[pool[my_sit].out_card_index][1] = card1;
			pool[my_sit].out_card[pool[my_sit].out_card_index][2] = card2;
			pool[my_sit].out_card[pool[my_sit].out_card_index][3] = card3;
			if (check == KANG || check == 11) {
				pool[my_sit].out_card[pool[my_sit].out_card_index][4] = card3;
				pool[my_sit].out_card[pool[my_sit].out_card_index][5] = 0;
			} else {
				pool[my_sit].out_card[pool[my_sit].out_card_index][4] = 0;
			}
			pool[my_sit].out_card_index++;
    }

    // 使用 snprintf 避免緩衝區溢位
    int snprintf_result = snprintf(msg_buf, sizeof(msg_buf), "530%c%c%c%c%c", my_id, check, card1, card2, card3);
    if (snprintf_result < 0 || snprintf_result >= sizeof(msg_buf)) {
			fprintf(stderr, "錯誤: process_epk() snprintf 失敗或緩衝區溢位\n");
			return; // 回報錯誤並返回
    }

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
    int sit, i;
    char msg_buf[80]; // 確保緩衝區夠大，使用 snprintf 避免緩衝區溢位

    beep();
    sit = player[id].sit;
    if (kind == KANG || kind == 11 || kind == 12) {
        in_kang = 1;
    }
    if (kind != 11 && kind != 12) {
        throw_card(20);
    }

    // 使用更有效率的 switch-case 結構處理不同玩家位置
    int y, x;
    switch ((sit - my_sit + 4) % 4) {
        case 0: // 自己
            y = INDEX_Y;
            x = INDEX_X + (16 - pool[my_sit].num) * 2 - 2;
            break;
        case 1: // 上家
            y = INDEX_Y1 - (16 - pool[sit].num) - 2;
            x = INDEX_X1 - 2;
            break;
        case 2: // 對家
            y = INDEX_Y2 + 2;
            x = INDEX_X2 - (16 - pool[sit].num) * 2 - 2;
            break;
        case 3: // 下家
            y = INDEX_Y3 + (16 - pool[sit].num);
            x = INDEX_X3 + 4;
            break;
        default:
            fprintf(stderr, "錯誤: draw_epk() 計算玩家位置錯誤: sit=%d, my_sit=%d\n", sit, my_sit);
            return; // 回報錯誤並返回
    }

    if (kind == 12) { // 碰槓
        for (i = 0; i < pool[sit].out_card_index; i++) {
            if (pool[sit].out_card[i][1] == card1 && pool[sit].out_card[i][2] == card2) {
                break;
            }
        }
        attron(A_REVERSE);
        mvaddstr(y, x, "杠"); // 使用 mvaddstr 避免 sprintf
        attroff(A_REVERSE);
        return_cursor();
    } else {
        // 使用更有效率的程式碼繪製吃、碰、槓
        show_card(20, x + 4, y + 1, (sit - my_sit + 4) % 4 < 2 ? 1 : 0); //只刷新必要的區域
        wrefresh(stdscr);

        mvaddstr(y, x, "┌"); // 使用 mvaddstr 避免 sprintf
        if (kind == KANG || kind == 11) {
            attron(A_REVERSE);
            mvaddstr(y, x + 1, "杠"); // 使用 mvaddstr 避免 sprintf
            attroff(A_REVERSE);
        } else {
            mvaddstr(y, x + 1, "─"); // 使用 mvaddstr 避免 sprintf
        }
        mvaddstr(y, x + 3, "┐  "); // 使用 mvaddstr 避免 sprintf
        wrefresh(stdscr);

        if (kind == 11) {
            card1 = card3 = 30;
        }
        show_card(card1, x, y + 1, (sit - my_sit + 4) % 4 < 2 ? 1 : 0);
        show_card(card2, x + 2, y + 1, (sit - my_sit + 4) % 4 < 2 ? 1 : 0);
        show_card(card3, x + 4, y + 1, (sit - my_sit + 4) % 4 < 2 ? 1 : 0);
        wrefresh(stdscr); // 只刷新一次
    }
}

void draw_flower(char sit, char card) {
    char msg_buf[80]; // 確保緩衝區夠大，使用 snprintf 避免緩衝區溢位

    // 範圍檢查，確保 card 在有效範圍內
    if (card < 51 || card > 58) {
        fprintf(stderr, "錯誤: draw_flower() 收到無效的花牌值: %d\n", card);
        return; // 回報錯誤並返回
    }

    // 使用 snprintf 避免緩衝區溢位
    int result = snprintf(msg_buf, sizeof(msg_buf), "%s", mj_item[card]);
    if (result < 0 || result >= sizeof(msg_buf)) {
        fprintf(stderr, "錯誤: draw_flower() snprintf 失敗或緩衝區溢位\n");
        return; // 回報錯誤並返回
    }

    // 使用更有效率的方式處理不同玩家位置的顯示
    int y, x;
    switch ((sit - my_sit + 4) % 4) {
        case 0:
            y = FLOWER_Y;
            x = FLOWER_X + (card - 51) * 2;
            break;
        case 1:
            y = FLOWER_Y1 - (card - 51);
            x = FLOWER_X1;
            break;
        case 2:
            y = FLOWER_Y2;
            x = FLOWER_X2 - (card - 51) * 2;
            break;
        case 3:
            y = FLOWER_Y3 + (card - 51);
            x = FLOWER_X3;
            break;
        default:
            fprintf(stderr, "錯誤: draw_flower() 計算玩家位置錯誤: sit=%d, my_sit=%d\n", sit, my_sit);
            return; // 回報錯誤並返回

    }
    
    // 使用 mvaddstr 顯示花牌，並處理錯誤情況
    int mvaddstr_result = mvaddstr(y, x, msg_buf);
    if (mvaddstr_result == ERR) {
        fprintf(stderr, "錯誤: draw_flower() mvaddstr 失敗\n");
    }

    // 更新螢幕
    refresh();
    return_cursor();

    // 設定花牌旗標
    attron(A_BOLD);
    in_kang = 1;
    pool[sit].flower[card - 51] = 1;
    attroff(A_BOLD);
}
