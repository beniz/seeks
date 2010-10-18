lib_LTLIBRARIES=libseeksdhtsgprotob.la
AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe\
            -I${srcdir}/../ -I${srcdir}/../../ -I${srcdir}/../../protob -I../../protob -I${srcdir}/../../../utils -I${srcdir}/../../../proxy -I${srcdir}/../../../lsh \
	    `pkg-config --cflags protobuf`

nodist_libseeksdhtsgprotob_la_SOURCES=$(protoc_outputs)
dist_libseeksdhtsgprotob_la_SOURCES=l2_protob_wrapper.cpp l2_protob_wrapper.h l2_data_protob_wrapper.cpp l2_data_protob_wrapper.h

EXTRA_DIST=dht_layer2_data.proto dht_layer2.proto

protoc_inputs=dht_layer2.proto dht_layer2_data.proto

protoc_outputs=dht_layer2.pb.cc dht_layer2.pb.h dht_layer2_data.pb.cc dht_layer2_data.pb.h 
BUILT_SOURCES = $(protoc_outputs)

$(protoc_outputs): proto_middleman

proto_middleman: protoc $(protoc_inputs)
	$^ --cpp_out=. --proto_path=${srcdir} --proto_path=${srcdir}/../../protob

clean-local:
	rm -f dht_layer2.pb.h dht_layer2.pb.cc dht_layer2_data.pb.h dht_layer2_data.pb.cc proto_middleman

.PHONY: protoc
