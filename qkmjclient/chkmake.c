#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define THREE_CARD 1
#define STRAIGHT_CARD 2
#define PAIR_CARD 3

#if defined(HAVE_LIBNCURSES)
  #include  <ncurses.h>
#endif

#include "qkmj.h"

struct card_info_type card_info[20];
char pool_buf[20];
struct component_type card_component[6][50];
int comb_count[6];
int count2;
int suit;

/********************************************************************/

/* 取得節點 */
NODEPTR getnode() {
  // 宣告節點指標
  NODEPTR p;
  // 分配記憶體給節點
  p = (NODEPTR)malloc(sizeof(struct nodetype));
  // 回傳節點指標
  return (p);
}

/* 建立樹 */
NODEPTR maketree(NODEPTR node) {
  // 宣告節點指標
  NODEPTR p;
  // 取得節點
  p = getnode();
  // 初始化節點資訊
  p->info[0] = 0;
  p->type = 0;
  p->end = 0;
  p->three = NULL;
  p->straight = NULL;
  p->pair = NULL;
  p->father = node;
  // 回傳節點指標
  return (p);
}

/* 建立樹的子節點 */
void build_tree(NODEPTR node) {
  // 檢查是否為樹的終點
  node->three = make_three(node);
  // 如果不是樹的終點，則建立順子節點
  if (!node->end) {
    node->straight = make_straight(node);
  } else {
    node->straight = NULL;
  }
  // 如果不是樹的終點且沒有三條節點，則建立對子節點
  if (!node->end && node->three == NULL) {
    node->pair = make_pair(node);
  } else {
    node->pair = NULL;
  }
}

/* 標記牌 */
void mark_card(NODEPTR node) {
  // 宣告變數
  int i, j;
  // 檢查是否為樹的根節點
  if (node->father == NULL) {
    return;
  }
  // 初始化索引
  i = 0;
  // 遍歷所有牌
  for (j = 0; card_info[j].info; j++) {
    // 檢查牌是否已存在節點中且未被標記
    if (node->info[i] == card_info[j].info && card_info[j].flag) {
      // 遞增索引
      i++;
      // 標記牌為已選取
      card_info[j].flag = 0;
      // 檢查是否還有牌
      if (!node->info[i]) {
        // 遞迴標記父節點
        mark_card(node->father);
        return;
      }
    }
  }
  // 找不到牌
  // display_comment("Error at marking");
  // 遞迴標記父節點
  mark_card(node->father);
}

/* Find the leading card */
int find_lead() {
  // 遍歷所有牌
  for (int i = 0; card_info[i].info; i++) {
    // 檢查牌是否被標記
    if (card_info[i].flag) {
      // 回傳被標記的牌
      return card_info[i].info;
    }
  }
  // 沒有找到被標記的牌
  return 0;
}

/* 建立三條節點 */
NODEPTR make_three(NODEPTR node) {
  // 宣告變數
  int i, j;
  int lead;
  NODEPTR p;

  // 重置標記
  for (i = 0; card_info[i].info; i++) {
    card_info[i].flag = 1;
  }
  // 標記牌
  mark_card(node);
  // 找出領頭牌
  lead = find_lead();
  // 檢查是否還有牌可以檢查
  if (lead == 0) {
    // 沒有牌可以檢查，設定為樹的終點
    node->end = 1;
    // 回傳空指標
    return NULL;
  }
  // 建立新的節點
  p = maketree(node);

  // 取得三張相同的牌
  j = 0;
  for (i = 0; card_info[i].info; i++) {
    // 檢查牌是否被標記且與領頭牌相同
    if (card_info[i].flag && card_info[i].info == lead) {
      // 將牌加入節點
      p->info[j++] = lead;
      // 取消標記
      card_info[i].flag = 0;
      // 檢查是否已取得三張牌
      if (j == 3) {
        break;
      }
    }
  }
  // 檢查是否已取得三張牌
  if (j != 3) {
    // 沒有取得三張牌，釋放節點記憶體
    free(p);
    // 回傳空指標
    return NULL;
  }
  // 設定節點資訊
  p->info[j] = 0;
  p->type = THREE_CARD;
  // 建立子節點
  build_tree(p);
  // 回傳節點指標
  return p;
}

/* 建立順子節點 */
NODEPTR make_straight(NODEPTR node) {
  // 宣告變數
  int i, j;
  int lead;
  NODEPTR p;

  // 重置標記
  for (i = 0; card_info[i].info; i++) {
    card_info[i].flag = 1;
  }
  // 標記牌
  mark_card(node);
  // 找出領頭牌
  lead = find_lead();
  // 檢查是否還有牌可以檢查
  if (lead == 0) {
    // 沒有牌可以檢查，設定為樹的終點
    node->end = 1;
    // 回傳空指標
    return NULL;
  }
  // 建立新的節點
  p = maketree(node);

  // 取得三張順子牌
  j = 0;
  for (i = 0; card_info[i].info; i++) {
    // 檢查牌是否被標記且與領頭牌相同
    if (card_info[i].flag && card_info[i].info == lead) {
      // 將牌加入節點
      p->info[j++] = lead;
      // 取消標記
      card_info[i].flag = 0;
      // 遞增領頭牌
      lead++;
      // 檢查是否已取得三張牌且領頭牌未超過最大值
      if (j == 3 && lead < 31) {
        break;
      }
    }
  }
  // 檢查是否已取得三張牌
  if (j != 3) {
    // 沒有取得三張牌，釋放節點記憶體
    free(p);
    // 回傳空指標
    return NULL;
  }
  // 設定節點資訊
  p->info[j] = 0;
  p->type = STRAIGHT_CARD;
  // 建立子節點
  build_tree(p);
  // 回傳節點指標
  return p;
}

/* 建立對子節點 */
NODEPTR make_pair(NODEPTR node) {
  // 宣告變數
  int i, j;
  int lead;
  NODEPTR p;

  // 重置標記
  for (i = 0; card_info[i].info; i++) {
    card_info[i].flag = 1;
  }
  // 標記牌
  mark_card(node);
  // 找出領頭牌
  lead = find_lead();
  // 檢查是否還有牌可以檢查
  if (lead == 0) {
    // 沒有牌可以檢查，設定為樹的終點
    node->end = 1;
    // 回傳空指標
    return NULL;
  }
  // 建立新的節點
  p = maketree(node);

  // 取得兩張相同的牌
  j = 0;
  for (i = 0; card_info[i].info; i++) {
    // 檢查牌是否被標記且與領頭牌相同
    if (card_info[i].flag && card_info[i].info == lead) {
      // 將牌加入節點
      p->info[j++] = lead;
      // 取消標記
      card_info[i].flag = 0;
      // 檢查是否已取得兩張牌
      if (j == 2) {
        break;
      }
    }
  }
  // 檢查是否已取得兩張牌
  if (j != 2) {
    // 沒有取得兩張牌，釋放節點記憶體
    free(p);
    // 回傳空指標
    return NULL;
  }
  // 設定節點資訊
  p->info[j] = 0;
  p->type = PAIR_CARD;
  // 建立子節點
  build_tree(p);
  // 回傳節點指標
  return p;
}

