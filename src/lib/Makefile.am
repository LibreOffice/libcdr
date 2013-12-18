if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_includedir = $(includedir)/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@/libcdr
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_include_HEADERS = \
	$(top_srcdir)/inc/libcdr/libcdr.h \
	$(top_srcdir)/inc/libcdr/libcdr_api.h \
	$(top_srcdir)/inc/libcdr/CDRDocument.h \
	$(top_srcdir)/inc/libcdr/CMXDocument.h

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(LCMS2_CFLAGS) $(ZLIB_CFLAGS) $(ICU_CFLAGS) $(DEBUG_CXXFLAGS) -DLIBCDR_BUILD=1

libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_LIBADD  = $(REVENGE_LIBS) $(LCMS2_LIBS) $(ZLIB_LIBS) $(ICU_LIBS) @LIBCDR_WIN32_RESOURCE@
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_DEPENDENCIES = @LIBCDR_WIN32_RESOURCE@
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_SOURCES = \
	CDRCollector.cpp \
	CDRContentCollector.cpp \
	CDRDocument.cpp \
	CDRInternalStream.cpp \
	CDROutputElementList.cpp \
	CDRParser.cpp \
	CDRPath.cpp \
	CDRStylesCollector.cpp \
	CDRTransforms.cpp \
	CDRTypes.cpp \
	CMXDocument.cpp \
	CMXParser.cpp \
	CommonParser.cpp \
	libcdr_utils.cpp \
	CDRCollector.h \
	CDRColorPalettes.h \
	CDRColorProfiles.h \
	CDRContentCollector.h \
	CDRDocumentStructure.h \
	CDRInternalStream.h \
	CDROutputElementList.h \
	CDRParser.h \
	CDRPath.h \
	CDRStylesCollector.h \
	CDRTransforms.h \
	CDRTypes.h \
	CMXDocumentStructure.h \
	CMXParser.h \
	CommonParser.h \
	libcdr_utils.h

if OS_WIN32

@LIBCDR_WIN32_RESOURCE@ : libcdr.rc $(libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libcdr.rc @LIBCDR_WIN32_RESOURCE@

endif


EXTRA_DIST = \
	libcdr.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libcdr.rc
