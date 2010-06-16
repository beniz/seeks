# automake for test rpc

bin_PROGRAMS=test_rpc_server test_rpc_client
test_rpc_server_SOURCES=test-rpc-server.cpp
test_rpc_client_SOURCES=test-rpc-client.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
            -I../ -I../../ -I../../protob -I../protob -I../../../utils -I../../../proxy \
	    -I../../sg -I../../sg/rpc -I../../sg/protob `pkg-config --cflags protobuf`
AM_LDFLAGS=-L../ -L../../ -L../../protob -L../protob -L../../../utils \
	   -L../../../proxy -L../../../lsh -L../../sg -L../../sg/rpc -L../../sg/protob `pkg-config --libs protobuf`

LDADD = -lseeksrpc -lseeksdht -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob -lrmd160 \
	-lseeksutils -lseeksproxy -lseeksplugins -lseekslsh \
	-lseeksdht -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob -lseeksdhtsg -lseeksdhtsgrpc -lseeksdhtsgprotob -lrmd160 -levent \
        -lpcre -lpcreposix -lprotobuf -lpthread -lz @CURL_LDADD@ @XML2_LDADD@
