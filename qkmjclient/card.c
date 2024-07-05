#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "qkmj.h"

int card_num=144;
char all_card[150]={1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,
                    7,7,7,7,8,8,8,8,9,9,9,9,11,11,11,11,12,12,12,12,
                    13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,
                    17,17,17,17,18,18,18,18,19,19,19,19,21,21,21,21,
                    22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,
                    26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,
                    31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,
                    41,41,41,41,42,42,42,42,43,43,43,43,51,52,53,54,
                    55,56,57,58};

void generate_card()
{
  // 使用時間和 process ID 來初始化亂數產生器，以獲得更隨機的結果
  srand(time(NULL) * getpid()); 

  // 初始化牌組
  memset(mj, 0, sizeof(mj));

  // 隨機分配牌
  int next_available = 0; // Track the next empty slot
  for (int i = 0; i < card_num; i++) {
    // 產生一個隨機索引
    int rand_int = generate_random(card_num - i);

    // 將牌分配到空位
    mj[next_available] = all_card[i];
    next_available++; // Move to the next empty slot
  }
}

int generate_random(int range)
{
  return rand() % range;
}
