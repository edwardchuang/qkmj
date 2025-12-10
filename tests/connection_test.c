#include <stdio.h>
#include <stdlib.h>
#include "mjdef.h"
#include "ai_client.h"
#include "qkmj.h"

// Mock globals
struct table_info info;
struct player_info player[MAX_PLAYER];
struct pool_info pool[5];
int table[5];
int my_sit = 1;
int turn = 1;
int check_flag[5][8];
int card_point = 0;
unsigned char my_name[50] = "TestUser";
int gps_sockfd = 0;

void display_comment(char* msg) {
    printf("[DISPLAY] %s\n", msg);
}

void write_msg(int fd, char* msg) {
    printf("[WRITE_MSG] %s\n", msg);
}

int main() {
    printf("Starting connection test...\n");
    ai_init();
    ai_set_enabled(1); // Should trigger registration
    
    // Test decision
    printf("Requesting decision...\n");
    ai_decision_t dec = ai_get_decision(AI_PHASE_DISCARD, 11, 0);
    
    printf("Decision action: %d\n", dec.action);
    ai_cleanup();
    return 0;
}

