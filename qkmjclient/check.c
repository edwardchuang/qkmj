#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
	if (card == 0) return 0;  // 及早返回

	for (int i = 0; i < pool[sit].num; i++) {
		if (pool[sit].card[i] == card) return i; // 找到牌後立即返回
	}
	return -1;
}

int check_kang(char sit, char card) {
	if (card == 0) return 0; // 及早返回

	// 優化：直接檢查是否有四張相同的牌
	int count = 0;
	for (int i = 0; i < pool[sit].num; i++) {
		if (pool[sit].card[i] == card) {
			count++;
			if(count == 4) return 1; // 找到暗槓
		}
	}

	// 檢查碰槓。此迴圈可能較慢，可考慮使用更有效率的資料結構儲存 out_card
	for (int i = 0; i < pool[sit].out_card_index; i++) {
		if (pool[sit].out_card[i][0] == 2 && pool[sit].out_card[i][1] == card && sit == card_owner) {
			return 2;
		}
	}

	return 0; // 無法槓
}

int check_pong(char sit, char card) {
	if (card == 0) return 0;

	int count = 0;
	for (int i = 0; i < pool[sit].num; i++) {
		if (pool[sit].card[i] == card) {
			count++;
			if (count == 2) return 1;  // 找到碰
		}
	}

	return 0;  // 無法碰
}

int check_eat(char sit, char card) {
	// 檢查玩家是否為下家且牌的範圍有效
	if (next_turn(turn) != sit || card < 1 || card > 29) {
		return 0; // 不是下家或牌無效，及早返回
	}

	int j = card % 10;

	// 使用 if-else 結構簡化邏輯，避免 switch statement
	if (search_card(sit, card + 1) >= 0 && search_card(sit, card + 2) >= 0) { // 檢查 1, 2, 8, 9
		if (j == 1 || j == 2 || j == 8 || j == 9) return 1;
	}
	if (search_card(sit, card - 1) >= 0 && search_card(sit, card + 1) >= 0) { // 檢查 2, 3-8
		if (j >= 2 && j <= 8) return 1;
	}
	if (search_card(sit, card - 2) >= 0 && search_card(sit, card - 1) >= 0) { // 檢查 3-9
		if (j >= 3 && j <= 9) return 1;
	}

	return 0; // 無法吃牌
}
 
void check_card(char sit, char card) { // 使用 char 類型參數
	clear_check_flag(sit);

	// 呼叫檢查函式並設定對應的檢查旗標
	check_flag[sit][1] = check_eat(sit, card);
	check_flag[sit][2] = check_pong(sit, card);
	check_flag[sit][3] = check_kang(sit, card);
	check_flag[sit][4] = check_make(sit, card, 0);
}

int check_begin_flower(char sit, char card, char position) {  /* command for server */
	char msg_buf[80];  // 確保緩衝區夠大

	// 使用範圍檢查，避免不必要的計算
	if (card >= 51 && card <= 58) {
		snprintf(msg_buf, sizeof(msg_buf), "525%c%c", sit, pool[sit].card[position]); // 使用 snprintf 確保安全，避免緩衝區溢位
		broadcast_msg(1, msg_buf);
		draw_flower(sit, card);
		card = mj[card_point++];
		if (sit == my_sit) {
			change_card(position, card);
		} else {
			pool[sit].card[position] = card;
			snprintf(msg_buf, sizeof(msg_buf), "301%c%c", position, card);  // 使用 snprintf 確保安全，避免緩衝區溢位
			write_msg(player[table[sit]].sockfd, msg_buf);
		}
		return 1; // 找到花牌
	} else {
		return 0; // 不是花牌
	}
}

int check_flower(char sit, char card) {
  char msg_buf[80];  // 確保緩衝區夠大

	// 使用範圍檢查，避免不必要的計算
	if (card >= 51 && card <= 58) {
		snprintf(msg_buf, sizeof(msg_buf), "525%c%c", sit, card); // 使用 snprintf 確保安全，避免緩衝區溢位
		draw_flower(sit, card);

		if (in_join) {
			write_msg(table_sockfd, msg_buf);
		} else {
			broadcast_msg(1, msg_buf);
			card = mj[card_point++];
			show_num(2, 70, 144 - card_point - 16, 2);
			card_owner = my_sit;

			snprintf(msg_buf, sizeof(msg_buf), "305%c", (char)my_sit); // 使用 snprintf 確保安全，避免緩衝區溢位
			broadcast_msg(1, msg_buf);

			snprintf(msg_buf, sizeof(msg_buf), "306%c", card_point);  // 使用 snprintf 確保安全，避免緩衝區溢位
			broadcast_msg(1, msg_buf);

			play_mode = GET_CARD;
			attron(A_REVERSE);
			show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
			attroff(A_REVERSE);
			return_cursor();
		}
		return 1; // 找到花牌
	} else {
		return 0; // 不是花牌
	}
}