/* 找出有效的樹並將其複製到陣列中 */
void list_path(NODEPTR p) {
  // 宣告變數
  int i = 0;
  // 遍歷節點中的所有牌
  while (p->info[i]) {
    // 將牌複製到陣列中
    card_component[suit][comb_count[suit]].info[count2][i] = p->info[i];
    // 遞增索引
    i++;
  }
  // 檢查節點類型是否為對子
  if (p->type == PAIR_CARD) {
    // 檢查陣列中是否已存在對子
    if (card_component[suit][comb_count[suit]].type >= 3) {
      // 已存在對子，設定為多於一對
      card_component[suit][comb_count[suit]].type = 4;
    } else {
      // 不存在對子，設定為一對
      card_component[suit][comb_count[suit]].type = 3;
    }
  }
  // 設定陣列中最後一個元素為 0
  card_component[suit][comb_count[suit]].info[count2][i] = 0;
  // 遞增索引
  count2++;
  // 檢查父節點是否有效
  if (p->father && p->father->father) {
    // 父節點有效，遞迴呼叫 list_path
    list_path(p->father);
  }
}
/* 遍歷樹 */
void pretrav(NODEPTR p) {
  // 檢查節點是否為空
  if (p != NULL) {
    // 檢查是否為樹的終點
    if (p->end) {
      // 重置索引
      count2 = 0;
      // 檢查是否為樹的根節點
      if (p->father != NULL) {
        // 設定組合類型為有效路徑
        card_component[suit][comb_count[suit]].type = 2;
        // 複製路徑到陣列
        list_path(p);
      } else {
        // 設定組合類型為此花色沒有牌
        card_component[suit][0].type = 1;
      }
      // 遞增組合計數器
      comb_count[suit]++;
    }
    // 遍歷三條節點
    if (p->three) {
      pretrav(p->three);
    }
    // 遍歷順子節點
    if (p->straight) {
      pretrav(p->straight);
    }
    // 遍歷對子節點
    if (p->pair) {
      pretrav(p->pair);
    }
  }
}

/* 釋放樹的記憶體 */
void free_tree(NODEPTR p) {
  // 遞迴釋放三條節點的記憶體
  if (p->three) {
    free_tree(p->three);
  }
  // 遞迴釋放順子節點的記憶體
  if (p->straight) {
    free_tree(p->straight);
  }
  // 遞迴釋放對子節點的記憶體
  if (p->pair) {
    free_tree(p->pair);
  }
  // 釋放節點的記憶體
  free(p);
}

/* 檢查組合類型是否有效 */
int valid_type(int type) {
  // 檢查組合類型是否為有效路徑、此花色沒有牌或一對
  return (type == 1 || type == 2 || type == 3);
}

/* 回傳找到的牌的數量 */
int exist_card(char sit, char card) {
  // 宣告變數
  int i, j;
  int exist = 0;

  // 檢查手牌中是否有該牌
  for (i = 0; i <= pool[sit].num; i++) {
    // 檢查手牌中的每張牌
    if (pool_buf[i] == card) {
      // 找到該牌，遞增計數器
      exist++;
    }
  }
  // 檢查打出的牌中是否有該牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
    // 檢查打出的牌中的每張牌
    j = 1;
    while (pool[sit].out_card[i][j]) {
      // 檢查打出的牌中的每張牌
      if (pool[sit].out_card[i][j] == card) {
        // 找到該牌，遞增計數器
        exist++;
      }
      // 遞增索引
      j++;
    }
  }
  // 回傳找到的牌的數量
  return exist;
}

/* 檢查組合中是否有三張相同的牌 */
int exist_3(char sit, char card, int comb) {
  // 宣告變數
  int i, set;
  int exist = 0;

  // 檢查組合中是否有三張相同的牌
  set = 0;
  while (card_comb[comb].info[set][0]) {
    // 檢查組合中的每組牌
    if (card_comb[comb].info[set][0] == 2 &&
        card_comb[comb].info[set][1] == card) {
      // 找到三張相同的牌，遞增計數器
      exist++;
    }
    // 遞增索引
    set++;
  }
  // 檢查打出的牌中是否有三張相同的牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
    // 檢查打出的牌中的每組牌
    if (pool[sit].out_card[i][1] == card && pool[sit].out_card[i][2] == card &&
        pool[sit].out_card[i][3] == card) {
      // 找到三張相同的牌，遞增計數器
      exist++;
    }
  }
  // 回傳找到的牌的數量
  return exist;
}

/* 檢查組合中是否有順子 */
int exist_straight(char sit, char card, int comb) {
  // 宣告變數
  int i, set;
  int exist = 0;

  // 檢查組合中是否有順子
  set = 0;
  while (card_comb[comb].info[set][0]) {
    // 檢查組合中的每組牌
    if (card_comb[comb].info[set][0] == 1 &&
        card_comb[comb].info[set][1] == card) {
      // 找到順子，遞增計數器
      exist++;
    }
    // 遞增索引
    set++;
  }
  // 檢查打出的牌中是否有順子
  for (i = 0; i < pool[sit].out_card_index; i++) {
    // 檢查打出的牌中的每組牌
    if (pool[sit].out_card[i][0] == 7 && pool[sit].out_card[i][2] == card) {
      // 找到順子，遞增計數器
      exist++;
    }
    if (pool[sit].out_card[i][0] == 8 && pool[sit].out_card[i][1] == card) {
      // 找到順子，遞增計數器
      exist++;
    }
    if (pool[sit].out_card[i][0] == 9 && pool[sit].out_card[i][1] == card) {
      // 找到順子，遞增計數器
      exist++;
    }
  }
  // 回傳找到的牌的數量
  return exist;
}

