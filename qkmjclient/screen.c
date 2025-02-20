#include "screen.h"

// 定義點數位置 (This was missing previously)
static const int point_coords[][2] = {
  {22, 66}, // 自己
  {21, 68}, // 下家
  {20, 66}, // 對家
  {21, 64}  // 上家
};

// 設定顏色
// fore: 前景色, back: 背景色
void set_color(int fore, int back)
{
  // 使用 ANSI escape code 設定顏色，ncurses 也支援此方式
  if (color) {
    printf("\033[%d;%dm", fore, back);
    fflush(stdout);
  }
}

// 設定顯示模式
// mode: 顯示模式
void set_mode(int mode)
{
  // 使用 ANSI escape code 設定顯示模式，ncurses 也支援此方式
  if (color) {
    printf("\033[%dm", mode);
    fflush(stdout);
  }
}

// 在視窗中指定位置列印字串
// win: 視窗指標, y: 列, x: 行, msg: 字串
void mvprintstr(WINDOW *win, int y, int x, const char *msg)
{
  mvwaddstr(win, y, x, msg);
}

// 在視窗中列印字串
// win: 視窗指標, str: 字串
void printstr(WINDOW *win, const char *str)
{
  waddstr(win, str);
}

// 在視窗中列印單一字元
// win: 視窗指標, ch: 字元
void printch(WINDOW *win, char ch)
{
  char msg[2];
  msg[0] = ch;
  msg[1] = '\0';
  waddstr(win, msg);
}

// 在視窗中指定位置列印單一字元
// win: 視窗指標, y: 列, x: 行, ch: 字元
void mvprintch(WINDOW *win, int y, int x, char ch)
{
  mvwaddch(win, y, x, ch);
}

// 清除螢幕區域
// ymin: 起始列, xmin: 起始行, height: 高度, width: 寬度
void clear_screen_area(int ymin, int xmin, int height, int width)
{
  int i;
  char line_buf[width + 1]; // Add null terminator space

  for (i = 0; i < width; i++) {
    line_buf[i] = ' ';
  }
  line_buf[width] = '\0';

  for (i = 0; i < height; i++) {
    mvwaddstr(stdscr, ymin + i, xmin, line_buf);
  }
  wrefresh(stdscr);
}

// 清除輸入列
void clear_input_line()
{
  werase(inputwin);
  talk_x = 0;
  wmove(inputwin, 0, talk_x);
  talk_buf_count = 0;
  talk_buf[0] = '\0';
  wrefresh(inputwin);
}

// 等待使用者按下按鍵
// msg: 提示訊息
void wait_a_key(const char *msg)
{
  int ch;
  werase(inputwin);
  mvwaddstr(inputwin, 0, 0, msg);
  wrefresh(inputwin);
  do {
    ch = my_getch();
  } while (ch != KEY_ENTER && ch != ENTER);
  werase(inputwin);
  mvwaddstr(inputwin, 0, 0, talk_buf);
  wrefresh(inputwin);
}

// 詢問使用者問題
// question: 問題, answer: 答案, ans_len: 答案長度, type: 輸入類型
void ask_question(const char *question, char *answer, int ans_len, int type)
{
  memset(answer, 0, ans_len);
  werase(inputwin);
  mvwaddstr(inputwin, 0, 0, question);
  wrefresh(inputwin);
  mvwgetstring(inputwin, 0, strlen(question), ans_len, answer, type);
  wrefresh(inputwin);
}


// 繪製選單索引
// max_item: 選單項目數
void draw_index(int max_item)
{
  int i;
  for (i = 1; i < max_item; i++) {
    mvwaddstr(stdscr, INDEX_Y, INDEX_X + (17 - max_item + i) * 2 - 2,
              menu_item[i]);
  }
  mvwaddstr(stdscr, INDEX_Y, INDEX_X + 16 * 2 + 1, menu_item[max_item]);
  return_cursor();
}

// 設定目前索引
// current: 目前索引
void current_index(int current)
{
  wmove(stdscr, INDEX_Y, INDEX_X + current * 2);
  wrefresh(stdscr);
}

