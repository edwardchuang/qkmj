#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> // For time()

#define MAX_CARDS 144
#define MAX_CARD_TYPES 150

#include "qkmj.h"

char all_card[150]={1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,
                    7,7,7,7,8,8,8,8,9,9,9,9,11,11,11,11,12,12,12,12,
                    13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,
                    17,17,17,17,18,18,18,18,19,19,19,19,21,21,21,21,
                    22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,
                    26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,
                    31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,
                    41,41,41,41,42,42,42,42,43,43,43,43,51,52,53,54,
                    55,56,57,58};

void generate_card() {
  srand48(time(NULL)); // Seed only once (better, but still not perfect)

  // Fisher-Yates Shuffle directly in all_card
  for (int i = MAX_CARDS - 1; i > 0; i--) {
    int j = (int)(drand48() * (i + 1)); // Important: i + 1 for correct range
    // Swap
    char temp = all_card[i];
    all_card[i] = all_card[j];
    all_card[j] = temp;
  }

  // Copy the shuffled cards to mj (if still needed)
    memcpy(mj, all_card, sizeof(char) * MAX_CARDS);
}

int generate_random(int range) {
    return (int)(drand48() * range);
}