/* 莊家 */
void check_tai0(char sit, char comb) {
  // 檢查玩家是否為莊家
  if (info.dealer == sit) {
    // 將莊家的台數加到組合的台數中
    card_comb[comb].tai_score[0] = tai[0].score;
  }
}

/* 門清 */
void check_tai1(char sit, char comb) {
  // 檢查玩家是否為門清
  if (pool[sit].num == 16) {
    // 將門清的台數加到組合的台數中
    card_comb[comb].tai_score[1] = tai[1].score;
  }
}

/* 自摸 */
void check_tai2(char sit, char comb) {
  // 檢查玩家是否為自摸
  if (sit == card_owner) {
    // 將自摸的台數加到組合的台數中
    card_comb[comb].tai_score[2] = tai[2].score;
  }
}

/* 斷么九 */
void check_tai3(char sit, char comb) {
  // 檢查玩家手牌中是否沒有么九牌
  if (!exist_card(sit, 1) && !exist_card(sit, 9) && !exist_card(sit, 11) &&
      !exist_card(sit, 19) && !exist_card(sit, 21) && !exist_card(sit, 29) &&
      !exist_card(sit, 31) && !exist_card(sit, 32) && !exist_card(sit, 33) &&
      !exist_card(sit, 34) && !exist_card(sit, 41) && !exist_card(sit, 42) &&
      !exist_card(sit, 43)) {
    // 將斷么九的台數加到組合的台數中
    card_comb[comb].tai_score[3] = tai[3].score;
  }
}

/* 雙龍抱 */
void check_tai4(char sit, char comb) {
  // 宣告變數
  int straight[30] = {0};
  int double_straight_num = 0;

  // 檢查玩家是否為門清
  if (pool[sit].num != 16) {
    return;
  }
  // 計算順子的數量
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) {
      straight[card_comb[comb].info[i][1]]++;
    }
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 7) {
      straight[pool[sit].out_card[i][2]]++;
    }
    if (pool[sit].out_card[i][0] == 8) {
      straight[pool[sit].out_card[i][1]]++;
    }
    if (pool[sit].out_card[i][0] == 9) {
      straight[pool[sit].out_card[i][1]]++;
    }
  }
  // 檢查是否有兩個或更多個順子
  for (int i = 0; i < 30; i++) {
    if (straight[i] >= 2) {
      double_straight_num++;
    }
  }
  // 將雙龍抱的台數加到組合的台數中
  if (double_straight_num == 1) {
    card_comb[comb].tai_score[4] = tai[4].score;
  }
  if (double_straight_num >= 2) {
    card_comb[comb].tai_score[28] = tai[28].score;
  }
}

/* 槓上開花 */
void check_tai5(char sit, char comb) {
  // 檢查玩家是否在槓狀態
  if (in_kang) {
    // 將槓上開花的台數加到組合的台數中
    card_comb[comb].tai_score[5] = tai[5].score;
  }
}

/* 海底摸月 */
void check_tai6(char sit, char comb) {
  // 檢查玩家是否為摸牌者且為第 16 張牌
  if ((144 - card_point) == 16 && sit == card_owner) {
    // 將海底摸月的台數加到組合的台數中
    card_comb[comb].tai_score[6] = tai[6].score;
  }
}

/* 河底撈魚 */
void check_tai7(char sit, char comb) {
  // 檢查是否為第 16 張牌且玩家不是摸牌者
  if ((144 - card_point) == 16 && sit != card_owner) {
    // 將河底撈魚的台數加到組合的台數中
    card_comb[comb].tai_score[7] = tai[7].score;
  }
}

/* 搶槓 */
void check_tai8(char sit, char comb) {
  // TODO: Implement check_tai8
}

/* 東風 */
void check_tai9(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張東風
  if (exist_card(sit, 31) >= 3) {
    // 檢查玩家的風向是否為東風
    if (info.wind == 1) {
      // 將東風的台數加到組合的台數中
      card_comb[comb].tai_score[9] += tai[9].score;
    }
    // 檢查玩家的門風是否為東風
    if (pool[sit].door_wind == 1) {
      // 將東風的台數加到組合的台數中
      card_comb[comb].tai_score[9] += tai[9].score;
    }
  }
  // 檢查玩家是否擁有 2 個東風的台數
  if (card_comb[comb].tai_score[9] == tai[9].score * 2) {
    // 將東風東的台數加到組合的台數中
    card_comb[comb].tai_score[17] = tai[17].score;
    // 重置東風的台數
    card_comb[comb].tai_score[9] = 0;
  }
}

/* 南風 */
void check_tai10(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張南風
  if (exist_card(sit, 32) >= 3) {
    // 檢查玩家的風向是否為南風
    if (info.wind == 2) {
      // 將南風的台數加到組合的台數中
      card_comb[comb].tai_score[10] += tai[10].score;
    }
    // 檢查玩家的門風是否為南風
    if (pool[sit].door_wind == 2) {
      // 將南風的台數加到組合的台數中
      card_comb[comb].tai_score[10] += tai[10].score;
    }
  }
  // 檢查玩家是否擁有 2 個南風的台數
  if (card_comb[comb].tai_score[10] == tai[10].score * 2) {
    // 將南風南的台數加到組合的台數中
    card_comb[comb].tai_score[18] = tai[18].score;
    // 重置南風的台數
    card_comb[comb].tai_score[10] = 0;
  }
}
/* 西風 */
void check_tai11(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張西風
  if (exist_card(sit, 33) >= 3) {
    // 檢查玩家的風向是否為西風
    if (info.wind == 3) {
      // 將西風的台數加到組合的台數中
      card_comb[comb].tai_score[11] += tai[11].score;
    }
    // 檢查玩家的門風是否為西風
    if (pool[sit].door_wind == 3) {
      // 將西風的台數加到組合的台數中
      card_comb[comb].tai_score[11] += tai[11].score;
    }
  }
  // 檢查玩家是否擁有 2 個西風的台數
  if (card_comb[comb].tai_score[11] == tai[11].score * 2) {
    // 將西風西的台數加到組合的台數中
    card_comb[comb].tai_score[19] = tai[19].score;
    // 重置西風的台數
    card_comb[comb].tai_score[11] = 0;
  }
}

