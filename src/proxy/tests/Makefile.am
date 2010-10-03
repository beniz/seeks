noinst_PROGRAMS=test_curl_mget shash 
if HAVE_PROTOBUF
if HAVE_TC
noinst_PROGRAMS += user_db_print user_db_clear test_user_db
endif
endif
test_curl_mget_SOURCES=test-curl-mget.cpp 
shash_SOURCES=shash.cpp
if HAVE_PROTOBUF
if HAVE_TC
user_db_print_SOURCES=user-db-print.cpp
user_db_clear_SOURCES=user-db-clear.cpp
test_user_db_SOURCES=test-user-db.cpp
endif
endif

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
	    -I${srcdir} -I${srcdir}/../ -I${srcdir}/../../utils -I${srcdir}/../../lsh @PCRE_CFLAGS@ @CURL_CFLAGS@ \
	    -I${srcdir}/../../dht \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config opencv --cflags`
if HAVE_PROTOBUF
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
endif
endif

if USE_PERL
AM_CPPFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

AM_LDFLAGS=-L. -L../ -L../../utils -L../../lsh -L../../dht `pkg-config opencv --libs` @XML2_LDADD@
if HAVE_PROTOBUF
if HAVE_TC
AM_LDFLAGS += `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
endif
endif
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = -lseeksutils -lseekslsh -lseeksproxy -lseeksplugins 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lseeksuserdb
endif
endif
LDADD += -lseeksdht -lrmd160 -lpcre -lpcreposix -lpthread -lz @CURL_LDADD@
if !HAVE_BSD
LDADD += -ldl
endif
