#put pcre here for now since
#its the smallest of the libs
#fix this-should be in ifcheck
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
		       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs autom4te.cache

#bin_SCRIPTS = enlightenment-config
EXTRA_DIST = README AUTHORS COPYING NEWS TODO

SUBDIRS = bundled interface src doc

libtool: @LIBTOOL_DEPS@
		cd $(srcdir) && \
		$(SHELL) ./config.status --recheck

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mudmagic.pc

bin_SCRIPTS = mudmagic

DISTCLEANFILES: mudmagic mudmagic.pc mudmagic.spec
