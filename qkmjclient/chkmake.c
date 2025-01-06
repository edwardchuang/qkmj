#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// 定義牌型種類
#define THREE_CARD 1  // 刻子
#define STRAIGHT_CARD 2 // 順子
#define PAIR_CARD 3 // 對子

#include "qkmj.h"

/*******************  Denifition of variables  **********************/

typedef struct nodetype *NODEPTR;
struct card_info_type card_info[20];
char pool_buf[20];
struct component_type card_component[6][50];
int comb_count[6];
int count2;
int suit;

/********************************************************************/

// 取得一個新的節點，並初始化其成員
NODEPTR getnode()
{
  NODEPTR p = (NODEPTR)calloc(1, sizeof(struct nodetype)); // 使用 calloc 一次性分配並初始化記憶體
  if (p == NULL) {
    // 處理記憶體配置失敗的情況，例如印出錯誤訊息或結束程式
    perror("Failed to allocate memory for node"); // 顯示錯誤訊息
    exit(EXIT_FAILURE); // 結束程式
  }
  return p;
}


// 建立一個新的樹節點
NODEPTR maketree(NODEPTR node)
{
  NODEPTR p = getnode();  // 使用 getnode() 來取得並初始化節點
  p->father = node;       // 設定父節點
  return p;
}

// 建構樹狀結構，遞迴呼叫 make_three, make_straight, make_pair
void build_tree(NODEPTR node)
{
  if (node == NULL) {    // 防禦式程式設計，避免空指標
    return;
  }

  node->three = make_three(node);  /* 檢查是否已達終端 */
  if (!node->end) {
    node->straight = make_straight(node);
  } 
  if (!node->end && node->three == NULL) { /* 若無刻子，才加入對子 */
    node->pair = make_pair(node);
  }
}

// 標記已選取的牌
void mark_card(NODEPTR node)
{
  int j;

  if (node == NULL || node->father == NULL) { // 防禦式程式設計 & 終止條件
    return;
  }

  for(j=0; card_info[j].info != 0; j++) { // 使用 0 作為牌資訊的終止條件，避免溢位
    if (strchr(node->info, card_info[j].info) && card_info[j].flag) { // 使用 strchr 查找牌
      card_info[j].flag = 0;   /* 標記此牌已選取 */
      break; // 一旦找到並標記牌，就跳出迴圈
    }
  }

  mark_card(node->father); // 遞迴呼叫父節點
}

// 尋找未被標記的領頭牌
int find_lead()
{
  int i;

  for (i = 0; card_info[i].info != 0; i++) { // 使用 0 作為牌資訊的終止條件，避免溢位
    if (card_info[i].flag) {
      return card_info[i].info; // 直接回傳找到的牌值
    }
  }
  return 0; // 沒有找到領頭牌，回傳 0
}

// 嘗試組成刻子
NODEPTR make_three(NODEPTR node)
{
  int i, j;
  int lead;
  NODEPTR p;

  // 重置標記
  for (i = 0; card_info[i].info != 0; i++) {
    card_info[i].flag = 1;
  }
  mark_card(node);

  lead = find_lead();
  if (lead == 0) {
    // 沒有更多牌可檢查
    node->end = 1;
    return NULL;
  }

  p = maketree(node);

  // 嘗試取得三張相同的牌
  j = 0;
  for (i = 0; card_info[i].info != 0; i++) { // 使用 0 作為牌資訊的終止條件，避免溢位
    if (card_info[i].flag && card_info[i].info == lead) {
      p->info[j++] = lead;
      card_info[i].flag = 0;
    }
    if (j == 3) {
      break; // 找到三張牌，跳出迴圈
    }
  }

  if (j != 3) {
    // 找不到三張相同的牌
    free(p);  // 釋放已分配的記憶體
    return NULL;
  }

  p->info[j] = 0; //  確保字串結尾
  p->type = THREE_CARD;
  build_tree(p);
  return p;
}

// 嘗試組成順子
NODEPTR make_straight(NODEPTR node)
{
  int i, j;
  int lead;
  NODEPTR p;

  // 重置標記
  for (i = 0; card_info[i].info != 0; i++) {
    card_info[i].flag = 1;
  }
  mark_card(node);

  lead = find_lead();
  if (lead == 0) {
    // 沒有更多牌可檢查
    node->end = 1;
    return NULL;
  }

  p = maketree(node);

  // 嘗試取得三張順子牌
  j = 0;
  for (i = 0; card_info[i].info != 0 ; i++) {
    if (card_info[i].flag && card_info[i].info == lead) {
      p->info[j++] = lead;
      card_info[i].flag = 0;
      lead++;  //  Increment lead to look for the next card in the straight
    }
      if (j == 3 && lead < 31) { // Check for a complete straight and valid card value
        break;
      }
  }

  if (j != 3) {
    // 找不到三張順子牌
    free(p); // 釋放已分配的記憶體
    return NULL;
  }

  p->info[j] = 0; // 確保字串結尾
  p->type = STRAIGHT_CARD;
  build_tree(p);
  return p;
}

// 嘗試組成對子
NODEPTR make_pair(NODEPTR node)
{
  int i, j;
  int lead;
  NODEPTR p;

  // 重置標記
  for (i = 0; card_info[i].info != 0; i++) {
    card_info[i].flag = 1;
  }
  mark_card(node);

  lead = find_lead();
  if (lead == 0) {
    // 沒有更多牌可檢查
    node->end = 1;
    return NULL;
  }

  p = maketree(node);

  // 嘗試取得兩張相同的牌
  j = 0;
  for (i = 0; card_info[i].info != 0; i++) {
    if (card_info[i].flag && card_info[i].info == lead) {
      p->info[j++] = lead;
      card_info[i].flag = 0;
    }
    if (j == 2) {
      break; //  找到兩張牌，跳出迴圈
    }
  }

  if (j != 2) {
    // 找不到兩張相同的牌
    free(p); // 釋放已分配的記憶體
    return NULL;
  }

  p->info[j] = 0; // 確保字串結尾
  p->type = PAIR_CARD;
  build_tree(p);
  return p;
}

