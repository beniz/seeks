#automake for rpc

lib_LTLIBRARIES=libseeksrpc.la
AM_CPPFLAGS=-I../ -I../../utils -I../../lsh -I../../proxy
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksrpc_la_SOURCES=rpc_server.cpp rpc_client.cpp
	               
SUBDIRS=. protob
