lib_LTLIBRARIES=libseeksutils.la librmd160.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../proxy
libseeksutils_la_SOURCES=miscutil.cpp mem_utils.cpp mutexes.cpp \
	                 miscutil.h stl_hash.h mutexes.h
librmd160_la_SOURCES=rmd160.cpp rmd160.h
noinst_HEADERS = mem_utils.h
