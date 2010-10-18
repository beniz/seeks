#automake for rpc protob

lib_LTLIBRARIES=libseeksrpcprotob.la
AM_CPPFLAGS=-I${srcdir}/../ -I${srcdir}/../../ -I../../protob -I${srcdir}/../../protob -I${srcdir}/../../../utils -I${srcdir}/../../../lsh -I${srcdir}/../../../proxy \
	    `pkg-config --cflags protobuf`
AM_CXXFLAGS=-Wall -g
libseeksrpcprotob_la_SOURCES=l1_protob_rpc_client.cpp l1_protob_rpc_server.cpp l1_protob_rpc_client.h l1_protob_rpc_server.h 