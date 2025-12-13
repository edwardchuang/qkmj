#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mjdef.h"
#include "qkmj.h"

#define THREE_CARD 1
#define STRAIGHT_CARD 2
#define PAIR_CARD 3

/*******************  Definition of variables  **********************/

typedef struct nodetype {
  char info[5];
  int end;
  int type;
  struct nodetype* three;    /* 刻子 */
  struct nodetype* straight; /* 順子 */
  struct nodetype* pair;     /* 對子 */
  struct nodetype* father;
} * NODEPTR;

struct card_info_type {
  int info;
  int flag; /*  0 if is chosen, 1 if not */
};
struct card_info_type card_info[20];

struct component_type {
  int type;
  /* 0:invalid   1:no card   2:normal   3:one pair  4:more than one pair */
  char info[10][5];
};
struct component_type card_component[6][50];

char pool_buf[20];
int comb_count[6];
int count2;
int suit;

/* Forward declarations */
NODEPTR make_three(NODEPTR node);
NODEPTR make_straight(NODEPTR node);
NODEPTR make_pair(NODEPTR node);
void full_check(int sit, int make_card);
void check_tai(int sit, int comb, int make_card);

/* Tai check prototypes */
void check_tai0(int sit, int comb);
void check_tai1(int sit, int comb);
void check_tai2(int sit, int comb);
void check_tai3(int sit, int comb);
void check_tai4(int sit, int comb);
void check_tai5(int sit, int comb);
void check_tai6(int sit, int comb);
void check_tai7(int sit, int comb);
// void check_tai8(int sit, int comb); // Empty
void check_tai9(int sit, int comb);
void check_tai10(int sit, int comb);
void check_tai11(int sit, int comb);
void check_tai12(int sit, int comb);
void check_tai13(int sit, int comb);
void check_tai14(int sit, int comb);
void check_tai15(int sit, int comb);
void check_tai16(int sit, int comb);
// void check_tai17(int sit, int comb); // Empty
// void check_tai18(int sit, int comb); // Empty
// void check_tai19(int sit, int comb); // Empty
// void check_tai20(int sit, int comb); // Empty
void check_tai21(int sit, int comb);
void check_tai22(int sit, int comb);
void check_tai23(int sit, int comb);
void check_tai24(int sit, int comb, int make_card);
void check_tai25(int sit, int comb);
void check_tai26(int sit, int comb);
void check_tai27(int sit, int comb);
// void check_tai28(int sit, int comb); // Empty
// void check_tai29(int sit, int comb); // Empty
// void check_tai30(int sit, int comb); // Empty
void check_tai31(int sit, int comb);
void check_tai32(int sit, int comb);
void check_tai33(int sit, int comb);
void check_tai34(int sit, int comb);
void check_tai35(int sit, int comb);
void check_tai36(int sit, int comb);
void check_tai37(int sit, int comb);
// void check_tai38(int sit, int comb); // Empty
void check_tai39(int sit, int comb);
void check_tai40(int sit, int comb);
void check_tai41(int sit, int comb);
void check_tai42(int sit, int comb);
void check_tai43(int sit, int comb);
// void check_tai44(int sit, int comb); // Empty
void check_tai45(int sit, int comb, int make_card);
void check_tai46(int sit, int comb);
void check_tai47(int sit, int comb);
// void check_tai48(int sit, int comb); // Empty
void check_tai49(int sit, int comb);
void check_tai50(int sit, int comb);
void check_tai51(int sit, int comb);
void check_tai52(int sit, int comb);

/********************************************************************/

NODEPTR getnode() {
  NODEPTR p = (NODEPTR)malloc(sizeof(struct nodetype));
  if (p == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }
  return p;
}

NODEPTR maketree(NODEPTR node) {
  NODEPTR p = getnode();
  p->info[0] = 0;
  p->type = 0;
  p->end = 0;
  p->three = NULL;
  p->straight = NULL;
  p->pair = NULL;
  p->father = node;
  return p;
}

void build_tree(NODEPTR node) {
  node->three = make_three(node); /* It will check if it's end */
  if (!node->end) {
    node->straight = make_straight(node);
  } else {
    node->straight = NULL;
  }
  if (!node->end && node->three == NULL) { /* add pair into tree if no three */
    node->pair = make_pair(node);
  } else {
    node->pair = NULL;
  }
}

void mark_card(NODEPTR node) {
  if (node->father == NULL) { /* The top of the tree */
    return;
  } else {
    int i = 0;
    for (int j = 0; card_info[j].info; j++) {
      if (node->info[i] == card_info[j].info && card_info[j].flag) {
        i++;
        card_info[j].flag = 0; /*  Mark this card as chosen */
        if (!node->info[i])    /* no more cards */
          goto marked;
      }
    }
    /* Can't find the card */
  marked:;
    mark_card(node->father);
  }
}

