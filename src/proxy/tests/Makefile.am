noinst_PROGRAMS=test_curl_mget shash
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I../ -I../../utils -I../../lsh @CURL_CFLAGS@

if USE_PERL
AM_CPPFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

AM_LDFLAGS=-L. -L../ -L../../utils -L../../lsh
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = @CURL_LDADD@ -lseeksutils -lseekslsh -lseeksproxy -lseeksplugins -lpcre -lpcreposix -lpthread -lz 
if !HAVE_FREEBSD
LDADD += -ldl
endif