/* 北風 */
void check_tai12(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張北風
  if (exist_card(sit, 34) >= 3) {
    // 檢查玩家的風向是否為北風
    if (info.wind == 4) {
      // 將北風的台數加到組合的台數中
      card_comb[comb].tai_score[12] += tai[12].score;
    }
    // 檢查玩家的門風是否為北風
    if (pool[sit].door_wind == 4) {
      // 將北風的台數加到組合的台數中
      card_comb[comb].tai_score[12] += tai[12].score;
    }
  }
  // 檢查玩家是否擁有 2 個北風的台數
  if (card_comb[comb].tai_score[12] == tai[12].score * 2) {
    // 將北風北的台數加到組合的台數中
    card_comb[comb].tai_score[20] = tai[20].score;
    // 重置北風的台數
    card_comb[comb].tai_score[12] = 0;
  }
}

/* 紅中 */
void check_tai13(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張紅中
  if (exist_card(sit, 41) >= 3) {
    // 將紅中的台數加到組合的台數中
    card_comb[comb].tai_score[13] = tai[13].score;
  }
}

/* 白板 */
void check_tai14(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張白板
  if (exist_card(sit, 42) >= 3) {
    // 將白板的台數加到組合的台數中
    card_comb[comb].tai_score[14] = tai[14].score;
  }
}

/* 青發 */
void check_tai15(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張青發
  if (exist_card(sit, 43) >= 3) {
    // 將青發的台數加到組合的台數中
    card_comb[comb].tai_score[15] = tai[15].score;
  }
}

/* 花牌 */
void check_tai16(char sit, char comb) {
  // 檢查玩家是否擁有花牌
  if (pool[sit].flower[pool[sit].door_wind - 1]) {
    // 將花牌的台數加到組合的台數中
    card_comb[comb].tai_score[16] += tai[16].score;
  }
  // 檢查玩家是否擁有花牌
  if (pool[sit].flower[pool[sit].door_wind + 3]) {
    // 將花牌的台數加到組合的台數中
    card_comb[comb].tai_score[16] += tai[16].score;
  }
}

/* 東風東 */
void check_tai17(char sit, char comb) {
  // TODO: Implement check_tai17
}

/* 西風西 */
void check_tai18(char sit, char comb) {
  // TODO: Implement check_tai18
}

/* 南風南 */
void check_tai19(char sit, char comb) {
  // TODO: Implement check_tai19
}

/* 北風北 */
void check_tai20(char sit, char comb) {
  // TODO: Implement check_tai20
}

/* 春夏秋冬 */
void check_tai21(char sit, char comb) {
  // 檢查玩家是否擁有所有四張季節花牌
  if (pool[sit].flower[0] && pool[sit].flower[1] && pool[sit].flower[2] &&
      pool[sit].flower[3]) {
    // 將春夏秋冬的台數加到組合的台數中
    card_comb[comb].tai_score[21] = tai[21].score;
    // 從組合的台數中減去花牌的台數
    card_comb[comb].tai_score[16] -= tai[16].score;
  }
}

/* 梅蘭菊竹 */
void check_tai22(char sit, char comb) {
  // 檢查玩家是否擁有所有四張植物花牌
  if (pool[sit].flower[4] && pool[sit].flower[5] && pool[sit].flower[6] &&
      pool[sit].flower[7]) {
    // 將梅蘭菊竹的台數加到組合的台數中
    card_comb[comb].tai_score[22] = tai[22].score;
    // 從組合的台數中減去花牌的台數
    card_comb[comb].tai_score[16] -= tai[16].score;
  }
}

/* 全求人 */
void check_tai23(char sit, char comb) {
  // 檢查玩家手牌中是否只有一張牌且不是摸牌者
  if (pool[sit].num == 1 && sit != card_owner) {
    // 將全求人的台數加到組合的台數中
    card_comb[comb].tai_score[23] = tai[23].score;
  }
}

/* 平胡 */
void check_tai24(char sit, char comb, char make_card) {
  // 檢查玩家是否擁有花牌
  if (pool[sit].flower[0] || pool[sit].flower[1] || pool[sit].flower[2] ||
      pool[sit].flower[3] || pool[sit].flower[4] || pool[sit].flower[5] ||
      pool[sit].flower[6] || pool[sit].flower[7]) {
    return;
  }
  // 檢查玩家是否擁有槓牌
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2 ||
        card_comb[comb].info[i][0] == 10) {
      return;
    }
  }
  // 檢查玩家是否為摸牌者
  if (turn == card_owner) {
    return;
  }
  // 檢查玩家是否擁有碰牌
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == pool[sit].out_card[i][2]) {
      return;
    }
  }
  // 檢查玩家是否可以利用摸到的牌組成有效的組合
  if (make_card % 10 <= 6 && check_make(sit, make_card + 3, 0)) {
    // 重置 pool_buf[]
    check_make(sit, make_card, 0);
    // 將平胡的台數加到組合的台數中
    card_comb[comb].tai_score[24] = tai[24].score;
    return;
  }
  if (make_card % 10 >= 4 && check_make(sit, make_card - 3, 0)) {
    // 重置 pool_buf[]
    check_make(sit, make_card, 0);
    // 將平胡的台數加到組合的台數中
    card_comb[comb].tai_score[24] = tai[24].score;
    return;
  }
}
/* 混帶么 */
void check_tai25(char sit, char comb) {
  // 檢查玩家是否擁有非么九牌
  bool has_non_yao = false;
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][1] < 30) {
      has_non_yao = true;
      // 檢查非么九牌是否為么九牌
      if (card_comb[comb].info[i][1] % 10 != 1 &&
          card_comb[comb].info[i][1] % 10 != 9 &&
          card_comb[comb].info[i][2] % 10 != 1 &&
          card_comb[comb].info[i][2] % 10 != 9 &&
          card_comb[comb].info[i][3] % 10 != 1 &&
          card_comb[comb].info[i][3] % 10 != 9) {
        return;
      }
    }
  }
  // 檢查玩家打出的牌中是否擁有非么九牌
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] < 30) {
      // 檢查非么九牌是否為么九牌
      if (pool[sit].out_card[i][1] % 10 != 1 &&
          pool[sit].out_card[i][1] % 10 != 9 &&
          pool[sit].out_card[i][2] % 10 != 1 &&
          pool[sit].out_card[i][2] % 10 != 9 &&
          pool[sit].out_card[i][3] % 10 != 1 &&
          pool[sit].out_card[i][3] % 10 != 9) {
        return;
      }
    }
  }
  // 檢查玩家是否擁有非么九牌
  if (has_non_yao) {
    // 將混帶么的台數加到組合的台數中
    card_comb[comb].tai_score[25] = tai[25].score;
    // 檢查玩家是否為門清
    if (pool[sit].num != 16) {
      // 玩家不是門清，將混帶么的台數減 1
      card_comb[comb].tai_score[25] -= 1;
    }
  }
}