// 顯示牌背
// sit: 座位
void show_cardback(char sit)
{
  int i;
  switch ((sit - my_sit + 4) % 4) {
  case 0:
    break;
  case 1:
    for (i = 0; i < pool[sit].num; i++) {
      show_card(40, INDEX_X1, INDEX_Y1 - i, 0);
    }
    break;
  case 2:
    for (i = 0; i < pool[sit].num; i++) {
      show_card(30, INDEX_X2 - i * 2, INDEX_Y2, 1);
    }
    break;
  case 3:
    for (i = 0; i < pool[sit].num; i++) {
      show_card(40, INDEX_X3, INDEX_Y3 + i, 0);
    }
    break;
  }
  return_cursor();
}

// 顯示所有牌
// sit: 座位
void show_allcard(char sit)
{
  int i;
  switch ((sit - my_sit + 4) % 4) {
  case 0:
    break;
  case 1:
    for (i = 0; i < pool[sit].num; i++) {
      show_card(pool[sit].card[i], INDEX_X1,
                INDEX_Y1 - (16 - pool[sit].num + i), 0);
    }
    break;
  case 2:
    for (i = 0; i < pool[sit].num; i++) {
      show_card(pool[sit].card[i], INDEX_X2 - (16 - pool[sit].num + i) * 2,
                INDEX_Y2, 1);
    }
    break;
  case 3:
    for (i = 0; i < pool[sit].num; i++) {
      show_card(pool[sit].card[i], INDEX_X3,
                INDEX_Y3 + (16 - pool[sit].num + i), 0);
    }
    break;
  }
  return_cursor();
}

// 顯示槓牌
// sit: 座位
void show_kang(char sit)
{
  int i;
  switch ((sit - my_sit + 4) % 4) {
  case 0:
    break;
  case 1:
    for (i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][0] == 11) {
        show_card(pool[sit].out_card[i][2], INDEX_X1, INDEX_Y1 - i * 3 - 1, 0);
      }
    }
    break;
  case 2:
    for (i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][0] == 11) {
        show_card(pool[sit].out_card[i][2], INDEX_X2 - i * 6 - 2, INDEX_Y2, 1);
      }
    }
    break;
  case 3:
    for (i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][0] == 11) {
        show_card(pool[sit].out_card[i][2], INDEX_X3, INDEX_Y3 + i * 3 + 1, 0);
      }
    }
    break;
  }
}
       
// 顯示新牌
// sit: 座位, type: 類型 (1: 摸牌, 2: 摸入, 3: 丟出, 4: 顯示)
void show_newcard(char sit, char type)
{
    int x, y;
    int card_index;

    // 計算牌的顯示位置
    switch ((sit - my_sit + 4) % 4) {
    case 0: // 自己
        x = INDEX_X;
        y = INDEX_Y + 1;
        break;
    case 1: // 下家
        x = INDEX_X1;
        y = INDEX_Y1 - 17;
        break;
    case 2: // 對家
        x = INDEX_X2 - 16 * 2 -1;
        y = INDEX_Y2;
        break;
    case 3: // 上家
        x = INDEX_X3;
        y = INDEX_Y3 + 17;
        break;
    default:
        return; // Invalid seat
    }

    // 根據類型顯示不同的牌
    switch (type) {
    case 1: // 摸牌  (顯示牌背)
        show_card(40, x, y, 0); // 40 represents card back
        break;
    case 2: // 摸入 (顯示牌背)
        show_card(40, x, y, 0); // 40 represents card back
        break;
    case 3: // 丟出 (顯示牌)
        show_card(current_card, x, y, 0);
        break;
    case 4: // 顯示 (顯示牌)
        show_card(current_card, x, y, 0);
        break;
    default:
        return; // Invalid type
    }

    return_cursor();
}

