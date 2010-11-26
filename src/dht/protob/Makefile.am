lib_LTLIBRARIES=libseeksdhtprotob.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe\
	    -I${srcdir}/../../utils -I${srcdir}/../../dht -I${srcdir}/../../proxy -I${srcdir}/../../lsh \
	    `pkg-config --cflags protobuf`
nodist_libseeksdhtprotob_la_SOURCES=$(protoc_outputs)
dist_libseeksdhtprotob_la_SOURCES=l1_protob_wrapper.cpp l1_protob_wrapper.h l1_data_protob_wrapper.cpp l1_data_protob_wrapper.h

EXTRA_DIST=dht_layer1_data.proto dht_layer1.proto

protoc_inputs=dht_layer1.proto dht_layer1_data.proto

protoc_outputs=dht_layer1.pb.cc dht_layer1.pb.h dht_layer1_data.pb.cc dht_layer1_data.pb.h 

BUILT_SOURCES = $(protoc_outputs)

$(protoc_outputs): proto_middleman

proto_middleman: protoc $(protoc_inputs)
	touch proto_middleman
	$^ --cpp_out=. --proto_path=${srcdir}

protoc:
	touch protoc

clean-local:
	rm -f dht_layer1.pb.h dht_layer1.pb.cc dht_layer1_data.pb.h dht_layer1_data.pb.cc protoc proto_middleman

