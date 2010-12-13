#
# This is the p2p messaging component of the Seeks project,
# a collaborative websearch overlay network.
#
# Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
# Copyright (C) 2010  Loic Dachary <loic@dachary.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#automake file for seeks dht tests

check_PROGRAMS=ut_dhtkey ut_dhtvirtualnode # ut_dht_bootstrap
ut_dhtkey_SOURCES=ut-dhtkey.cpp
ut_dhtvirtualnode_SOURCES=ut-dhtvirtualnode.cpp
#ut_dht_bootstrap_SOURCES=ut-dht-bootstrap.cpp

TESTS = $(check_PROGRAMS)

bin_PROGRAMS=test_hash_key test_keys  grab_mac dhtkey_gen test_dhtnode # dht_cli test_dht_net_utils test_stabilizer

test_hash_key_SOURCES=test-hash-key.cpp
test_keys_SOURCES=test-keys.cpp 
test_dhtnode_SOURCES=test-dhtnode.cpp
#test_sgnode_SOURCES=test-sgnode.cpp
#test_dht_net_utils_SOURCES=test-dht-net-utils.cpp
#test_stabilizer_SOURCES=test-stabilizer.cpp
grab_mac_SOURCES=grab-mac.cpp
dhtkey_gen_SOURCES=dhtkey-gen.cpp
#dht_cli_SOURCES=dht-cli.cpp

include $(top_srcdir)/src/Makefile.include
