#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([qkmj], [0.1])
AC_CONFIG_SRCDIR([qkmjclient/socket.c])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([m], [main])
AC_CHECK_LIB([ncurses], [main])
AC_CHECK_LIB([crypt], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h sgtty.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termio.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([alarm bzero gethostbyaddr gethostbyname gethostname gettimeofday inet_ntoa select setenv socket strerror strstr])

AC_CONFIG_FILES([Makefile 
                 qkmjclient/Makefile 
                 qkmjserver/Makefile])
AC_OUTPUT