// 顯示牌
// card: 牌, x: x 座標, y: y 座標, type: 類型 (0: row, 1: column)
void show_card(char card, int x, int y, int type) {
  const char* tile_representation;
  int color_pair;
  size_t len;

  // 錯誤檢查: 檢查 card 索引是否有效
  if (card < 1 || card >= sizeof(mj_item) / sizeof(mj_item[0])) {
      mvprintw(y, x, "Invalid Card"); // 顯示錯誤訊息
      return;
  }

  tile_representation = mj_item[card];  // 取得麻將牌字串
  len = strlen(tile_representation);    // 取得字串長度

  // 顏色對應 (使用更具描述性的常數)
  const int CARD_BACK_COLOR = 32;
  const int DEFAULT_COLOR = 37;
  const int DEFAULT_BACKGROUND = 40;

  // 使用陣列映射牌到顏色
  const int card_color_map[] = {
      31, 31, 31, 31, 31, 31, 31, 31, 31, // 萬子
      32, 32, 32, 32, 32, 32, 32, 32, 32, // 筒子
      36, 36, 36, 36, 36, 36, 36, 36, 36, // 條子
      33, 33, 33, 33,                     // 風牌
      35, 35, 35                          // 箭牌
  };

  if (card == 40) {
      color_pair = CARD_BACK_COLOR; // 牌背的顏色
  } else if (card >= 1 && card <= sizeof(card_color_map) / sizeof(card_color_map[0])) {
      color_pair = card_color_map[card - 1]; // 其他牌的顏色
  } else {
      color_pair = DEFAULT_COLOR; // 預設顏色
  }

  set_color(color_pair, DEFAULT_BACKGROUND); // 設定顏色

  if (type == 1) { // 垂直顯示
      // 確保不會超出字串範圍
      if (len >= 2) mvwaddstr(stdscr, y, x, tile_representation);
      if (len >= 4) mvwaddstr(stdscr, y + 1, x, tile_representation + 2);
  } else { // 水平顯示
      mvwaddstr(stdscr, y, x, tile_representation);
  }

  wrefresh(stdscr);
  set_color(DEFAULT_COLOR, DEFAULT_BACKGROUND); // 還原顏色
}

// 繪製標題
void draw_title()
{
  int x, y;
  for (y = 0; y < 24; y++) {
    mvprintstr(stdscr, y, 0, "│");
    mvprintstr(stdscr, y, 76, "│");
  }
  mvprintstr(stdscr, 22, 0, "├");
  for (x = 2; x <= 75; x += 2) {
    printstr(stdscr, "─");
  }
  mvprintstr(stdscr, 22, 76, "┤");
  mvprintstr(stdscr, 23, 2, "【對話】");
  wrefresh(stdscr);
}

// 初始化遊戲畫面
void init_playing_screen()
{
    // 清除畫面
    clear();

    // 初始化遊戲資訊
    info.wind = 1;
    info.dealer = 1;
    info.cont_dealer = 0;

    // 設定輸入視窗參數
    talk_right = 55;
    inputwin = playing_win;
    talk_x = 0;
    talk_y = 0;
    talk_buf_count = 0;
    talk_buf[0] = '\0';

    // 設定留言視窗參數
    comment_right = 55;
    comment_up = 19;
    comment_bottom = 22;
    comment_y = 19;

    // 建立留言視窗
    commentwin = newwin(comment_bottom - comment_up + 1, 
                        comment_right - comment_left + 1,
                        comment_up, comment_left);

    // 設定留言視窗滾動功能
    scrollok(commentwin, TRUE);

    // 設定螢幕模式
    screen_mode = PLAYING_SCREEN_MODE;

    // 繪製遊戲畫面
    draw_playing_screen();

    // 設定游標位置
    wmove(inputwin, talk_y, talk_x);
    wrefresh(inputwin);
}

