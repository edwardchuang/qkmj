#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([qkmj], [0.1])
AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([qkmjclient/socket.c])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

AC_CHECK_LIB(ncursesw, killwchar, [],
[
    AC_CHECK_LIB(ncurses, killwchar, [], 
    [
        AC_SUBST([NCURSESW_CFLAGS])
        AC_SUBST([NCURSESW_LIBS])
        if pkg-config --exists ncursesw
        then
            AC_MSG_NOTICE([Module ncursesw found])
            NCURSESW_CFLAGS=`pkg-config --cflags ncursesw`
            NCURSESW_LIBS=`pkg-config --libs ncursesw`
        else
            if pkg-config --exists ncurses
            then
                AC_MSG_NOTICE([Module ncurses found])
                NCURSESW_CFLAGS=`pkg-config --cflags ncursesw`
                NCURSESW_LIBS=`pkg-config --libs ncurses`
            else
                AS_CASE([$host_os],
                  [darwin*],
                    [
                      AC_CHECK_LIB(ncurses, killwchar, [], [AC_MSG_ERROR([Could not find ncurses library])])
                      AC_CHECK_HEADER(curses.h)
                    ],
                  [
                    AC_CHECK_LIB(ncursesw, killwchar, [], [AC_MSG_ERROR([Could not find ncursesw library])])
                    AC_CHECK_HEADER(ncursesw/curses.h)
                  ]
                )
            fi
        fi
    ]) # FAIL of ncurses
]) # FAIL of ncursesw

AC_CHECK_HEADERS([ncursesw/curses.h], [],
    [AC_CHECK_HEADERS([ncurses/ncurses.h], [],
        [AC_CHECK_HEADERS([ncurses/curses.h], [],
        [AC_CHECK_HEADERS([ncurses.h], [],
            [AC_MSG_ERROR([can not find required ncurses header file])])])])])

AC_CHECK_LIB([m], [main])
AC_CHECK_LIB([crypt], [main])
AC_CHECK_LIB([cjson], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h sgtty.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termio.h termios.h unistd.h cjson/cJSON.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([alarm bzero gethostbyaddr gethostbyname gethostname gettimeofday inet_ntoa select setenv socket strerror strstr])

AC_CONFIG_FILES([Makefile 
                 qkmjclient/Makefile 
                 qkmjserver/Makefile])
AC_OUTPUT
