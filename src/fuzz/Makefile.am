noinst_PROGRAMS = cdrfuzzer cmxfuzzer

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

fuzzer_ldadd = \
	$(top_builddir)/src/lib/libcdr-@CDR_MAJOR_VERSION@.@CDR_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	-lFuzzingEngine

cdrfuzzer_LDADD = $(fuzzer_ldadd)

cdrfuzzer_SOURCES = \
	cdrfuzzer.cpp

cmxfuzzer_LDADD = $(fuzzer_ldadd)

cmxfuzzer_SOURCES = \
	cmxfuzzer.cpp
