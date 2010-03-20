bin_PROGRAMS=seeks
seeks_SOURCES=seeks.cpp

AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -Iutils -Ilsh -Iproxy @PCRE_CFLAGS@
AM_LDFLAGS=-Lutils/ -Llsh/ -Lproxy/ @PCRE_LDADD@ @CURL_LDADD@ @XML2_LDADD@

if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD = -lseeksutils -lseeksproxy -lseeksplugins -lpcre -lpcreposix -lseekslsh -lpthread -lz @CURL_LDADD@ @XML2_LDADD@
if !HAVE_FREEBSD
LDADD += -ldl
endif

SUBDIRS=utils lsh proxy plugins .

nobase_data_DATA=config \
	         public/css/* \
		 public/images/*.png \
		 public/images/*.ico \
		 public/yui/3.0.0/cssreset/reset-min.css \
		 public/yui/3.0.0/cssfonts/fonts-min.css \
		 public/yui/3.0.0/cssgrids/grids-min.css \
		 public/yui/3.0.0/yui/yui-min.js \
		 lsh/lsh-config \
		 proxy/templates/*
