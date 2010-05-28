#automake for dht

lib_LTLIBRARIES=libseeksdht.la
AM_CPPFLAGS=-Iprotob -I../utils -I../lsh -I../proxy -Iprotob -Irpc -Irpc/protob \
	     `pkg-config --cflags protobuf`
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdht_la_SOURCES=NetAddress.cpp DHTKey.cpp dht_configuration.cpp FingerTable.cpp \
		       DHTVirtualNode.cpp Location.cpp LocationTable.cpp DHTNode.cpp Stabilizer.cpp \
		       BstTimeCb.cpp SuccList.cpp RouteIterator.cpp

SUBDIRS=protob rpc . sg protob/tests rpc/tests tests
