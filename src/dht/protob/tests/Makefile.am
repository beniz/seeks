bin_PROGRAMS=test_serialize
test_serialize_SOURCES=test-serialize.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I${srcdir}/.. -I.. -I${srcdir}/../../ -I${srcdir}/../../rpc -I${srcdir}/../../rpc/protob -I../../rpc/protob \
	    -I${srcdir}/../../../utils -I${srcdir}/../../../proxy `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
AM_LDFLAGS=-L../ -L../../ -L../../rpc -L../../rpc/protob  \
           -L../../../utils -L../../../proxy -L../../../lsh `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`

LDADD = -lseeksdht -lrmd160 -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob  \
        -lrmd160 -levent -lseeksutils -lseeksproxy -lseeksplugins \
	-lseekslsh -lpcre -lpcreposix -lprotobuf -lpthread -lz @CURL_LDADD@ @XML2_LDADD@