/* Find the leading card */
int find_lead() {
  int lead = 0;
  for (int i = 0; card_info[i].info; i++) {
    if (card_info[i].flag) {
      lead = card_info[i].info;
      break;
    }
  }
  return lead;
}

NODEPTR make_three(NODEPTR node) {
  /* Reset the flag */
  for (int i = 0; card_info[i].info; i++) {
    card_info[i].flag = 1;
  }
  mark_card(node);
  int lead = find_lead();
  if (lead == 0) {
    /* no more cards can be checked */
    node->end = 1;
    return NULL;
  }
  NODEPTR p = maketree(node);

  /* Get three same cards */
  int j = 0;
  for (int i = 0; card_info[i].info; i++) {
    if (card_info[i].flag && card_info[i].info == lead) {
      p->info[j++] = (char)lead;
      card_info[i].flag = 0;
    }
    if (j == 3) goto found_three;
  }
  /* can't find 3 cards */
  free(p);
  return NULL;
found_three:;
  p->info[j] = 0;
  p->type = THREE_CARD;
  build_tree(p);
  return p;
}

NODEPTR make_straight(NODEPTR node) {
  /* Reset the flag */
  for (int i = 0; card_info[i].info; i++) {
    card_info[i].flag = 1;
  }
  mark_card(node);
  int lead = find_lead();
  if (lead == 0) {
    node->end = 1;
    return NULL;
  }
  NODEPTR p = maketree(node);

  /* Get three straight cards */
  int j = 0;
  for (int i = 0; card_info[i].info; i++) {
    if (card_info[i].flag && card_info[i].info == lead) {
      p->info[j++] = (char)lead;
      card_info[i].flag = 0;
      lead++;
    }
    if (j == 3 && lead < 31) goto found_straight;
  }
  free(p);
  return NULL;
found_straight:;
  p->info[j] = 0;
  p->type = STRAIGHT_CARD;
  build_tree(p);
  return p;
}

NODEPTR make_pair(NODEPTR node) {
  /* Reset the flag */
  for (int i = 0; card_info[i].info; i++) {
    card_info[i].flag = 1;
  }
  mark_card(node);
  int lead = find_lead();
  if (lead == 0) {
    node->end = 1;
    return NULL;
  }
  NODEPTR p = maketree(node);

  /* Get two same cards */
  int j = 0;
  for (int i = 0; card_info[i].info; i++) {
    if (card_info[i].flag && card_info[i].info == lead) {
      p->info[j++] = (char)lead;
      card_info[i].flag = 0;
    }
    if (j == 2) goto found_two;
  }
  free(p);
  return NULL;
found_two:;
  p->info[j] = 0;
  p->type = PAIR_CARD;
  build_tree(p);
  return p;
}

/* Find a valid tree and copy it to the array */
void list_path(NODEPTR p) {
  int i = 0;
  while (p->info[i]) {
    card_component[suit][comb_count[suit]].info[count2][i] = p->info[i];
    i++;
  }
  if (p->type == PAIR_CARD) {
    if (card_component[suit][comb_count[suit]].type >= 3) {
      card_component[suit][comb_count[suit]].type = 4;
    } else {
      card_component[suit][comb_count[suit]].type = 3;
    }
  }
  card_component[suit][comb_count[suit]].info[count2][i] = 0;
  count2++;
  if (p->father->father) list_path(p->father);
}

void pretrav(NODEPTR p) {
  if (p != NULL) {
    if (p->end) /* no more cards */
    {
      count2 = 0;
      if (p->father != NULL) /* not the head */
      {
        card_component[suit][comb_count[suit]].type = 2;
        list_path(p);
      } else /* no card in this suit */
      {
        card_component[suit][0].type = 1;
      }
      comb_count[suit]++;
    }
    pretrav(p->three);
    pretrav(p->straight);
    pretrav(p->pair);
  }
}

void free_tree(NODEPTR p) {
  if (p->three) free_tree(p->three);
  if (p->straight) free_tree(p->straight);
  if (p->pair) free_tree(p->pair);
  free(p);
}

/*
 * 檢查是否胡牌 (Check Make)
 * 這是遞迴檢查的核心函式。
 * 嘗試將手牌組合成 面子 (Mentsu) 和 雀頭 (Jantou)。
 */
