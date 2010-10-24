#automake for dht

lib_LTLIBRARIES=libseeksdht.la
AM_CPPFLAGS=-I${srcdir}/protob -Iprotob -I${srcdir}/../utils -I${srcdir}/../lsh -I${srcdir}/../proxy  -I${srcdir}/rpc -I${srcdir}/rpc/protob -Irpc/protob \
	     `pkg-config --cflags protobuf` @LCOV_CFLAGS@
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdht_la_SOURCES=\
	NetAddress.cpp DHTKey.cpp dht_configuration.cpp FingerTable.cpp \
	DHTVirtualNode.cpp Location.cpp LocationTable.cpp DHTNode.cpp Stabilizer.cpp \
	BstTimeCb.cpp SuccList.cpp RouteIterator.cpp dht_api.cpp \
	Bst.h \
	BstTimeCb.h \
	callback.h \
	dht_api.h \
	dht_configuration.h \
	dht_err.h \
	dht_exception.h \
	DHTKey.h \
	DHTNode.h \
	DHTVirtualNode.h \
	FingerTable.h \
	Location.h \
	LocationTable.h \
	NetAddress.h \
	RouteIterator.h \
	Stabilizer.h \
	SuccList.h

SUBDIRS=protob rpc sg #protob/tests rpc/tests tests
