#bundle directory
BD = $(top_srcdir)/bundled
libdir = @bundle_libdir@

BUNDLE_LIBS     = @SQLITE_LIBS@ @PYTHON_LIBS@ @GTK_LIBS@ @PCRE_LIBS@ @ZLIB_LIBS@ @MXP_LIBS@ @LIBGLADE_LIBS@ @CURL_LIBS@
BUNDLE_LDFLAGS  = -version-info $(MUDMAGIC_LIB_VERSION) -export-dynamic
lib_LTLIBRARIES =
AM_CFLAGS	= @LIBGLADE_CFLAGS@ @GTK_CFLAGS@ @LIBXML_CFLAGS@

libsqlite3_la_SOURCES = \
	$(BD)/sqlite3/attach.c $(BD)/sqlite3/date.c $(BD)/sqlite3/hash.c  \
	$(BD)/sqlite3/random.c $(BD)/sqlite3/update.c $(BD)/sqlite3/os_unix.c \
	$(BD)/sqlite3/vdbeapi.c $(BD)/sqlite3/where.c $(BD)/sqlite3/auth.c $(BD)/sqlite3/delete.c \
	$(BD)/sqlite3/insert.c $(BD)/sqlite3/pager.c $(BD)/sqlite3/select.c \
	$(BD)/sqlite3/utf.c $(BD)/sqlite3/vdbeaux.c $(BD)/sqlite3/btree.c $(BD)/sqlite3/expr.c \
	$(BD)/sqlite3/legacy.c $(BD)/sqlite3/pragma.c $(BD)/sqlite3/shell.c \
	$(BD)/sqlite3/tokenize.c $(BD)/sqlite3/util.c $(BD)/sqlite3/vdbe.c \
	$(BD)/sqlite3/build.c $(BD)/sqlite3/func.c $(BD)/sqlite3/main.c $(BD)/sqlite3/printf.c \
	$(BD)/sqlite3/table.c $(BD)/sqlite3/trigger.c $(BD)/sqlite3/vacuum.c $(BD)/sqlite3/vdbemem.c \
	$(BD)/sqlite3/opcodes.c $(BD)/sqlite3/parse.c $(BD)/sqlite3/alter.c \
	$(BD)/sqlite3/analyze.c $(BD)/sqlite3/callback.c $(BD)/sqlite3/complete.c \
	$(BD)/sqlite3/prepare.c $(BD)/sqlite3/vdbefifo.c
	\
	$(BD)/sqlite3/btree.h $(BD)/sqlite3/os_common.h \
	$(BD)/sqlite3/pager.h $(BD)/sqlite3/vdbe.h $(BD)/sqlite3/hash.h $(BD)/sqlite3/os.h \
	$(BD)/sqlite3/sqliteInt.h $(BD)/sqlite3/vdbeInt.h \
	$(BD)/sqlite3/parse.h $(BD)/sqlite3/opcodes.h $(BD)/sqlite3/sqlite3.h \
	$(BD)/sqlite3/keywordhash.h
	
libmxp_la_SOURCES = \
	$(BD)/libmxp/centitymanager.cpp $(BD)/libmxp/cmxpcolors.cpp $(BD)/libmxp/cmxpparser.cpp \
	$(BD)/libmxp/libmxp.cpp $(BD)/libmxp/celementmanager.cpp \
	$(BD)/libmxp/cmxpstate.cpp $(BD)/libmxp/cresulthandler.cpp \
	$(BD)/libmxp/cmxpprocessor.cpp $(BD)/libmxp/stringops.cpp \
	\
	$(BD)/libmxp/rgbops.h $(BD)/libmxp/cmxpstate.h $(BD)/libmxp/cmxpprocessor.h \
	$(BD)/libmxp/libmxp.h $(BD)/libmxp/celementmanager.h $(BD)/libmxp/cmxpcolors.h \
	$(BD)/libmxp/entitylist.h $(BD)/libmxp/colorlist.h \
	$(BD)/libmxp/distro/config.h.in $(BD)/libmxp/distro/acconfig.h \
	$(BD)/libmxp/stringops.h $(BD)/libmxp/cresulthandler.h \
	$(BD)/libmxp/centitymanager.h $(BD)/libmxp/cmxpparser.h

libpcre_la_SOURCES = \
	$(BD)/pcre/pcre.c \
	$(BD)/pcre/get.c
	\
	$(BD)/pcre/config.h $(BD)/pcre/internal.h \
	$(BD)/pcre/pcre.h

libzlib_la_SOURCES = \
	$(BD)/zlib/inflate.c $(BD)/zlib/zutil.c $(BD)/zlib/inftrees.c \
	$(BD)/zlib/infback.c $(BD)/zlib/deflate.c $(BD)/zlib/inffast.c \
	$(BD)/zlib/crc32.c $(BD)/zlib/compress.c $(BD)/zlib/gzio.c \
	$(BD)/zlib/trees.c $(BD)/zlib/minigzip.c $(BD)/zlib/uncompr.c \
	\
	$(BD)/zlib/crc32.h $(BD)/zlib/inffixed.h $(BD)/zlib/inffast.h \
	$(BD)/zlib/zutil.h $(BD)/zlib/deflate.h $(BD)/zlib/zlib.h \
	$(BD)/zlib/inflate.h $(BD)/zlib/zconf.h  \
	$(BD)/zlib/inftrees.h $(BD)/zlib/trees.h

