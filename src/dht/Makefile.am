#automake for dht                                                                                                                                                        
lib_LTLIBRARIES=libseeksdht.la
AM_CPPFLAGS=-I${srcdir}/protob -I${srcdir}/../utils -I${srcdir}/../lsh
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdht_la_SOURCES=DHTKey.cpp DHTKey.h
noinst_HEADERS=DHTKey.h

SUBDIRS=. tests