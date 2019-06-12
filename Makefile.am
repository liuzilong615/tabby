AUTOMAKE_OPTIONS = foreign
SUBDIRS = src
lib_LTLIBRARIES = libtabby.la
libtabby_la_SOURCES =
libtabby_la_LDFLAGS = -version-info 0:0:0
libtabby_la_LIBADD = \
         src/list/liblist.la \
         src/lock/liblock.la

pkginclude_HEADERS = \
         include/tabby_atom.h  \
         include/tabby_list.h  \
         include/tabby_lock.h  \
         include/tabby_types.h \
         include/tabby_macro.h \
         include/tabby_assert.h 
