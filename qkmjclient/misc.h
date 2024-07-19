#ifndef _MISC_H_
#define _MISC_H_

#if defined(HAVE_LIBNCURSES)
  #include  <ncurses.h>
#endif

float thinktime();
void beep1();
void beepbeep();
void mvwgetstring(WINDOW *win,int y,int x,int max_len,unsigned char *str_buf,int mode);

#endif
