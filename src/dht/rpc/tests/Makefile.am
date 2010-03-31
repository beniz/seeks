# automake for test rpc

bin_PROGRAMS=test_rpc_server test_rpc_client
test_rpc_server_SOURCES=test-rpc-server.cpp
test_rpc_client_SOURCES=test-rpc-client.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
            -I../ -I../../ -I../../protob -I../protob -I../../../utils -I../../../proxy
AM_LDFLAGS=-L../ -L../../ -L../../protob -L../protob -L../../../utils \
	   -L../../../proxy -L../../../lsh

LDADD = -lseeksrpc -lseeksdht -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob -lrmd160 \
	-lseeksutils -lseeksproxy -lseeksplugins \
        -lseekslsh -lpcre -lpcreposix -lprotobuf -lpthread -lz @CURL_LDADD@ @XML2_LDADD@
