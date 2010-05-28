lib_LTLIBRARIES=libseeksdhtsgprotob.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe\
            -I../ -I../../ -I../../protob -I../../../utils -I../../../proxy -I../../../lsh \
	    `pkg-config --cflags protobuf`
libseeksdhtsgprotob_la_SOURCES=dht_layer2.pb.cc l2_protob_wrapper.cpp

dht_layer2.pb.cc:
	protoc --proto_path=. --proto_path=../../protob --cpp_out=. dht_layer2.proto

clean-local:
	rm dht_layer2.pb.h dht_layer2.pb.cc