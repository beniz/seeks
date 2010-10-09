#automake for query_capture plugin.

querycapturepluginlibdir = $(libdir)/seeks/plugins/query_capture
querycaptureplugindatadir = $(datadir)/seeks/plugins/query_capture

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS= -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh -I${srcdir}/../../dht \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` \
            @PCRE_CFLAGS@
ACLOCAL_AMFLAGS=-I m4

protoc_inputs=db_query_record_msg.proto
protoc_outputs=db_query_record_msg.pb.cc db_query_record_msg.pb.h
$(protoc_outputs): $(protoc_inputs)
	protoc --proto_path=. --proto_path=../../proxy --cpp_out=. $<

querycapturepluginlib_LTLIBRARIES=libquerycaptureplugin.la
dist_libquerycaptureplugin_la_SOURCES=query_capture.cpp db_query_record.cpp \
	                         query_capture_configuration.cpp query_capture.h \
				 db_query_record.h query_capture_configuration.h
nodist_libquerycaptureplugin_la_SOURCES=$(protoc_outputs)

BUILT_SOURCES = $(protoc_outputs)

clean-local:
	rm -f $(protoc_outputs)

#nobase_dist_querycaptureplugindata_DATA=query-patterns

EXTRA_DIST = \
	$(protoc_inputs)