int check_make(int sit, int card,
               int method) /* 0 for general check, 1 for complete check */
{
  NODEPTR p[6]; /* p[0]=萬 p[1]=筒 p[2]=索 p[3]=風牌 p[4]=三元牌 */
  int i;

  /* Copy the pool to buffer */
  for (i = 0; i < pool[sit].num; i++) pool_buf[i] = pool[sit].card[i];
  pool_buf[i] = (char)card;
  pool_buf[i + 1] = 0;
  /* Sort buffer */
  for (i = 0; i <= pool[sit].num; i++)
    for (int j = 0; j < pool[sit].num - i; j++)
      if (pool_buf[j] > pool_buf[j + 1]) {
        char tmp = pool_buf[j];
        pool_buf[j] = pool_buf[j + 1];
        pool_buf[j + 1] = tmp;
      }
  for (i = 0; i < 5; i++) {
    for (int j = 0; j < 20; j++) {
      card_component[i][j].type = 0;
      for (int k = 0; k < 10; k++) card_component[i][j].info[k][0] = 0;
    }
    comb_count[i] = 0;
  }
  /****** Build tree for each suits ******/
  /* j: the pointer of cards */
  /* card_info: each suit of cards */
  int j = 0;
  for (suit = 0; suit < 5; suit++) {
    int k = 0;
    while (pool_buf[j] < suit * 10 + 10 && pool_buf[j] > suit * 10) {
      card_info[k].info = pool_buf[j];
      card_info[k].flag = 1;
      j++;
      k++;
    }
    card_info[k].info = 0;

    p[suit] = maketree(NULL); /* Root of the tree */
    card_component[suit][0].type = 0;
    build_tree(p[suit]);
    pretrav(p[suit]);
    free_tree(p[suit]);
  }

  int pair = 0;
  int make = 1;
  for (i = 0; i < 5; i++) /* Check for 5 suits */
  {
    switch (card_component[i][0].type) {
      case 0: /* Not a valid path */
        make = 0;
        goto finish;
        break;
      case 1: /* No cards in this suit */
      case 2: /* This path is valid */
        break;
      case 3: /* Found a pair */
        pair++;
        break;
      case 4: /* Find more than one pair in a suit*/
        make = 0;
        goto finish;
        break;
      default:
        break;
    }
  }
finish:;
  if (pair != 1) make = 0;
  if (make && method) {
    full_check(sit, card);
  }
  return (make);
}

int valid_type(int type) {
  if (type == 1 || type == 2 || type == 3)
    return (1);
  else
    return (0);
}

/* Find the combination of the card with the highest score */
void full_check(int sit, int make_card) {
  int i, j;
  int suit_count[6], set1, set2, card;

  int count = 0;
  for (i = 0; i < 5; i++) {
    card_comb[i].set_count = 0;
    suit_count[i] = 0;
  }
  /* Find all valid combinations */
  for (suit_count[0] = 0; suit_count[0] < comb_count[0]; suit_count[0]++) {
    if (valid_type(card_component[0][suit_count[0]].type))
      for (suit_count[1] = 0; suit_count[1] < comb_count[1]; suit_count[1]++) {
        if (valid_type(card_component[1][suit_count[1]].type))
          for (suit_count[2] = 0; suit_count[2] < comb_count[2];
               suit_count[2]++) {
            if (valid_type(card_component[2][suit_count[2]].type)) {
              set2 = 0;
              for (i = 0; i < 5; i++) {
                set1 = 0;
                while (card_component[i][suit_count[i]].info[set1][0]) {
                  card = 0;
                  while (card_component[i][suit_count[i]].info[set1][card]) {
                    card_comb[count].info[set2][card + 1] =
                        card_component[i][suit_count[i]].info[set1][card];
                    card++;
                  }
                  if (card_comb[count].info[set2][1] ==
                      card_comb[count].info[set2][2])
                    card_comb[count].info[set2][0] = 2;
                  else
                    card_comb[count].info[set2][0] = 1;
                  if (card == 2) card_comb[count].info[set2][0] = 10;
                  card_comb[count].info[set2][card + 1] = 0;
                  set1++;
                  set2++;
                }
                card_comb[count].info[set2][0] = 0;
                card_comb[count].set_count = set2;
              }
              count++;
            }
          }
      }
  }
  for (i = 0; i < count; i++) {
    check_tai(sit, i, make_card);
    card_comb[i].tai_sum = 0;
    for (j = 0; j <= 52; j++) {
      card_comb[i].tai_sum += card_comb[i].tai_score[j];
    }
  }
  comb_num = count;
}

/* return the number of the card found */
int exist_card(int sit, int card) {
  int exist = 0;

  for (int i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] == card) {
      exist++;
    }
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    int j = 1;
    while (pool[sit].out_card[i][j]) {
      if (pool[sit].out_card[i][j] == card) exist++;
      j++;
    }
  }
  return exist;
}

int exist_3(int sit, int card, int comb) {
  int exist = 0;

  int set = 0;
  while (card_comb[comb].info[set][0]) {
    if (card_comb[comb].info[set][0] == 2 &&
        card_comb[comb].info[set][1] == card)
      exist++;
    set++;
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == card && pool[sit].out_card[i][2] == card &&
        pool[sit].out_card[i][3] == card)
      exist++;
  }
  return exist;
}

