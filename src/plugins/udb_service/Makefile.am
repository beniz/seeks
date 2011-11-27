#automake for udb_service plugin.

udbservicepluginlibdir = $(libdir)/seeks/plugins/udb_service
udbserviceplugindatadir = $(datadir)/seeks/plugins/udb_service

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS=-I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh -I${srcdir}/../../dht \
            -I${srcdir}/../websearch -I${srcdir}/../query_capture -I${srcdir}/../uri_capture -I${srcdir}/../cf \
            -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
            @PCRE_CFLAGS@ -I../../proxy/

ACLOCAL_AMFLAGS=-I m4

if HAVE_PROTOBUF
protoc_inputs=halo_msg.proto
protoc_outputs=halo_msg.pb.h halo_msg.pb.cc

$(protoc_outputs): $(protoc_inputs)
	protoc -I$(srcdir) --cpp_out=. $<
nodist_libudbserviceplugin_la_SOURCES=$(protoc_outputs)
endif

udbservicepluginlib_LTLIBRARIES=libudbserviceplugin.la
libudbserviceplugin_la_SOURCES=udb_service.cpp udb_server.cpp udb_client.cpp \
			       halo_msg_wrapper.cpp udb_service_configuration.cpp \
	                       udb_service.h udb_server.h udb_client.h \
	                       halo_msg_wrapper.h udbs_err.h udb_service_configuration.h

libudbserviceplugin_la_LIBADD = -L../../utils -lrmd160
if HAVE_PROTOBUF
if HAVE_TC
libudbserviceplugin_la_LIBADD += -L../uri_capture -luricaptureplugin -L../cf -lcfplugin -L../websearch -lseekswebsearchplugin @ICU_LDADD@
endif
endif

udbservicepluginconfigdir = $(sysconfdir)/seeks
dist_udbservicepluginconfig_DATA=udb-service-config

BUILT_SOURCES = $(protoc_outputs)

clean-local:
	rm -f $(protoc_outputs)

EXTRA_DIST = $(protoc_inputs)

#SUBDIRS=. tests