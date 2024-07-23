#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "command.h"
#include "qkmj.h"

int convert_msg_id(unsigned char *msg, int player_id) {
  // 檢查訊息 ID 是否為有效的 3 位數
  if (msg[0] < '0' || msg[0] > '9' ||
      msg[1] < '0' || msg[1] > '9' ||
      msg[2] < '0' || msg[2] > '9') {
    char msg_buf[1024];
    display_comment("無效的訊息 ID");
    // 使用 snprintf 避免緩衝區溢出
    snprintf(msg_buf, sizeof(msg_buf), "來自玩家 %d (%d) 長度=%lu 訊息內容: %s",
             player_id, gps_sockfd, strlen((char *)msg), msg);
    display_comment(msg_buf);
    return -1; // 錯誤
  }
  // 將訊息 ID 轉換為整數
  return (msg[0] - '0') * 100 + (msg[1] - '0') * 10 + (msg[2] - '0');
}

void process_msg(int player_id, unsigned char* id_buf, int msg_type) {
  int msg_id;
  unsigned char buf[255];
  char msg_buf[1024];
  char ans_buf[255];
  char ans_buf1[255];
  int i, j, sit;
  bool check_mode_active = false;

  strncpy((char *)buf, (char *)id_buf, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  msg_id = convert_msg_id(id_buf, player_id);
  if (msg_id == -1) {
    return;  // Handle invalid message ID
  }

  switch (msg_type) {
    case FROM_GPS:
      // 處理來自 GPS Server 的訊息
      if (msg_id != 102) {
        // 讀取訊息內容
        read_msg(gps_sockfd, (char *)buf + 3);
      }
      switch (msg_id) {
        case 2:
          // 處理密碼驗證
          if (my_pass[0] != 0) {
            // 使用 strncpy 避免緩衝區溢出
            strncpy(ans_buf, (char *)my_pass, sizeof(ans_buf) - 1);
            ans_buf[sizeof(ans_buf) - 1] = '\0';
          } else {
            // 詢問密碼
            ans_buf[0] = 0;
            ask_question("請輸入你的密碼：", ans_buf, sizeof(ans_buf) - 1, 0);
          }
          // 組成訊息並發送至 GPS Server
          snprintf(msg_buf, sizeof(msg_buf), "102%s", ans_buf);
          write_msg(gps_sockfd, msg_buf);
          // 更新本地密碼
          strncpy((char *)my_pass, ans_buf, sizeof(my_pass) - 1);
          my_pass[sizeof(my_pass) - 1] = '\0';
          break;
        case 3:
          // 處理登入成功
          pass_login = 1;
          input_mode = TALK_MODE;
          display_comment("請打 /HELP 查看簡單指令說明，/Exit 離開");
          snprintf(msg_buf, sizeof(msg_buf), "004%s", my_note);
          write_msg(gps_sockfd, msg_buf);
          break;
        case 4:
          // 處理密碼錯誤
          pass_count++;
          my_pass[0] = 0;
          if (pass_count == 3) {
            leave();
          }
          // 重新輸入名稱
          do {
            ans_buf[0] = 0;
            ask_question("密碼錯誤! 請重新輸入你的名稱：", ans_buf, sizeof(ans_buf) - 1, 1);
          } while (ans_buf[0] == 0);
          snprintf(msg_buf, sizeof(msg_buf), "101%s", ans_buf);
          write_msg(gps_sockfd, msg_buf);
          strncpy((char *)my_name, ans_buf, sizeof(my_name) - 1);
          my_name[sizeof(my_name) - 1] = '\0';
          break;
        case 5:
          // 處理建立新帳戶
          ans_buf[0] = 0;
          ask_question("看來你是個新朋友, 你要使用這個名稱嗎？(y/N)：",
                        ans_buf, sizeof(ans_buf) - 1, 1);
          if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {
            ans_buf[0] = 0;
            ask_question("看來你是個新朋友, 你要使用這個名稱嗎？(y/N)：",
                        ans_buf, sizeof(ans_buf) - 1, 1);
            if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {
              char password[9]; // 密碼長度限制為 8 個字元
              do {
                ans_buf[0] = 0;
                ask_question("請輸入你的密碼：", password, sizeof(password) - 1, 0);
                password[sizeof(password) - 1] = 0; // 確保結尾為空字元
                ans_buf1[0] = 0;
                ask_question("請再輸入一次確認：", ans_buf1, sizeof(ans_buf1) - 1, 0);
                ans_buf1[sizeof(ans_buf1) - 1] = 0; // 確保結尾為空字元
              } while (strncmp(password, ans_buf1, sizeof(password)) != 0);
              snprintf(msg_buf, sizeof(msg_buf), "103%s", password);
              write_msg(gps_sockfd, msg_buf);
              strncpy((char *)my_pass, password, sizeof(my_pass) - 1);
              my_pass[sizeof(my_pass) - 1] = '\0';
            } else {
              do {
                ans_buf[0] = 0;
                ask_question("請重新輸入你的名稱：", ans_buf, sizeof(ans_buf) - 1, 1);
              } while (ans_buf[0] == 0);
              snprintf(msg_buf, sizeof(msg_buf), "101%s", ans_buf);
              write_msg(gps_sockfd, msg_buf);
              strncpy((char *)my_name, ans_buf, sizeof(my_name) - 1);
              my_name[sizeof(my_name) - 1] = '\0';
            }
            break;
          } else {
            do {
              ans_buf[0] = 0;
              ask_question("請重新輸入你的名稱：", ans_buf, sizeof(ans_buf) - 1, 1);
            } while (ans_buf[0] == 0);
            snprintf(msg_buf, sizeof(msg_buf), "101%s", ans_buf);
            write_msg(gps_sockfd, msg_buf);
            strncpy((char *)my_name, ans_buf, sizeof(my_name) - 1);
            my_name[sizeof(my_name) - 1] = '\0';
          }
          break;
        case 6:
          // 處理重複登入
          ans_buf[0] = 0;
          ask_question("重覆進入! 你要殺掉另一個帳號嗎? (y/N)：", ans_buf, sizeof(ans_buf) - 1, 1);
          if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {
            write_msg(gps_sockfd, "105");
          } else {
            leave();
          }
          break;
        case 10:
          // 處理離線
          leave();
          break;
        case 11:
          // 處理加入牌桌
          switch (buf[3]) {
            case '0':
              // 處理加入牌桌成功
              Tokenize((char *)buf + 4);
              send_gps_line("與該桌連線中...");
              // 使用 init_socket 函式初始化牌桌連線
              int ret = init_socket((char *)cmd_argv[1], atoi((char *)cmd_argv[2]),
                                      &table_sockfd);
              // 將牌桌連線加入監聽集合
              FD_SET(table_sockfd, &afds);
              in_join = 1;
              break;
            case '1':
              // 處理找不到牌桌
              send_gps_line("查無此桌，請重新再試。（可用 /FREE 查詢空桌）");
              break;
            case '2':
              // 處理連線失敗
              send_gps_line("無法連線");
              break;
          }
          break;
        case 12:
          // 處理建立新牌桌
          init_serv_socket();
          snprintf(msg_buf, sizeof(msg_buf), "012%d", SERV_PORT - 1);
          write_msg(gps_sockfd, msg_buf);
          my_id = 1;
          in_serv = 1;
          on_seat = 0;
          player_in_table = 1;
          player[1].sit = 1;
          player[1].money = my_money;
          player[1].id = my_gps_id;
          strncpy(player[1].name, (char *)my_name, sizeof(player[1].name) - 1);
          player[1].name[sizeof(player[1].name) - 1] = '\0';
          my_sit = 1;
          // 初始化牌桌狀態
          for (i = 0; i <= 4; i++) {
            table[i] = 0;
          }
          table[1] = 1;
          if (player_in_table == PLAYER_NUM) {
            // 處理牌桌人數已滿
            init_playing_screen();
            opening();
            open_deal();
          }
          strncpy(player[1].name, (char *)my_name, sizeof(player[1].name) - 1);
          player[1].name[sizeof(player[1].name) - 1] = '\0';
          player[1].in_table = 1;
          send_gps_line("您已建立新桌，目前人數1人，可使用 /who 查詢本桌清單");
          send_gps_line("如要關桌請輸入 /Leave (/L) 踢除使用者請用 /Kick ");
          send_gps_line(
              "請用 /Note <附註> 設定附註，其他人查詢空桌時將可參考。");
          break;
        case 101:
          // 處理來自 GPS Server 的訊息
          send_gps_line((char *)buf + 3);
          break;
        case 102:
          // 處理來自 GPS Server 的訊息
          display_news(gps_sockfd);
          break;
        case 120:
          // 處理來自 GPS Server 的訊息
          // 檢查 buf 是否有足夠空間存放空字元
          if (sizeof(buf) < 9) {
            // 處理緩衝區溢出
            break;
          }
          // 使用 strncpy 避免緩衝區溢出
          strncpy(msg_buf, (char *)buf + 3, 5);
          msg_buf[5] = '\0';
          new_client_id = atoi(msg_buf);
          new_client_money = atol((char *)buf + 8);
          if (!in_serv) {
            my_gps_id = new_client_id;
            my_money = new_client_money;
          }
          break;
        case 200:
          // 處理來自 GPS Server 的訊息
          close(gps_sockfd);
          endwin();
          break;
        case 211:
          // 處理來自 GPS Server 的訊息
          strncpy(new_client_name, (char *)buf + 3,
                  sizeof(new_client_name) - 1);
          new_client_name[sizeof(new_client_name) - 1] = '\0';
          new_client = 1;
          break;
        default:
          // 處理未知的訊息 ID
          snprintf(msg_buf, sizeof(msg_buf), "msg_id=%d", msg_id);
          display_comment(msg_buf);
      }
      break;
    case FROM_CLIENT:
      // 處理來自客戶端的訊息
      read_msg(player[player_id].sockfd, (char *)buf + 3);
      switch (msg_id) {
        case 101:
          // 處理來自客戶端的訊息
          send_gps_line((char *)buf + 3);
          broadcast_msg(player_id, (char *)buf);
          break;
        case 102:
          // 處理來自客戶端的訊息
          display_comment((char *)buf + 3);
          broadcast_msg(player_id, (char *)buf);
          break;
        case 200:
          // 處理其他玩家離開
          close_client(player_id);
          break;
        case 290:
          // 處理其他玩家離開
          broadcast_msg(player_id, (char *)buf);
          opening();
          open_deal();
          break;
        case 313:
          // 處理發送牌給客戶端
          send_card_request = 1;
          break;
        case 315:
          // 處理客戶端完成動作
          // next_player_request=1;
          break;
        case 401:
          // 處理其他玩家丟棄牌
          pool[player[player_id].sit].time += thinktime();
          display_time(player[player_id].sit);
          snprintf(msg_buf, sizeof(msg_buf), "312%c%f",
                   player[player_id].sit,
                   pool[player[player_id].sit].time);
          broadcast_msg(1, msg_buf);
          pool[player[player_id].sit].first_round = 0;
          in_kang = 0;
          show_newcard(player[player_id].sit, 3);
          snprintf(msg_buf, sizeof(msg_buf), "314%c%c",
                   player[player_id].sit, 3);
          broadcast_msg(player_id, msg_buf);
          snprintf(msg_buf, sizeof(msg_buf), "402%c%c", player_id, buf[3]);
          broadcast_msg(player_id, msg_buf);
          current_id = player_id;
          current_card = buf[3];
          show_cardmsg(player[player_id].sit, buf[3]);
          throw_card(buf[3]);
          return_cursor();
          sit = player[player_id].sit;
          // 移除丟棄的牌
          for (i = 0; i < pool[sit].num; i++) {
            if (pool[sit].card[i] == current_card) {
              break;
            }
          }
          pool[sit].card[i] = pool[sit].card[pool[sit].num];
          sort_pool(sit);
          check_on = 1;
          send_card_on = 0;
          next_player_on = 0;
          // 檢查其他玩家是否可以吃碰槓
          for (i = 1; i <= 4; i++) {
            if (table[i] > 0 && table[i] != player_id) {
              check_card(i, buf[3]);
            }
          }
          // 通知其他玩家可以檢查
          for (i = 1; i <= 4; i++) {
            if (table[i] != 1 && i != turn) {
              for (j = 1; j < check_number; j++) {
                if (check_flag[i][j]) {
                  snprintf(msg_buf, sizeof(msg_buf), "501%c%c%c%c",
                           check_flag[i][1] + '0', check_flag[i][2] + '0',
                           check_flag[i][3] + '0', check_flag[i][4] + '0');
                  write_msg(player[table[i]].sockfd, msg_buf);
                  in_check[i] = 1;
                  check_mode_active = true;
                  break; // 檢查下一個玩家
                } else {
                  in_check[i] = 0;
                }
              }
            }
          }
          if (check_mode_active) {
            init_check_mode();
            in_check[1] = 1;
          } else {
            in_check[1] = 0;
          }
          break;
        case 450:
          wait_hit[player[player_id].sit] = 1;
          break;
        case 501:
          //who(player[player_id].sockfd);
          list_players();
          break;
        case 510:
          in_check[player[player_id].sit] = 0;
          check_for[player[player_id].sit] = buf[3] - '0';
          break;
        case 515:
          next_player_request = 0;
          turn = player[player_id].sit;
          snprintf(msg_buf, sizeof(msg_buf), "310%c", turn);
          broadcast_msg(player_id, msg_buf);
          display_point(turn);
          return_cursor();
          break;
        case 525:
          send_card_request = 1;
          draw_flower(buf[3], buf[4]);
          broadcast_msg(player_id, (char *)buf);
          break;
        case 530:
          // 其他玩家吃碰槓
          gettimeofday(&before, (struct timezone*)0);
          turn = player[buf[3]].sit;
          display_point(turn);
          if (buf[4] == 12) {
            // 處理槓牌
            for (i = 0; i < pool[turn].out_card_index; i++) {
              if (pool[turn].out_card[i][0] == 2 &&
                  pool[turn].out_card[i][1] == buf[5]) {
                pool[turn].out_card[i][0] = 12;
                pool[turn].out_card[i][4] = buf[5];
                pool[turn].out_card[i][5] = 0;
                break;
              }
            }
          } else {
            // 處理吃碰
            for (i = 0; i <= 3; i++) {
              pool[turn].out_card[pool[turn].out_card_index][i] = buf[i + 4];
            }
            if (buf[4] == 3 || buf[4] == 11) { // 槓牌
              pool[turn].out_card[pool[turn].out_card_index][4] = buf[7];
              pool[turn].out_card[pool[turn].out_card_index][5] = 0;
            } else {
              pool[turn].out_card[pool[turn].out_card_index][4] = 0;
            }
            pool[turn].out_card_index++;
          }
          draw_epk(buf[3], buf[4], buf[5], buf[6], buf[7]);
          broadcast_msg(player_id, (char *)buf);
          return_cursor();
          switch (buf[4]) {
            case 2:
              // 處理碰牌
              for (i = 0; i < pool[turn].num; i++) {
                if (pool[turn].card[i] == buf[6]) {
                  pool[turn].card[i] = pool[turn].card[pool[turn].num - 1];
                  pool[turn].card[i + 1] = pool[turn].card[pool[turn].num - 2];
                  break;
                }
              }
              pool[turn].num -= 3;
              sort_pool(turn);
              break;
            case 3:
            case 11:
              // 處理槓牌
              for (i = 0; i < pool[turn].num; i++) {
                if (pool[turn].card[i] == buf[6]) {
                  pool[turn].card[i] = pool[turn].card[pool[turn].num - 1];
                  pool[turn].card[i + 1] = pool[turn].card[pool[turn].num - 2];
                  pool[turn].card[i + 2] = pool[turn].card[pool[turn].num - 3];
                  break;
                }
              }
              pool[turn].num -= 3;
              sort_pool(turn);
              break;
            case 7:
            case 8:
            case 9:
              // 處理胡牌
              pool[turn].card[search_card(turn, buf[5])] =
                  pool[turn].card[pool[turn].num - 1];
              pool[turn].card[search_card(turn, buf[7])] =
                  pool[turn].card[pool[turn].num - 2];
              pool[turn].num -= 3;
              sort_pool(turn);
              break;
          }
          break;
        default:
          break;
      }
      break;
    case FROM_SERV:
      read_msg(table_sockfd, (char *)buf + 3);
      switch (msg_id) {
        case 101:
          send_gps_line((char *)buf + 3);
          break;
        case 102:
          display_comment((char *)buf + 3);
          break;
        case 199:  // LEAVE 開桌者離開牌桌
          write_msg(gps_sockfd, "205");  // 通知 GPS Server
          close(table_sockfd);
          FD_CLR(table_sockfd, &afds);
          in_join = 0;
          input_mode = TALK_MODE;
          init_global_screen();
          display_comment("開桌者已離開牌桌");
          display_comment("-------------------");
          write_msg(gps_sockfd,
                    "201");  // 更新一下目前線上人數跟內容
          break;
        case 200:  // LEAVE 離開牌桌
          write_msg(gps_sockfd, "205");  // 通知 GPS Server
          close(table_sockfd);
          FD_CLR(table_sockfd, &afds);
          in_join = 0;
          input_mode = TALK_MODE;
          init_global_screen();
          write_msg(gps_sockfd,
                    "201");  // 更新一下目前線上人數跟內容
          break;
        case 201: {  // get the new comer's info
          strncpy(player[buf[3]].name, (char *)buf + 6,
                  sizeof(player[buf[3]].name) - 1);
          player[buf[3]].name[sizeof(player[buf[3]].name) - 1] = '\0';
          player[buf[3]].in_table = 1;
          player[buf[3]].sit = buf[4];
          player_in_table = buf[5];
          if (strncmp((char *)my_name, player[buf[3]].name, sizeof(my_name)) == 0) {
            snprintf(msg_buf, sizeof(msg_buf), "您已加入此桌，目前人數 %d ",
                     player_in_table);
          } else {
            snprintf(msg_buf, sizeof(msg_buf),
                     "%s 加入此桌，目前人數 %d ",
                     player[buf[3]].name, player_in_table);
          }
          send_gps_line(msg_buf);
          if (player[buf[3]].sit) {
            table[player[buf[3]].sit] = buf[3];
          }
          break;
        }
        case 202:
          strncpy(msg_buf, (char *)buf + 4, 5);
          msg_buf[5] = '\0';
          player[buf[3]].id = atoi(msg_buf);
          player[buf[3]].money = atol((char *)buf + 9);
          break;
        case 203:  // get others info
          strncpy(player[buf[3]].name, (char *)buf + 5,
                  sizeof(player[buf[3]].name) - 1);
          player[buf[3]].name[sizeof(player[buf[3]].name) - 1] = '\0';
          player[buf[3]].sit = buf[4];
          player[buf[3]].in_table = 1;
          table[buf[4]] = buf[3];
          break;
        case 204:
          for (i = 1; i <= 4; i++) {
            table[i] = buf[2 + i] - '0';
            if (table[i]) {
              player[table[i]].sit = i;
              on_seat++;
            }
          }
          break;
        case 205:  // NOTICE: need player_in_table++ ?
                   // NOTICE: did he get table[]????
          my_id = buf[3];
          strncpy(player[my_id].name, (char *)buf + 5,
                  sizeof(player[my_id].name) - 1);
          player[my_id].name[sizeof(player[my_id].name) - 1] = '\0';
          my_sit = buf[4];
          player[my_id].sit = my_sit;
          player[my_id].in_table = 1;
          player[my_id].id = my_gps_id;
          player[my_id].money = my_money;
          table[my_sit] = my_id;
          break;
        case 206:
          if (player_in_table == 4) {
            init_global_screen();
            input_mode = TALK_MODE;
          }
          player_in_table = buf[4];
          snprintf(msg_buf, sizeof(msg_buf),
                   "%s 離開此桌，目前人數剩下 %d 人",
                   player[buf[3]].name, player_in_table);
          display_comment(msg_buf);
          player[buf[3]].in_table = 0;
          break;
        case 290:
          opening();
          break;
        case 300:
          if (screen_mode == GLOBAL_SCREEN_MODE) {
            init_playing_screen();
          }
          opening();
          break;
        case 301:  // change card
          change_card(buf[3], buf[4]);
          break;
        case 302:  // get 16 cards
          for (i = 0; i < 16; i++) {
            pool[my_sit].card[i] = buf[i + 3];
          }
          sort_card(0);
          break;
        case 303:  // can get a card
          play_mode = GET_CARD;
          attron(A_REVERSE);
          show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
          attroff(A_REVERSE);
          wrefresh(stdscr);
          beep1();
          return_cursor();
          break;
        case 304:  // get a card
          process_new_card(my_sit, buf[3]);
          break;
        case 305:
          card_owner = buf[3];
          if (card_owner != my_sit) {
            show_cardmsg(card_owner, 0);
          }
          break;
        case 306:  // others got a card ---> change the card number
          card_point = buf[3];
          show_num(2, 70, 144 - card_point - 16, 2);
          return_cursor();
          break;
        case 308:  // sort cards
          sort_card(buf[3] - '0');
          break;
        case 310:  // Player pointer ---  point to new player
          turn = buf[3];
          display_point(buf[3]);
          return_cursor();
          break;
        case 312:
          pool[buf[3]].time = atof((char *)buf + 4);
          display_time(buf[3]);
          break;
        case 314:  // process new cardback
          show_newcard(buf[3], buf[4]);
          return_cursor();
          break;
        case 330:  // 海底流局
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
        case 402:  // others throw a card
          in_kang = 0;
          pool[player[buf[3]].sit].first_round = 0;
          show_cardmsg(player[buf[3]].sit, buf[4]);
          current_card = buf[4];
          throw_card(buf[4]);
          return_cursor();
          current_id = buf[3];
          break;
        case 501:  // ask for check card
          go_to_check = 0;
          for (i = 1; i <= 4; i++) {
            check_flag[my_sit][i] = buf[2 + i] - '0';
            if (check_flag[my_sit][i]) {
              go_to_check = 1;
            }
          }
          if (go_to_check) {
            init_check_mode();
          }
          go_to_check = 0;
          break;
        case 518:
          for (i = 1; i <= 4; i++) {
            pool[i].door_wind = buf[2 + i];
          }
          wmvaddstr(stdscr, 2, 64, sit_name[pool[my_sit].door_wind]);
          return_cursor();
          break;
        case 520:
          process_epk(buf[3]);
          break;
        case 521:
          for (i = 0; i < pool[1].num; i++) {
            pool[1].card[i] = buf[3 + i];
          }
          for (i = 0; i < pool[2].num; i++) {
            pool[2].card[i] = buf[19 + i];
          }
          for (i = 0; i < pool[3].num; i++) {
            pool[3].card[i] = buf[35 + i];
          }
          for (i = 0; i < pool[4].num; i++) {
            pool[4].card[i] = buf[51 + i];
          }
          break;
        case 522:
          process_make(buf[3], buf[4]);
          break;
        case 525:
          draw_flower(buf[3], buf[4]);
          break;
        case 530:  // from server
          turn = player[buf[3]].sit;
          card_owner = turn;
          display_point(turn);
          if (buf[4] == 12) {
            for (i = 0; i < pool[turn].out_card_index; i++) {
              if (pool[turn].out_card[i][1] == buf[5] &&
                  pool[turn].out_card[i][2] == buf[6]) {
                pool[turn].out_card[i][0] = 12;
                pool[turn].out_card[i][4] = buf[5];
                pool[turn].out_card[i][5] = 0;
              }
            }
            draw_epk(buf[3], buf[4], buf[5], buf[6], buf[7]);
          } else {
            for (i = 0; i <= 3; i++) {
              pool[turn].out_card[pool[turn].out_card_index][i] = buf[i + 4];
            }
            if (buf[4] == 3 || buf[4] == 11) {
              pool[turn].out_card[pool[turn].out_card_index][4] = buf[7];
              pool[turn]
                  .out_card[pool[turn].out_card_index][5] = 0;
            } else {
              pool[turn].out_card[pool[turn].out_card_index][4] = 0;
            }
            draw_epk(buf[3], buf[4], buf[5], buf[6], buf[7]);
            pool[turn].out_card_index++;
            pool[turn].num -= 3;
          }
          return_cursor();
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}