#automake file for seeks dht tests

check_PROGRAMS=ut_dhtkey
ut_dhtkey_SOURCES=ut-dhtkey.cpp

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
