INCLUDES = -I$(srcdir)/main	\
			-I$(srcdir)/configuration \
			-I$(srcdir)/network	\
			-I$(srcdir)/utils	\
			-I$(srcdir)/interface \
			-I$(srcdir)/protocols \
			-I$(srcdir)/module	\
			-I$(srcdir)/log	\
			-I$(srcdir)/script	\
			-I$(srcdir)/website \
			-I$(srcdir)/modules

MAINTAINERCLEANFILES = Makefile.in

datadir	= @datadir@

bin_PROGRAMS	= mudmagic-bin

mudmagic_libdir = @mudmagic_libdir@

#MUDMAGICHEADERS = mudmagic.h

mudmagic_bin_SOURCES = main/main.c

AM_CFLAGS = $(CFLAGS) @DEBUG@ @GTK_CFLAGS@ @PROFILE@ \
		  -DDATADIR=\"$(datadir)/mudmagic\" @PYTHON_CFLAGS@ @LIBGLADE_CFLAGS@ \
		  @PCRE_CFLAGS@ @MXP_CFLAGS@ @ZLIB_CFLAGS@ @LIBGLADE_CFLAGS@ @LIBXML_CFLAGS@ @SQLITE_CFLAGS@ @CURL_CFLAGS@

mudmagic_bin_LDFLAGS = -export-dynamic 
mudmagic_bin_LDADD	=  $(srcdir)/libmudmagic.la @BUNDLED_LIBS@

lib_LTLIBRARIES	= libmudmagic.la

libmudmagic_la_SOURCES = \
			 log/debug.c		\
			 utils/muderr.c		\
			 utils/gamelist.c	\
			 utils/alias_triggers.c \
			 utils/strings.c	\
			 utils/utils.c		\
			 utils/variables.c	\
			 utils/gauges.c 	\
			 utils/statusvars.c	\
			 utils/owindows.c	\
			 utils/mudaccel.c	\
			 utils/directories.c	\
			 configuration/configuration.c \
			 interface/data.c	\
			 interface/cmdentry.c \
			 interface/interface.c \
			 interface/internal.c \
			 interface/modules.c \
			 interface/sessions.c \
			 interface/settings.c \
			 interface/tools.c \
			 interface/theme_select.c \
			 log/log.c 			\
			 network/network.c	\
			 network/proxy.c	\
			 module/module.c	\
			 script/script.c	\
			 script/iscript.c	\
			 website/website.c	\
			 protocols/http.c	\
			 protocols/telnet.c protocols/msp.c protocols/mxp.c protocols/zmp-core.c protocols/zmp-manage.c \
			 modules/automapper.c \
			 modules/database.c \
			 modules/notes.c \
			 modules/recorder.c

libmudmagic_la_LDFLAGS	= -export-dynamic -version-info $(MUDMAGIC_LIB_VERSION)
libmudmagic_la_LIBADD	= @PYTHON_LIBS@ @GTK_LIBS@ @PCRE_LIBS@ @LIBGLADE_LIBS@ @MXP_LIBS@ @SQLITE_LIBS@ @ZLIB_LIBS@ @LIBXML_LIBS@ @CURL_LIBS@
