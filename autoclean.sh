#!/bin/bash

rm -rf aclocal.m4 compile config.status install-sh Makefile.in autom4te.cache config.log configure depcomp missing configure.scan

rm -rf autoscan.log config.guess config.sub libtool ltmain.sh Makefile .deps

rm -rf src/Makefile.in src/Makefile src/.deps

rm -rf src/list/Makefile.in src/list/Makefile src/list/.deps

rm -rf src/lock/Makefile.in src/lock/Makefile src/lock/.deps
