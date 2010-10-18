#automake for dht

lib_LTLIBRARIES=libseeksdhtsg.la
AM_CPPFLAGS=-I${srcdir}/rpc -I${srcdir}/protob -Iprotob -I${srcdir}/.. -I${srcdir}/rpc -I${srcdir}/../protob -I${srcdir}/../../utils -I${srcdir}/../../lsh \
	    -I${srcdir}/../../proxy -I${srcdir}/../protob -I../protob -I${srcdir}/../rpc -I${srcdir}/../rpc/protob \
	    `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdhtsg_la_SOURCES=subscriber.cpp searchgroup.cpp sg_configuration.cpp sg_manager.cpp SGNode.cpp SGVirtualNode.cpp sg_api.cpp sg_sweeper.cpp sg_db.cpp searchgroup.h sg_api.h sg_configuration.h sg_db.h sg_manager.h SGNode.h SGVirtualNode.h sg_sweeper.h subscriber.h

SUBDIRS=protob rpc #protob/tests rpc/tests tests