/* 三色同順 */
void check_tai26(char sit, char comb) {
  // 檢查玩家是否擁有三色同順
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) {
      int num = card_comb[comb].info[i][1] % 10;
      // 檢查玩家是否擁有三種花色的順子
      if (exist_straight(sit, num, comb) &&
          exist_straight(sit, num + 10, comb) &&
          exist_straight(sit, num + 20, comb)) {
        // 將三色同順的台數加到組合的台數中
        card_comb[comb].tai_score[26] = tai[26].score;
        return;
      }
    }
  }
  // 檢查玩家打出的牌中是否擁有三色同順
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] >= 7 && pool[sit].out_card[i][0] <= 9) {
      int num = (pool[sit].out_card[i][0] == 7)
                   ? pool[sit].out_card[i][2] % 10
                   : pool[sit].out_card[i][1] % 10;
      // 檢查玩家是否擁有三種花色的順子
      if (exist_straight(sit, num, comb) &&
          exist_straight(sit, num + 10, comb) &&
          exist_straight(sit, num + 20, comb)) {
        // 將三色同順的台數加到組合的台數中
        card_comb[comb].tai_score[26] = tai[26].score;
        return;
      }
    }
  }
  // 檢查玩家是否為門清
  if (pool[sit].num != 16) {
    // 玩家不是門清，將三色同順的台數減 1
    card_comb[comb].tai_score[26] -= 1;
  }
}

/* 一條龍 */
void check_tai27(char sit, char comb) {
  // 檢查玩家是否擁有三種花色的順子
  if ((exist_straight(sit, 1, comb) && exist_straight(sit, 4, comb) &&
       exist_straight(sit, 7, comb)) ||
      (exist_straight(sit, 11, comb) && exist_straight(sit, 14, comb) &&
       exist_straight(sit, 17, comb)) ||
      (exist_straight(sit, 21, comb) && exist_straight(sit, 24, comb) &&
       exist_straight(sit, 27, comb))) {
    // 將一條龍的台數加到組合的台數中
    card_comb[comb].tai_score[27] = tai[27].score;
    // 檢查玩家是否為門清
    if (pool[sit].num != 16) {
      // 玩家不是門清，將一條龍的台數減 1
      card_comb[comb].tai_score[27] -= 1;
    }
  }
}

/* 雙龍抱 */
void check_tai28(char sit, char comb) {
  // TODO: Implement check_tai28
}

/* 三暗刻 */
void check_tai29(char sit, char comb) {
  // TODO: Implement check_tai29
}

/* 三槓子 */
void check_tai30(char sit, char comb) {
  // TODO: Implement check_tai30
}

/* 三色同刻 */
void check_tai31(char sit, char comb) {
  // 檢查玩家是否擁有三色同刻
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2) {
      int num = card_comb[comb].info[i][1] % 10;
      // 檢查玩家是否擁有三種花色的刻子
      if (exist_3(sit, num, comb) && exist_3(sit, num + 10, comb) &&
          exist_3(sit, num + 20, comb)) {
        // 將三色同刻的台數加到組合的台數中
        card_comb[comb].tai_score[31] = tai[31].score;
        return;
      }
    }
  }
  // 檢查玩家打出的牌中是否擁有三色同刻
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == pool[sit].out_card[i][2] &&
        pool[sit].out_card[i][1] == pool[sit].out_card[i][3]) {
      int num = pool[sit].out_card[i][1] % 10;
      // 檢查玩家是否擁有三種花色的刻子
      if (exist_3(sit, num, comb) && exist_3(sit, num + 10, comb) &&
          exist_3(sit, num + 20, comb)) {
        // 將三色同刻的台數加到組合的台數中
        card_comb[comb].tai_score[31] = tai[31].score;
        return;
      }
    }
  }
}

/* 門清自摸 */
void check_tai32(char sit, char comb) {
  // 檢查玩家是否為門清且自摸
  if (pool[sit].num == 16 && sit == card_owner) {
    // 將門清自摸的台數加到組合的台數中
    card_comb[comb].tai_score[32] = tai[32].score;
    // 重置門清和自摸的台數
    card_comb[comb].tai_score[1] = 0;
    card_comb[comb].tai_score[2] = 0;
  }
}

/* 碰碰胡 */
void check_tai33(char sit, char comb) {
  // 檢查玩家是否擁有碰碰胡
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) {
      return;
    }
  }
  // 檢查玩家打出的牌中是否擁有碰碰胡
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] != pool[sit].out_card[i][2]) {
      return;
    }
  }
  // 將碰碰胡的台數加到組合的台數中
  card_comb[comb].tai_score[33] = tai[33].score;
}
/* 混一色 */
void check_tai34(char sit, char comb) {
  // 檢查玩家是否擁有混一色
  int kind = pool_buf[0] / 10;
  // 檢查玩家手牌是否包含非字牌
  for (int i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] / 10 <= 2) {
      // 檢查玩家打出的牌是否包含非字牌
      for (int j = 0; j < pool[sit].out_card_index; j++) {
        if (pool[sit].out_card[j][1] / 10 <= 2) {
          // 檢查玩家手牌和打出的牌是否都是同一種花色
          if (kind == pool_buf[i] / 10 &&
              kind == pool[sit].out_card[j][1] / 10) {
            // 將混一色的台數加到組合的台數中
            card_comb[comb].tai_score[34] = tai[34].score;
            return;
          }
        }
      }
    }
  }
}

/* 純帶么 */
void check_tai35(char sit, char comb) {
  // 檢查玩家是否擁有純帶么
  bool has_non_yao = false;
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][1] > 30) {
      return;
    }
    if (card_comb[comb].info[i][1] % 10 == 1 ||
        card_comb[comb].info[i][1] % 10 == 9 ||
        card_comb[comb].info[i][2] % 10 == 1 ||
        card_comb[comb].info[i][2] % 10 == 9 ||
        card_comb[comb].info[i][3] % 10 == 1 ||
        card_comb[comb].info[i][3] % 10 == 9) {
      continue;
    } else {
      has_non_yao = true;
    }
  }
  // 檢查玩家打出的牌中是否擁有非么九牌
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] > 30) {
      return;
    }
    if (pool[sit].out_card[i][1] % 10 == 1 ||
        pool[sit].out_card[i][1] % 10 == 9 ||
        pool[sit].out_card[i][2] % 10 == 1 ||
        pool[sit].out_card[i][2] % 10 == 9 ||
        pool[sit].out_card[i][3] % 10 == 1 ||
        pool[sit].out_card[i][3] % 10 == 9) {
      continue;
    } else {
      has_non_yao = true;
    }
  }
  // 檢查玩家是否擁有非么九牌
  if (has_non_yao) {
    // 將純帶么的台數加到組合的台數中
    card_comb[comb].tai_score[35] = tai[35].score;
    // 重置混帶么的台數
    card_comb[comb].tai_score[25] = 0;
  }
}

/* 混老頭 */
void check_tai36(char sit, char comb) {
  // 檢查玩家是否擁有混老頭
  bool has_non_yao = false;
  for (int i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] < 30) {
      if (pool_buf[i] % 10 != 1 && pool_buf[i] % 10 != 9) {
        return;
      }
    } else {
      has_non_yao = true;
    }
  }
  // 檢查玩家打出的牌中是否擁有非么九牌
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] / 10 < 30) {
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
    } else {
      has_non_yao = true;
    }
  }
  // 檢查玩家是否擁有非么九牌
  if (has_non_yao) {
    // 將混老頭的台數加到組合的台數中
    card_comb[comb].tai_score[36] = tai[36].score;
    // 重置混帶么和碰碰胡的台數
    card_comb[comb].tai_score[25] = 0;
    card_comb[comb].tai_score[33] = 0;
  }
}

/* 小三元 */
void check_tai37(char sit, char comb) {
  // 檢查玩家是否擁有至少 2 張紅中、白板、發財
  if (exist_card(sit, 41) >= 2 && exist_card(sit, 42) >= 2 &&
      exist_card(sit, 43) >= 2) {
    // 將小三元的台數加到組合的台數中
    card_comb[comb].tai_score[37] = tai[37].score;
    // 重置紅中、白板、發財的台數
    card_comb[comb].tai_score[13] = 0;
    card_comb[comb].tai_score[14] = 0;
    card_comb[comb].tai_score[15] = 0;
  }
}

/* 四暗刻 */
void check_tai38(char sit, char comb) {
  // TODO: Implement check_tai38
}

/* 四槓子 */
void check_tai39(char sit, char comb) {
  // 計算玩家打出的槓牌數量
  int kang_count = 0;
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 3 || pool[sit].out_card[i][0] == 11 ||
        pool[sit].out_card[i][0] == 12) {
      kang_count++;
    }
  }
  // 檢查槓牌數量，並將對應的台數加到組合的台數中
  if (kang_count == 3) {
    card_comb[comb].tai_score[30] = tai[30].score;
  }
  if (kang_count == 4) {
    card_comb[comb].tai_score[39] = tai[39].score;
  }
}

/* 大三元 */
void check_tai40(char sit, char comb) {
  // Check if the player has at least 3 red dragon, white dragon, and green dragon cards
  if (exist_card(sit, 41) >= 3 && exist_card(sit, 42) >= 3 &&
      exist_card(sit, 43) >= 3) {
    // 將大三元的台數加到組合的台數中
    card_comb[comb].tai_score[40] = tai[40].score;
    // 重置紅中、白板、發財的台數
    card_comb[comb].tai_score[13] = 0;
    card_comb[comb].tai_score[14] = 0;
    card_comb[comb].tai_score[15] = 0;
    // 重置小三元的台數
    card_comb[comb].tai_score[37] = 0;
  }
}

/* 小四喜 */
void check_tai41(char sit, char comb) {
  // Check if the player has at least 2 east wind, south wind, west wind, and north wind cards
  if (exist_card(sit, 31) >= 2 && exist_card(sit, 32) >= 2 &&
      exist_card(sit, 33) >= 2 && exist_card(sit, 34) >= 2) {
    // 將小四喜的台數加到組合的台數中
    card_comb[comb].tai_score[41] = tai[41].score;
    // 重置風牌的台數
    card_comb[comb].tai_score[9] = 0;
    card_comb[comb].tai_score[10] = 0;
    card_comb[comb].tai_score[11] = 0;
    card_comb[comb].tai_score[12] = 0;
    card_comb[comb].tai_score[17] = 0;
    card_comb[comb].tai_score[18] = 0;
    card_comb[comb].tai_score[19] = 0;
    card_comb[comb].tai_score[20] = 0;
  }
}

/* 清一色 */
void check_tai42(char sit, char comb) {
  int i;
  int kind;

  // 初始化清一色的台數
  card_comb[comb].tai_score[42] = tai[42].score;
  // 檢查玩家手牌是否包含字牌
  kind = pool_buf[0] / 10;
  if (kind >= 3) {
    // 玩家手牌包含字牌，清一色不成立
    card_comb[comb].tai_score[42] = 0;
    return;
  }
  // 檢查玩家手牌是否都是同一種花色
  for (i = 1; i <= pool[sit].num; i++) {
    if (kind != pool_buf[i] / 10) {
      // 玩家手牌包含不同花色，清一色不成立
      card_comb[comb].tai_score[42] = 0;
      return;
    }
  }
  // 檢查玩家打出的牌是否都是同一種花色
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (kind != pool[sit].out_card[i][1] / 10) {
      // 玩家打出的牌包含不同花色，清一色不成立
      card_comb[comb].tai_score[42] = 0;
      return;
    }
  }
  // 符合清一色條件，將混一色的台數歸零
  card_comb[comb].tai_score[34] = 0;
}

/* 字一色 */
void check_tai43(char sit, char comb) {
  int i;
  int j;

  // 檢查玩家手牌是否都是字牌
  for (i = 0; i <= pool[sit].num; i++) {
    if (!((pool_buf[i] <= 34 && pool_buf[i] >= 31) ||
          (pool_buf[i] <= 43 && pool_buf[i] >= 41))) {
      return;
    }
  }
  // 檢查玩家打出的牌是否都是字牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
    j = 1;
    while (pool[sit].out_card[i][j]) {
      if (!((pool[sit].out_card[i][j] <= 34 &&
              pool[sit].out_card[i][j] >= 31) ||
            (pool[sit].out_card[i][j] <= 43 &&
             pool[sit].out_card[i][j] >= 41))) {
        return;
      }
      j++;
    }
  }
  // 符合字一色條件，將台數加到組合的台數中
  card_comb[comb].tai_score[43] = tai[43].score;
  // 重置碰碰胡的台數
  card_comb[comb].tai_score[33] = 0;
}

