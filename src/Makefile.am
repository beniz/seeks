

publicdir=$(datadir)/seeks
proxydir=$(datadir)/seeks


bin_PROGRAMS=seeks
seeks_SOURCES=seeks.cpp

AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -Iutils -Ilsh -Iproxy @PCRE_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' \
	    -Iplugins/websearch `pkg-config opencv --cflags`
AM_LDFLAGS=`pkg-config opencv --libs` -Lutils/ -Llsh/ -Lproxy/ @PCRE_LDADD@ @CURL_LDADD@ @XML2_LDADD@ -Lplugins/websearch

if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=-lseekswebsearchplugin
if HAVE_HTTPSERV_PLUGIN
AM_LDFLAGS+=-Lplugins/img_websearch
LDADD += -levent -lseeksimgwebsearchplugin
endif
LDADD += -lseeksutils -lseeksproxy -lseeksplugins -lpcre -lpcreposix -lseekslsh -lpthread -lz @CURL_LDADD@ @XML2_LDADD@ 
if !HAVE_FREEBSD
LDADD += -ldl
endif

SUBDIRS=utils lsh proxy plugins .

configdir=$(sysconfdir)/seeks
config_DATA=config \
		 lsh/lsh-config

nobase_public_DATA=public/css/* \
		 public/images/*.png \
		 public/images/*.ico \
		 public/yui/3.0.0/cssreset/reset-min.css \
		 public/yui/3.0.0/cssfonts/fonts-min.css \
		 public/yui/3.0.0/cssgrids/grids-min.css \
		 public/yui/3.0.0/yui/yui-min.js \
		 lsh/swl/*.txt

nobase_proxy_DATA=proxy/templates/*
