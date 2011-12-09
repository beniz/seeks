#automake for cf plugin.

cfpluginlibdir = $(libdir)/seeks/plugins/cf
cfplugindatadir = $(datadir)/seeks/plugins/cf

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS=-I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh -I${srcdir}/../../dht \
            -I${srcdir}/../websearch -I${srcdir}/../query_capture -I${srcdir}/../uri_capture -I${srcdir}/../udb_service \
            -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
            @PCRE_CFLAGS@ -I../../proxy/

if HAVE_IMG_WEBSEARCH_PLUGIN
AM_CPPFLAGS += -I${srcdir}/../img_websearch
endif
if HAVE_OPENCV2
AM_CPPFLAGS += `pkg-config opencv --cflags`
endif

ACLOCAL_AMFLAGS=-I m4

if HAVE_XSLSERIALIZER_PLUGIN
AM_CPPFLAGS += -I${srcdir}/../xsl_serializer @XML2_CFLAGS@
endif


cfpluginlib_LTLIBRARIES=libcfplugin.la
libcfplugin_la_SOURCES=cf.cpp rank_estimators.cpp query_recommender.cpp cf_configuration.cpp cr_store.cpp peer_list.cpp \
                       cf.h rank_estimators.h query_recommender.h cf_configuration.h cr_store.h peer_list.h

cfpluginconfigdir = $(sysconfdir)/seeks
dist_cfpluginconfig_DATA=cf-config


SUBDIRS=. tests