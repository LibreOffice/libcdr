ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig

pkgconf_DATA = libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.pc

SUBDIRS = build inc src docs

EXTRA_DIST = \
	AUTHORS \
	COPYING.MPL \
	NEWS \
	README \
	autogen.sh \
	libcdr.pc.in \
	libcdr-zip.in \
	libcdr.spec \
	libcdr.spec.in

distclean-local:
	rm -rf *.cache *~ *.out *.pc

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

zip: all install
	sh libcdr-zip 

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog

astyle:
	astyle --options=astyle.options \*.cpp \*.h
