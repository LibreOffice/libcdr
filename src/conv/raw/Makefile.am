bin_PROGRAMS = cdr2raw

AM_CXXFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib $(LIBCDR_CXXFLAGS) $(DEBUG_CXXFLAGS)

cdr2raw_DEPENDENCIES = @CDR2RAW_WIN32_RESOURCE@

if STATIC_TOOLS
cdr2raw_LDADD = ../../lib/@CDR_OBJDIR@/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.a $(LIBCDR_LIBS) @CDR2RAW_WIN32_RESOURCE@
cdr2raw_LDFLAGS = -all-static 
else
cdr2raw_LDADD = ../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la $(LIBCDR_LIBS) @CDR2RAW_WIN32_RESOURCE@ 
endif

cdr2raw_SOURCES = \
	cdr2raw.cpp

if OS_WIN32

@CDR2RAW_WIN32_RESOURCE@ : cdr2raw.rc $(cdr2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2raw.rc @CDR2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(cdr2raw_SOURCES) \
	cdr2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	cdr2raw.rc	 
