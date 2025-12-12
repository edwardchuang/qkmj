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

// Mock WINDOW for tests
typedef struct _win_st { int dummy; } WINDOW;
WINDOW mock_win;
WINDOW *stdscr = &mock_win;

void display_comment(char* msg) {
    printf("[DISPLAY] %s\n", msg);
}

void wmvaddstr(WINDOW* win, int y, int x, char* str) {
    printf("[SCREEN] (%d,%d) %s\n", y, x, str);
}

void redraw_screen() {
    printf("[SCREEN] Redraw\n");
}

// Mock ncurses attribute functions
int wattr_on(WINDOW *win, attr_t attrs, void *opts) { return 0; }
int wattr_off(WINDOW *win, attr_t attrs, void *opts) { return 0; }

#undef mvwinch
#undef mvwaddch

int wmove(WINDOW *win, int y, int x) { return 0; }
chtype winch(WINDOW *win) { return 0; }
int waddch(WINDOW *win, const chtype ch) { return 0; }
int wrefresh(WINDOW *win) { return 0; }

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

