if BUILD_TOOLS

bin_PROGRAMS = cdr2xhtml cmx2xhtml

AM_CXXFLAGS = \
	-I$(top_srcdir)/inc \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

cdr2xhtml_DEPENDENCIES = @CDR2XHTML_WIN32_RESOURCE@

cdr2xhtml_LDADD = \
	../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(ICU_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@CDR2XHTML_WIN32_RESOURCE@ 

cdr2xhtml_SOURCES = \
	cdr2xhtml.cpp

cmx2xhtml_DEPENDENCIES = @CMX2XHTML_WIN32_RESOURCE@

cmx2xhtml_LDADD = \
	../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(ICU_LIBS) \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@CMX2XHTML_WIN32_RESOURCE@ 

cmx2xhtml_SOURCES = \
	cmx2xhtml.cpp

if OS_WIN32

@CDR2XHTML_WIN32_RESOURCE@ : cdr2xhtml.rc $(cdr2xhtml_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2xhtml.rc @CDR2XHTML_WIN32_RESOURCE@

@CMX2XHTML_WIN32_RESOURCE@ : cmx2xhtml.rc $(cmx2xhtml_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cmx2xhtml.rc @CMX2XHTML_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(cdr2xhtml_SOURCES) \
	$(cmx2xhtml_SOURCES) \
	cdr2xhtml.rc.in \
	cmx2xhtml.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	cdr2xhtml.rc \
	cmx2xhtml.rc

endif
