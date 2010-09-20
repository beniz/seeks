noinst_PROGRAMS=test_curl_mget shash user_db_print user_db_clear
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp
user_db_print_SOURCES=user-db-print.cpp
user_db_clear_SOURCES=user-db-clear.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I${srcdir} -I${srcdir}/../ -I${srcdir}/../../utils -I${srcdir}/../../lsh @PCRE_CFLAGS@ @CURL_CFLAGS@ \
	    -I${srcdir}/../../plugins/websearch -I${srcdir}/../../dht \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
	    `pkg-config opencv --cflags`

if USE_PERL
AM_CPPFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

AM_LDFLAGS=-L. -L../ -L../../utils -L../../lsh -L../../dht `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet` \
           `pkg-config opencv --libs` -L../../plugins/websearch -L../../plugins/img_websearch @XML2_LDADD@
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = -lseekswebsearchplugin -lseeksimgwebsearchplugin -lseeksutils -lseekslsh -lseeksproxy -lseeksplugins -lseeksuserdb \
        -lseeksdht -lrmd160 -lpcre -lpcreposix -lpthread -lz @CURL_LDADD@
if !HAVE_BSD
LDADD += -ldl
endif
