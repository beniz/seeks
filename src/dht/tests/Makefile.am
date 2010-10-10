# automake file for dht.

noinst_PROGRAMS=test_dht_keys
test_dht_keys_SOURCES=test-dht-keys.cpp

AM_CPPFLAGS=-Wall -g -Wno-deprecated -pipe \
            -I${srcdir}/../ -I${srcdir}/../../utils -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'
AM_LDFLAGS=-L../ -L../../utils
LDADD = -lseeksdht -lrmd160