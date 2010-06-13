lib_LTLIBRARIES=libseeksdhtsgrpc.la
AM_CPPFLAGS=-I../ -I../../ -I../../protob -I../../rpc -I../../rpc/protob -I../../../lsh -I../../../proxy \
	    -I../protob -I../../../utils `pkg-config --cflags protobuf`
AM_CXXFLAGS=-Wall -g
libseeksdhtsgrpc_la_SOURCES=l2_protob_rpc_server.cpp l2_protob_rpc_client.cpp