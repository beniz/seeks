check_PROGRAMS = 

check_PROGRAMS += ut_miscutil
ut_miscutil_SOURCES = ut-miscutil.cpp
ut_miscutil_CPPFLAGS = $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)
ut_miscutil_LDFLAGS = $(GTEST_LDFLAGS) -L.
ut_miscutil_LDADD = $(GTEST_LIBS) -lseeksutils @LCOV_LIBS@

TESTS = $(check_PROGRAMS)

lib_LTLIBRARIES=libseeksutils.la librmd160.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../proxy @LCOV_CFLAGS@
libseeksutils_la_SOURCES=miscutil.cpp mem_utils.cpp mutexes.cpp
librmd160_la_SOURCES=rmd160.cpp
noinst_HEADERS = mem_utils.h rmd160.h miscutil.h stl_hash.h mutexes.h serialize.h
