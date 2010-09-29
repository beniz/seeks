# automake for httpserv plugin.

httpservpluginlibdir = $(libdir)/seeks/plugins/httpserv
httpservplugindatadir = $(datadir)/seeks/plugins/httpserv

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I${srcdir} -I${srcdir}/../websearch -I${srcdir}/../../utils -I${srcdir}/../../lsh -I${srcdir}/../../proxy \
	    -I${srcdir}/../../dht -I${srcdir}/../query_capture `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
if HAVE_IMG_WEBSEARCH_PLUGIN
AM_CPPFLAGS+=-I${srcdir}/../img_websearch
endif
ACLOCAL_AMFLAGS=-I m4
httpservpluginlib_LTLIBRARIES=libseekshttpservplugin.la
libseekshttpservplugin_la_SOURCES=httpserv_configuration.cpp httpserv.cpp httpserv.h httpserv_configuration.h
libseekshttpservplugin_la_CXXFLAGS=-Wall -g -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

httpservconfigdir=$(sysconfdir)/seeks
dist_httpservconfig_DATA=httpserv-config