void write_check(char check) {
	char msg_buf[80];  // 確保緩衝區夠大

	if (in_join) {
		snprintf(msg_buf, sizeof(msg_buf), "510%c", check + '0'); // 使用 snprintf 確保安全，避免緩衝區溢位
		write_msg(table_sockfd, msg_buf);
	} else {
		in_check[player[1].sit] = 0;
		check_for[player[1].sit] = check;
	}
}

void send_pool_card() {
	int j;
	// 大幅增加緩衝區大小，以容納所有玩家數據 + 訊息標頭。
	// 計算安全的緩衝區大小。 例如：3 (標頭) + 4 個玩家 * 每個玩家 16 張牌 + 1 (空終止符)
	char msg_buf[3 + (4 * 16) + 1];  // 或定義一個常數表示此訊息大小

	snprintf(msg_buf, 3, "521");  // 新增訊息標頭 "521"

	for (int player_index = 1; player_index <= 4; player_index++) {
		for (j = 0; j < 16; j++) {
			// 計算 msg_buf 內的正確偏移量
			int offset = 3 + (player_index - 1) * 16 + j;

			// 確保我們不會寫入超出緩衝區邊界！
			if (offset < sizeof(msg_buf)) {
				msg_buf[offset] = pool[player_index].card[j];
			} else {
				// 錯誤處理 - 記錄或回報錯誤。緩衝區太小！
				fprintf(stderr, "錯誤: send_pool_card() 緩衝區溢位！\n");
				return; // 或以更適合您的應用程序的方式處理錯誤
			}
		}
	}

	msg_buf[sizeof(msg_buf) - 1] = '\0';  // 空終止字串，確保是有效的 C 字串

	broadcast_msg(1, msg_buf);
}

void compare_check() {
	int i, j;
	char msg_buf[80];

	next_player_request = 0;

	// 優化：合併迴圈，減少迭代次數
	i = next_turn(card_owner); // 從出牌者的下家開始檢查
	for (j = 0; j < 4; j++) {
		// 檢查胡牌
		if (check_for[i] == MAKE) {
			send_pool_card();
			snprintf(msg_buf, sizeof(msg_buf), "522%c%c", i, current_card); // 使用 snprintf 防止緩衝區溢位
			broadcast_msg(1, msg_buf);
			process_make(i, current_card);
			return; // 胡牌後直接返回，無需繼續檢查
		} else if (check_for[i] == KANG || check_for[i] == PONG || (check_for[i] >= 7 && check_for[i] <= 9)) {
			card_owner = i;
			if (table[i] == 1) { // 本機玩家
				// 根據不同的 check_for 值呼叫 process_epk
				switch (check_for[i]) {
					case KANG:
						process_epk(search_card(i, current_card) >= 0 ? (turn == i ? 11 : KANG) : 12);
						break;
					case PONG:
						process_epk(PONG);
						break;
					default: // 吃牌 (7, 8, 9)
						process_epk(check_for[i]);
				}
			} else { // 遠端玩家
				char action = 0;
				switch(check_for[i]) {
					case KANG:
						action = search_card(i,current_card) >=0 ? (turn ==i ? 11 : KANG) : 12;
						break;
					case PONG:
						action = PONG;
						break;
					default: // 吃牌
						action = check_for[i];
				}
				snprintf(msg_buf, sizeof(msg_buf), "520%c", action); // 使用 snprintf 防止緩衝區溢位
				write_msg(player[table[i]].sockfd, msg_buf);

				show_newcard(i, check_for[i] == KANG ? 1 : 2); // 槓牌更新一張牌，碰/吃更新兩張牌
				return_cursor();
			}

			turn = i;
			snprintf(msg_buf, sizeof(msg_buf), "314%c%c", i, check_for[i] == KANG ? 1 : 2); // 使用 snprintf 防止緩衝區溢位
			broadcast_msg(table[i], msg_buf);
			return; // 槓/碰/吃後直接返回
		}

		i = next_turn(i); // 檢查下一個玩家
	}

	if (!getting_card) {
		next_player_request = 1;
	}

	// 清除所有玩家的 check_for 標記 (使用 memset 更有效率)
  memset(check_for, 0, sizeof(check_for)); 
  getting_card = 0;
}
