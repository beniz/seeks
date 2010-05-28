lib_LTLIBRARIES=libseeksdhtprotob.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe\
	    -I../../utils -I../ -I../../proxy -I../../lsh \
	    `pkg-config --cflags protobuf`
libseeksdhtprotob_la_SOURCES=dht_layer1.pb.cc dht_layer1_data.pb.cc \
		             l1_protob_wrapper.cpp l1_data_protob_wrapper.cpp

dht_layer1.pb.cc:
	protoc --cpp_out=. dht_layer1.proto

dht_layer1_data.pb.cc:
	protoc --cpp_out=. dht_layer1_data.proto

clean-local:
	rm dht_layer1.pb.h dht_layer1.pb.cc dht_layer1_data.pb.h dht_layer1_data.pb.cc