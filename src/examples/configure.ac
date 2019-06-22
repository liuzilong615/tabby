#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([examples], [1.0], [liuzilong615@163.com])
AC_CONFIG_SRCDIR([atom/atom32.c])

#AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(tabby, tabby_list_new)
AC_CHECK_LIB(ncurses, initscr)

# Checks for header files.
AC_CHECK_HEADERS(pthread.h stdio.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T

# Checks for library functions.

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable DEBUG mode(default=no)]),
        [],
        [enable_debug=no])
AS_IF([test "x$enable_debug"="xyes"], [CFLAGS="-g -DDEBUG -Wall"],
        [test "x$enable_debug"="xno"], [CFLAGS="-O2 -Wall"],
        [])

AC_OUTPUT(Makefile atom/Makefile list/Makefile tree/Makefile)