// 初始化全局畫面
void init_global_screen()
{
    char msg_buf[MAX_MSG_LENGTH]; // Use MAX_MSG_LENGTH for safety
    char ans_buf[MAX_MSG_LENGTH]; // Use MAX_MSG_LENGTH for safety

    // 初始化參數
    comment_left = comment_x = org_comment_x;
    comment_right = 72;
    comment_up = 0;
    comment_bottom = 21;
    comment_y = org_comment_y;
    talk_left = 11;
    talk_right = 74;
    talk_x = 0;
    talk_buf_count = 0;
    talk_buf[0] = '\0';
    screen_mode = GLOBAL_SCREEN_MODE;

    // 建立留言視窗
    commentwin = newwin(comment_bottom, comment_right - comment_left +1, 
                        comment_up, comment_left);
    scrollok(commentwin, TRUE);

    // 建立輸入視窗
    inputwin = newwin(1, talk_right - talk_left, org_talk_y, talk_left);

    // 繪製全局畫面
    draw_global_screen();

    // 設定輸入模式
    input_mode = TALK_MODE;
    talk_left = 11;
    inputwin = global_win;
    mvwaddstr(stdscr, 23, 2, "【對話】");
    return_cursor();
}

// 在視窗中指定位置列印字串
int wmvaddstr(WINDOW *win, int y, int x, const char *str)
{
  return mvwaddstr(win, y, x, str);
}

// 繪製桌子
void draw_table()
{
  int x, y;
  mvwaddstr(stdscr, 4, 10, "□");
  for (x = 13; x <= 45; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "□");
  for (y = 5; y <= 12; y++) {
    mvwaddstr(stdscr, y, 10, "│");
    mvwaddstr(stdscr, y, 46, "│");
  }
  mvwaddstr(stdscr, 13, 10, "□");
  for (x = 12; x <= 44; x += 2) {
    waddstr(stdscr, "─");
  }
  waddstr(stdscr, "□");
}

// 繪製全局畫面
void draw_global_screen()
{
  clear();
  draw_title();
}

// 繪製遊戲畫面
void draw_playing_screen()
{
    int x, y;
    clear(); // Clear the screen

    // 繪製外框
    for (y = 0; y <= 23; y++) {
        mvwaddstr(stdscr, y, 0, "│");
        mvwaddstr(stdscr, y, 56, "│");
        mvwaddstr(stdscr, y, 76, "│");
    }
    mvwaddstr(stdscr, 18, 0, "├");
    for (x = 3; x <= 77; x += 2) {
        waddstr(stdscr, "─");
    }
    mvwaddstr(stdscr, 18, 76, "┤");
    mvwaddstr(stdscr, 18, 56, "┼");
    mvwaddstr(stdscr, 23, 2, "【對話】 ");

    // 繪製桌子外框 (This part could be further modularized into a separate function)
    mvwaddstr(stdscr, 1, 56, "├");
    for (x = 58; x <= 74; x += 2) {
        waddstr(stdscr, "─");
    }
    waddstr(stdscr, "┤");
    mvwaddstr(stdscr, 3, 56, "├");
    for (x = 58; x <= 74; x += 2) {
        waddstr(stdscr, "─");
    }
    waddstr(stdscr, "┤");
    mvwaddstr(stdscr, 12, 56, "├");
    for (x = 58; x <= 74; x += 2) {
        waddstr(stdscr, "─");
    }
    waddstr(stdscr, "┤");
    mvwaddstr(stdscr, 0, 66, "│");
    mvwaddstr(stdscr, 1, 66, "┼");
    mvwaddstr(stdscr, 2, 66, "│");
    mvwaddstr(stdscr, 3, 66, "┴");

    // 繪製玩家資訊 (This section could also be refactored)
    mvwaddstr(stdscr, 0, 60, "風  局");
    mvwaddstr(stdscr, 0, 68, "連    莊");
    mvwaddstr(stdscr, 2, 58, "門風：");
    mvwaddstr(stdscr, 2, 68, "剩    張");
    mvwaddstr(stdscr, 4, 58, "東家：");
    mvwaddstr(stdscr, 6, 58, "南家：");
    mvwaddstr(stdscr, 8, 58, "西家：");
    mvwaddstr(stdscr, 10, 58, "北家：");
    mvwaddstr(stdscr, 5, 62, "＄");
    mvwaddstr(stdscr, 7, 62, "＄");
    mvwaddstr(stdscr, 9, 62, "＄");
    mvwaddstr(stdscr, 11, 62, "＄");
    mvwaddstr(stdscr, 13, 60, "0  1  2  3  4");
    mvwaddstr(stdscr, 14, 60, "無 吃 碰 杠 胡");
    mvwaddstr(stdscr, 20, 64, "┌  ┐");
    mvwaddstr(stdscr, 22, 64, "└  ┘");

    wrefresh(stdscr);
    wmove(inputwin, talk_y, talk_x);
    wrefresh(inputwin);
}

