#automake for rpc

lib_LTLIBRARIES=libseeksrpc.la
AM_CPPFLAGS=-I${srcdir}/../../utils -I${srcdir}/../../dht -I${srcdir}/../../proxy -I${srcdir}/../../lsh @LCOV_CFLAGS@
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksrpc_la_SOURCES=rpc_server.cpp rpc_client.cpp l1_rpc_client.h l1_rpc_interface.h l1_rpc_server.h rpc_client.h rpc_server.h

SUBDIRS=. protob
