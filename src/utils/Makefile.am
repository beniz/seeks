lib_LTLIBRARIES=libseeksutils.la librmd160.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../proxy
libseeksutils_la_SOURCES=miscutil.cpp mem_utils.cpp mutexes.cpp
librmd160_la_SOURCES=rmd160.cpp
noinst_HEADERS = mem_utils.h rmd160.h miscutil.h stl_hash.h mutexes.h serialize.h
