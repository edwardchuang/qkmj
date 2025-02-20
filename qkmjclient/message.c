#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>

#include "qkmj.h"
#include "message.h"

// 將訊息 ID 字串轉換成整數
int convert_msg_id(const unsigned char *msg, int player_id) {
  char msg_id_str[4]; // 使用固定長度的陣列，確保安全性

  if (strlen((const char*)msg) >= 4) {
    display_comment("convert_msg_id(): error: message ID is too long");
    return -1;
  }

  // 複製訊息 ID 部分到新的字串緩衝區，避免修改原始資料
  strncpy(msg_id_str, (const char*)msg, 3);
  msg_id_str[3] = '\0'; // 確保字串以 null 結束

  // 錯誤檢查：驗證訊息 ID 是否為數字
  for (size_t i = 0; i < 3; ++i) {
    if (!isdigit(msg_id_str[i])) {
      char error_message[255];
      snprintf(error_message, sizeof(error_message),
               "Invalid message ID from player %d: '%s'",
               player_id, msg_id_str);
      display_comment(error_message);
      return -1; // 返回 -1 表示錯誤
    }
  }

  // 使用 atoi 安全地將字串轉換為整數
  return atoi(msg_id_str);
}

// 處理接收到的訊息
void process_msg(int player_id, const unsigned char *id_buf,
                 int msg_type) {
  int msg_id;
  unsigned char buf[MAX_MSG_LEN];
  char msg_buf[MAX_MSG_LEN]; //訊息緩衝區，用於顯示或傳輸訊息

  // 複製訊息，避免直接修改原始資料
  strncpy((char *)buf, (char *)id_buf, MAX_MSG_LEN - 1);
  buf[MAX_MSG_LEN - 1] = '\0'; // 確保字串以 null 結尾

  msg_id = convert_msg_id(id_buf, player_id);
  if (msg_id == -1) return; // 檢查轉換是否成功

  switch (msg_type) {
  case FROM_GPS:
    process_msg_from_gps(player_id, buf, msg_id);
    break;
  case FROM_CLIENT:
    process_msg_from_client(player_id, buf, msg_id);
    break;
  case FROM_SERV:
    process_msg_from_server(player_id, buf, msg_id);
    break;
  default:
    snprintf(msg_buf, MAX_MSG_LEN, "未知的訊息類型: %d", msg_type);
    display_comment(msg_buf); //顯示未知的訊息類型
    break;
  }
}

