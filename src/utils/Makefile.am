lib_LTLIBRARIES=libseeksutils.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../proxy -I${srcdir}/../lsh -I${srcdir}/../dht @LCOV_CFLAGS@
libseeksutils_la_SOURCES=miscutil.cpp mem_utils.cpp mutexes.cpp
noinst_HEADERS = mem_utils.h miscutil.h stl_hash.h mutexes.h serialize.h charset_conv.h

SUBDIRS = . tests
