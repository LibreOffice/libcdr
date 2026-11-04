/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// The code page guess of text without a charset; ICU's detector guesses differently
#ifndef WITH_ICU

#include <cstring>
#include <string>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libcdr_utils.h"

namespace test
{

class CDRTextEncodingTest : public CPPUNIT_NS::TestFixture
{
private:
  CPPUNIT_TEST_SUITE(CDRTextEncodingTest);
  CPPUNIT_TEST(testMacCentralEurope);
  CPPUNIT_TEST_SUITE_END();

  void testMacCentralEurope();
};

static std::string convert(const char *bytes)
{
  librevenge::RVNGString text;
  libcdr::appendCharacters(text, std::vector<unsigned char>(bytes, bytes + strlen(bytes)), 0);
  return text.cstr();
}

void CDRTextEncodingTest::testMacCentralEurope()
{
  // windows-1252 punctuation that is Mac Central European letters
  CPPUNIT_ASSERT_EQUAL(std::string("don’t"), convert("don\x92t"));
  CPPUNIT_ASSERT_EQUAL(std::string("Wait… it’s — done"), convert("Wait\x85 it\x92s \x97 done"));
  CPPUNIT_ASSERT_EQUAL(std::string("“Hi” – it’s š‡"), convert("\x93Hi\x94 \x96 it\x92s \x9a\x87"));
  CPPUNIT_ASSERT_EQUAL(std::string("café"), convert("caf\xe9"));
  // CorelDRAW for Mac text
  CPPUNIT_ASSERT_EQUAL(std::string("árvíztükör fúrógép"), convert("\x87rv\x92zt\x9fk\x9ar f\x9cr\x97g\x8ep"));
  CPPUNIT_ASSERT_EQUAL(std::string("Éní"), convert("\x83n\x92"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(CDRTextEncodingTest);

}

#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
