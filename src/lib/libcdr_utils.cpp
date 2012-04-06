/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* libcdr
 * Version: MPL 1.1 / GPLv2+ / LGPLv2+
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License or as specified alternatively below. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Major Contributor(s):
 * Copyright (C) 2012 Fridrich Strba <fridrich.strba@bluewin.ch>
 * Copyright (C) 2011 Eilidh McAdam <tibbylickle@gmail.com>
 *
 *
 * All Rights Reserved.
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPLv2+"), or
 * the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
 * in which case the provisions of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
 */

#include "libcdr_utils.h"

#define CDR_NUM_ELEMENTS(array) sizeof(array)/sizeof(array[0])

uint8_t libcdr::readU8(WPXInputStream *input, bool /* bigEndian */)
{
  if (!input || input->atEOS())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint8_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint8_t))
    return *(uint8_t const *)(p);
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

uint16_t libcdr::readU16(WPXInputStream *input, bool bigEndian)
{
  if (!input || input->atEOS())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint16_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint16_t))
  {
    if (bigEndian)
      return (uint16_t)p[1]|((uint16_t)p[0]<<8);
    return (uint16_t)p[0]|((uint16_t)p[1]<<8);
  }
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

uint32_t libcdr::readU32(WPXInputStream *input, bool bigEndian)
{
  if (!input || input->atEOS())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint32_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint32_t))
  {
    if (bigEndian)
      return (uint32_t)p[3]|((uint32_t)p[2]<<8)|((uint32_t)p[1]<<16)|((uint32_t)p[0]<<24);
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
  }
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int32_t libcdr::readS32(WPXInputStream *input, bool bigEndian)
{
  return (int32_t)readU32(input, bigEndian);
}

uint64_t libcdr::readU64(WPXInputStream *input, bool bigEndian)
{
  if (!input || input->atEOS())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint64_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint64_t))
  {
    if (bigEndian)
      return (uint64_t)p[7]|((uint64_t)p[6]<<8)|((uint64_t)p[5]<<16)|((uint64_t)p[4]<<24)|((uint64_t)p[3]<<32)|((uint64_t)p[2]<<40)|((uint64_t)p[1]<<48)|((uint64_t)p[0]<<56);
    return (uint64_t)p[0]|((uint64_t)p[1]<<8)|((uint64_t)p[2]<<16)|((uint64_t)p[3]<<24)|((uint64_t)p[4]<<32)|((uint64_t)p[5]<<40)|((uint64_t)p[6]<<48)|((uint64_t)p[7]<<56);
  }
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

double libcdr::readDouble(WPXInputStream *input, bool bigEndian)
{
  union
  {
    uint64_t u;
    double d;
  } tmpUnion;

  tmpUnion.u = readU64(input, bigEndian);

  return tmpUnion.d;
}

#ifdef DEBUG
const char *libcdr::toFourCC(unsigned value, bool bigEndian)
{
  static char sValue[5] = { 0, 0, 0, 0, 0 };
  if (bigEndian)
  {
    sValue[3] = value & 0xff;
    sValue[2] = (value & 0xff00) >> 8;
    sValue[1] = (value & 0xff0000) >> 16;
    sValue[0] = (value & 0xff000000) >> 24;
  }
  else
  {
    sValue[0] = value & 0xff;
    sValue[1] = (value & 0xff00) >> 8;
    sValue[2] = (value & 0xff0000) >> 16;
    sValue[3] = (value & 0xff000000) >> 24;
  }
  return sValue;
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
