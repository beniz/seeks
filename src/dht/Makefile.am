#automake for dht                                                                                                                                                        
lib_LTLIBRARIES=libseeksdht.la
AM_CPPFLAGS=-I${srcdir}/protob -I${srcdir}/../utils -I${srcdir}/../lsh
AM_CXXFLAGS=-Wall -g -Wno-deprecated @LCOV_CFLAGS@
libseeksdht_la_SOURCES=DHTKey.cpp DHTKey.h
noinst_HEADERS=DHTKey.h
libseeksdht_la_LIBADD=-L../utils -lrmd160

SUBDIRS=. tests