// 將有效路徑複製到陣列
void list_path(NODEPTR p)
{
  int i = 0;

  while (p->info[i] != 0)
  {
    // 使用 memcpy 複製字串，避免潛在的溢位問題
    memcpy(card_component[suit][comb_count[suit]].info[count2], p->info, sizeof(p->info)); // 使用 sizeof 確保複製正確的大小
    i++;
  }

  if (p->type == PAIR_CARD) {
    card_component[suit][comb_count[suit]].type = (card_component[suit][comb_count[suit]].type >= 3) ? 4 : 3; // 簡化條件判斷
  }

  count2++;
  if (p->father && p->father->father)  // 檢查父節點和祖父節點是否存在
  {
    list_path(p->father);
  }
}

// 前序遍歷樹狀結構
void pretrav(NODEPTR p)
{
  if (p != NULL)
  {
    if (p->end)  // 檢查節點是否為終端節點
    {
      count2 = 0;
      if (p->father != NULL) // 檢查父節點是否存在
      {
        card_component[suit][comb_count[suit]].type = 2;
        list_path(p);
      }
      else
      {
        card_component[suit][0].type = 1; //  沒有牌在此花色
      }
      comb_count[suit]++;
    }
    pretrav(p->three);
    pretrav(p->straight);
    pretrav(p->pair);
  }
}

// 釋放樹狀結構的記憶體
void free_tree(NODEPTR p)
{
  if (p == NULL) { // 處理空指標的情況
    return;
  }

  // 使用後序遍歷釋放子樹記憶體，以避免 dangling pointers
  free_tree(p->three);
  free_tree(p->straight);
  free_tree(p->pair);

  free(p); // 釋放當前節點的記憶體
}

// 比較函數，用於 qsort 排序
int char_compare(const void *a, const void *b) {
  return (*(char *)a - *(char *)b);
}

// 檢查是否胡牌，並計算分數
int check_make(char sit, char card, char method)
{
  NODEPTR p[5];  // 每個花色的節點指標陣列
  int i, j, k;
  int pair = 0; // 初始化對子計數
  int make = 1; // 初始化胡牌旗標（預設為胡牌）

  // 複製牌池到緩衝區，並加入要檢查的牌
  for (i = 0; i < pool[sit].num; i++) {
    pool_buf[i] = pool[sit].card[i];
  }
  pool_buf[i] = card;
  pool_buf[i + 1] = 0;

  // 使用 qsort 高效排序手牌（包含新增的牌）
  qsort(pool_buf, pool[sit].num + 1, sizeof(char), char_compare);

  // 初始化牌型組合
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 20; j++) {
      card_component[i][j].type = 0;
      for (k = 0; k < 10; k++) {
        card_component[i][j].info[k][0] = 0;
      }
    }
    comb_count[i] = 0;
  }

  // 建立組合樹並分析手牌
  j = 0;
  for (suit = 0; suit < 5; suit++) {
    k = 0;
    while (pool_buf[j] < suit * 10 + 10 && pool_buf[j] > suit * 10) {
      card_info[k].info = pool_buf[j];
      card_info[k].flag = 1;
      j++;
      k++;
    }
    card_info[k].info = 0;

    p[suit] = maketree(NULL);
    card_component[suit][0].type = 0;
    build_tree(p[suit]);
    pretrav(p[suit]);
    free_tree(p[suit]);

    // 在樹狀結構處理過程中檢查無效牌型
    switch (card_component[suit][0].type) {
      case 0: // 無效組合
        make = 0;
        break; // 提前結束花色迴圈
      case 3: // 找到一個對子
        pair++;
        break;
      case 4: // 找到多個對子，無效牌型
        make = 0;
        break;  // 提前結束花色迴圈
      default:
        break; // type 1 和 2 為有效牌型
    }

    // 如果已發現無效牌型，則提前結束迴圈
    if (!make) {
      break;
    }
  }

  // 在處理完所有花色後進行最終檢查
  if (make) {  // 僅在 make 仍然為 true 時才進行檢查
    if (pair != 1) { // 必須恰好有一個對子
      make = 0;
    }
  }


  // 如果需要，則進行進一步的計分分析
  if (make && method) {
    full_check(sit, card);
  }

  return make;
}

// 驗證牌型是否有效
int valid_type(int type)
{
  return (type >= 1 && type <= 3);
}

// 找出得分最高的牌型組合
void full_check(char sit, char make_card)
{
  int i, j, set1, set2, card, count;
  int suit_count[5];

  count = 0;
  for (i = 0; i < 5; i++) {
    card_comb[i].set_count = 0;
  }

  // 使用 memset 初始化 suit_count 陣列
  memset(suit_count, 0, sizeof(suit_count));

  //  提前計算 valid_type 結果，避免重複計算
  int valid_counts[5];
  for (i = 0; i < 5; i++) {
    valid_counts[i] = 0;
    for (j = 0; j < comb_count[i]; j++) {
      if (valid_type(card_component[i][j].type)) {
        valid_counts[i]++;
      }
    }
  }

  // 使用改良的迴圈結構，只遍歷有效組合，大幅提升效能
  int valid_indices[5];
  memset(valid_indices, 0, sizeof(valid_indices)); // 初始化索引

  while (true) {  // 外層迴圈控制所有花色的組合
    set2 = 0;
    for (i = 0; i < 5; i++) {
      int current_valid_index = 0;
      for (j = 0; j < comb_count[i]; j++) {
        if (valid_type(card_component[i][j].type)) {
          if (current_valid_index == valid_indices[i]) {
             // 處理有效組合
            set1 = 0;
            while (card_component[i][j].info[set1][0]) {
              card = 0;
              while (card_component[i][j].info[set1][card]) {
                card_comb[count].info[set2][card + 1] = card_component[i][j].info[set1][card];
                card++;
              }
              card_comb[count].info[set2][0] = (card_comb[count].info[set2][1] == card_comb[count].info[set2][2]) ? 2 : 1;
              if (card == 2) {
                card_comb[count].info[set2][0] = 10; // 對子
              }
              card_comb[count].info[set2][card + 1] = 0;
              set1++;
              set2++;
            }
            break; // 跳出內層迴圈
          }
          current_valid_index++;
        }
      }
    }

    card_comb[count].set_count = set2;
    check_tai(sit, count, make_card);
    count++;

    // 更新索引，類似計數器進位
    int carry = 1;
    for (i = 4; i >= 0; i--) {
      valid_indices[i] += carry;
      if (valid_indices[i] == valid_counts[i]) {
        valid_indices[i] = 0;
        carry = 1;
      } else {
        carry = 0;
        break;
      }
    }
    if (carry == 1) {  // 所有組合都已遍歷
      break;  // 跳出外層迴圈
    }

  } // while loop

  comb_num = count;
}

// 檢查牌是否存在於手牌和棄牌中
int exist_card(char sit, char card)
{
  int i, exist = 0;

  // 使用二分搜尋法，提升效能 (假設 pool_buf 已排序)
  char* found = bsearch(&card, pool_buf, pool[sit].num + 1, sizeof(char), char_compare);
  if (found != NULL) {
    // 計算找到的牌的數量 (處理重複牌)
    for (i = found - pool_buf; i <= pool[sit].num; i++) {  // 向後搜尋
      if (pool_buf[i] == card) {
        exist++;
      } else {
        break;  // 停止搜尋
      }
    }
    for (i = found - pool_buf -1; i >= 0; i--) {   // 向前搜尋
        if (pool_buf[i] == card) {
            exist++;
        } else {
            break; // 停止搜尋
        }
    }

  }


  // 檢查棄牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
    // 使用 strchr 搜尋棄牌，提升效能
    if (strchr(pool[sit].out_card[i] + 1, card) != NULL) { // +1 跳過第一個元素 (out_card type)
      exist++;
    }
  }
  return exist;
}

// 檢查刻子是否存在於手牌和棄牌中
int exist_3(char sit, char card, int comb)
{
  int set, exist = 0;

  // 檢查手牌組合
  set = 0;
  while (card_comb[comb].info[set][0]) {
    if (card_comb[comb].info[set][0] == 2 && card_comb[comb].info[set][1] == card) {
      exist++;
    }
    set++;
  }

  // 檢查棄牌 (假設已排序)
    for (int i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][1] == card && pool[sit].out_card[i][2] == card && pool[sit].out_card[i][3] == card) {
        exist++;
      }
    }
  return exist;
}

// 檢查順子是否存在於手牌和棄牌中
int exist_straight(char sit, char card, int comb)
{
  int i, set, exist = 0;

  // 檢查手牌組合
  set = 0;
  while (card_comb[comb].info[set][0]) {
    if (card_comb[comb].info[set][0] == 1 &&
        card_comb[comb].info[set][1] == card) {
      exist++;
    }
    set++;
  }

  // 檢查棄牌 (最佳化：直接檢查順子，避免字元比對)
  for (i = 0; i < pool[sit].out_card_index; i++) {
    // 判斷棄牌類型是否為順子 (假設 pool[sit].out_card 已排序）
    if (pool[sit].out_card[i][0] >= 7 && // 7, 8, 9 代表順子類型
        pool[sit].out_card[i][0] <= 9) {

      if (pool[sit].out_card[i][0] == 7 && 
          pool[sit].out_card[i][2] == card) {
        exist++;
      } else if (pool[sit].out_card[i][0] == 8 &&
                 pool[sit].out_card[i][1] == card) {
        exist++;
      } else if (pool[sit].out_card[i][0] == 9 &&
                 pool[sit].out_card[i][1] == card) {
        exist++;
      }
    }
  }
  return exist;
}

// 檢查所有台數
void check_tai(char sit, char comb, char make_card)
{
  // --- 風牌和字牌相關台數，合併檢查以減少 exist_card() 呼叫次數 ---
  int east = exist_card(sit, 31);
  int south = exist_card(sit, 32);
  int west = exist_card(sit, 33);
  int north = exist_card(sit, 34);
  int red = exist_card(sit, 41);
  int white = exist_card(sit, 42);
  int green = exist_card(sit, 43);

  // 初始化台數分數
  memset(card_comb[comb].tai_score, 0, sizeof(card_comb[comb].tai_score));

  // --- 獨立的台數檢查 ---
  check_tai0(sit, comb);    // 莊家
  check_tai1(sit, comb);    // 門清
  check_tai2(sit, comb);    // 自摸
  check_tai3(sit, comb);    // 斷么九
  check_tai5(sit, comb);    // 槓上開花
  check_tai6(sit, comb);    // 海底撈月
  check_tai7(sit, comb);    // 河底撈魚
  check_tai8(sit, comb);    // 搶槓
  check_tai44(sit, comb);  // 七搶一  (與其他風牌台數無關)
  check_tai16(sit, comb);  // 花牌
  check_tai21(sit, comb);  // 春夏秋冬
  check_tai22(sit, comb);  // 梅蘭菊竹
  check_tai23(sit, comb);  // 全求人
  check_tai49(sit, comb);  // 天胡
  check_tai50(sit, comb);  // 地胡
  check_tai51(sit, comb);  // 人胡
  check_tai52(sit, comb);  // 連莊

  // 風牌台數
  if (east >= 3) check_tai9(sit, comb, east);    // 東風/東風東
  if (south >= 3) check_tai10(sit, comb, south);  // 南風/南風南
  if (west >= 3) check_tai11(sit, comb, west);   // 西風/西風西
  if (north >= 3) check_tai12(sit, comb, north); // 北風/北風北

  if (east >= 2 && south >= 2 && west >= 2 && north >= 2) {
    check_tai41(sit, comb);  // 小四喜
  }
  if (east >= 3 && south >= 3 && west >= 3 && north >= 3) {
    check_tai47(sit, comb);  // 大四喜
  }

  check_tai48(sit, comb);  // 八仙過海 (需要在風牌台數計算後檢查)

  // 字牌台數
  if (red >= 3) check_tai13(sit, comb);     // 紅中
  if (white >= 3) check_tai14(sit, comb);   // 白板
  if (green >= 3) check_tai15(sit, comb);  // 青發

  if (red >= 2 && white >= 2 && green >= 2) {
    check_tai37(sit, comb);  // 小三元
  }
  if (red >= 3 && white >= 3 && green >= 3) {
    check_tai40(sit, comb);  // 大三元
  }

  // --- 其他需要 make_card 參數的台數 ---
  check_tai24(sit, comb, make_card); // 平胡
  check_tai45(sit, comb, make_card); // 三/四/五暗刻

  // --- 其他台數 ---
  check_tai4(sit, comb);   // 雙龍抱/雙雙龍抱
  check_tai25(sit, comb);  // 混帶么
  check_tai26(sit, comb);  // 三色同順
  check_tai27(sit, comb);  // 一條龍
  check_tai30(sit, comb);  // 三杠子
  check_tai31(sit, comb);  // 三色同刻
  check_tai32(sit, comb);  // 門清自摸
  check_tai33(sit, comb);  // 碰碰胡
  check_tai34(sit, comb);  // 混一色
  check_tai35(sit, comb);  // 純帶么
  check_tai36(sit, comb);  // 混老頭
  check_tai39(sit, comb);  // 四杠子/三杠子
  check_tai42(sit, comb);  // 清一色
  check_tai43(sit, comb);  // 字一色
  check_tai46(sit, comb);  // 清老頭
}
// 莊家
void check_tai0(char sit, char comb)
{
  if (info.dealer == sit) {
    card_comb[comb].tai_score[0] = tai[0].score;
  }
}

// 門清
void check_tai1(char sit, char comb)
{
  if (pool[sit].num == 16) {
    card_comb[comb].tai_score[1] = tai[1].score;
  }
}

// 自摸
void check_tai2(char sit, char comb)
{
  if (sit == card_owner) {
    card_comb[comb].tai_score[2] = tai[2].score;
  }
}

// 斷么九
void check_tai3(char sit, char comb)
{
  // 使用區域變數提升效能，避免重複呼叫 exist_card()
  bool exist1 = exist_card(sit, 1);
  bool exist9 = exist_card(sit, 9);
  bool exist11 = exist_card(sit, 11);
  bool exist19 = exist_card(sit, 19);
  bool exist21 = exist_card(sit, 21);
  bool exist29 = exist_card(sit, 29);
  bool exist31 = exist_card(sit, 31);
  bool exist32 = exist_card(sit, 32);
  bool exist33 = exist_card(sit, 33);
  bool exist34 = exist_card(sit, 34);
  bool exist41 = exist_card(sit, 41);
  bool exist42 = exist_card(sit, 42);
  bool exist43 = exist_card(sit, 43);


  // 簡化條件判斷，將多個條件合併成一個
  if (!exist1 && !exist9 && !exist11 && !exist19 && !exist21 && 
      !exist29 && !exist31 && !exist32 && !exist33 && !exist34 &&
      !exist41 && !exist42 && !exist43) {
    card_comb[comb].tai_score[3] = tai[3].score;
  }
}

// 雙龍抱
void check_tai4(char sit, char comb)
{
  if (pool[sit].num != 16) {  // 必須門清
    return;
  }

  int straight[30] = {0}; // 初始化為 0
  int double_straight_num = 0;
  int i;

  // 統計手牌中的順子
  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) {
      straight[card_comb[comb].info[i][1]]++;
    }
  }

  // 統計棄牌中的順子
  for (i = 0; i < pool[sit].out_card_index; i++) {
    switch (pool[sit].out_card[i][0]) {
      case 7:
        straight[pool[sit].out_card[i][2]]++;
        break;
      case 8:
      case 9:
        straight[pool[sit].out_card[i][1]]++;
        break;
    }
  }

  // 計算雙龍抱數量
  for (i = 0; i < 30; i++) {
    if (straight[i] >= 2) {
      double_straight_num++;
    }
  }

  if (double_straight_num == 1) {
    card_comb[comb].tai_score[4] = tai[4].score;
  } else if (double_straight_num >= 2) {
    card_comb[comb].tai_score[28] = tai[28].score; // 雙雙龍抱
  }
}

// 槓上開花
void check_tai5(char sit, char comb)
{
  if (in_kang) {
    card_comb[comb].tai_score[5] = tai[5].score;
  }
}

// 海底摸月
void check_tai6(char sit, char comb)
{
  if ((144 - card_point) == 16 && sit == card_owner) {
    card_comb[comb].tai_score[6] = tai[6].score;
  }
}

// 河底撈魚
void check_tai7(char sit, char comb)
{
  if ((144 - card_point) == 16 && sit != card_owner) {
    card_comb[comb].tai_score[7] = tai[7].score;
  }
}

/* 搶槓 */
void check_tai8(char sit, char comb)
{
  //  目前沒有任何邏輯，保留空函數
}

/* 東風 */
void check_tai9(char sit, char comb, int count) {
  if (count >= 3) {
    card_comb[comb].tai_score[9] =
        (info.wind == 1 ? tai[9].score : 0) +
        (pool[sit].door_wind == 1 ? tai[9].score : 0);

    if (card_comb[comb].tai_score[9] == tai[9].score * 2) {
      card_comb[comb].tai_score[17] = tai[17].score;
      card_comb[comb].tai_score[9] = 0;
    }
  }
}

/* 南風 */
void check_tai10(char sit, char comb, int count)
{
  if (count >= 3) {
    card_comb[comb].tai_score[10] =
        (info.wind == 2 ? tai[10].score : 0) +
        (pool[sit].door_wind == 2 ? tai[10].score : 0);

    if (card_comb[comb].tai_score[10] == tai[10].score * 2) {
      card_comb[comb].tai_score[18] = tai[18].score;
      card_comb[comb].tai_score[10] = 0;
    }
  }
}

/* 西風 */
void check_tai11(char sit, char comb, int count)
{
  if (count >= 3) {
    card_comb[comb].tai_score[11] =
        (info.wind == 3 ? tai[11].score : 0) +
        (pool[sit].door_wind == 3 ? tai[11].score : 0);

    if (card_comb[comb].tai_score[11] == tai[11].score * 2) {
      card_comb[comb].tai_score[19] = tai[19].score;
      card_comb[comb].tai_score[11] = 0;
    }
  }
}

/* 北風 */
void check_tai12(char sit, char comb, int count)
{
  if (count >= 3) {
    card_comb[comb].tai_score[12] =
        (info.wind == 4 ? tai[12].score : 0) +
        (pool[sit].door_wind == 4 ? tai[12].score : 0);

    if (card_comb[comb].tai_score[12] == tai[12].score * 2) {
      card_comb[comb].tai_score[20] = tai[20].score; // 修正：應該是 20，不是 20.tai_score[20]
      card_comb[comb].tai_score[12] = 0;
    }
  }
}

/* 紅中 */
void check_tai13(char sit, char comb)
{
  if (exist_card(sit, 41) >= 3) {
    card_comb[comb].tai_score[13] = tai[13].score;
  }
}

/* 白板 */
void check_tai14(char sit, char comb)
{
  if (exist_card(sit, 42) >= 3) {
    card_comb[comb].tai_score[14] = tai[14].score;
  }
}

/* 青發 */
void check_tai15(char sit, char comb)
{
  if (exist_card(sit, 43) >= 3) {
    card_comb[comb].tai_score[15] = tai[15].score;
  }
}

/* 花牌 */
void check_tai16(char sit, char comb)
{
  // 使用加法簡化，避免重複計算 tai[16].score
  card_comb[comb].tai_score[16] =
      (pool[sit].flower[pool[sit].door_wind - 1] ? tai[16].score : 0) +
      (pool[sit].flower[pool[sit].door_wind + 3] ? tai[16].score : 0);
}

/* 東風東 */
void check_tai17(char sit, char comb)  //  空函數，與 check_tai9() 合併處理
{
  //  此台數已在 check_tai9() 中處理
}


/* 南風南 */
void check_tai18(char sit, char comb) //  空函數，與 check_tai10() 合併處理
{
  // 此台數已在 check_tai10() 中處理
}

/* 西風西 */
void check_tai19(char sit, char comb) //  空函數，與 check_tai11() 合併處理
{
  // 此台數已在 check_tai11() 中處理
}

/* 北風北 */
void check_tai20(char sit, char comb)  //  空函數，與 check_tai12() 合併處理
{
  // 此台數已在 check_tai12() 中處理
}

/* 春夏秋冬 */
void check_tai21(char sit, char comb)
{
  if (pool[sit].flower[0] && pool[sit].flower[1] &&
      pool[sit].flower[2] && pool[sit].flower[3]) {
    card_comb[comb].tai_score[21] = tai[21].score;
    card_comb[comb].tai_score[16] -= tai[16].score * 4; // 修正：一次扣除四個花牌的分數
  }
}

/* 梅蘭菊竹 */
void check_tai22(char sit, char comb)
{
  if (pool[sit].flower[4] && pool[sit].flower[5] &&
      pool[sit].flower[6] && pool[sit].flower[7]) {
    card_comb[comb].tai_score[22] = tai[22].score;
    card_comb[comb].tai_score[16] -= tai[16].score * 4; // 修正：一次扣除四個花牌的分數
  }
}

/* 全求人 */
void check_tai23(char sit, char comb)
{
  if (pool[sit].num == 1 && sit != card_owner) {
    card_comb[comb].tai_score[23] = tai[23].score;
  }
}

/* 平胡 */
void check_tai24(char sit, char comb, char make_card)
{
  int i;

  // 使用迴圈和提前 return 提升效能
  for (i = 0; i < 8; i++) {
    if (pool[sit].flower[i]) {
      return; // 有花牌，非平胡
    }
  }

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2 || // 有刻子，非平胡
        (card_comb[comb].info[i][0] == 10 && // 對子是胡牌的牌或字牌
         (card_comb[comb].info[i][1] == make_card ||
          card_comb[comb].info[i][1] > 30))) {
      return;
    }
  }

  //if (pool[sit].out_card_index == 0) return;
  // 移除此條件，因為題目沒有說明平胡需要有棄牌


  if (turn == card_owner) {   // 自摸，非平胡
    return;
  }

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == pool[sit].out_card[i][2]) {
      return; // 棄牌中有刻子，非平胡
    }
  }

  // 兩面聽牌檢查
  if (make_card % 10 <= 6 && check_make(sit, make_card + 3, 0)) {
    card_comb[comb].tai_score[24] = tai[24].score;
    return;
  }
  if (make_card % 10 >= 4 && check_make(sit, make_card - 3, 0)) {
    card_comb[comb].tai_score[24] = tai[24].score;
    return;
  }

  //check_make(sit, make_card, 0); // 重置 pool_buf[] (似乎多餘，已在 check_make() 內部處理)
}

