check_PROGRAMS=ut_rpc_server

ut_rpc_server_SOURCES=ut-rpc-server.cpp

TESTS = $(check_PROGRAMS)

# automake for test rpc

bin_PROGRAMS=test_rpc_server test_rpc_client
test_rpc_server_SOURCES=test-rpc-server.cpp
test_rpc_client_SOURCES=test-rpc-client.cpp

include $(top_srcdir)/src/Makefile.include
