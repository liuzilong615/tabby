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

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T

# Checks for library functions.

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable DEBUG mode(default=no)]),
        [],
        [enable_debug=no])
AS_IF([test "x$enable_debug"="xyes"], [CFLAGS="-g -DDEBUG -Wall"],
        [test "x$enable_debug"="xno"], [CFLAGS="-O2 -Wall"],
        [])

AC_OUTPUT(Makefile atom/Makefile)
