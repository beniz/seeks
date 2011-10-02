#automake for xsl_serializer plugin.

xslserializerpluginlibdir = $(libdir)/seeks/plugins/xsl_serializer
xslserializerplugindatadir = $(datadir)/seeks/plugins/xsl_serializer

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
	    -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy \
	    -I${srcdir}/../../dht -I${srcdir}/../websearch

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

xslserializerpluginlib_LTLIBRARIES=libseeksxslserializerplugin.la
libseeksxslserializerplugin_la_SOURCES=xml_renderer.cpp xsl_serializer.cpp xsl_serializer_configuration.cpp \
				       xml_renderer.h   xsl_serializer.h xsl_serializer_configuration.h 

libseeksxslserializerplugin_la_CXXFLAGS = -Wall -g -I${srcdir}/../../ @PCRE_CFLAGS@ @CURL_CFLAGS@ @XML2_CFLAGS@ @XSLT_CFLAGS@ @LCOV_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

if USE_PERL
libseeksxslserializerplugin_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
endif

SUBDIRS=. tests

xslserializerconfigdir=$(sysconfdir)/seeks
dist_xslserializerconfig_DATA=xslserializer-config

nobase_dist_xslserializerplugindata_DATA= \
			  xml/stylesheets/opensearch-result.xsl
