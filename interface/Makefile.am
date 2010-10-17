appsdir = $(datadir)/applications
apps_DATA = mudmagic.desktop

#Extra Files
MUDMAGIC_FIGURES =  src/main/mudmagic.ico src/main/mudmagic.rc src/main/Makefile.win

#add mudmagic icon to the local pixmaps dir
distpixmapdir = $(datadir)/pixmaps
distpixmap_DATA = mudmagic.png

Applicationsdir = $(datadir)/applications
Applications_in_files = mudmagic.desktop
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)

EXTRA_DIST = mudmagic.desktop   \
	     $(Applications_DATA) 

#add images that the client requires
SHARE_IMAGES =  apple-red.png   \
		gnome-foot.png  \
		gnome-gmush.png \
		gnome-gsame.png \
		gnu-keys.png	\
		mudmagic.png	\
		nymph.png	\
		version.png	\
		background.xpm  \
		mudmagic.xpm	\
		mudmagic2.xpm	\
		wizard.xpm	\
		s_wizard.xpm	\
		xs_wizard.xpm	\
		interface.glade \
		automapper.glade \
		database.glade  \
		notes.glade	\
		pixmaps/featured.png \
		pixmaps/mudmagic_hosted.png \
		pixmaps/top30.png \
		pixmaps/mmenterstar.png

#add flat-text files that the client requires
SHARE_DOCS =    doc/close.gif 		\
		doc/explain.html 	\
		doc/favicon.ico 	\
		doc/index.html 		\
		doc/introduction.html 	\
		doc/lightbox.css 	\
		doc/lightbox.js 	\
		doc/loading.gif 	\
		doc/nav.html		\
		doc/overlay.png		\
		doc/readme.txt		\
		doc/treeview.css	\
		doc/alias.html		\
		doc/connecting.html	\
		doc/mudmagic_scripting.html	\
		doc/python_scripting.html	\
		doc/developers.html	\
		doc/triggers.html	\
		doc/configuration.html	\
		doc/macros.html		\
		doc/mudmagic.html	\
		doc/macintosh.txt	\
		doc/wincompile.txt	\
		doc/license.txt

SHARE_GRFX =	doc/grafix/alias1_big.png	\
		doc/grafix/alias1_thumb.png	\
		doc/grafix/alias2_big.png	\
		doc/grafix/alias2_thumb.png	\
		doc/grafix/atm_big.png		\
		doc/grafix/font_big.png		\
		doc/grafix/ix_copy.gif		\
		doc/grafix/jshtext.gif		\
		doc/grafix/mxp_thumb.png	\
		doc/grafix/theme_thumb.png	\
		doc/grafix/atm_thumb.png	\
		doc/grafix/font_thumb.png	\
		doc/grafix/ix_text.gif		\
		doc/grafix/modules_big.png	\
		doc/grafix/savedchar.png	\
		doc/grafix/wizard_big.png	\
		doc/grafix/delayed_big.png	\
		doc/grafix/ix_zip.gif		\
		doc/grafix/modules_thumb.png	\
		doc/grafix/savedchar_thumb.png	\
		doc/grafix/wizard_thumb.png	\
		doc/grafix/delayed_thumb.png	\
		doc/grafix/jshlogo.gif		\
		doc/grafix/mxp_big.png		\
		doc/grafix/theme_big.png	\
		doc/grafix/connect_big.png	\
		doc/grafix/connect_thumb.png	\
		doc/grafix/open_big.png		\
		doc/grafix/open_thumb.png	\
		doc/grafix/proxy_big.png	\
		doc/grafix/proxy_thumb.png	\
		doc/grafix/seperator_big.png	\
		doc/grafix/seperator_thumb.png	\
		doc/grafix/sound1_big.png	\
		doc/grafix/sound1_thumb.png	\
		doc/grafix/sound2_big.png	\
		doc/grafix/sound2_thumb.png
		
SHARE_HLP =	doc/grafix/hlp/ix_down.gif	\
		doc/grafix/hlp/ix_endm.gif	\
		doc/grafix/hlp/ix_leaf.gif	\
		doc/grafix/hlp/ix_link.gif	\
		doc/grafix/hlp/ix_listm.gif	\
		doc/grafix/hlp/ix_open.gif	\
		doc/grafix/hlp/ix_up.gif	\
		doc/grafix/hlp/ix_book.gif	\
		doc/grafix/hlp/ix_end.gif	\
		doc/grafix/hlp/ix_endp.gif	\
		doc/grafix/hlp/ix_line.gif	\
		doc/grafix/hlp/ix_list.gif	\
		doc/grafix/hlp/ix_listp.gif	\
		doc/grafix/hlp/ix_space.gif


CLEANFILES=*.bak

install-exec-local:

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/interface;
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/doc;
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/doc/grafix;
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/doc/grafix/hlp;

	-for file in $(SHARE_IMAGES); do \
		if test -f $$file; then \
			$(INSTALL_DATA) $(srcdir)/$$file $(DESTDIR)$(pkgdatadir)/interface; \
		fi \
	done
	-for file in $(SHARE_DOCS); do \
		if test -f $$file; then \
			$(INSTALL_DATA) $(srcdir)/$$file $(DESTDIR)$(pkgdatadir)/doc; \
		fi \
	done
	 -for file in $(SHARE_GRFX); do \
                if test -f $$file; then \
                        $(INSTALL_DATA) $(srcdir)/$$file $(DESTDIR)$(pkgdatadir)/doc/grafix; \
                fi \
        done
	 -for file in $(SHARE_HLP); do \
                if test -f $$file; then \
                        $(INSTALL_DATA) $(srcdir)/$$file $(DESTDIR)$(pkgdatadir)/doc/grafix/hlp; \
                fi \
        done

uninstall-local:
	-rm -fr $(DESTDIR)$(pkgdatadir)/interface
	-rm -fr $(DESTDIR)$(pkgdatadir)/doc
	-rm -fr $(DESTDIR)$(pkgdatadir)

