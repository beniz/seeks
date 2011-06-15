TESTS = $(check_PROGRAMS)

check_PROGRAMS = ut_cf_sre ut_cr_store ut_peer_list
ut_cf_sre_SOURCES = ut-cf-sre.cpp
ut_cr_store_SOURCES = ut-cr-store.cpp
ut_peer_list_SOURCES = ut-peer-list.cpp

include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I../../../proxy/