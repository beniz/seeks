lib_LTLIBRARIES=libseeksdhtprotob.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -march=native -mtune=native -pipe\
	    -I../../utils -I../ -I../../proxy
libseeksdhtprotob_la_SOURCES=dht_layer1.pb.cc l1_protob_wrapper.cpp

dht_layer1.pb.cc:
	protoc --cpp_out=. dht_layer1.proto