// 尋找點數位置
void find_point(int pos)
{
  int row = point_coords[pos][0];
  int col = point_coords[pos][1];

  wmove(stdscr, row, col);
}

// 顯示點數
void display_point(int current_turn)
{
  static int last_turn = 0;
  char msg_buf[10]; // Increased buffer size for safety
  int pos;

  // 清除上一次的顯示
  if (last_turn) {
    pos = (last_turn + 4 - my_sit) % 4;
    find_point(pos);
    mvwaddstr(stdscr, point_coords[pos][0], point_coords[pos][1], "  ");
    wrefresh(stdscr);
    find_point(pos);
    mvwaddstr(stdscr, point_coords[pos][0], point_coords[pos][1], sit_name[last_turn]);
  }

  // 顯示目前的點數
  pos = (current_turn + 4 - my_sit) % 4;
  find_point(pos);
  mvwaddstr(stdscr, point_coords[pos][0], point_coords[pos][1], "  ");
  wrefresh(stdscr);
  find_point(pos);
  wattron(stdscr, A_REVERSE);
  mvwaddstr(stdscr, point_coords[pos][0], point_coords[pos][1], sit_name[current_turn]);
  wattroff(stdscr, A_REVERSE);
  last_turn = current_turn;
  wrefresh(stdscr);
}


// 顯示時間
void display_time(char sit)
{
  char msg_buf[10]; // Increased buffer size for safety
  int pos;
  int min, sec;

  pos = (sit - my_sit + 4) % 4;
  min = (int)pool[sit].time / 60;
  min = min % 60;
  sec = (int)pool[sit].time % 60;

  // 使用snprintf for safety
  snprintf(msg_buf, sizeof(msg_buf), "%2d:%02d", min, sec);

  int row = point_coords[pos][0];
  int col = point_coords[pos][1];

  mvwaddstr(stdscr, row, col, "     "); // Clear previous time
  mvwaddstr(stdscr, row, col, msg_buf);
  wrefresh(stdscr);
  return_cursor();
}

// 顯示資訊
void display_info()
{
    int i;
    char money_str[20]; // Increased buffer size for safety

    // 清除舊資訊 (Clear previous information)
    mvwaddstr(stdscr, 0, 58, "          "); // Clear wind and dealer info
    mvwaddstr(stdscr, 0, 68, "        "); // Clear continuous dealer info

    mvwaddstr(stdscr, 4, 64, "            "); // Clear east player money
    mvwaddstr(stdscr, 6, 64, "            "); // Clear south player money
    mvwaddstr(stdscr, 8, 64, "            "); // Clear west player money
    mvwaddstr(stdscr, 10, 64, "            "); // Clear north player money

    mvwaddstr(stdscr, 4, 74, "  "); // Clear east player dealer mark
    mvwaddstr(stdscr, 6, 74, "  "); // Clear south player dealer mark
    mvwaddstr(stdscr, 8, 74, "  "); // Clear west player dealer mark
    mvwaddstr(stdscr, 10, 74, "  "); // Clear north player dealer mark
    wrefresh(stdscr);

    // 顯示風向、莊家、連莊次數 (Display wind direction, dealer, continuous dealer count)
    mvwaddstr(stdscr, 0, 58, sit_name[info.wind]);
    mvwaddstr(stdscr, 0, 62, sit_name[info.dealer]);
    show_num(0, 70, info.cont_dealer, intlog10(info.cont_dealer) + 1);

    // 顯示玩家名稱和金錢 (Display player names and money)
    for (i = 1; i <= 4; i++) {
      if(table[i] != 0) { // Check if seat is occupied
        mvwaddstr(stdscr, 4 + (i -1) * 2, 58, sit_name[i]);
        snprintf(money_str, sizeof(money_str), "%ld", player[table[i]].money);
        mvwaddstr(stdscr, 4 + (i - 1) * 2, 64, money_str);
        if (i == info.dealer) {
            mvwaddstr(stdscr, 4 + (i - 1) * 2, 74, "莊"); // Mark the dealer
        }
      }
    }

    // 顯示其他資訊 (Display other information)
    display_time(my_sit);

    wrefresh(stdscr);
    wmove(inputwin, talk_y, talk_x);
    wrefresh(inputwin);
}

