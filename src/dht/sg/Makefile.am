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
#automake for dht

lib_LTLIBRARIES=libseeksdhtsg.la
AM_CPPFLAGS=-I${srcdir}/rpc -I${srcdir}/protob -Iprotob -I${srcdir}/.. -I${srcdir}/rpc -I${srcdir}/../protob -I${srcdir}/../../utils -I${srcdir}/../../lsh \
	    -I${srcdir}/../../proxy -I${srcdir}/../protob -I../protob -I${srcdir}/../rpc -I${srcdir}/../rpc/protob \
	    `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdhtsg_la_SOURCES=subscriber.cpp searchgroup.cpp sg_configuration.cpp sg_manager.cpp SGNode.cpp SGVirtualNode.cpp sg_api.cpp sg_sweeper.cpp sg_db.cpp searchgroup.h sg_api.h sg_configuration.h sg_db.h sg_manager.h SGNode.h SGVirtualNode.h sg_sweeper.h subscriber.h

SUBDIRS=protob rpc #protob/tests rpc/tests tests
