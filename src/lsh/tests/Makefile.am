# automake for lsh
TESTS = $(check_PROGRAMS)

bin_PROGRAMS=gen_mrf_query_160
check_PROGRAMS=ut_mrf_query_160 ut_mrf

ut_mrf_query_160_SOURCES=ut-mrf-query-160.cpp
gen_mrf_query_160_SOURCES=gen-mrf-query-160.cpp
ut_mrf_SOURCES=ut-mrf.cpp

include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I../../../proxy/