#define MAX_LINE_LENGTH 1024

static int more_size=0, more_num=0;
static char more_buf[4096];
// 讀取一行資料
int readln(int fd, char *buf, int *end_flag)
{
  int len = 0;
  int ch;
  int bytes_read;
  char error_message[1024]; // 錯誤訊息緩衝區

  *end_flag = 0;
  while (len < MAX_LINE_LENGTH - 1) { // -1 for null terminator
    if (more_num >= more_size) {
      more_size = read(fd, more_buf, sizeof(more_buf));
      if (more_size <= 0) {
        *end_flag = 1; // 讀取結束或發生錯誤
        if (more_size < 0) {
          err("讀取資料失敗");
        }
        break;
      }
      more_num = 0;
    }
    ch = more_buf[more_num++];
    if (ch == '\n') {
      break;
    } else if (ch == '\0') {
      *end_flag = 1; // 遇到 null terminator
      break;
    } else {
      *buf++ = ch;
      len++;
    }
  }
  *buf = '\0'; // 加入 null terminator 以確保字串安全
  return len + 1; // 傳回讀取長度，包含 null terminator
}

// 顯示新聞
void display_news(int fd)
{
  char buf[MAX_LINE_LENGTH]; // Use MAX_LINE_LENGTH for safety
  int line_count = 0;
  int end_flag;
  WINDOW *news_win;
  char error_message[1024]; // For error messages

  news_win = newwin(22, 74, 0, 2);
  if (news_win == NULL) {
    err("建立新聞視窗失敗");
    return;
  }

    wclear(news_win);
    wmove(news_win, 0, 0);

  while (readln(fd, buf, &end_flag)) {
    if (end_flag == 1) break;
    if (line_count == 22) {
      wrefresh(news_win);
      wait_a_key("請按 <ENTER> 至下一頁......");
      line_count = 0;
      wclear(news_win);
      wmove(news_win, 0, 0);
    }
    printstr(news_win, buf);
    wrefresh(news_win);
    line_count++;
  }
  wrefresh(news_win);
  wait_a_key("請按 <ENTER> 繼續.......");
  delwin(news_win);
  redraw_screen();
}

// 顯示留言
void display_comment(const char *comment)
{
    // Using snprintf is safer to prevent potential buffer overflows
    char formatted_comment[MAX_MSG_LENGTH];
    snprintf(formatted_comment, sizeof(formatted_comment), "%s\n", comment);

    waddstr(commentwin, formatted_comment);
    wattrset(commentwin, COLOR_PAIR(1));
    wattron(commentwin, COLOR_PAIR(1));
    wrefresh(commentwin);
    return_cursor();
}

// 送出對話
void send_talk_line(const char *talk)
{
  char comment[MAX_MSG_LENGTH]; // Increased buffer size for safety
  char msg_buf[MAX_MSG_LENGTH]; // Increased buffer size for safety

  // 建立留言訊息 (Construct the comment message)
  snprintf(comment, sizeof(comment), "<%s> %s", my_name, talk);
  display_comment(comment);

  // 建立訊息 (Construct the message to send)
  snprintf(msg_buf, sizeof(msg_buf), "102%s", comment);

  // 根據連線狀態送出訊息 (Send the message based on connection status)
  if (in_serv) {
    broadcast_msg(1, msg_buf);
  }
  if (in_join) {
    write_msg(table_sockfd, msg_buf);
  }
}