// 處理來自 GPS 伺服器的訊息
static void process_msg_from_gps(int player_id, unsigned char *buf,
                                 int msg_id) {
  char msg_buf[MAX_MSG_LEN]; //訊息緩衝區
  char ans_buf[MAX_MSG_LEN];
  char ans_buf1[MAX_MSG_LEN];

  if (msg_id != 102) {
    read_msg(gps_sockfd, (char *)buf + 3);
  }

  switch (msg_id) {
    case 2: { // 請求密碼
      if (my_pass[0] != 0) {
        strncpy(ans_buf, (char *)my_pass, MAX_MSG_LEN - 1);
        ans_buf[MAX_MSG_LEN - 1] = '\0';
      } else {
        ans_buf[0] = 0;
        ask_question("請輸入你的密碼：", ans_buf, PASSWORD_LENGTH - 1,
                    0);
        ans_buf[PASSWORD_LENGTH - 1] = 0;
      }
      snprintf(msg_buf, MAX_MSG_LEN, "102%s", ans_buf);
      write_msg(gps_sockfd, msg_buf);
      strncpy((char *)my_pass, ans_buf, PASSWORD_LENGTH - 1);
      my_pass[PASSWORD_LENGTH - 1] = '\0';
      break;
    }

    case 3: { // 登入成功
      pass_login = 1;
      input_mode = TALK_MODE;
      display_comment("請打 /HELP 查看簡單指令說明，/Exit 離開");
      snprintf(msg_buf, MAX_MSG_LEN, "004%s", my_note);
      write_msg(gps_sockfd, msg_buf);
      break;
    }

    case 4: { // 密碼錯誤
      pass_count++;
      my_pass[0] = 0;
      if (pass_count == 3) leave();

      for (;;) { // 使用迴圈代替 goto
        ans_buf[0] = 0;
        ask_question("密碼錯誤! 請重新輸入你的名稱：", ans_buf,
                    NAME_LENGTH - 1, 1);
        if (ans_buf[0] != 0) {
          ans_buf[NAME_LENGTH - 1] = 0;
          snprintf(msg_buf, MAX_MSG_LEN, "101%s", ans_buf);
          write_msg(gps_sockfd, msg_buf);
          strncpy((char *)my_name, ans_buf, NAME_LENGTH - 1);
          my_name[NAME_LENGTH - 1] = '\0';
          break;
        }
      }
      break;
    }

    case 5: { // 創建新帳號
      ans_buf[0] = 0;
      ask_question("看來你是個新朋友, 你要使用這個名稱嗎？(y/N)：",
                  ans_buf, 1, 1);
      if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {

        for (;;) {
          ans_buf[0] = 0;
          ask_question("請輸入你的密碼：", ans_buf, PASSWORD_LENGTH - 1,
                      0);
          ans_buf1[0] = 0;
          ask_question("請再輸入一次確認：", ans_buf1,
                      PASSWORD_LENGTH - 1, 0);
          ans_buf[PASSWORD_LENGTH - 1] = 0;  // Null-terminate
          ans_buf1[PASSWORD_LENGTH - 1] = 0; // Null-terminate

          if (strcmp(ans_buf, ans_buf1) == 0) {
            snprintf(msg_buf, MAX_MSG_LEN, "103%s", ans_buf);
            write_msg(gps_sockfd, msg_buf);
            strncpy((char *)my_pass, ans_buf, PASSWORD_LENGTH - 1);
            my_pass[PASSWORD_LENGTH - 1] = '\0';
            break;  // 成功，跳出迴圈
          } else {
            display_comment("密碼不符，請重新輸入");
          }
        }

      } else {
        for (;;) {
          ans_buf[0] = 0;
          ask_question("請重新輸入你的名稱：", ans_buf,
                      NAME_LENGTH - 1, 1);

          if (ans_buf[0] != 0) {  // 輸入不為空
            ans_buf[NAME_LENGTH - 1] = 0;
            snprintf(msg_buf, MAX_MSG_LEN, "101%s", ans_buf);
            write_msg(gps_sockfd, msg_buf);
            strncpy((char *)my_name, ans_buf, NAME_LENGTH - 1);
            my_name[NAME_LENGTH - 1] = '\0';
            break;
          }
        }
      }
      break;
    }

    case 6: { // 重複登入
      ans_buf[0] = 0;
      ask_question("重覆進入! 你要殺掉另一個帳號嗎? (y/N)：",
                  ans_buf, 1, 1);
      if (ans_buf[0] == 'y' || ans_buf[0] == 'Y') {
        write_msg(gps_sockfd, "105");
      } else
        leave();
      break;
    }

    case 10: { // 離線
      leave();
      break;
    }

    case 11: { // 加入伺服器
      switch (buf[3]) {
      case '0': {
        Tokenize((char *)buf + 4);
        send_gps_line("與該桌連線中...");
        int ret = init_socket((char *)cmd_argv[1], atoi((char *)cmd_argv[2]),
                                  &table_sockfd);
        FD_SET(table_sockfd, &afds);
        in_join = 1;
        break;
      }
      case '1':
        send_gps_line("查無此桌，請重新再試。（可用 /FREE 查詢空桌）");
        break;
      case '2':
        send_gps_line("無法連線");
        break;
      default:
        break;
      }
      break;
    }

    case 12: { // 開桌
      init_serv_socket();
      snprintf(msg_buf, MAX_MSG_LEN, "012%d", SERV_PORT - 1);
      write_msg(gps_sockfd, msg_buf);
      my_id = 1;
      in_serv = 1;
      on_seat = 0;
      player_in_table = 1;
      player[1].sit = 1;
      player[1].money = my_money;
      player[1].id = my_gps_id;
      strncpy(player[1].name, (char *)my_name, NAME_LENGTH - 1);
      player[1].name[NAME_LENGTH - 1] = '\0';
      my_sit = 1;
      for (int i = 0; i <= 4; i++) {
        table[i] = 0;
      }
      table[1] = 1;
      if (player_in_table == PLAYER_NUM) {
        init_playing_screen();
        opening();
        open_deal();
      }
      strncpy(player[1].name, (char *)my_name, NAME_LENGTH - 1);
      player[1].name[NAME_LENGTH - 1] = '\0';
      player[1].in_table = 1;
      send_gps_line("您已建立新桌，目前人數1人，可使用 /who 查詢本桌清單");
      send_gps_line("如要關桌請輸入 /Leave (/L) 踢除使用者請用 /Kick ");
      send_gps_line("請用 /Note <附註> 設定附註，其他人查詢空桌時將可參考。");
      break;
    }

    case 101: { // 顯示訊息
      send_gps_line((char *)buf + 3);
      break;
    }

    case 102: { // 顯示新聞
      display_news(gps_sockfd);
      break;
    }

    case 120: { // 取得客戶端 ID 和金錢
      char temp_buf[6] = {0};
      strncpy(temp_buf, (char *)buf + 3, 5); // 複製字串，避免溢位
      temp_buf[5] = '\0';          // 確保字串結束符

      char temp_buf2[16] = {0};
      strncpy(temp_buf2, (char *)buf + 8, 15); // 複製字串，避免溢位
      temp_buf2[15] = '\0';          // 確保字串結束符

      int new_client_id = atoi(temp_buf);
      long new_client_money = atol(temp_buf2);

      if (!in_serv) {
        my_gps_id = new_client_id;
        my_money = new_client_money;
      }
      break;
    }

    case 200: { // 關閉連線
      close(gps_sockfd);
      endwin();
      break;
    }

    case 211: { // 取得新客戶端名稱
      strncpy(new_client_name, (char *)buf + 3, NAME_LENGTH - 1);
      new_client_name[NAME_LENGTH - 1] = '\0';
      new_client = 1;
      break;
    }

    default: {  // 未知的訊息 ID
      snprintf(msg_buf, MAX_MSG_LEN, "msg_id=%d", msg_id);
      display_comment(msg_buf);
    }
  }
}

