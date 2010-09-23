#automake for uri_capture plugin.

uricapturepluginlibdir = $(libdir)/seeks/plugins/uri_capture
uricaptureplugindatadir = $(datadir)/seeks/plugins/uri_capture

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS= -I${srcdir} -I${srcdir}/../../utils -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
ACLOCAL_AMFLAGS=-I m4

uricapturepluginlib_LTLIBRARIES=liburicaptureplugin.la
liburicaptureplugin_la_SOURCES=uri_capture.cpp db_uri_record_msg.pb.cc db_uri_record.cc uri_capture.h

db_uri_record_msg.pb.cc:
	protoc --proto_path=. --proto_path=../../proxy --cpp_out=. db_uri_record_msg.proto

clean-local:
	rm -f db_uri_record_msg.pb.cc db_uri_record_msg.pb.h

nobase_dist_uricaptureplugindata_DATA=uri-patterns
