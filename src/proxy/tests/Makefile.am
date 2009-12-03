bin_PROGRAMS=test_curl_mget shash
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -march=native -mtune=native -pipe \
	    -I../ -I../../utils @CURL_CFLAGS@

if USE_PERL
AM_CPPFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

AM_LDFLAGS=-L. -L../
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = @CURL_LDADD@ -lseeksproxy -lseeksplugins -lnsl -lpcre -lpcreposix -lpthread -lz -ldl
