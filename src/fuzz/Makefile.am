noinst_PROGRAMS = cdrfuzzer

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

cdrfuzzer_LDADD = \
	$(top_builddir)/src/lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	-lFuzzingEngine

cdrfuzzer_SOURCES = \
	cdrfuzzer.cpp