/* 七搶一 */
void check_tai44(char sit, char comb) {
  // TODO: Implement check_tai44
}

/* 五暗刻 */
void check_tai45(char sit, char comb, char make_card) {
  int i;
  int three_card = 0;
  char msg_buf[80];

  // 檢查組合中是否有暗刻
  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2) {
      // 自摸可算暗刻
      if (card_comb[comb].info[i][1] != make_card || sit == card_owner) {
        three_card++;
      }
    }
  }
  // 檢查玩家打出的牌中是否有暗刻
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 11) {
      three_card++;
    }
  }
  // 檢查暗刻數量，並將對應的台數加到組合的台數中
  if (three_card == 3) {
    card_comb[comb].tai_score[29] = tai[29].score;
  }
  if (three_card == 4) {
    card_comb[comb].tai_score[38] = tai[38].score;
  }
  if (three_card == 5) {
    card_comb[comb].tai_score[45] = tai[45].score;
    card_comb[comb].tai_score[33] = 0;
  }
}

/* 清老頭 */
void check_tai46(char sit, char comb) {
  int i;

  // 檢查玩家手牌是否都是么九牌
  for (i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] % 10 != 1 && pool_buf[i] % 10 != 9 ||
        pool_buf[i] > 30) {
      return;
    }
  }
  // 檢查玩家打出的牌是否都是么九牌
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] > 30) {
      return;
    }
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
  // 符合清老頭條件，將台數加到組合的台數中
  card_comb[comb].tai_score[46] = tai[46].score;
  // 重置碰碰胡、純帶么、混老頭的台數
  card_comb[comb].tai_score[33] = 0;
  card_comb[comb].tai_score[35] = 0;
  card_comb[comb].tai_score[36] = 0;
}

/* 大四喜 */
void check_tai47(char sit, char comb) {
  // 檢查玩家是否擁有至少 3 張東風、南風、西風、北風
  if (exist_card(sit, 31) >= 3 && exist_card(sit, 32) >= 3 &&
      exist_card(sit, 33) >= 3 && exist_card(sit, 34) >= 3) {
    // 將大四喜的台數加到組合的台數中
    card_comb[comb].tai_score[47] = tai[47].score;
    // 重置風牌的台數
    card_comb[comb].tai_score[9] = 0;
    card_comb[comb].tai_score[10] = 0;
    card_comb[comb].tai_score[11] = 0;
    card_comb[comb].tai_score[12] = 0;
    card_comb[comb].tai_score[17] = 0;
    card_comb[comb].tai_score[18] = 0;
    card_comb[comb].tai_score[19] = 0;
    card_comb[comb].tai_score[20] = 0;
    card_comb[comb].tai_score[41] = 0;
  }
}

/* 八仙過海 */
void check_tai48(char sit, char comb) {
  // TODO: Implement check_tai48
}

/* 天胡 */
void check_tai49(char sit, char comb) {
  char msg_buf[80];
  // 檢查玩家是否為莊家且為第一輪
  if (pool[sit].first_round && sit == info.dealer) {
    // 將天胡的台數加到組合的台數中
    card_comb[comb].tai_score[49] = tai[49].score;
  }
}

/* 地胡 */
void check_tai50(char sit, char comb) {
  // 檢查玩家是否為自摸且為第一輪且非莊家
  if (pool[sit].first_round && sit == card_owner && sit != info.dealer) {
    // 將地胡的台數加到組合的台數中
    card_comb[comb].tai_score[50] = tai[50].score;
  }
}

/* 人胡 */
void check_tai51(char sit, char comb) {
  // 檢查玩家是否為非自摸且為第一輪且非莊家
  if (pool[sit].first_round && sit != card_owner && sit != info.dealer) {
    // 將人胡的台數加到組合的台數中
    card_comb[comb].tai_score[51] = tai[51].score;
  }
}

/* 連莊 */
void check_tai52(char sit, char comb) {
  // 檢查玩家是否為莊家且連莊
  if (info.cont_dealer && info.dealer == sit) {
    // 將連莊的台數加到組合的台數中
    card_comb[comb].tai_score[52] = tai[52].score * info.cont_dealer;
  }
} 

