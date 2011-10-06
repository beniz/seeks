#automake for seeks cli.

seeksclilibdir = $(libdir)/seeks/cli
seeksclidatadir = $(datadir)/seeks/cli

AM_CXXFLAGS=-Wall -g -pipe
AM_CPPFLAGS=-I${srcdir} -I${srcdir}/../proxy -I${srcdir}/../utils \
            @CURL_CFLAGS@

seeksclilib_LIBRARIES=libseekscli.a
libseekscli_a_SOURCES=cli.cpp ../proxy/encode.cpp

bin_PROGRAMS = seeks_cli
seeks_cli_SOURCES = seeks_cli.cpp ../proxy/curl_mget.cpp ../proxy/errlog.cpp

AM_LDFLAGS=-L${srcdir} -L${srcdir}/../proxy -L${srcdir}/../utils
LDADD=-lseekscli -lseeksutils @CURL_LDADD@