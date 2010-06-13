

publicdir=$(datadir)/seeks
proxydir=$(datadir)/seeks


bin_PROGRAMS=seeks
seeks_SOURCES=seeks.cpp

AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -Iutils -Ilsh -Iproxy 
if HAVE_DHT
AM_CPPFLAGS += -Idht -Idht/protob -Idht/rpc -Idht/rpc/protob -Idht/sg -Idht/sg/protob -Idht/sg/rpc
endif
AM_CPPFLAGS += @PCRE_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' \
	    -DSEEKS_LOCALSTATEDIR='"$(localstatedir)/seeks"'
AM_LDFLAGS=-Lutils/ -Llsh/ -Lproxy/ @PCRE_LDADD@ @CURL_LDADD@ @XML2_LDADD@

if HAVE_HTTPSERV_PLUGIN
AM_CPPFLAGS+=-Iplugins/websearch
AM_LDFLAGS+=-Lplugins/websearch
endif

if HAVE_DHT
 AM_LDFLAGS += -Ldht -Ldht/rpc -Ldht/protob -Ldht/rpc/protob -Ldht/sg -Ldht/sg/rpc -Ldht/sg/protob `pkg-config --libs protobuf`
endif
if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=
if HAVE_HTTPSERV_PLUGIN
LDADD += -lseekswebsearchplugin -levent
endif
LDADD += -lseeksutils 
if HAVE_DHT
LDADD += -lseeksdht -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob -lseeksdhtsg -lseeksdhtsgrpc -lseeksdhtsgprotob -lrmd160 -levent
endif
LDADD += -lseeksproxy -lseeksplugins -lpcre -lpcreposix -lseekslsh -lpthread -lz @CURL_LDADD@ @XML2_LDADD@

if !HAVE_FREEBSD
LDADD += -ldl
endif

SUBDIRS=utils 
if HAVE_DHT
SUBDIRS += dht 
endif
SUBDIRS += lsh proxy plugins .

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