/* Find the combination of the card with the highest score */
void full_check(char sit, char make_card) {
  // 宣告變數
  int i, j;
  int suit_count[6], set1, set2, card, count, score;
  char msg_buf[80];

  // 初始化計數器
  count = 0;
  for (i = 0; i < 5; i++) {
    card_comb[i].set_count = 0;
    suit_count[i] = 0;
  }

  // 找出所有有效的組合
  for (suit_count[0] = 0; suit_count[0] < comb_count[0]; suit_count[0]++) {
    if (valid_type(card_component[0][suit_count[0]].type)) {
      for (suit_count[1] = 0; suit_count[1] < comb_count[1];
           suit_count[1]++) {
        if (valid_type(card_component[1][suit_count[1]].type)) {
          for (suit_count[2] = 0; suit_count[2] < comb_count[2];
               suit_count[2]++) {
            if (valid_type(card_component[2][suit_count[2]].type)) {
              // 初始化組合的索引
              set2 = 0;
              // 遍歷所有花色
              for (i = 0; i < 5; i++) {
                // 初始化花色的索引
                set1 = 0;
                // 遍歷花色中的所有組合
                while (card_component[i][suit_count[i]].info[set1][0]) {
                  // 初始化牌的索引
                  card = 0;
                  // 遍歷組合中的所有牌
                  while (card_component[i][suit_count[i]].info[set1][card]) {
                    // 將牌複製到組合中
                    card_comb[count].info[set2][card + 1] =
                        card_component[i][suit_count[i]].info[set1][card];
                    // 遞增牌的索引
                    card++;
                  }
                  // 判斷組合的類型
                  if (card_comb[count].info[set2][1] ==
                      card_comb[count].info[set2][2]) {
                    card_comb[count].info[set2][0] = 2;
                  } else {
                    card_comb[count].info[set2][0] = 1;
                  }
                  // 判斷組合是否為槓子
                  if (card == 2) {
                    card_comb[count].info[set2][0] = 10;
                  }
                  // 將組合的最後一個元素設為 0
                  card_comb[count].info[set2][card + 1] = 0;
                  // 遞增花色的索引
                  set1++;
                  // 遞增組合的索引
                  set2++;
                }
                // 將組合的最後一個元素設為 0
                card_comb[count].info[set2][0] = 0;
                // 設定組合的牌數
                card_comb[count].set_count = set2;
              }
              // 遞增組合的計數器
              count++;
            }
          }
        }
      }
    }
  }

  // 檢查所有組合的台數
  for (i = 0; i < count; i++) {
    // 檢查組合的台數
    check_tai(sit, i, make_card);
    // 初始化組合的總台數
    card_comb[i].tai_sum = 0;
    // 計算組合的總台數
    for (j = 0; j <= 52; j++) {
      card_comb[i].tai_sum += card_comb[i].tai_score[j];
    }
  }

  // 輸出組合的資訊
  /*
  for(i=0;i<card_comb[0].set_count;i++)
  {
    sprintf(msg_buf,"([%d] %d %d %d %d)",i,card_comb[0].info[i][0],card_comb[0].info[i][1],card_comb[0].info[i][2],card_comb[0].info[i][3]);
display_comment(msg_buf);
  }
  */

  // 設定組合的數量
  comb_num = count;
}
/* 檢查是否可以胡牌 */
int check_make(char sit, char card, int method) {
  // 宣告變數
  NODEPTR p[6];  /* p[0]=萬 p[1]=筒 p[2]=索 p[3]=風牌 p[4]=三元牌 */
  int i, j, k, l, len, pair, make, tmp;
  char msg_buf[80];

  // 將玩家手牌複製到緩衝區
  for (i = 0; i < pool[sit].num; i++) {
    pool_buf[i] = pool[sit].card[i];
  }
  pool_buf[i] = card;
  pool_buf[i + 1] = 0;

  // 對緩衝區進行排序
  for (i = 0; i <= pool[sit].num; i++) {
    for (j = 0; j < pool[sit].num - i; j++) {
      if (pool_buf[j] > pool_buf[j + 1]) {
        tmp = pool_buf[j];
        pool_buf[j] = pool_buf[j + 1];
        pool_buf[j + 1] = tmp;
      }
    }
  }

  // 初始化組合資訊
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 20; j++) {
      card_component[i][j].type = 0;
      for (k = 0; k < 10; k++) {
        card_component[i][j].info[k][0] = 0;
      }
    }
    comb_count[i] = 0;
  }

  // 建立每種花色的樹狀結構
  // j: 牌的索引
  // card_info: 每種花色的牌
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

    // 建立樹的根節點
    p[suit] = maketree(NULL);
    card_component[suit][0].type = 0;
    // 建立樹的子節點
    build_tree(p[suit]);
    // 遍歷樹
    pretrav(p[suit]);
    // 釋放樹的記憶體
    free_tree(p[suit]);
  }

  // 初始化計數器
  pair = 0;
  make = 1;
  // 檢查五種花色
  for (i = 0; i < 5; i++) {
    switch (card_component[i][0].type) {
      case 0:  // 無效路徑
        make = 0;
        // 退出迴圈
        break;
      case 1:  // 此花色沒有牌
      case 2:  // 此路徑有效
        break;
      case 3:  // 找到一對
        pair++;
        break;
      case 4:  // 在同一花色中找到多於一對
        make = 0;
        // 退出迴圈
        break;
      default:
        break;
    }
  }

  // 檢查是否只有一對
  if (pair != 1) {
    make = 0;
  }
  // 檢查是否需要進行完整檢查
  if (make && method) {
    full_check(sit, card);
  }
  // 回傳檢查結果
  return (make);
}

/* 檢查台數 */
int check_tai(char sit, char comb, char make_card) {
  // 初始化組合的台數
  for (int i = 0; i < 100; i++) {
    card_comb[comb].tai_score[i] = 0;
  }
  // 檢查莊家
  check_tai0(sit, comb);
  // 檢查門清
  check_tai1(sit, comb);
  // 檢查自摸
  check_tai2(sit, comb);
  // 檢查斷么九
  check_tai3(sit, comb);
  // 檢查雙龍抱
  check_tai4(sit, comb);
  // 檢查槓上開花
  check_tai5(sit, comb);
  // 檢查海底摸月
  check_tai6(sit, comb);
  // 檢查河底撈魚
  check_tai7(sit, comb);
  // 檢查搶槓
  check_tai8(sit, comb);
  // 檢查東風
  check_tai9(sit, comb);
  // 檢查南風
  check_tai10(sit, comb);
  // 檢查西風
  check_tai11(sit, comb);
  // 檢查北風
  check_tai12(sit, comb);
  // 檢查紅中
  check_tai13(sit, comb);
  // 檢查白板
  check_tai14(sit, comb);
  // 檢查青發
  check_tai15(sit, comb);
  // 檢查花牌
  check_tai16(sit, comb);
  // 檢查春夏秋冬
  check_tai21(sit, comb);
  // 檢查梅蘭菊竹
  check_tai22(sit, comb);
  // 檢查全求人
  check_tai23(sit, comb);
  // 檢查平胡
  check_tai24(sit, comb, make_card);
  // 檢查混帶么
  check_tai25(sit, comb);
  // 檢查三色同順
  check_tai26(sit, comb);
  // 檢查一條龍
  check_tai27(sit, comb);
  // 檢查三槓子
  check_tai30(sit, comb);
  // 檢查三色同刻
  check_tai31(sit, comb);
  // 檢查門清自摸
  check_tai32(sit, comb);
  // 檢查碰碰胡
  check_tai33(sit, comb);
  // 檢查混一色
  check_tai34(sit, comb);
  // 檢查純帶么
  check_tai35(sit, comb);
  // 檢查混老頭
  check_tai36(sit, comb);
  // 檢查小三元
  check_tai37(sit, comb);
  // 檢查四槓子
  check_tai39(sit, comb);
  // 檢查大三元
  check_tai40(sit, comb);
  // 檢查小四喜
  check_tai41(sit, comb);
  // 檢查清一色
  check_tai42(sit, comb);
  // 檢查字一色
  check_tai43(sit, comb);
  // 檢查七搶一
  check_tai44(sit, comb);
  // 檢查五暗刻
  check_tai45(sit, comb, make_card);
  // 檢查清老頭
  check_tai46(sit, comb);
  // 檢查大四喜
  check_tai47(sit, comb);
  // 檢查八仙過海
  check_tai48(sit, comb);
  // 檢查天胡
  check_tai49(sit, comb);
  // 檢查地胡
  check_tai50(sit, comb);
  // 檢查人胡
  check_tai51(sit, comb);
  // 檢查連莊
  check_tai52(sit, comb);

  return 0;
}