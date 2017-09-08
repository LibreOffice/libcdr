if BUILD_TOOLS

bin_PROGRAMS = cdr2raw cmx2raw

AM_CXXFLAGS = \
	-I$(top_srcdir)/inc \
	$(REVENGE_CFLAGS) \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

cdr2raw_DEPENDENCIES = @CDR2RAW_WIN32_RESOURCE@

cdr2raw_LDADD = \
	../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(ICU_LIBS) \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@CDR2RAW_WIN32_RESOURCE@ 

cdr2raw_SOURCES = \
	cdr2raw.cpp

cmx2raw_DEPENDENCIES = @CMX2RAW_WIN32_RESOURCE@

cmx2raw_LDADD = \
	../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(ICU_LIBS) \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@CMX2RAW_WIN32_RESOURCE@ 

cmx2raw_SOURCES = \
	cmx2raw.cpp

if OS_WIN32

@CDR2RAW_WIN32_RESOURCE@ : cdr2raw.rc $(cdr2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2raw.rc @CDR2RAW_WIN32_RESOURCE@

@CMX2RAW_WIN32_RESOURCE@ : cmx2raw.rc $(cmx2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cmx2raw.rc @CMX2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(cdr2raw_SOURCES) \
	$(cmx2raw_SOURCES) \
	cdr2raw.rc.in \
	cmx2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	cdr2raw.rc \
	cmx2raw.rc	 

endif
