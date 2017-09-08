if BUILD_TOOLS

bin_PROGRAMS = cdr2text cmx2text

AM_CXXFLAGS = \
	-I$(top_srcdir)/inc \
	$(REVENGE_CFLAGS) \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

cdr2text_DEPENDENCIES = @CDR2TEXT_WIN32_RESOURCE@

cdr2text_LDADD = \
	../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(ICU_LIBS) \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@CDR2TEXT_WIN32_RESOURCE@ 

cdr2text_SOURCES = \
	cdr2text.cpp

cmx2text_DEPENDENCIES = @CMX2TEXT_WIN32_RESOURCE@

cmx2text_LDADD = \
	../../lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(ICU_LIBS) \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	@CMX2TEXT_WIN32_RESOURCE@ 

cmx2text_SOURCES = \
	cmx2text.cpp

if OS_WIN32

@CDR2TEXT_WIN32_RESOURCE@ : cdr2text.rc $(cdr2text_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2text.rc @CDR2TEXT_WIN32_RESOURCE@

@CMX2TEXT_WIN32_RESOURCE@ : cmx2text.rc $(cmx2text_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cmx2text.rc @CMX2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(cdr2text_SOURCES) \
	$(cmx2text_SOURCES) \
	cdr2text.rc.in \
	cmx2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	cdr2text.rc	\
	cmx2text.rc	 

endif
