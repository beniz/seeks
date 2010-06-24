#automake file for seeks dht tests
bin_PROGRAMS=test_hash_key test_keys test_dhtnode test_sgnode grab_mac dhtkey_gen dht_cli # test_dht_net_utils test_stabilizer

test_hash_key_SOURCES=test-hash-key.cpp
test_keys_SOURCES=test-keys.cpp 
test_dhtnode_SOURCES=test-dhtnode.cpp
test_sgnode_SOURCES=test-sgnode.cpp
#test_dht_net_utils_SOURCES=test-dht-net-utils.cpp
#test_stabilizer_SOURCES=test-stabilizer.cpp
grab_mac_SOURCES=grab-mac.cc
dhtkey_gen_SOURCES=dhtkey-gen.cc
dht_cli_SOURCES=dht-cli.cc

AM_CPPFLAGS=-I../ -I../../utils -I../../lsh -I../../proxy -I../../utils \
	    -I../rpc -I../rpc/protob -I../protob -I../sg -I../sg/rpc -I../sg/protob \
            `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet` -Wall -g
AM_LDFLAGS=-L../ -L../../utils -L../../lsh -L../../proxy -L../../utils \
	   -L../rpc -L../rpc/protob -L../protob \
	   -L../sg -L../sg/rpc -L../sg/protob `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
LDADD=-lseeksdht -lseekslsh -lrmd160 -lseeksutils -lseeksproxy -lseeksplugins \
      -lseekslsh -lseeksrpc -lseeksrpcprotob \
      -lseeksdht -lseeksdhtprotob -lseeksrpc -lseeksrpcprotob -lseeksdhtsg -lseeksdhtsgrpc -lseeksdhtsgprotob -lrmd160 -levent \
      -lpcre -lpcreposix -lprotobuf -lpthread -lz @CURL_LDADD@ @XML2_LDADD@