int exist_straight(int sit, int card, int comb) {
  int exist = 0;

  int set = 0;
  while (card_comb[comb].info[set][0]) {
    if (card_comb[comb].info[set][0] == 1 &&
        card_comb[comb].info[set][1] == card)
      exist++;
    set++;
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 7 && pool[sit].out_card[i][2] == card)
      exist++;
    if (pool[sit].out_card[i][0] == 8 && pool[sit].out_card[i][1] == card)
      exist++;
    if (pool[sit].out_card[i][0] == 9 && pool[sit].out_card[i][1] == card)
      exist++;
  }
  return exist;
}

void check_tai(int sit, int comb, int make_card) {
  for (int i = 0; i < 100; i++) card_comb[comb].tai_score[i] = 0;
  check_tai0(sit, comb);
  check_tai1(sit, comb);
  check_tai2(sit, comb);
  check_tai3(sit, comb);
  check_tai4(sit, comb);
  check_tai5(sit, comb);
  check_tai6(sit, comb);
  check_tai7(sit, comb);
  // check_tai8(sit, comb);
  check_tai9(sit, comb);
  check_tai10(sit, comb);
  check_tai11(sit, comb);
  check_tai12(sit, comb);
  check_tai13(sit, comb);
  check_tai14(sit, comb);
  check_tai15(sit, comb);
  check_tai16(sit, comb);
  check_tai21(sit, comb);
  check_tai22(sit, comb);
  check_tai23(sit, comb);
  check_tai24(sit, comb, make_card);
  check_tai25(sit, comb);
  check_tai26(sit, comb);
  check_tai27(sit, comb);
  // check_tai30(sit, comb);
  check_tai31(sit, comb);
  check_tai32(sit, comb);
  check_tai33(sit, comb);
  check_tai34(sit, comb);
  check_tai35(sit, comb);
  check_tai36(sit, comb);
  check_tai37(sit, comb);
  check_tai39(sit, comb);
  check_tai40(sit, comb);
  check_tai41(sit, comb);
  check_tai42(sit, comb);
  check_tai43(sit, comb);
  // check_tai44(sit, comb);
  check_tai45(sit, comb, make_card);
  check_tai46(sit, comb);
  check_tai47(sit, comb);
  // check_tai48(sit, comb);
  check_tai49(sit, comb);
  check_tai50(sit, comb);
  check_tai51(sit, comb);
  check_tai52(sit, comb);
}

/* 莊家 */
void check_tai0(int sit, int comb) {
  if (info.dealer == sit) card_comb[comb].tai_score[0] = tai[0].score;
}

/* 門清 */
void check_tai1(int sit, int comb) {
  if (pool[sit].num == 16) card_comb[comb].tai_score[1] = tai[1].score;
}

/* 自摸 */
void check_tai2(int sit, int comb) {
  if (sit == card_owner) card_comb[comb].tai_score[2] = tai[2].score;
}

/* 斷麼九 */
void check_tai3(int sit, int comb) {
  if (!exist_card(sit, 1) && !exist_card(sit, 9) && !exist_card(sit, 11) &&
      !exist_card(sit, 19) && !exist_card(sit, 21) && !exist_card(sit, 29) &&
      !exist_card(sit, 31) && !exist_card(sit, 32) && !exist_card(sit, 33) &&
      !exist_card(sit, 34) && !exist_card(sit, 41) && !exist_card(sit, 42) &&
      !exist_card(sit, 43))
    card_comb[comb].tai_score[3] = tai[3].score;
}

/* 雙龍抱 */
void check_tai4(int sit, int comb) {
  int straight[30], double_straight_num = 0;

  if (pool[sit].num != 16) /* 必須門清 */
    return;
  for (int i = 0; i < 30; i++) straight[i] = 0;
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) straight[card_comb[comb].info[i][1]]++;
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 7) straight[pool[sit].out_card[i][2]]++;
    if (pool[sit].out_card[i][0] == 8) straight[pool[sit].out_card[i][1]]++;
    if (pool[sit].out_card[i][0] == 9) straight[pool[sit].out_card[i][1]]++;
  }
  for (int i = 0; i < 30; i++) {
    if (straight[i] >= 2) double_straight_num++;
  }
  if (double_straight_num == 1) card_comb[comb].tai_score[4] = tai[4].score;
  if (double_straight_num >= 2) card_comb[comb].tai_score[28] = tai[28].score;
}

/* 杠上開花 */
void check_tai5(int sit, int comb) {
  if (in_kang) card_comb[comb].tai_score[5] = tai[5].score;
}

/* 海底摸月 */
void check_tai6(int sit, int comb) {
  if ((144 - card_point) == 16 && sit == card_owner)
    card_comb[comb].tai_score[6] = tai[6].score;
}

/* 河底撈魚 */
void check_tai7(int sit, int comb) {
  if ((144 - card_point) == 16 && sit != card_owner)
    card_comb[comb].tai_score[7] = tai[7].score;
}

