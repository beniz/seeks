# automake for websearch_api_compat

websearchapicompatpluginlibdir = $(libdir)/seeks/plugins/websearch_api_compat
websearchapicompatplugindatadir = $(datadir)/seeks/plugins/websearch_api_compat

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe -I${srcdir}/../websearch \
	    -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy \
	    -I${srcdir}/../../dht 
if HAVE_IMG_WEBSEARCH_PLUGIN
AM_CPPFLAGS += -I${srcdir}/../img_websearch
endif
if HAVE_PROTOBUF
AM_CPPFLAGS += -I${srcdir}/../query_capture -I${srcdir}/../cf -I../../proxy \
               `pkg-config --cflags protobuf`
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags tokyocabinet`
endif

if HAVE_TT
AM_CPPFLAGS += `pkg-config --cflags tokyotyrant`
endif
endif

ACLOCAL_AMFLAGS=-I m4
websearchapicompatpluginlib_LTLIBRARIES=libseekswebsearchapicompatplugin.la
libseekswebsearchapicompatplugin_la_SOURCES=websearch_api_compat.cpp websearch_api_compat.h

libseekswebsearchapicompatplugin_la_CXXFLAGS = -Wall -g -I${srcdir}/../../ @PCRE_CFLAGS@ @CURL_CFLAGS@ @XML2_CFLAGS@ @LCOV_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'