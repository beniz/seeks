#automake for dht

lib_LTLIBRARIES=libseeksdhtsg.la
AM_CPPFLAGS=-Irpc -Iprotob -I../ -Irpc -I../protob -I../../utils -I../../lsh \
	    -I../../proxy -I../protob -I../rpc -I../rpc/protob \
	    `pkg-config --cflags protobuf`
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdhtsg_la_SOURCES=subscriber.cpp searchgroup.cpp sg_configuration.cpp sg_manager.cpp SGNode.cpp \
		         sg_api.cpp sg_sweeper.cpp

SUBDIRS=protob . rpc #protob/tests rpc/tests tests
