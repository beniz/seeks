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

lib_LTLIBRARIES=libseeksdht.la
AM_CPPFLAGS=-I${srcdir}/protob -Iprotob -I${srcdir}/../utils -I${srcdir}/../lsh -I${srcdir}/../proxy  -I${srcdir}/rpc -I${srcdir}/rpc/protob -Irpc/protob \
	     `pkg-config --cflags protobuf` @LCOV_CFLAGS@
AM_CXXFLAGS=-Wall -g -Wno-deprecated
libseeksdht_la_SOURCES=\
	NetAddress.cpp DHTKey.cpp dht_configuration.cpp FingerTable.cpp \
	DHTVirtualNode.cpp Location.cpp LocationTable.cpp DHTNode.cpp Transport.cpp Transport.h Stabilizer.cpp \
	BstTimeCb.cpp SuccList.cpp RouteIterator.cpp dht_api.cpp \
	Bst.h \
	BstTimeCb.h \
	callback.h \
	dht_api.h \
	dht_configuration.h \
	dht_err.h \
	dht_exception.h \
	DHTKey.h \
	DHTNode.h \
	DHTVirtualNode.h \
	FingerTable.h \
	Location.h \
	LocationTable.h \
	NetAddress.h \
	RouteIterator.h \
	Stabilizer.h \
	SuccList.h

SUBDIRS=protob rpc sg #protob/tests rpc/tests tests
