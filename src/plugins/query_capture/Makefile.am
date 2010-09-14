#automake for query_capture plugin.

querycapturepluginlibdir = $(libdir)/seeks/plugins/query_capture
querycaptureplugindatadir = $(datadir)/seeks/plugins/query_capture

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS= -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
ACLOCAL_AMFLAGS=-I m4

querycapturepluginlib_LTLIBRARIES=libquerycaptureplugin.la
libquerycaptureplugin_la_SOURCES=query_capture.cpp db_query_record_msg.pb.cc db_query_record.cc query_capture.h

db_query_record_msg.pb.cc:
	protoc --proto_path=. --proto_path=../../proxy --cpp_out=. db_query_record_msg.proto

clean-local:
	rm db_query_record_msg.pb.cc db_query_record_msg.pb.h

#nobase_dist_querycaptureplugindata_DATA=query-patterns
