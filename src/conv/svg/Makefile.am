bin_PROGRAMS = cdr2xhtml

AM_CXXFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib $(LIBCDR_CXXFLAGS) $(DEBUG_CXXFLAGS)

cdr2xhtml_DEPENDENCIES = @CDR2XHTML_WIN32_RESOURCE@

if STATIC_TOOLS
cdr2xhtml_LDADD = ../../lib/@CDR_OBJDIR@/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.a $(LIBCDR_LIBS) @CDR2XHTML_WIN32_RESOURCE@
cdr2xhtml_LDFLAGS = -all-static 
else
cdr2xhtml_LDADD = ../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la $(LIBCDR_LIBS) @CDR2XHTML_WIN32_RESOURCE@ 
endif

cdr2xhtml_SOURCES = \
	cdr2xhtml.cpp

if OS_WIN32

@CDR2XHTML_WIN32_RESOURCE@ : cdr2xhtml.rc $(cdr2xhtml_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2xhtml.rc @CDR2XHTML_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(cdr2xhtml_SOURCES) \
	cdr2xhtml.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	cdr2xhtml.rc
