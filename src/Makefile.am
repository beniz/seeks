publicdir=$(datadir)/seeks
proxydir=$(datadir)/seeks

bin_PROGRAMS=seeks
seeks_SOURCES=seeks.cpp

AM_CXXFLAGS=-Wall -Wno-deprecated -g -pipe \
	    -I${srcdir} -I${srcdir}/utils -I${srcdir}/lsh -I${srcdir}/proxy @PCRE_CFLAGS@ @ICU_CFLAGS@ -DSEEKS_CONFIGDIR='"$(sysconfdir)/seeks/"' \
	    -I${srcdir}/plugins/websearch -I${srcdir}/plugins/query_capture -I${srcdir}/plugins/uri_capture -I${srcdir}/plugins/cf -I${srcdir}/plugins/udb_service -I${srcdir}/dht \
	    `pkg-config opencv --cflags` 
if HAVE_PROTOBUF
AM_CXXFLAGS += -Iproxy/ `pkg-config --cflags protobuf`

if HAVE_TC
AM_CXXFLAGS += `pkg-config --cflags tokyocabinet`
endif

if HAVE_TT
AM_CXXFLAGS += `pkg-config --cflags tokyotyrant`
endif

endif
AM_LDFLAGS=`pkg-config opencv --libs` -Lutils/ -Llsh/ -Lproxy/ @PCRE_LDADD@ @CURL_LDADD@ @XML2_LDADD@ @ICU_LDADD@ -Lplugins/websearch \
	    -Lplugins/query_capture -Lplugins/uri_capture -Lplugins/udb_service -Ldht/ 
if HAVE_PROTOBUF
AM_LDFLAGS += -Lplugins/cf `pkg-config --libs protobuf` 

if HAVE_TC
AM_LDFLAGS += `pkg-config --libs tokyocabinet`
endif

if HAVE_TT
AM_LDFLAGS += `pkg-config --libs tokyotyrant`
endif
endif

if HAVE_XSLSERIALIZER_PLUGIN
AM_CXXFLAGS += -I${top_srcdir}/src/plugins/xsl_serializer \
	@XSLT_CFLAGS@
AM_LDFLAGS += -Lplugins/xsl_serializer
endif

if USE_PERL
AM_LDFLAGS += `perl -MExtUtils::Embed -e ldopts`
endif
LDADD=-lseekswebsearchplugin 
if HAVE_PROTOBUF
if HAVE_TC
LDADD += -lquerycaptureplugin -luricaptureplugin -lcfplugin -ludbserviceplugin
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
LDADD += -lseeksuserdb -lseeksuserdbfix
endif
if HAVE_XSLSERIALIZER_PLUGIN
LDADD += \
	-lseeksxslserializerplugin \
	@XSLT_LDADD@
endif
endif
LDADD += -lpcre -lpcreposix -lseekslsh -lseeksdht -lrmd160 -lpthread -lz @CURL_LDADD@
if !HAVE_BSD
LDADD += -ldl
endif

SUBDIRS=utils proxy lsh dht plugins proxy/tests lsh/tests .

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
			  public/yui/3.2.0/cssreset/reset-min.css \
			  public/yui/3.2.0/cssfonts/fonts-min.css \
			  public/yui/3.2.0/cssgrids/grids-min.css \
			  public/yui/3.2.0/yui/yui-min.js \
			  public/yui/3.2.0/loader/loader-min.js \
			  public/yui/3.2.0/dom/dom-base-min.js \
			  public/yui/3.2.0/dom/selector-css2-min.js \
			  public/yui/3.2.0/dom/selector-native-min.js \
			  public/yui/3.2.0/event-custom/event-custom-base-min.js \
			  public/yui/3.2.0/event-custom/event-custom-min.js \
			  public/yui/3.2.0/event/event-synthetic-min.js \
			  public/yui/3.2.0/event/event-base-min.js \
			  public/yui/3.2.0/event/event-key-min.js \
			  public/yui/3.2.0/node/node-base-min.js \
			  public/yui/3.2.0/oop/oop-min.js \
	                  public/yui/3.2.0/jsonp/jsonp-min.js \
	                  public/yui/3.2.0/substitute/substitute-min.js \
	                  public/yui/3.2.0/dom/dom-min.js \
	                  public/yui/3.2.0/node/node-style-min.js \
	                  public/yui/3.2.0/transition/transition-min.js \
	                  public/yui/3.2.0/collection/array-extras-min.js \
	                  public/yui/3.2.0/querystring/querystring-min.js \
			  lsh/swl/arabicST.txt \
			  lsh/swl/bulgarianST.txt \
			  lsh/swl/czechST.txt \
			  lsh/swl/englishST.txt \
			  lsh/swl/finnishST.txt \
			  lsh/swl/frenchST.txt \
			  lsh/swl/germanST.txt \
			  lsh/swl/hungarianST.txt \
			  lsh/swl/italianST.txt \
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

EXTRA_DIST = public/images/seek_icon.svg \
	     lsh/swl/License.txt
