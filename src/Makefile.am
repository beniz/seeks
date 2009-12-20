bin_PROGRAMS=seeks
seeks_SOURCES=seeks.cpp

AM_CPPFLAGS=-Wall -Wno-deprecated -g -march=native -mtune=native -pipe \
	    -Iutils -Ilsh -Iproxy @PCRE_CFLAGS@
AM_LDFLAGS=-Lutils/ -Llsh/ -Lproxy/ @PCRE_LDADD@ @CURL_LDADD@ @XML2_LDADD@

if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = -lseeksutils -lseeksproxy -lseeksplugins -lnsl -lpcre -lpcreposix -lseekslsh -lpthread -lz -ldl @CURL_LDADD@ @XML2_LDADD@

SUBDIRS=utils lsh proxy plugins .
# dht
