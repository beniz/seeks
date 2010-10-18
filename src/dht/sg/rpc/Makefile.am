lib_LTLIBRARIES=libseeksdhtsgrpc.la
AM_CPPFLAGS=-I${srcdir}/../ -I${srcdir}/../../ -I${srcdir}/../../protob -I../../protob -I${srcdir}/../../rpc -I${srcdir}/../../rpc/protob -I${srcdir}/../../../lsh -I${srcdir}/../../../proxy \
	    -I${srcdir}/../protob -I../protob -I${srcdir}/../../../utils `pkg-config --cflags protobuf`
AM_CXXFLAGS=-Wall -g
libseeksdhtsgrpc_la_SOURCES=l2_protob_rpc_server.cpp l2_protob_rpc_client.cpp l2_protob_rpc_client.h l2_protob_rpc_server.h l2_rpc_interface.h
