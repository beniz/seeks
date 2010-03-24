

ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES=libseeksproxy.la libseeksplugins.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I../utils -I../lsh
libseeksproxy_la_SOURCES=seeks_proxy.cpp proxy_dts.cpp errlog.cpp \
                        cgi.cpp encode.cpp spsockets.cpp filters.cpp gateway.cpp\
                        parsers.cpp pcrs.cpp cgisimple.cpp loaders.cpp \
                        urlmatch.cpp sweeper.cpp \
                        configuration_spec.cpp proxy_configuration.cpp iso639.cpp
libseeksplugins_la_SOURCES=plugin_manager.cpp plugin.cpp plugin_element.cpp \
                           interceptor_plugin.cpp filter_plugin.cpp pcrs_std_filter.cpp \
                           curl_mget.cpp

libseeksproxy_la_CXXFLAGS=-Wall -g @CURL_CFLAGS@ @PCRE_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' -DSEEKS_DATADIR='"$(datadir)/seeks/"' -DSEEKS_PLUGINS_LIBDIR='"$(libdir)/seeks/plugins/"'
libseeksplugins_la_CXXFLAGS=-Wall -g @CURL_CFLAGS@ @PCRE_CFLAGS@
if USE_PERL
libseeksplugins_la_SOURCES += perl_std_filter.cpp xsinit.cpp
libseeksplugins_la_CXXFLAGS +=`perl -MExtUtils::Embed -e ccopts`
libseeksproxy_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
xsinit.cpp:
        perl -MExtUtils::Embed -e xsinit -- -o xsinit.cpp
endif

SUBDIRS= . tests
