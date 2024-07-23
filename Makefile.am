# Makefile.am 

bin_PROGRAMS = mjgps qkmj
mjgps_SOURCES = qkmjserver/mjgps.h qkmjserver/mjgps.c 
qkmj_SOURCES = qkmjclient/qkmj.c \
    qkmjclient/input.c \
    qkmjclient/screen.c \
    qkmjclient/socket.c \
    qkmjclient/command.c \
    qkmjclient/message.c \
    qkmjclient/check.c \
    qkmjclient/checkscr.c \
    qkmjclient/card.c \
    qkmjclient/chkmake.c \
    qkmjclient/misc.c \
    qkmjclient/input.h \
    qkmjclient/socket.h \
    qkmjclient/mjdef.h \
    qkmjclient/misc.h
qkmj_LDADD = $(NCURSES_LIBS)