/* 東風 */
void check_tai9(int sit, int comb) {
  if (exist_card(sit, 31) >= 3) {
    if (info.wind == 1) card_comb[comb].tai_score[9] += tai[9].score;
    if (pool[sit].door_wind == 1) card_comb[comb].tai_score[9] += tai[9].score;
  }
  if (card_comb[comb].tai_score[9] == tai[9].score * 2) {
    card_comb[comb].tai_score[17] = tai[17].score;
    card_comb[comb].tai_score[9] = 0;
  }
}

/* 南風 */
void check_tai10(int sit, int comb) {
  if (exist_card(sit, 32) >= 3) {
    if (info.wind == 2) card_comb[comb].tai_score[10] += tai[10].score;
    if (pool[sit].door_wind == 2)
      card_comb[comb].tai_score[10] += tai[10].score;
  }
  if (card_comb[comb].tai_score[10] == tai[10].score * 2) {
    card_comb[comb].tai_score[18] = tai[18].score;
    card_comb[comb].tai_score[10] = 0;
  }
}

/* 西風 */
void check_tai11(int sit, int comb) {
  if (exist_card(sit, 33) >= 3) {
    if (info.wind == 3) card_comb[comb].tai_score[11] += tai[11].score;
    if (pool[sit].door_wind == 3)
      card_comb[comb].tai_score[11] += tai[11].score;
  }
  if (card_comb[comb].tai_score[11] == tai[11].score * 2) {
    card_comb[comb].tai_score[19] = tai[19].score;
    card_comb[comb].tai_score[11] = 0;
  }
}

/* 北風 */
void check_tai12(int sit, int comb) {
  if (exist_card(sit, 34) >= 3) {
    if (info.wind == 4) card_comb[comb].tai_score[12] += tai[12].score;
    if (pool[sit].door_wind == 4)
      card_comb[comb].tai_score[12] += tai[12].score;
  }
  if (card_comb[comb].tai_score[12] == tai[12].score * 2) {
    card_comb[comb].tai_score[20] = tai[20].score;
    card_comb[comb].tai_score[12] = 0;
  }
}

/* 紅中 */
void check_tai13(int sit, int comb) {
  if (exist_card(sit, 41) >= 3) card_comb[comb].tai_score[13] = tai[13].score;
}

/* 白板 */
void check_tai14(int sit, int comb) {
  if (exist_card(sit, 42) >= 3) card_comb[comb].tai_score[14] = tai[14].score;
}

/* 青發 */
void check_tai15(int sit, int comb) {
  if (exist_card(sit, 43) >= 3) card_comb[comb].tai_score[15] = tai[15].score;
}

/* 花牌 */
void check_tai16(int sit, int comb) {
  if (pool[sit].flower[pool[sit].door_wind - 1])
    card_comb[comb].tai_score[16] += tai[16].score;
  if (pool[sit].flower[pool[sit].door_wind + 3])
    card_comb[comb].tai_score[16] += tai[16].score;
}

/* 春夏秋冬 */
void check_tai21(int sit, int comb) {
  if (pool[sit].flower[0] && pool[sit].flower[1] && pool[sit].flower[2] &&
      pool[sit].flower[3]) {
    card_comb[comb].tai_score[21] = tai[21].score;
    card_comb[comb].tai_score[16] -= tai[16].score;
  }
}

/* 梅蘭菊竹 */
void check_tai22(int sit, int comb) {
  if (pool[sit].flower[4] && pool[sit].flower[5] && pool[sit].flower[6] &&
      pool[sit].flower[7]) {
    card_comb[comb].tai_score[22] = tai[22].score;
    card_comb[comb].tai_score[16] -= tai[16].score;
  }
}

/* 全求人 */
void check_tai23(int sit, int comb) {
  if (pool[sit].num == 1 && sit != card_owner)
    card_comb[comb].tai_score[23] = tai[23].score;
}

/* 平胡 */
void check_tai24(int sit, int comb, int make_card) {
  int i;

  /* 花牌 */
  for (i = 0; i < 8; i++)
    if (pool[sit].flower[i]) goto fail24;
  /* 順子 */
  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2) goto fail24;
    if (card_comb[comb].info[i][0] == 10) {
      if (card_comb[comb].info[i][1] == make_card) goto fail24;
      if (card_comb[comb].info[i][1] > 30) goto fail24;
    }
  }
  /* 門清 */
  /*
    if(pool[sit].out_card_index==0)
      goto fail24;
  */
  /* 自摸 */
  if (turn == card_owner) goto fail24;
  /* 順子 */
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == pool[sit].out_card[i][2]) goto fail24;
  }
  /* 兩面聽牌 */
  if (make_card % 10 <= 6)
    if (check_make(sit, make_card + 3, 0)) goto finish24;
  if (make_card % 10 >= 4)
    if (check_make(sit, make_card - 3, 0)) goto finish24;
  goto fail24;