// 送出遊戲訊息
void send_gps_line(const char *msg)
{
  char comment[MAX_MSG_LENGTH]; // Increased buffer size for safety

  // 複製訊息到 comment 變數 (Copy the message to the comment variable)
  strncpy(comment, msg, sizeof(comment) - 1);
  comment[sizeof(comment) - 1] = '\0'; // Ensure null-termination

  display_comment(comment); // 顯示留言 (Display the comment)
}


// 計算數字位數
int intlog10(int num)
{
  // 使用數學函式庫中的 log10 函式計算位數
  // 這比迴圈更有效率，但需要處理 num = 0 的情況

  if (num == 0) {
      return 1; // Special case for 0
  } else if (num < 0) {
      return intlog10(-num); //Handle negative numbers
  }
  return (int)floor(log10(num)) + 1;
}

// 將數字轉換為字串
void convert_num(char *str, int number, int digit)
{
  int i;
  int tmp[10]; // 使用局部變數，避免記憶體洩漏

  memset(tmp, 0, sizeof(tmp));
  for (i = digit - 1; i >= 0; i--) {
    tmp[i] = number % 10;
    number /= 10;
  }
  for (i = 0; i < digit; i++) {
    strncpy(str + i * 2, number_item[tmp[i]], 2); // 使用strncpy避免緩衝區溢位
    str[i * 2 + 2] = '\0'; //確保null-terminated
  }
}


// 顯示數字
void show_num(int y, int x, int number, int digit)
{
  char msg_buf[255]; // 使用局部變數，避免記憶體洩漏
  int i;

  // 使用snprintf來避免緩衝區溢位問題
  snprintf(msg_buf, sizeof(msg_buf), "%*d", digit, number);

  mvwaddstr(stdscr, y, x, msg_buf);
  wrefresh(stdscr);
}


// 顯示牌訊息
void show_cardmsg(int sit, char card)
{
  int pos;
  char msg_buf[MAX_MSG_LENGTH]; // Use a buffer for string safety


  pos = (sit - my_sit + 4) % 4;
  clear_screen_area(15, 58, 3, 18); // 清除顯示區域


  if (card) {
    // 使用snprintf來避免緩衝區溢位問題
    snprintf(msg_buf, sizeof(msg_buf), "┌───────┐\n│");

    switch (pos) {
    case 0:
      strcat(msg_buf, "玩");
      break;
    case 1:
      strcat(msg_buf, "下");
      break;
    case 2:
      strcat(msg_buf, "對");
      break;
    case 3:
      strcat(msg_buf, "上");
      break;
    }
    strcat(msg_buf, "家打（    ）│\n└───────┘");

    mvwaddstr(stdscr, 15, 58, msg_buf);
    show_card(card, 68, 16, 0);
  }
  return_cursor();
}

// 重繪螢幕
void redraw_screen()
{
  int i, j;

  // 清除螢幕並強制更新
  clearok(stdscr, TRUE);
  wrefresh(stdscr);

  // 更新留言視窗
  touchwin(commentwin);
  wrefresh(commentwin);

  // 更新輸入視窗
  touchwin(inputwin);
  wrefresh(inputwin);

  return_cursor(); // 將游標移回正確位置
}

// 重設游標
void reset_cursor()
{
  // 清除游標所在位置
  mvwaddstr(stdscr, 23, 75, " ");
  wrefresh(stdscr);
}

// 返回游標
void return_cursor()
{
  // 根據目前的輸入模式設定游標位置
  switch (input_mode) {
  case ASK_MODE:
  case TALK_MODE:
    reset_cursor();
    wmove(inputwin, talk_y, talk_x);
    wrefresh(inputwin);
    break;
  case CHECK_MODE:
    wmove(stdscr, org_check_y, check_x);
    wrefresh(stdscr);
    break;
  case PLAY_MODE:
    wmove(stdscr, pos_y, pos_x);
    wrefresh(stdscr);
    break;
  case EAT_MODE:
    wmove(stdscr, eat_y, eat_x);
    wrefresh(stdscr);
    break;
  }
}
