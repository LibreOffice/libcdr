if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_includedir = $(includedir)/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@/libcdr
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_include_HEADERS = \
	libcdr.h \
	CDRDocument.h \
	CDRStringVector.h

AM_CXXFLAGS = $(LIBCDR_CXXFLAGS) $(ZLIB_CFLAGS) $(DEBUG_CXXFLAGS)

libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_LIBADD  = $(LIBCDR_LIBS) $(ZLIB_LIBS) @LIBCDR_WIN32_RESOURCE@
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_DEPENDENCIES = @LIBCDR_WIN32_RESOURCE@
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no_undefined
libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_SOURCES = \
	CDRCollector.cpp \
	CDRDocument.cpp \
	CDRInternalStream.cpp \
	CDROutputElementList.cpp \
	CDRParser.cpp \
	CDRPath.cpp \
	CDRStringVector.cpp \
	CDRSVGGenerator.cpp \
	CDRTypes.cpp \
	libcdr_utils.cpp \
	CDRCollector.h \
	CDRColorProfiles.h \
	CDRInternalStream.h \
	CDROutputElementList.h \
	CDRParser.h \
	CDRPath.h \
	CDRSVGGenerator.h \
	CDRTypes.h \
	libcdr_utils.h

if OS_WIN32

@LIBCDR_WIN32_RESOURCE@ : libcdr.rc $(libcdr_@CDR_MAJOR_VERSION@_@CDR_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libcdr.rc @LIBCDR_WIN32_RESOURCE@

endif


EXTRA_DIST = \
	libcdr.rc.in \
	makefile.mk

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libcdr.rc
