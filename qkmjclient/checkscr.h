#ifndef _CHECKSCR_H_
#define _CHECKSCR_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#ifdef HAVE_CJSON_CJSON_H
#include <cjson/cJSON.h>
#endif

#include "mjdef.h"

void init_check_mode();
void process_make(char sit, char card);
void process_epk(char check);
void draw_epk(char id, char kind, char card1, char card2, char card3);
void draw_flower(char sit, char card);
static void calculate_money_change(long change_money[], int sit, int card_owner, int max_index);
static void update_player_money(long change_money[]);
static void display_money_summary(long change_money[]);

#endif
