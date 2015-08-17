## -*- Mode: make; tab-width: 4; indent-tabs-mode: tabs -*-

target_test = test

check_PROGRAMS = $(target_test)

AM_CXXFLAGS = \
	-I$(top_srcdir)/inc \
	-I$(top_srcdir)/src/lib \
	$(CPPUNIT_CFLAGS) \
	$(LCMS2_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(BOOST_CFLAGS) \
	$(DEBUG_CXXFLAGS)

test_LDFLAGS = -L$(top_srcdir)/src/lib
test_LDADD = \
	$(top_builddir)/src/lib/libcdr-internal.la \
	$(CPPUNIT_LIBS) \
	$(ICU_LIBS) \
	$(LCMS2_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	$(ZLIB_LIBS)

test_SOURCES = \
	CDRInternalStreamTest.cpp \
	test.cpp

TESTS = $(target_test)

## vim:set shiftwidth=4 tabstop=4 noexpandtab:
