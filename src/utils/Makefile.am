lib_LTLIBRARIES=libseeksutils.la librmd160.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../proxy -I${srcdir}/../lsh -I${srcdir}/../dht @LCOV_CFLAGS@
libseeksutils_la_SOURCES=miscutil.cpp mem_utils.cpp mutexes.cpp
librmd160_la_SOURCES=rmd160.cpp
noinst_HEADERS = mem_utils.h rmd160.h miscutil.h stl_hash.h mutexes.h serialize.h charset_conv.h

libseeksutils_la_LIBADD = -L../proxy -lseeksproxy -lseeksplugins

SUBDIRS = . tests
