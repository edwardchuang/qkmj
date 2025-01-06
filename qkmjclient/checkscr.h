#ifndef _CHECKSCR_H_
#define _CHECKSCR_H_

#include "mjdef.h"

void init_check_mode();
void process_make(char sit, char card);
void process_epk(char check);
void draw_epk(char id, char kind, char card1, char card2, char card3);
void draw_flower(char sit, char card);

#endif
