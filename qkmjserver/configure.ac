#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([qkmjserver], [0.1], [root@localhost])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([mjgps.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
# FIXME: Replace 'main' with a function in '-lcrypt':
AC_CHECK_LIB([crypt], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h sys/file.h sys/param.h sys/socket.h sys/time.h termio.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([alarm bzero gethostbyaddr inet_ntoa select socket])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
