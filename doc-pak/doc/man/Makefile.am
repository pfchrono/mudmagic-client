man_MANS = mudmagic.1

EXTRA_DIST = $(man_MANS)

mudmagic.1.txt: mudmagic.1
	groff -t -e -mdoc -Tascii mudmagic.1 | col -bx > $@
