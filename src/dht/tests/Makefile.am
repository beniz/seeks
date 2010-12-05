#automake file for seeks dht tests

check_PROGRAMS=ut_dhtkey ut_dht_bootstrap ut_dht_protocol #ut_dht_protocol_preliminary
ut_dhtkey_SOURCES=ut-dhtkey.cpp
ut_dht_protocol_preliminary_SOURCES=ut-dht-protocol-preliminary.cpp
ut_dht_protocol_SOURCES=ut-dht-protocol.cpp
ut_dht_bootstrap_SOURCES=ut-dht-bootstrap.cpp

TESTS = $(check_PROGRAMS)

bin_PROGRAMS=test_hash_key test_keys test_dhtnode test_sgnode grab_mac dhtkey_gen dht_cli # test_dht_net_utils test_stabilizer

test_hash_key_SOURCES=test-hash-key.cpp
test_keys_SOURCES=test-keys.cpp 
test_dhtnode_SOURCES=test-dhtnode.cpp
test_sgnode_SOURCES=test-sgnode.cpp
#test_dht_net_utils_SOURCES=test-dht-net-utils.cpp
#test_stabilizer_SOURCES=test-stabilizer.cpp
grab_mac_SOURCES=grab-mac.cpp
dhtkey_gen_SOURCES=dhtkey-gen.cpp
dht_cli_SOURCES=dht-cli.cpp

include $(top_srcdir)/src/Makefile.include
