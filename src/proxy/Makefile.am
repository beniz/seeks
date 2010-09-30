

ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES=
if HAVE_TC
if HAVE_PROTOBUF
lib_LTLIBRARIES += libseeksuserdb.la 
endif
endif
lib_LTLIBRARIES += libseeksproxy.la libseeksplugins.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/../utils -I${srcdir}/../lsh 
if HAVE_TC
if HAVE_PROTOBUF
AM_CPPFLAGS += `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
endif
endif
libseeksproxy_la_SOURCES=seeks_proxy.cpp proxy_dts.cpp errlog.cpp \
                        cgi.cpp encode.cpp spsockets.cpp filters.cpp gateway.cpp\
                        parsers.cpp pcrs.cpp cgisimple.cpp loaders.cpp \
                        urlmatch.cpp sweeper.cpp \
                        configuration_spec.cpp proxy_configuration.cpp iso639.cpp
libseeksplugins_la_SOURCES=plugin_manager.cpp plugin.cpp plugin_element.cpp \
                           interceptor_plugin.cpp filter_plugin.cpp pcrs_std_filter.cpp \
                           curl_mget.cpp

if HAVE_TC
if HAVE_PROTOBUF
libseeksuserdb_la_SOURCES=db_record_msg.pb.cc db_record.cpp user_db.cpp
endif
endif

noinst_HEADERS = \
	action_plugin.h \
	cgi.h \
	cgisimple.h \
	configuration_spec.h \
	curl_mget.h \
	encode.h \
	errlog.h \
	filter_plugin.h \
	filters.h \
	gateway.h \
	interceptor_plugin.h \
	iso639.h \
	loaders.h \
	parsers.h \
	pcrs.h \
	pcrs_std_filter.h \
	perl_std_filter.h \
	plugin_element.h \
	plugin.h \
	plugin_manager.h \
	proxy_configuration.h \
	proxy_dts.h \
	seeks_proxy.h \
	sp_err.h \
	spsockets.h \
	sweeper.h \
	urlmatch.h 

libseeksproxy_la_CXXFLAGS=-Wall -g @CURL_CFLAGS@ @PCRE_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' -DSEEKS_DATADIR='"$(datadir)/seeks/"' -DSEEKS_PLUGINS_LIBDIR='"$(libdir)/seeks/plugins/"'
libseeksplugins_la_CXXFLAGS=-Wall -g @CURL_CFLAGS@ @PCRE_CFLAGS@
if HAVE_TC
if HAVE_PROTOBUF
libseeksuserdb_la_CXXFLAGS=-Wall -g
endif
endif
if USE_PERL
libseeksplugins_la_SOURCES += perl_std_filter.cpp xsinit.cpp
libseeksplugins_la_CXXFLAGS +=`perl -MExtUtils::Embed -e ccopts`
libseeksproxy_la_CXXFLAGS += `perl -MExtUtils::Embed -e ccopts`
xsinit.cpp:
        perl -MExtUtils::Embed -e xsinit -- -o xsinit.cpp
endif

db_record_msg.pb.cc:
	protoc --cpp_out=. db_record_msg.proto

# SUBDIRS= . tests

EXTRA_DIST = templates/blocked \
	    templates/cgi-error-404 \
	    templates/cgi-error-bad-param \
	    templates/cgi-error-disabled \
	    templates/cgi-error-file \
	    templates/cgi-error-file-read-only \
	    templates/cgi-error-modified \
	    templates/cgi-error-parse \
	    templates/cgi-style.css \
	    templates/connect-failed \
	    templates/connection-timeout \
	    templates/default \
	    templates/forwarding-failed \
	    templates/info-head \
	    templates/mod-support-and-service \
	    templates/mod-title \
	    templates/mod-unstable-warning \
	    templates/no-server-data \
	    templates/no-such-domain \
	    templates/show-request \
	    templates/show-status \
	    templates/show-status-file \
	    templates/show-status-plugin \
	    templates/show-url-info \
	    templates/show-version \
	    templates/toggle \
	    templates/toggle-mini \
	    templates/url-info-osd.xml
