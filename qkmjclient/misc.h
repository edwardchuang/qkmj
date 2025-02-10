#ifndef MISC_H
#define MISC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <sys/time.h>
#include <time.h> // Include for time functions
#include <errno.h> // Include for errno
#include "mjdef.h"
#include "qkmj.h"

// 計算思考時間
float thinktime(struct timeval before);
// 從 ncurses 視窗讀取字串
void mvwgetstring(WINDOW *win, int y, int x, int max_len, char *str_buf, int mode);

#endif // MISC_H