// 處理來自客戶端的訊息
static void process_msg_from_client(int player_id, unsigned char *buf,
                                    int msg_id) {
  char msg_buf[MAX_MSG_LEN];
  int i;
  int sit;
  int j;

  read_msg(player[player_id].sockfd, (char *)buf + 3);

  switch (msg_id) {
    case 101: { // 聊天訊息
      send_gps_line((char *)buf + 3);
      broadcast_msg(player_id, (char *)buf);
      break;
    }
    case 102: { // 顯示訊息
      display_comment((char *)buf + 3);
      broadcast_msg(player_id, (char *)buf);
      break;
    }
    case 200: { // 其他玩家離開
      close_client(player_id);
      break;
    }
    case 290: { // 開始遊戲
      broadcast_msg(player_id, (char *)buf);
      opening();
      open_deal();
      break;
    }
    case 313: { // 請求發牌給客戶端
      send_card_request = 1;
      break;
    }
    case 315: { // 客戶端完成
      break;
    }
    case 401: { // 其他玩家丟牌
      pool[player[player_id].sit].time += thinktime(before);
      display_time(player[player_id].sit);
      snprintf(msg_buf, MAX_MSG_LEN, "312%c%f", player[player_id].sit,
              pool[player[player_id].sit].time);
      broadcast_msg(1, msg_buf);
      pool[player[player_id].sit].first_round = 0;
      in_kang = 0;
      show_newcard(player[player_id].sit, 3);
      snprintf(msg_buf, MAX_MSG_LEN, "314%c%c", player[player_id].sit, 3);
      broadcast_msg(player_id, msg_buf);
      snprintf(msg_buf, MAX_MSG_LEN, "402%c%c", player_id, buf[3]);
      broadcast_msg(player_id, msg_buf);
      current_id = player_id;
      current_card = buf[3];
      show_cardmsg(player[player_id].sit, buf[3]);
      throw_card(buf[3]);
      return_cursor();
      sit = player[player_id].sit;

      // 尋找丟出的牌在手牌中的位置
      int card_index = -1;
      for (i = 0; i < pool[sit].num; i++) {
        if (pool[sit].card[i] == current_card) {
          card_index = i;
          break;
        }
      }

      // 如果找到，將該牌替換為手牌中的最後一張牌
      if (card_index != -1) {
        pool[sit].card[card_index] = pool[sit].card[pool[sit].num - 1];
        pool[sit].num--;  // 手牌數量減少
      }
      sort_pool(sit);
      check_on = 1;
      send_card_on = 0;
      next_player_on = 0;

      // set in_check flag for players except the current player
      for (i = 1; i <= 4; i++) {
        if (table[i] > 0 && table[i] != player_id)
          check_card(i, buf[3]);
      }

      // 檢查所有玩家可否碰/槓/吃
      for (i = 1; i <= 4; i++) {
        if (table[i] != 1 && i != turn) {
          bool need_break = false; // 用來判斷內層迴圈是否要跳脫
          for (j = 1; j < check_number; j++) {
            if (check_flag[i][j]) {
              snprintf(msg_buf, MAX_MSG_LEN, "501%c%c%c%c",
                      check_flag[i][1] + '0', check_flag[i][2] + '0',
                      check_flag[i][3] + '0', check_flag[i][4] + '0');
              write_msg(player[table[i]].sockfd, msg_buf);
              in_check[i] = 1;
              need_break = true;
              break;  // check next player
            } else {
              in_check[i] = 0;
            }
          }
          if (need_break) continue;
        }
      }

      // 檢查自己可否碰/槓/吃
      bool need_check = false;
      for (j = 1; j < check_number; j++)
        if (check_flag[my_sit][j]) {
          init_check_mode();
          in_check[1] = 1;
          need_check = true;
          break;
        }
      in_check[1] = 0;
      break;
    }

    case 450: {  // 等待叫牌
      wait_hit[player[player_id].sit] = 1;
      break;
    }

    case 501: {  // 查詢玩家資訊
      who((char *)player[player_id].sockfd); //FIX-ME
      break;
    }

    case 510: {  // 取消檢查
      in_check[player[player_id].sit] = 0;
      check_for[player[player_id].sit] = buf[3] - '0';
      break;
    }

    case 515: {  // 下一位玩家的請求
      next_player_request = 0;
      turn = player[player_id].sit;
      snprintf(msg_buf, MAX_MSG_LEN, "310%c", turn);
      broadcast_msg(player_id, msg_buf);
      display_point(turn);
      return_cursor();
      break;
    }

    case 525: {  // 送花
      send_card_request = 1;
      draw_flower(buf[3], buf[4]);
      broadcast_msg(player_id, (char *)buf);
      break;
    }

    case 530: {  // 其他人碰/槓牌
      gettimeofday(&before, (struct timezone *)0);
      turn = player[buf[3]].sit;
      display_point(turn);

      if (buf[4] == 12) {
        // 如果是明槓
        for (i = 0; i < pool[turn].out_card_index; i++)
          if (pool[turn].out_card[i][0] == 2 &&
              pool[turn].out_card[i][1] == buf[5]) {
            pool[turn].out_card[i][0] = 12;
            pool[turn].out_card[i][4] = buf[5];
            pool[turn].out_card[i][5] = 0;
            break;
          }
      } else {
        // 如果是碰或暗槓
        for (i = 0; i <= 3; i++)
          pool[turn].out_card[pool[turn].out_card_index][i] = buf[i + 4];
        if (buf[4] == 3 || buf[4] == 11) {
          // 如果是槓牌
          pool[turn].out_card[pool[turn].out_card_index][4] = buf[7];
          pool[turn].out_card[pool[turn].out_card_index][5] = 0;
        } else
          pool[turn].out_card[pool[turn].out_card_index][4] = 0;
        pool[turn].out_card_index++;
      }
      draw_epk(buf[3], buf[4], buf[5], buf[6], buf[7]);
      broadcast_msg(player_id, (char *)buf);
      return_cursor();

      switch (buf[4]) {
        case 2:  // 碰牌
          for (i = 0; i < pool[turn].num; i++)
            if (pool[turn].card[i] == buf[6]) {
              pool[turn].card[i] = pool[turn].card[pool[turn].num - 1];
              pool[turn].card[i + 1] = pool[turn].card[pool[turn].num - 2];
              break;
            }
          pool[turn].num -= 3;
          sort_pool(turn);
          break;
        case 3:  // 明槓
        case 11:  // 暗槓
          for (i = 0; i < pool[turn].num; i++)
            if (pool[turn].card[i] == buf[6]) {
              pool[turn].card[i] = pool[turn].card[pool[turn].num - 1];
              pool[turn].card[i + 1] = pool[turn].card[pool[turn].num - 2];
              pool[turn].card[i + 2] = pool[turn].card[pool[turn].num - 3];
              break;
            }
          pool[turn].num -= 3;
          sort_pool(turn);
          break;
        case 7:  // 吃牌
        case 8:  // 組合吃牌
        case 9:  // 組合吃牌
          pool[turn].card[search_card(turn, buf[5])] =
              pool[turn].card[pool[turn].num - 1];
          pool[turn].card[search_card(turn, buf[7])] =
              pool[turn].card[pool[turn].num - 2];
          pool[turn].num -= 3;
          sort_pool(turn);
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

// 處理來自伺服器的訊息
static void process_msg_from_server(int player_id, unsigned char *buf,
                                    int msg_id) {
  char msg_buf[MAX_MSG_LEN]; //訊息緩衝區
  int i;

  read_msg(table_sockfd, (char *)buf + 3);

  switch (msg_id) {
    case 101: { // 顯示訊息
      send_gps_line((char *)buf + 3);
      break;
    }
    case 102: { // 顯示訊息
      display_comment((char *)buf + 3);
      break;
    }
    case 199: { // LEAVE 開桌者離開牌桌
      write_msg(gps_sockfd, "205");  // 通知 GPS Server
      close(table_sockfd);
      FD_CLR(table_sockfd, &afds);
      in_join = 0;
      input_mode = TALK_MODE;
      init_global_screen();
      display_comment("開桌者已離開牌桌");
      display_comment("-------------------");
      write_msg(gps_sockfd, "201"); // 更新一下目前線上人數跟內容
      break;
    }
    case 200: { // LEAVE 離開牌桌
      write_msg(gps_sockfd, "205");  // 通知 GPS Server
      close(table_sockfd);
      FD_CLR(table_sockfd, &afds);
      in_join = 0;
      input_mode = TALK_MODE;
      init_global_screen();
      write_msg(gps_sockfd, "201"); // 更新一下目前線上人數跟內容
      break;
    }
    case 201: {  // get the new comer's info
      strncpy(player[buf[3]].name, (const char *)buf + 6,
              NAME_LENGTH - 1);
      player[buf[3]].name[NAME_LENGTH - 1] = '\0';
      player[buf[3]].in_table = 1;
      player[buf[3]].sit = buf[4];
      player_in_table = buf[5];
      if (strcmp((char *)my_name, player[buf[3]].name) == 0) {
        snprintf(msg_buf, MAX_MSG_LEN, "您已加入此桌，目前人數 %d ",
                player_in_table);
      } else {
        snprintf(msg_buf, MAX_MSG_LEN, "%s 加入此桌，目前人數 %d ",
                player[buf[3]].name, player_in_table);
      }
      send_gps_line(msg_buf);
      if (player[buf[3]].sit) table[player[buf[3]].sit] = buf[3];
      break;
    }
    case 202: {
      char temp_buf[6] = {0};
      strncpy(temp_buf, (const char *)buf + 4, 5); //複製id字串，避免溢位
      temp_buf[5] = '\0';                           //確保字串結束符
      player[buf[3]].id = atoi(temp_buf);
      player[buf[3]].money = atol((const char *)buf + 9);
      break;
    }
    case 203: {  // get others info
      strncpy(player[buf[3]].name, (const char *)buf + 5,
              NAME_LENGTH - 1);
      player[buf[3]].name[NAME_LENGTH - 1] = '\0';
      player[buf[3]].sit = buf[4];
      player[buf[3]].in_table = 1;
      table[buf[4]] = buf[3];
      break;
    }
    case 204: {
      for (i = 1; i <= 4; i++) {
        table[i] = buf[2 + i] - '0';
        if (table[i]) {
          player[table[i]].sit = i;
          on_seat++;
        }
      }
      break;
    }
    case 205: {  /* NOTICE: need player_in_table++ ? */
      /* NOTICE: did he get table[]???? */
      my_id = buf[3];
      strncpy(player[my_id].name, (const char *)buf + 5,
              NAME_LENGTH - 1);
      player[my_id].name[NAME_LENGTH - 1] = '\0';
      my_sit = buf[4];
      player[my_id].sit = my_sit;
      player[my_id].in_table = 1;
      player[my_id].id = my_gps_id;
      player[my_id].money = my_money;
      table[my_sit] = my_id;
      break;
    }
    case 206: {
      if (player_in_table == 4) {
        init_global_screen();
        input_mode = TALK_MODE;
      }
      player_in_table = buf[4];
      snprintf(msg_buf, MAX_MSG_LEN,
              "%s 離開此桌，目前人數剩下 %d 人", player[buf[3]].name,
              player_in_table);
      display_comment(msg_buf);
      player[buf[3]].in_table = 0;
      break;
    }
    case 290: {
      opening();
      break;
    }
    case 300: {
      if (screen_mode == GLOBAL_SCREEN_MODE) {
        init_playing_screen();
      }
      opening();
      break;
    }
    case 301: {  /* change card */
      change_card(buf[3], buf[4]);
      break;
    }
    case 302: {  /* get 16 cards */
      for (i = 0; i < 16; i++) pool[my_sit].card[i] = buf[i + 3];
      sort_card(0);
      break;
    }
    case 303: {  /* can get a card */
      play_mode = GET_CARD;
      attron(A_REVERSE);
      show_card(10, INDEX_X + 16 * 2 + 1, INDEX_Y + 1, 1);
      attroff(A_REVERSE);
      wrefresh(stdscr);
      return_cursor();
      break;
    }
    case 304: {  /* get a card */
      process_new_card(my_sit, buf[3]);
      break;
    }
    case 305: {
      card_owner = buf[3];
      if (card_owner != my_sit) show_cardmsg(card_owner, 0);
      break;
    }
    case 306: {  /* others got a card ---> change the card number */
      card_point = buf[3];
      show_num(2, 70, 144 - card_point - 16, 2);
      return_cursor();
      break;
    }
    case 308: {  /* sort cards */
      sort_card(buf[3] - '0');
      break;
    }
    case 310: {  /* Player pointer ---  point to new player */
      turn = buf[3];
      display_point(buf[3]);
      return_cursor();
      break;
    }
    case 312: {
      char temp_buf[20] = {0}; // 假設時間字串不會超過 20 位元
      strncpy(temp_buf, (const char *)buf + 4, 19);
      temp_buf[19] = '\0'; // 確保字串結束符
      pool[buf[3]].time = atof(temp_buf);
      display_time(buf[3]);
      break;
    }
    case 314: {  /* process new cardback */
      show_newcard(buf[3], buf[4]);
      return_cursor();
      break;
    }
    case 330: {  /* 海底流局 */
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
    }
    case 402: {  /* others throw a card */
      in_kang = 0;
      pool[player[buf[3]].sit].first_round = 0;
      show_cardmsg(player[buf[3]].sit, buf[4]);
      current_card = buf[4];
      throw_card(buf[4]);
      return_cursor();
      current_id = buf[3];
      break;
    }
    case 501: {  /* ask for check card */
      go_to_check = 0;
      for (i = 1; i <= 4; i++) {
        check_flag[my_sit][i] = buf[2 + i] - '0';
        if (check_flag[my_sit][i]) go_to_check = 1;
      }
      if (go_to_check) init_check_mode();
      go_to_check = 0;
      break;
    }
    case 518: {
      for (i = 1; i <= 4; i++) pool[i].door_wind = buf[2 + i];
      wmvaddstr(stdscr, 2, 64, sit_name[pool[my_sit].door_wind]);
      return_cursor();
      break;
    }
    case 520: {
      process_epk(buf[3]);
      break;
    }
    case 521: {
      for (i = 0; i < pool[1].num; i++) pool[1].card[i] = buf[3 + i];
      for (i = 0; i < pool[2].num; i++) pool[2].card[i] = buf[19 + i];
      for (i = 0; i < pool[3].num; i++) pool[3].card[i] = buf[35 + i];
      for (i = 0; i < pool[4].num; i++) pool[4].card[i] = buf[51 + i];
      break;
    }
    case 522: {
      process_make(buf[3], buf[4]);
      break;
    }
    case 525: {
      draw_flower(buf[3], buf[4]);
      break;
    }
    case 530: {  /* from server */
      turn = player[buf[3]].sit;
      card_owner = turn;
      display_point(turn);
      if (buf[4] == 12) {
        for (i = 0; i < pool[turn].out_card_index; i++)
          if (pool[turn].out_card[i][1] == buf[5] &&
              pool[turn].out_card[i][2] == buf[6]) {
            pool[turn].out_card[i][0] = 12;
            pool[turn].out_card[i][4] = buf[5];
            pool[turn].out_card[i][5] = 0;
          }
        draw_epk(buf[3], buf[4], buf[5], buf[6], buf[7]);
      } else {
        for (i = 0; i <= 3; i++)
          pool[turn].out_card[pool[turn].out_card_index][i] = buf[i + 4];
        if (buf[4] == 3 || buf[4] == 11) {
          pool[turn].out_card[pool[turn].out_card_index][4] = buf[7];
          pool[turn].out_card[pool[turn].out_card_index][5] = 0;
        } else
          pool[turn].out_card[pool[turn].out_card_index][4] = 0;
        draw_epk(buf[3], buf[4], buf[5], buf[6], buf[7]);
        pool[turn].out_card_index++;
      }
      return_cursor();
      break;
    }
    default:
      break;
  }
}