/* 混帶么 */
void check_tai25(char sit, char comb)
{
  int i, exist19 = 0;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][1] < 30) {
      exist19 = 1;
      if (!(card_comb[comb].info[i][1] % 10 == 1 ||
            card_comb[comb].info[i][1] % 10 == 9 ||
            card_comb[comb].info[i][2] % 10 == 1 ||
            card_comb[comb].info[i][2] % 10 == 9 ||
            card_comb[comb].info[i][3] % 10 == 1 ||
            card_comb[comb].info[i][3] % 10 == 9)) {
        return; // 非么九牌
      }
    }
  }

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] < 30) {
      if (!(pool[sit].out_card[i][1] % 10 == 1 ||
            pool[sit].out_card[i][1] % 10 == 9 ||
            pool[sit].out_card[i][2] % 10 == 1 ||
            pool[sit].out_card[i][2] % 10 == 9 ||
            pool[sit].out_card[i][3] % 10 == 1 ||
            pool[sit].out_card[i][3] % 10 == 9)) {
        return; // 非么九牌
      }
    }
  }

  if (exist19) {
    card_comb[comb].tai_score[25] = tai[25].score - (pool[sit].num != 16); // 簡化條件運算
  }
}

/* 三色同順 */
void check_tai26(char sit, char comb)
{
  int i, num;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) {
      num = card_comb[comb].info[i][1] % 10;
      if (exist_straight(sit, num, comb) &&
          exist_straight(sit, num + 10, comb) &&
          exist_straight(sit, num + 20, comb)) {
        card_comb[comb].tai_score[26] = tai[26].score - (pool[sit].num != 16); // 簡化條件運算
        return;
      }
    }
  }

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] >= 7 && pool[sit].out_card[i][0] <= 9) {
      num = (pool[sit].out_card[i][0] == 7) ?
              pool[sit].out_card[i][2] % 10 : pool[sit].out_card[i][1] % 10;
      if (exist_straight(sit, num, comb) &&
          exist_straight(sit, num + 10, comb) &&
          exist_straight(sit, num + 20, comb)) {
        card_comb[comb].tai_score[26] = tai[26].score - (pool[sit].num != 16); //簡化條件運算
        return;
      }
    }
  }
}

/* 一條龍 */
void check_tai27(char sit, char comb)
{
  if ((exist_straight(sit, 1, comb) &&
       exist_straight(sit, 4, comb) &&
       exist_straight(sit, 7, comb)) ||
      (exist_straight(sit, 11, comb) &&
       exist_straight(sit, 14, comb) &&
       exist_straight(sit, 17, comb)) ||
      (exist_straight(sit, 21, comb) &&
       exist_straight(sit, 24, comb) &&
       exist_straight(sit, 27, comb))) {

    card_comb[comb].tai_score[27] = tai[27].score - (pool[sit].num != 16); // 簡化條件運算
  }
}

/* 雙雙龍抱 */
void check_tai28(char sit, char comb) // 已在 check_tai4 處理
{
   //  此台數已在 check_tai4() 中處理
}

/* 三暗刻 */
void check_tai29(char sit, char comb) // 已在 check_tai45 處理
{
    // 此台數已在 check_tai45() 中處理
}

/* 三杠子 */
void check_tai30(char sit, char comb) // 已在 check_tai39 處理
{
    // 此台數已在 check_tai39() 中處理
}

/* 三色同刻 */
void check_tai31(char sit, char comb)
{
  int i, num;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2) {
      num = card_comb[comb].info[i][1] % 10;
      if (exist_3(sit, num, comb) &&
          exist_3(sit, num + 10, comb) &&
          exist_3(sit, num + 20, comb)) {
        card_comb[comb].tai_score[31] = tai[31].score;
        return; // 找到組合，提前返回
      }
    }
  }

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == pool[sit].out_card[i][2] &&
        pool[sit].out_card[i][1] == pool[sit].out_card[i][3]) {

      num = pool[sit].out_card[i][1] % 10;

      if (exist_3(sit, num, comb) &&
          exist_3(sit, num + 10, comb) &&
          exist_3(sit, num + 20, comb)) {
        card_comb[comb].tai_score[31] = tai[31].score;
        return; // 找到組合，提前返回
      }
    }
  }
}

/* 門清自摸 */
void check_tai32(char sit, char comb)
{
  if (pool[sit].num == 16 && sit == card_owner) {
    card_comb[comb].tai_score[32] = tai[32].score;
    card_comb[comb].tai_score[1] = 0; // 取消門清
    card_comb[comb].tai_score[2] = 0; // 取消自摸
  }
}

/* 碰碰胡 */
void check_tai33(char sit, char comb)
{
  int i;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) { // 有順子，非碰碰胡
      return; // 提前返回
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] != pool[sit].out_card[i][2]) { // 棄牌非碰或槓
      return; // 提前返回
    }
  }
  card_comb[comb].tai_score[33] = tai[33].score;
}

/* 混一色 */
void check_tai34(char sit, char comb)
{
  int i, kind;

  // 先設置台數，稍後判斷是否取消
  card_comb[comb].tai_score[34] = tai[34].score;

  // 找到第一張牌的花色
  for (i = 0; i <= pool[sit].num; i++) {
    kind = pool_buf[i] / 10;
    if (kind <= 2) {
      break; // 找到花色，跳出迴圈
    }
  }

  // 檢查手牌和棄牌是否符合混一色
  for (i = 0; i <= pool[sit].num; i++) {
    if ((pool_buf[i] / 10 != kind && pool_buf[i] / 10 <= 2) || // 手牌花色不符
         pool_buf[i] > 43) {                                    // 字牌檢查
      card_comb[comb].tai_score[34] = 0;
      return;
    }
  }

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if ((pool[sit].out_card[i][1] / 10 != kind && 
         pool[sit].out_card[i][1] / 10 <= 2) || // 棄牌花色不符
         pool[sit].out_card[i][1] > 43) {           // 字牌檢查
      card_comb[comb].tai_score[34] = 0;
      return;
    }
  }

}

