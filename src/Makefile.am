publicdir=$(datadir)/seeks
proxydir=$(datadir)/seeks

bin_PROGRAMS=seeks
seeks_SOURCES=seeks.cpp

AM_CPPFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/utils -I${srcdir}/lsh -I${srcdir}/proxy @PCRE_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' \
	    -I${srcdir}/plugins/websearch -I${srcdir}/plugins/query_capture -I${srcdir}/plugins/cf -I${srcdir}/dht \
	    `pkg-config opencv --cflags` 
if HAVE_PROTOBUF
if HAVE_TC
AM_CPPFLAGS += `pkg-config --cflags protobuf` `pkg-config --cflags tokyocabinet`
endif
endif
AM_LDFLAGS=`pkg-config opencv --libs` -Lutils/ -Llsh/ -Lproxy/ @PCRE_LDADD@ @CURL_LDADD@ @XML2_LDADD@ -Lplugins/websearch \
	    -Lplugins/query_capture -Ldht/ 
if HAVE_PROTOBUF
if HAVE_TC
AM_LDFLAGS += -Lplugins/cf `pkg-config --libs protobuf` `pkg-config --libs tokyocabinet`
endif
endif

if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=-lseekswebsearchplugin 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lquerycaptureplugin -lcfplugin
endif
endif
if HAVE_HTTPSERV_PLUGIN
LDADD += -levent
endif
if HAVE_IMG_WEBSEARCH_PLUGIN
AM_LDFLAGS += -Lplugins/img_websearch
LDADD += -lseeksimgwebsearchplugin
endif
LDADD += -lseeksutils -lseeksproxy -lseeksplugins 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lseeksuserdb 
endif
endif
LDADD += -lpcre -lpcreposix -lseekslsh -lseeksdht -lrmd160 -lpthread -lz @CURL_LDADD@
if !HAVE_BSD
LDADD += -ldl
endif

SUBDIRS=utils proxy lsh dht proxy/tests plugins lsh/tests .

configdir=$(sysconfdir)/seeks
dist_config_DATA=config \
		 lsh/lsh-config

nobase_dist_public_DATA=\
			  public/css/base.css \
			  public/images/seek_icon_16x16_transparent.png \
			  public/images/seek_icon_32x32_transparent.png \
			  public/images/seek_icon_48x48_transparent.png \
			  public/images/seek_icon_64x64_transparent.png \
			  public/images/seek_icon_16x16_transparent.ico \
		 	  public/yui/3.0.0/cssreset/reset-min.css \
		 	  public/yui/3.0.0/cssfonts/fonts-min.css \
		 	  public/yui/3.0.0/cssgrids/grids-min.css \
		 	  public/yui/3.0.0/yui/yui-min.js \
			  public/yui/3.0.0/loader/loader-min.js \
			  lsh/swl/arabicST.txt \
			  lsh/swl/bulgarianST.txt \
			  lsh/swl/czechST.txt \
			  lsh/swl/englishST.txt \
			  lsh/swl/finnishST.txt \
			  lsh/swl/frenchST.txt \
			  lsh/swl/germanST.txt \
			  lsh/swl/hungarianST.txt \
			  lsh/swl/italianST.txt \
			  lsh/swl/License.txt \
			  lsh/swl/polishST.txt \
			  lsh/swl/portugueseST.txt \
			  lsh/swl/roumanianST.txt \
			  lsh/swl/russianST.txt \
			  lsh/swl/spanishST.txt \
			  lsh/swl/swedishST.txt

nobase_dist_proxy_DATA=\
			  proxy/templates/mod-unstable-warning \
			  proxy/templates/url-info-osd.xml \
			  proxy/templates/connect-failed \
			  proxy/templates/cgi-error-file \
			  proxy/templates/show-status-file \
			  proxy/templates/show-version \
			  proxy/templates/cgi-style.css \
			  proxy/templates/info-head \
			  proxy/templates/forwarding-failed \
			  proxy/templates/show-request \
			  proxy/templates/mod-support-and-service \
			  proxy/templates/toggle-mini \
			  proxy/templates/mod-title \
			  proxy/templates/show-status-plugin \
			  proxy/templates/cgi-error-modified \
			  proxy/templates/cgi-error-parse \
			  proxy/templates/show-status \
			  proxy/templates/cgi-error-404 \
			  proxy/templates/no-server-data \
			  proxy/templates/cgi-error-bad-param \
			  proxy/templates/toggle \
			  proxy/templates/blocked \
			  proxy/templates/cgi-error-disabled \
			  proxy/templates/cgi-error-file-read-only \
			  proxy/templates/connection-timeout \
			  proxy/templates/default \
			  proxy/templates/show-url-info \
			  proxy/templates/no-such-domain

EXTRA_DIST = public/images/seek_icon.svg
