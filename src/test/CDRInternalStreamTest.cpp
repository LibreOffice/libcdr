/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <algorithm>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <librevenge-stream/librevenge-stream.h>

#include "CDRInternalStream.h"

namespace test
{

using libcdr::CDRInternalStream;

class CDRInternalStreamTest : public CPPUNIT_NS::TestFixture
{
public:
  virtual void setUp();
  virtual void tearDown();

private:
  CPPUNIT_TEST_SUITE(CDRInternalStreamTest);
  CPPUNIT_TEST(testRead);
  CPPUNIT_TEST(testSeek);
  CPPUNIT_TEST_SUITE_END();

private:
  void testRead();
  void testSeek();
};

void CDRInternalStreamTest::setUp()
{
}

void CDRInternalStreamTest::tearDown()
{
}

void CDRInternalStreamTest::testRead()
{
  const unsigned char data[] = "abc dee fgh";
  CDRInternalStream strm(std::vector<unsigned char>(data, data + sizeof(data)));

  CPPUNIT_ASSERT_MESSAGE("stream is already exhausted before starting to read", !strm.isEnd());

  for (int i = 0; sizeof(data) != i; ++i)
  {
    unsigned long readBytes = 0;
    const unsigned char *s = strm.read(1, readBytes);

    CPPUNIT_ASSERT(1 == readBytes);
    CPPUNIT_ASSERT_EQUAL(data[i], s[0]);
    CPPUNIT_ASSERT(((sizeof(data) - 1) == i) || !strm.isEnd());
  }

  CPPUNIT_ASSERT_MESSAGE("reading did not exhaust the stream", strm.isEnd());

  strm.seek(0, librevenge::RVNG_SEEK_SET);

  unsigned long readBytes = 0;
  const unsigned char *s = strm.read(sizeof(data), readBytes);
  CPPUNIT_ASSERT(sizeof(data) == readBytes);
  CPPUNIT_ASSERT(std::equal(data, data + sizeof(data), s));
}

void CDRInternalStreamTest::testSeek()
{
  const unsigned char data[] = "abc dee fgh";
  CDRInternalStream strm(std::vector<unsigned char>(data, data + sizeof(data)));

  strm.seek(0, librevenge::RVNG_SEEK_SET);
  CPPUNIT_ASSERT(0 == strm.tell());
  strm.seek(2, librevenge::RVNG_SEEK_SET);
  CPPUNIT_ASSERT(2 == strm.tell());

  strm.seek(1, librevenge::RVNG_SEEK_CUR);
  CPPUNIT_ASSERT(3 == strm.tell());
  strm.seek(-2, librevenge::RVNG_SEEK_CUR);
  CPPUNIT_ASSERT(1 == strm.tell());

  CPPUNIT_ASSERT(!strm.isEnd());
  CPPUNIT_ASSERT(0 == strm.seek(0, librevenge::RVNG_SEEK_END));
  CPPUNIT_ASSERT(strm.isEnd());
  CPPUNIT_ASSERT(sizeof(data) == strm.tell());
  CPPUNIT_ASSERT(0 != strm.seek(1, librevenge::RVNG_SEEK_END)); // cannot seek after the end
  CPPUNIT_ASSERT(strm.isEnd());
  CPPUNIT_ASSERT(0 == strm.seek(-1, librevenge::RVNG_SEEK_END)); // but can seek before it
  CPPUNIT_ASSERT(!strm.isEnd());
  CPPUNIT_ASSERT((sizeof(data) - 1) == strm.tell());
}

CPPUNIT_TEST_SUITE_REGISTRATION(CDRInternalStreamTest);

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