libglade_2_0_la_SOURCES  = \
	$(BD)/libglade/glade-parser.c  $(BD)/libglade/glade-xml.c \
	$(BD)/libglade/glade-init.c $(BD)/libglade/glade-gtk.c

libcurl_la_SOURCES  = \
	$(BD)/curl/base64.c \
	$(BD)/curl/connect.c \
	$(BD)/curl/content_encoding.c \
	$(BD)/curl/cookie.c \
	$(BD)/curl/dict.c \
	$(BD)/curl/easy.c \
	$(BD)/curl/escape.c \
	$(BD)/curl/file.c \
	$(BD)/curl/formdata.c \
	$(BD)/curl/ftp.c \
	$(BD)/curl/getenv.c \
	$(BD)/curl/getinfo.c \
	$(BD)/curl/gtls.c \
	$(BD)/curl/curl_hash.c \
	$(BD)/curl/hostares.c \
	$(BD)/curl/hostasyn.c \
	$(BD)/curl/hostip.c \
	$(BD)/curl/hostip4.c \
	$(BD)/curl/hostip6.c \
	$(BD)/curl/hostsyn.c \
	$(BD)/curl/hostthre.c \
	$(BD)/curl/http.c \
	$(BD)/curl/http_chunks.c \
	$(BD)/curl/http_digest.c \
	$(BD)/curl/http_negotiate.c \
	$(BD)/curl/if2ip.c \
	$(BD)/curl/inet_ntop.c \
	$(BD)/curl/inet_pton.c \
	$(BD)/curl/krb4.c \
	$(BD)/curl/ldap.c \
	$(BD)/curl/llist.c \
	$(BD)/curl/memdebug.c \
	$(BD)/curl/mprintf.c \
	$(BD)/curl/multi.c \
	$(BD)/curl/netrc.c \
	$(BD)/curl/parsedate.c \
	$(BD)/curl/progress.c \
	$(BD)/curl/security.c \
	$(BD)/curl/curl_select.c \
	$(BD)/curl/sendf.c \
	$(BD)/curl/share.c \
	$(BD)/curl/speedcheck.c \
	$(BD)/curl/strequal.c \
	$(BD)/curl/strerror.c \
	$(BD)/curl/strtok.c \
	$(BD)/curl/strtoofft.c \
	$(BD)/curl/telnet.c \
	$(BD)/curl/tftp.c \
	$(BD)/curl/timeval.c \
	$(BD)/curl/transfer.c \
	$(BD)/curl/sslgen.c \
	$(BD)/curl/url.c \
	$(BD)/curl/version.c

#	$(BD)/curl/md5.c \
#	$(BD)/curl/http_ntlm.c \
#	$(BD)/curl/ssluse.c \
#########
# Sqlite3
#########
if MISSING_SQLITE
lib_LTLIBRARIES+=libsqlite3.la
libsqlite3_la_LDFLAGS = $(BUNDLE_LDFLAGS) -rpath @bundle_libdir@
libsqlite3_la_LIBADD = $(BUNDLE_LIBS)
endif

#####
# MXP
#####
if MISSING_LIBMXP
lib_LTLIBRARIES+=libmxp.la
libmxp_la_LDFLAGS = $(BUNDLE_LDFLAGS) -rpath @bundle_libdir@
libmxp_la_LIBADD = $(BUNDLE_LIBS)
endif

######
# PCRE
######
if MISSING_PCRE
lib_LTLIBRARIES+=libpcre.la
libpcre_la_LDFLAGS = $(BUNDLE_LDFLAGS) -rpath @bundle_libdir@
libpcre_la_LIBADD = $(BUNDLE_LIBS)
endif

########
# Zlib
########
if MISSING_ZLIB
lib_LTLIBRARIES+=libzlib.la
libzlib_la_LDFLAGS = $(BUNDLE_LDFLAGS) -rpath @bundle_libdir@
libzlib_la_LIBADD = $(BUNDLE_LIBS)
endif

########
# LibGlade
########
if MISSING_LIBGLADE
lib_LTLIBRARIES+=libglade-2.0.la
libglade_2_0_la_LDFLAGS = $(BUNDLE_LDFLAGS) -rpath @bundle_libdir@
libglade_2_0_la_LIBADD = $(BUNDLE_LIBS)
endif

########
# LibCurl
########
if MISSING_LIBCURL
lib_LTLIBRARIES+=libcurl.la
libcurl_la_LDFLAGS = $(BUNDLE_LDFLAGS) -rpath @bundle_libdir@
libcurl_la_LIBADD = $(BUNDLE_LIBS)
endif

uninstall-local:
	-rm -fr @bundle_libdir@
	-rm -fr @mudmagic_libdir@

