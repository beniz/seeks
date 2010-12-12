noinst_PROGRAMS=test_curl_mget shash
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I${srcdir} -I${srcdir}/../ -I${srcdir}/../../utils -I${srcdir}/../../lsh @CURL_CFLAGS@

if USE_PERL
AM_CPPFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

AM_LDFLAGS=-L. -L../ -L../../utils -L../../lsh
if HAVE_DHT
AM_LDFLAGS += -L../../dht -L../../dht/rpc -L../../dht/rpc/protob -L../../dht/protob \
	      -L../../proxy `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
endif
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = @CURL_LDADD@ -lseeksutils
if HAVE_DHT
LDADD += -lseeksdht -lseeksrpc -lseeksrpcprotob -lseeksdhtprotob -lrmd160 -levent -lseeksproxy
endif
LDADD +=  -lseekslsh -lseeksproxy -lseeksplugins -lpcre -lpcreposix -lpthread -lz
if !HAVE_BSD
LDADD += -ldl
endif