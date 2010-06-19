bin_PROGRAMS=test_serialize
test_serialize_SOURCES=test-serialize.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I../ -I../../ -I../../rpc -I../../rpc/protob -I../../sg -I../../sg/rpc -I../../sg/protob \
	    -I../../../utils -I../../../proxy `pkg-config --cflags protobuf`
AM_LDFLAGS=-L../ -L../../ -L../../rpc -L../../rpc/protob -L../../sg -L../../sg/rpc -L../../sg/protob \
           -L../../../utils -L../../../proxy -L../../../lsh `pkg-config --libs protobuf`

LDADD = -lseeksdht -lrmd160 -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob -lseeksdhtsg -lseeksdhtsgrpc \
        -lseeksdhtsgprotob -lrmd160 -levent -lseeksutils -lseeksproxy -lseeksplugins \
	-lseekslsh -lpcre -lpcreposix -lprotobuf -lpthread -lz @CURL_LDADD@ @XML2_LDADD@
