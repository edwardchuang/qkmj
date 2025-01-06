#ifndef _CHECKMAKE_H_
#define _CHECKMAKE_H_

/*******************  Denifition of variables  **********************/

struct nodetype {
  char info[5];
  int end;
  int type;
  struct nodetype *three;       /* 刻子 */
  struct nodetype *straight;    /* 順子 */
  struct nodetype *pair;        /* 對子 */
  struct nodetype *father;
};
typedef struct nodetype *NODEPTR;

// 牌型資訊
struct card_info_type {
    int info;
    int flag; // 紀錄牌是否已使用
};

NODEPTR make_three(NODEPTR node);
NODEPTR make_straight(NODEPTR node);
NODEPTR make_pair(NODEPTR node);

struct component_type{
  int type;
  /* 0:invalid   1:no card   2:normal   3:one pair  4:more than one pair */
  char info[10][5];
};

NODEPTR getnode(void);
NODEPTR maketree(NODEPTR node);
void build_tree(NODEPTR node);
void mark_card(NODEPTR node);
int find_lead();
NODEPTR make_three(NODEPTR node);
NODEPTR make_straight(NODEPTR node);
NODEPTR make_pair(NODEPTR node);
void list_path(NODEPTR p);
void pretrav(NODEPTR p);
void free_tree(NODEPTR p);
int valid_type(int type);
int exist_card(char sit, char card);
int exist_3(char sit, char card, int comb);
int exist_straight(char sit, char card, int comb);
void check_tai0(char sit, char comb);
void check_tai1(char sit, char comb);
void check_tai2(char sit, char comb);
void check_tai3(char sit, char comb);
void check_tai4(char sit, char comb);
void check_tai5(char sit, char comb);
void check_tai6(char sit, char comb);
void check_tai7(char sit, char comb);
void check_tai8(char sit, char comb);
void check_tai9(char sit, char comb, int count);
void check_tai10(char sit, char comb, int count);
void check_tai11(char sit, char comb, int count);
void check_tai12(char sit, char comb, int count);
void check_tai13(char sit, char comb);
void check_tai14(char sit, char comb);
void check_tai15(char sit, char comb);
void check_tai16(char sit, char comb);
void check_tai17(char sit, char comb);
void check_tai18(char sit, char comb);
void check_tai19(char sit, char comb);
void check_tai20(char sit, char comb);
void check_tai21(char sit, char comb);
void check_tai22(char sit, char comb);
void check_tai23(char sit, char comb);
void check_tai24(char sit, char comb, char make_card);
void check_tai25(char sit, char comb);
void check_tai26(char sit, char comb);
void check_tai27(char sit, char comb);
void check_tai28(char sit, char comb);
void check_tai29(char sit, char comb);
void check_tai30(char sit, char comb);
void check_tai31(char sit, char comb);
void check_tai32(char sit, char comb);
void check_tai33(char sit, char comb);
void check_tai34(char sit, char comb);
void check_tai35(char sit, char comb);
void check_tai36(char sit, char comb);
void check_tai37(char sit, char comb);
void check_tai38(char sit, char comb);
void check_tai39(char sit, char comb);
void check_tai40(char sit, char comb);
void check_tai41(char sit, char comb);
void check_tai42(char sit, char comb);
void check_tai43(char sit, char comb);
void check_tai44(char sit, char comb);
void check_tai45(char sit, char comb, char make_card);
void check_tai46(char sit, char comb);
void check_tai47(char sit, char comb);
void check_tai48(char sit, char comb);
void check_tai49(char sit, char comb);
void check_tai50(char sit, char comb);
void check_tai51(char sit, char comb);
void check_tai52(char sit, char comb);
void full_check(char sit, char make_card);
int check_make(char playerIndex, char winningTile, char completeCheck);
void check_tai(char sit, char comb, char make_card);

#endif/*_CHECKMAKE_H_*/
