bin_PROGRAMS = cdr2svg

AM_CXXFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib $(LIBCDR_CXXFLAGS) $(DEBUG_CXXFLAGS)

cdr2svg_DEPENDENCIES = @CDR2SVG_WIN32_RESOURCE@

if STATIC_TOOLS
cdr2svg_LDADD = ../../lib/@CDR_OBJDIR@/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.a $(LIBCDR_LIBS) @CDR2SVG_WIN32_RESOURCE@
cdr2svg_LDFLAGS = -all-static 
else
cdr2svg_LDADD = ../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la $(LIBCDR_LIBS) @CDR2SVG_WIN32_RESOURCE@ 
endif

cdr2svg_SOURCES = \
	cdr2svg.cpp

if OS_WIN32

@CDR2SVG_WIN32_RESOURCE@ : cdr2svg.rc $(cdr2svg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2svg.rc @CDR2SVG_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(cdr2svg_SOURCES) \
	cdr2svg.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	cdr2svg.rc
