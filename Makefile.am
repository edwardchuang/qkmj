# Makefile.am 

bin_PROGRAMS = mjgps qkmj
mjgps_SOURCES = qkmjserver/mjgps.h qkmjserver/mjgps.c 
qkmj_SOURCES = \
    qkmjclient/card.c \
    qkmjclient/check.c \
    qkmjclient/checkscr.c \
    qkmjclient/chkmake.c \
    qkmjclient/command.c \
    qkmjclient/input.c \
    qkmjclient/input.h \
    qkmjclient/message.c \
    qkmjclient/misc.c \
    qkmjclient/misc.h \
    qkmjclient/mjdef.h \
    qkmjclient/qkmj.c \
    qkmjclient/screen.c \
    qkmjclient/socket.c \
    qkmjclient/socket.h
qkmj_LDADD = $(NCURSES_LIBS)
