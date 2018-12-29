if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

noinst_LTLIBRARIES = libcdr-internal.la
lib_LTLIBRARIES = libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_CFLAGS) \
	$(LCMS2_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(ICU_CFLAGS) \
	$(DEBUG_CXXFLAGS) \
	-DLIBCDR_BUILD=1 \
	-DBOOST_ERROR_CODE_HEADER_ONLY \
	-DBOOST_SYSTEM_NO_DEPRECATED

if HAVE_VISIBILITY
AM_CXXFLAGS += -fvisibility=hidden -DLIBCDR_VISIBILITY
endif

libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_LIBADD  = libcdr-internal.la $(REVENGE_LIBS) $(LCMS2_LIBS) $(ZLIB_LIBS) $(ICU_LIBS) @LIBCDR_WIN32_RESOURCE@
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_DEPENDENCIES = libcdr-internal.la @LIBCDR_WIN32_RESOURCE@
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_SOURCES = \
	CDRDocument.cpp \
	CMXDocument.cpp

libcdr_internal_la_SOURCES = \
	CDRCollector.cpp \
	CDRContentCollector.cpp \
	CDRInternalStream.cpp \
	CDROutputElementList.cpp \
	CDRParser.cpp \
	CDRPath.cpp \
	CDRStylesCollector.cpp \
	CDRTransforms.cpp \
	CDRTypes.cpp \
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
