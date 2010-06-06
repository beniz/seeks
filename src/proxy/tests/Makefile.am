noinst_PROGRAMS=test_curl_mget shash
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I../ -I../../utils -I../../lsh @CURL_CFLAGS@

if USE_PERL
AM_CPPFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

AM_LDFLAGS=-L. -L../ -L../../utils -L../../lsh
if HAVE_DHT
AM_LDFLAGS += -L../../dht -L../../dht/rpc -L../../dht/rpc/protob -L../../dht/protob \
	      `pkg-config --libs protobuf`
endif
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = @CURL_LDADD@ -lseeksutils 
if HAVE_DHT
LDADD += -lseeksdht -lseeksrpc -lseeksrpcprotob -lseeksdhtprotob -lrmd160
endif
LDADD +=  -lseekslsh -lseeksproxy -lseeksplugins -lpcre -lpcreposix -lpthread -lz
if !HAVE_FREEBSD
LDADD += -ldl
endif