/* 純帶么 */
void check_tai35(char sit, char comb)
{
  int i;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][1] > 30) { // 有字牌
      return;
    }
    // 檢查是否為么九牌
    if (!(card_comb[comb].info[i][1] % 10 == 1 || 
          card_comb[comb].info[i][1] % 10 == 9 ||
          card_comb[comb].info[i][2] % 10 == 1 ||
          card_comb[comb].info[i][2] % 10 == 9 ||
          card_comb[comb].info[i][3] % 10 == 1 ||
          card_comb[comb].info[i][3] % 10 == 9)) {
      return;
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] > 30) { // 有字牌
      return;
    }
    if (!(pool[sit].out_card[i][1] % 10 == 1 ||
          pool[sit].out_card[i][1] % 10 == 9 ||
          pool[sit].out_card[i][2] % 10 == 1 ||
          pool[sit].out_card[i][2] % 10 == 9 ||
          pool[sit].out_card[i][3] % 10 == 1 ||
          pool[sit].out_card[i][3] % 10 == 9)) {
      return;
    }

  }
  card_comb[comb].tai_score[35] = tai[35].score;
  card_comb[comb].tai_score[25] = 0; // 取消混帶么
}

/* 混老頭 */
void check_tai36(char sit, char comb)
{
  int i, exist19 = 0;

  for (i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] < 30) {
      exist19 = 1;
      if (pool_buf[i] % 10 != 1 && pool_buf[i] % 10 != 9) {
        return; // 非么九牌
      }
    }
  }

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] < 30) {
      exist19 = 1;
      if (pool[sit].out_card[i][1] % 10 != 1 && 
          pool[sit].out_card[i][1] % 10 != 9) {
        return;
      }
        if (pool[sit].out_card[i][2] % 10 != 1 && 
          pool[sit].out_card[i][2] % 10 != 9) {
        return;
      }
      if (pool[sit].out_card[i][3] % 10 != 1 && 
          pool[sit].out_card[i][3] % 10 != 9) {
        return;
      }


    }
  }

  if (exist19) {
    card_comb[comb].tai_score[36] = tai[36].score;
    card_comb[comb].tai_score[25] = 0; // 取消混帶么
    card_comb[comb].tai_score[33] = 0; // 取消碰碰胡
  }
}

/* 小三元 */
void check_tai37(char sit, char comb)
{
  // 使用區域變數避免重複呼叫 exist_card
  int red = exist_card(sit, 41);
  int white = exist_card(sit, 42);
  int green = exist_card(sit, 43);

  if (red >= 2 && white >= 2 && green >= 2) {
    card_comb[comb].tai_score[37] = tai[37].score;
    card_comb[comb].tai_score[13] = 0; // 取消紅中
    card_comb[comb].tai_score[14] = 0; // 取消白板
    card_comb[comb].tai_score[15] = 0; // 取消青發
  }
}

/* 四暗刻 */ // 此函數與 check_tai45 重複，將在 check_tai45 處理
void check_tai38(char sit, char comb)
{
    //  此台數已在 check_tai45() 中處理
}

/* 四杠子 */
void check_tai39(char sit, char comb)
{
  int i, kang_count = 0;

  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 3 || // 明槓
        pool[sit].out_card[i][0] == 11 || // 暗槓
        pool[sit].out_card[i][0] == 12    // 加槓
       ) {
      kang_count++;
    }
  }

  if (kang_count == 3) {
    card_comb[comb].tai_score[30] = tai[30].score;  // 三杠子
  } else if (kang_count == 4) {
    card_comb[comb].tai_score[39] = tai[39].score;  // 四杠子
  }
}

/* 大三元 */
void check_tai40(char sit, char comb)
{
  // 使用區域變數避免重複呼叫 exist_card
  int red = exist_card(sit, 41);
  int white = exist_card(sit, 42);
  int green = exist_card(sit, 43);

  if (red >= 3 && white >= 3 && green >= 3) {
    card_comb[comb].tai_score[40] = tai[40].score;
    card_comb[comb].tai_score[13] = 0; // 取消紅中
    card_comb[comb].tai_score[14] = 0; // 取消白板
    card_comb[comb].tai_score[15] = 0; // 取消青發
    card_comb[comb].tai_score[37] = 0; // 取消小三元
  }
}

/* 小四喜 */
void check_tai41(char sit, char comb)
{
  // 使用區域變數避免重複呼叫 exist_card
  int east = exist_card(sit, 31);
  int south = exist_card(sit, 32);
  int west = exist_card(sit, 33);
  int north = exist_card(sit, 34);

  if (east >= 2 && south >= 2 && west >= 2 && north >= 2) {
    card_comb[comb].tai_score[41] = tai[41].score;
    card_comb[comb].tai_score[9] = 0;   // 取消東風
    card_comb[comb].tai_score[10] = 0;  // 取消南風
    card_comb[comb].tai_score[11] = 0;  // 取消西風
    card_comb[comb].tai_score[12] = 0;  // 取消北風
    card_comb[comb].tai_score[17] = 0;  // 取消東風東
    card_comb[comb].tai_score[18] = 0;  // 取消南風南
    card_comb[comb].tai_score[19] = 0;  // 取消西風西
    card_comb[comb].tai_score[20] = 0;  // 取消北風北
  }
}

/* 清一色 */
void check_tai42(char sit, char comb)
{
  int i, kind;

  card_comb[comb].tai_score[42] = tai[42].score;

  // 找到第一張牌的花色
  kind = pool_buf[0] / 10;
  if (kind >= 3) {  // 字牌，非清一色
    card_comb[comb].tai_score[42] = 0;
    return;
  }

  // 檢查手牌
  for (i = 1; i <= pool[sit].num; i++) {
    if (pool_buf[i] / 10 != kind) {
      card_comb[comb].tai_score[42] = 0;
      return;
    }
  }

  // 檢查棄牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] / 10 != kind) {
      card_comb[comb].tai_score[42] = 0;
      return;
    }
  }

  card_comb[comb].tai_score[34] = 0; // 取消混一色
}