finish24:;
  check_make(sit, make_card, 0); /* Reset pool_buf[] */
  card_comb[comb].tai_score[24] = tai[24].score;
fail24:;
}

/* 混帶麼 */
void check_tai25(int sit, int comb) {
  int i, exist19 = 0;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][1] < 30) {
      exist19 = 1;
      if (card_comb[comb].info[i][1] % 10 == 1 ||
          card_comb[comb].info[i][1] % 10 == 9 ||
          card_comb[comb].info[i][2] % 10 == 1 ||
          card_comb[comb].info[i][2] % 10 == 9 ||
          card_comb[comb].info[i][3] % 10 == 1 ||
          card_comb[comb].info[i][3] % 10 == 9)
        continue;
      else
        goto fail25;
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] < 30) {
      if (pool[sit].out_card[i][1] % 10 == 1 ||
          pool[sit].out_card[i][1] % 10 == 9 ||
          pool[sit].out_card[i][2] % 10 == 1 ||
          pool[sit].out_card[i][2] % 10 == 9 ||
          pool[sit].out_card[i][3] % 10 == 1 ||
          pool[sit].out_card[i][3] % 10 == 9)
        continue;
      else
        goto fail25;
    }
  }
  if (exist19) {
    card_comb[comb].tai_score[25] = tai[25].score;
    if (pool[sit].num != 16) card_comb[comb].tai_score[25] -= 1;
  }
fail25:;
}

/* 三色同順 */
void check_tai26(int sit, int comb) {
  int num;

  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) {
      num = card_comb[comb].info[i][1] % 10;
      if (exist_straight(sit, num, comb) &&
          exist_straight(sit, num + 10, comb) &&
          exist_straight(sit, num + 20, comb)) {
        card_comb[comb].tai_score[26] = tai[26].score;
        goto finish26;
      }
    }
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] >= 7 && pool[sit].out_card[i][0] <= 9) {
      if (pool[sit].out_card[i][0] == 7)
        num = pool[sit].out_card[i][2] % 10;
      else
        num = pool[sit].out_card[i][1] % 10;
      if (exist_straight(sit, num, comb) &&
          exist_straight(sit, num + 10, comb) &&
          exist_straight(sit, num + 20, comb)) {
        card_comb[comb].tai_score[26] = tai[26].score;
        goto finish26;
      }
    }
  }
  return;
finish26:;
  if (pool[sit].num != 16) card_comb[comb].tai_score[26] -= 1;
}

/* 一條龍 */
void check_tai27(int sit, int comb) {
  if (exist_straight(sit, 1, comb) && exist_straight(sit, 4, comb) &&
      exist_straight(sit, 7, comb))
    card_comb[comb].tai_score[27] = tai[27].score;
  if (exist_straight(sit, 11, comb) && exist_straight(sit, 14, comb) &&
      exist_straight(sit, 17, comb))
    card_comb[comb].tai_score[27] = tai[27].score;
  if (exist_straight(sit, 21, comb) && exist_straight(sit, 24, comb) &&
      exist_straight(sit, 27, comb))
    card_comb[comb].tai_score[27] = tai[27].score;
  if (card_comb[comb].tai_score[27] && pool[sit].num != 16)
    card_comb[comb].tai_score[27] -= 1;
}

/* 三色同刻 */
void check_tai31(int sit, int comb) {
  int num;

  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2) {
      num = card_comb[comb].info[i][1] % 10;
      if (exist_3(sit, num, comb) && exist_3(sit, num + 10, comb) &&
          exist_3(sit, num + 20, comb)) {
        card_comb[comb].tai_score[31] = tai[31].score;
        goto finish31;
      }
    }
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] == pool[sit].out_card[i][2] &&
        pool[sit].out_card[i][1] == pool[sit].out_card[i][3]) {
      num = pool[sit].out_card[i][1] % 10;
      if (exist_3(sit, num, comb) && exist_3(sit, num + 10, comb) &&
          exist_3(sit, num + 20, comb)) {
        card_comb[comb].tai_score[31] = tai[31].score;
        goto finish31;
      }
    }
  }
finish31:;
}

/* 門清自摸 */
void check_tai32(int sit, int comb) {
  if (pool[sit].num == 16 && sit == card_owner) {
    card_comb[comb].tai_score[32] = tai[32].score;
    card_comb[comb].tai_score[1] = 0;
    card_comb[comb].tai_score[2] = 0;
  }
}

/* 碰碰胡 */
void check_tai33(int sit, int comb) {
  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 1) goto fail33;
  }
  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] != pool[sit].out_card[i][2]) {
      goto fail33;
    }
  }
  card_comb[comb].tai_score[33] = tai[33].score;
fail33:;
}

/* 混一色 */
void check_tai34(int sit, int comb) {
  int i, kind;

  card_comb[comb].tai_score[34] = tai[34].score;
  for (i = 0; i <= pool[sit].num; i++) {
    kind = pool_buf[i] / 10;
    if (kind <= 2) goto found34;
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    kind = pool[sit].out_card[i][1] / 10;
    if (kind <= 2) goto found34;
  }
  card_comb[comb].tai_score[34] = 0;
  goto fail34;
found34:;
  for (i = 1; i <= pool[sit].num; i++)
    if (kind != pool_buf[i] / 10 && pool_buf[i] / 10 <= 2) {
      card_comb[comb].tai_score[34] = 0;
      goto fail34;
    }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (kind != pool[sit].out_card[i][1] / 10 &&
        pool[sit].out_card[i][1] / 10 <= 2) {
      card_comb[comb].tai_score[34] = 0;
      goto fail34;
    }
  }
fail34:;
}

/* 純帶麼 */
void check_tai35(int sit, int comb) {
  int i, exist19 = 0;

  for (i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][1] > 30) goto fail35;
    if (card_comb[comb].info[i][1] % 10 == 1 ||
        card_comb[comb].info[i][1] % 10 == 9 ||
        card_comb[comb].info[i][2] % 10 == 1 ||
        card_comb[comb].info[i][2] % 10 == 9 ||
        card_comb[comb].info[i][3] % 10 == 1 ||
        card_comb[comb].info[i][3] % 10 == 9)
      continue;
    else
      goto fail35;
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] > 30) goto fail35;
    if (pool[sit].out_card[i][1] % 10 == 1 ||
        pool[sit].out_card[i][1] % 10 == 9 ||
        pool[sit].out_card[i][2] % 10 == 1 ||
        pool[sit].out_card[i][2] % 10 == 9 ||
        pool[sit].out_card[i][3] % 10 == 1 ||
        pool[sit].out_card[i][3] % 10 == 9)
      continue;
    else
      goto fail35;
  }
  card_comb[comb].tai_score[35] = tai[35].score;
  card_comb[comb].tai_score[25] = 0;
fail35:;
}

/* 混老頭 */
void check_tai36(int sit, int comb) {
  int i, exist19 = 0;

  for (i = 0; i <= pool[sit].num; i++) {
    if (pool_buf[i] < 30) {
      exist19 = 1;
      if (pool_buf[i] % 10 != 1 && pool_buf[i] % 10 != 9) {
        goto fail36;
      }
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] / 10 < 30) /* if 字牌 --> 檢查下一組 */
    {
      exist19 = 1;
      if (pool[sit].out_card[i][1] % 10 != 1 &&
          pool[sit].out_card[i][1] % 10 != 9)
        goto fail36;
      if (pool[sit].out_card[i][2] % 10 != 1 &&
          pool[sit].out_card[i][2] % 10 != 9)
        goto fail36;
      if (pool[sit].out_card[i][3] % 10 != 1 &&
          pool[sit].out_card[i][3] % 10 != 9)
        goto fail36;
    }
  }
  if (exist19) {
    card_comb[comb].tai_score[36] = tai[36].score;
    card_comb[comb].tai_score[25] = 0;
    card_comb[comb].tai_score[33] = 0;
  }
fail36:;
}

/* 小三元 */
void check_tai37(int sit, int comb) {
  if (exist_card(sit, 41) >= 2 && exist_card(sit, 42) >= 2 &&
      exist_card(sit, 43) >= 2) {
    card_comb[comb].tai_score[37] = tai[37].score;
    card_comb[comb].tai_score[13] = 0;
    card_comb[comb].tai_score[14] = 0;
    card_comb[comb].tai_score[15] = 0;
  }
}

/* 四杠子 */
void check_tai39(int sit, int comb) {
  int kang_count = 0;

  for (int i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][0] == 3 || pool[sit].out_card[i][0] == 11 ||
        pool[sit].out_card[i][0] == 12)
      kang_count++;
  }
  if (kang_count == 3) card_comb[comb].tai_score[30] = tai[30].score;
  if (kang_count == 4) card_comb[comb].tai_score[39] = tai[39].score;
}

/* 大三元 */
void check_tai40(int sit, int comb) {
  if (exist_card(sit, 41) >= 3 && exist_card(sit, 42) >= 3 &&
      exist_card(sit, 43) >= 3) {
    card_comb[comb].tai_score[40] = tai[40].score;
    card_comb[comb].tai_score[13] = 0;
    card_comb[comb].tai_score[14] = 0;
    card_comb[comb].tai_score[15] = 0;
    card_comb[comb].tai_score[37] = 0;
  }
}

