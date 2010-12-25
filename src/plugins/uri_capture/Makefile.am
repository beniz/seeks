#automake for uri_capture plugin.

uricapturepluginlibdir = $(libdir)/seeks/plugins/uri_capture
uricaptureplugindatadir = $(datadir)/seeks/plugins/uri_capture

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS= -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
	    -I../../proxy/

ACLOCAL_AMFLAGS=-I m4

protoc_inputs = db_uri_record_msg.proto
protoc_outputs = db_uri_record_msg.pb.cc db_uri_record_msg.pb.h
$(protoc_outputs): $(protoc_inputs)
	protoc -I$(srcdir) -I$(srcdir)/../../proxy/ --cpp_out=. $<

uricapturepluginlib_LTLIBRARIES=liburicaptureplugin.la
liburicaptureplugin_la_SOURCES=uri_capture.cpp db_uri_record.cpp uri_capture.h \
	                       uc_configuration.cpp uc_configuration.h db_uri_record.h
nodist_liburicaptureplugin_la_SOURCES=$(protoc_outputs)

BUILT_SOURCES = $(protoc_outputs)

clean-local:
	rm -f $(protoc_outputs)

uricapturepluginconfigdir=$(sysconfdir)/seeks
dist_uricapturepluginconfig_DATA=uri-capture-config
nobase_dist_uricaptureplugindata_DATA=uri-patterns

EXTRA_DIST = \
	$(protoc_inputs)

SUBDIRS = . tests