/* 字一色 */
void check_tai43(char sit, char comb)
{
  int i;

  // 檢查手牌
  for (i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] < 31 || (pool_buf[i] > 34 && pool_buf[i] < 41) || pool_buf[i] > 43) { // 非字牌
      return;
    }
  }
  // 檢查棄牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
      if (pool[sit].out_card[i][1] < 31 || (pool[sit].out_card[i][1] > 34 && pool[sit].out_card[i][1] < 41) || pool[sit].out_card[i][1] > 43) {
          return;
      }

  }

  card_comb[comb].tai_score[43] = tai[43].score;
  card_comb[comb].tai_score[33] = 0; // 取消碰碰胡
}

/* 七搶一 */
void check_tai44(char sit, char comb)
{
  // 目前沒有邏輯，保留空函數
}

/* 五暗刻 */
void check_tai45(char sit, char comb, char make_card)
{
  int i, three_card = 0;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2) {
      if (card_comb[comb].info[i][1] != make_card || sit == card_owner) {
        three_card++; // 自摸可算暗刻
      }
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 11) { // 暗槓
      three_card++;
    }
  }

  if (three_card == 3) {
    card_comb[comb].tai_score[29] = tai[29].score;  // 三暗刻
  } else if (three_card == 4) {
    card_comb[comb].tai_score[38] = tai[38].score;  // 四暗刻
  } else if (three_card == 5) {
    card_comb[comb].tai_score[45] = tai[45].score;  // 五暗刻
    card_comb[comb].tai_score[33] = 0; // 取消碰碰胡
  }
}

/* 清老頭 */
void check_tai46(char sit, char comb)
{
  int i;

  for (i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] % 10 != 1 && pool_buf[i] % 10 != 9 || pool_buf[i] > 30) {
      return;  // 提前返回，非清老頭
    }
  }
    for (i = 0; i < pool[sit].out_card_index; i++) {
        if (pool[sit].out_card[i][1] > 30 || pool[sit].out_card[i][1]%10 != 1 && pool[sit].out_card[i][1]%10 != 9)
            return;
        if (pool[sit].out_card[i][2] > 30 || pool[sit].out_card[i][2]%10 != 1 && pool[sit].out_card[i][2]%10 != 9)
            return;
        if (pool[sit].out_card[i][3] > 30 || pool[sit].out_card[i][3]%10 != 1 && pool[sit].out_card[i][3]%10 != 9)
             return;
    }

  card_comb[comb].tai_score[46] = tai[46].score;
  card_comb[comb].tai_score[33] = 0; // 取消碰碰胡
  card_comb[comb].tai_score[35] = 0; // 取消純帶么
  card_comb[comb].tai_score[36] = 0; // 取消混老頭
}

/* 大四喜 */
void check_tai47(char sit, char comb)
{
  // 使用區域變數避免重複呼叫 exist_card
  int east = exist_card(sit, 31);
  int south = exist_card(sit, 32);
  int west = exist_card(sit, 33);
  int north = exist_card(sit, 34);

  if (east >= 3 && south >= 3 && west >= 3 && north >= 3) {
    card_comb[comb].tai_score[47] = tai[47].score;
    card_comb[comb].tai_score[9] = 0;   // 取消東風
    card_comb[comb].tai_score[10] = 0;  // 取消南風
    card_comb[comb].tai_score[11] = 0;  // 取消西風
    card_comb[comb].tai_score[12] = 0;  // 取消北風
    card_comb[comb].tai_score[17] = 0;  // 取消東風東
    card_comb[comb].tai_score[18] = 0;  // 取消南風南
    card_comb[comb].tai_score[19] = 0;  // 取消西風西
    card_comb[comb].tai_score[20] = 0;  // 取消北風北
    card_comb[comb].tai_score[41] = 0;  // 取消小四喜
  }
}

/* 八仙過海 */
void check_tai48(char sit, char comb)
{
  int i, wind_count = 0;
  bool winds[4] = {false, false, false, false}; // 東南西北

  // 檢查手牌中的風牌
  for (i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] >= 31 && pool_buf[i] <= 34) { // 風牌範圍
      winds[pool_buf[i] - 31] = true;
    }
  }

  // 檢查棄牌中的風牌 (碰/槓)
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] >= 31 &&
        pool[sit].out_card[i][1] <= 34) {
      winds[pool[sit].out_card[i][1] - 31] = true;
    }
  }

  // 計算已收集的風牌種類
  for (i = 0; i < 4; i++) {
    if (winds[i]) {
      wind_count++;
    }
  }

  if (wind_count >= 8) {  // 八仙過海
    card_comb[comb].tai_score[48] = tai[48].score;

    // 清除已計算的風牌台數
    for (i = 9; i <= 12; i++) { // 東南西北風
      card_comb[comb].tai_score[i] = 0;
    }
    for (i = 17; i <= 20; i++) { // 東東風...北北風
      card_comb[comb].tai_score[i] = 0;
    }
    card_comb[comb].tai_score[41] = 0; // 小四喜
    card_comb[comb].tai_score[47] = 0; // 大四喜
  }

}

/* 天胡 */
void check_tai49(char sit, char comb)
{
  // 簡化條件判斷
  if (pool[sit].first_round && sit == info.dealer) {
    card_comb[comb].tai_score[49] = tai[49].score;
  }
}

/* 地胡 */
void check_tai50(char sit, char comb)
{
  // 簡化條件判斷
  if (pool[sit].first_round && sit == card_owner && sit != info.dealer) {
    card_comb[comb].tai_score[50] = tai[50].score;
  }
}

/* 人胡 */
void check_tai51(char sit, char comb)
{
  // 簡化條件判斷
  if (pool[sit].first_round && sit != card_owner && sit != info.dealer) {
    card_comb[comb].tai_score[51] = tai[51].score;
  }
}

/* 連莊 */
void check_tai52(char sit, char comb)
{
  // 簡化條件判斷
  if (info.cont_dealer && info.dealer == sit) {
    card_comb[comb].tai_score[52] = tai[52].score * info.cont_dealer;
  }
}