/* 小四喜 */
void check_tai41(int sit, int comb) {
  if (exist_card(sit, 31) >= 2 && exist_card(sit, 32) >= 2 &&
      exist_card(sit, 33) >= 2 && exist_card(sit, 34) >= 2) {
    card_comb[comb].tai_score[41] = tai[41].score;
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
void check_tai42(int sit, int comb) {
  int i, kind;

  card_comb[comb].tai_score[42] = tai[42].score;
  kind = pool_buf[0] / 10;
  if (kind >= 3) {
    card_comb[comb].tai_score[42] = 0;
    goto fail42;
  }
  for (i = 1; i <= pool[sit].num; i++)
    if (kind != pool_buf[i] / 10) {
      card_comb[comb].tai_score[42] = 0;
      goto fail42;
    }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (kind != pool[sit].out_card[i][1] / 10) {
      card_comb[comb].tai_score[42] = 0;
      goto fail42;
    }
  }
  card_comb[comb].tai_score[34] = 0;
fail42:;
}

/* 字一色 */
void check_tai43(int sit, int comb) {
  int i, j;

  for (i = 0; i <= pool[sit].num; i++) {
    if (!(pool_buf[i] <= 34 && pool_buf[i] >= 31) &&
        !(pool_buf[i] <= 43 && pool_buf[i] >= 41)) {
      goto fail43;
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    j = 1;
    while (pool[sit].out_card[i][j]) {
      if (!(pool[sit].out_card[i][j] <= 34 && pool[sit].out_card[i][j] >= 31) &&
          !(pool[sit].out_card[i][j] <= 43 &&
            pool[sit].out_card[i][j] >= 41)) {
        goto fail43;
      }
      j++;
    }
  }
  card_comb[comb].tai_score[43] = tai[43].score;
  card_comb[comb].tai_score[33] = 0;
fail43:;
}

/* 五暗刻 */
void check_tai45(int sit, int comb, int make_card) {
  int three_card = 0;

  for (int i = 0; i < card_comb[comb].set_count; i++) {
    if (card_comb[comb].info[i][0] == 2)
      if (card_comb[comb].info[i][1] != make_card || sit == card_owner)
        three_card++; /* 自摸可算暗刻 */
  }
  for (int i = 0; i < pool[sit].out_card_index; i++)
    if (pool[sit].out_card[i][0] == 11) three_card++;
  if (three_card == 3) card_comb[comb].tai_score[29] = tai[29].score;
  if (three_card == 4) card_comb[comb].tai_score[38] = tai[38].score;
  if (three_card == 5) {
    card_comb[comb].tai_score[45] = tai[45].score;
    card_comb[comb].tai_score[33] = 0;
  }
}

/* 清老頭 */
void check_tai46(int sit, int comb) {
  int i;

  for (i = 0; i <= pool[sit].num; i++) {
    if ((pool_buf[i] % 10 != 1 && pool_buf[i] % 10 != 9) || pool_buf[i] > 30) {
      goto fail46;
    }
  }
  for (i = 0; i < pool[sit].out_card_index; i++) {
    if (pool[sit].out_card[i][1] > 30) goto fail46;
    if (pool[sit].out_card[i][1] % 10 != 1 &&
        pool[sit].out_card[i][1] % 10 != 9)
      goto fail46;
    if (pool[sit].out_card[i][2] % 10 != 1 &&
        pool[sit].out_card[i][2] % 10 != 9)
      goto fail46;
    if (pool[sit].out_card[i][3] % 10 != 1 &&
        pool[sit].out_card[i][3] % 10 != 9)
      goto fail46;
  }
  card_comb[comb].tai_score[46] = tai[46].score;
  card_comb[comb].tai_score[33] = 0;
  card_comb[comb].tai_score[35] = 0;
  card_comb[comb].tai_score[36] = 0;
fail46:;
}

/* 大四喜 */
void check_tai47(int sit, int comb) {
  if (exist_card(sit, 31) >= 3 && exist_card(sit, 32) >= 3 &&
      exist_card(sit, 33) >= 3 && exist_card(sit, 34) >= 3) {
    card_comb[comb].tai_score[47] = tai[47].score;
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

/* 天胡 */
void check_tai49(int sit, int comb) {
  if (pool[sit].first_round && sit == info.dealer)
    card_comb[comb].tai_score[49] = tai[49].score;
}

/* 地胡 */
void check_tai50(int sit, int comb) {
  if (pool[sit].first_round && sit == card_owner && sit != info.dealer)
    card_comb[comb].tai_score[50] = tai[50].score;
}

/* 人胡 */
void check_tai51(int sit, int comb) {
  if (pool[sit].first_round && sit != card_owner && sit != info.dealer)
    card_comb[comb].tai_score[51] = tai[51].score;
}

/* 連莊 */
void check_tai52(int sit, int comb) {
  if (info.cont_dealer && info.dealer == sit)
    card_comb[comb].tai_score[52] = tai[52].score * info.cont_dealer;
}