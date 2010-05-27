# automake for httpserv plugin.

httpservpluginlibdir = $(libdir)/seeks/plugins/httpserv
httpservplugindatadir = $(datadir)/seeks/plugins/httpserv

AM_CPPFLAGS=-Wno-deprecated -Wall -g -pipe \
            -I../websearch -I../../utils -I../../lsh -I../../proxy
ACLOCAL_AMFLAGS=-I m4
httpservpluginlib_LTLIBRARIES=libseekshttpservplugin.la
libseekshttpservplugin_la_SOURCES=httpserv_configuration.cpp httpserv.cpp
libseekshttpservplugin_la_CXXFLAGS=-Wall -g -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"'

httpservconfigdir=$(sysconfdir)/seeks
httpservconfig_DATA=httpserv-config
