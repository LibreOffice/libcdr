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

uint8_t libcdr::readU8(WPXInputStream *input)
{
  if (!input || input->atEOS())
    throw EndOfStreamException();
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint8_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint8_t))
    return *(uint8_t const *)(p);
  throw EndOfStreamException();
}

uint16_t libcdr::readU16(WPXInputStream *input)
{
  uint16_t p0 = (uint16_t)readU8(input);
  uint16_t p1 = (uint16_t)readU8(input);
  return (uint16_t)(p0|(p1<<8));
}

uint32_t libcdr::readU32(WPXInputStream *input)
{
  uint32_t p0 = (uint32_t)readU8(input);
  uint32_t p1 = (uint32_t)readU8(input);
  uint32_t p2 = (uint32_t)readU8(input);
  uint32_t p3 = (uint32_t)readU8(input);
  return (uint32_t)(p0|(p1<<8)|(p2<<16)|(p3<<24));
}

int32_t libcdr::readS32(WPXInputStream *input)
{
  return (int32_t)readU32(input);
}

uint64_t libcdr::readU64(WPXInputStream *input)
{
  uint64_t p0 = (uint64_t)readU8(input);
  uint64_t p1 = (uint64_t)readU8(input);
  uint64_t p2 = (uint64_t)readU8(input);
  uint64_t p3 = (uint64_t)readU8(input);
  uint64_t p4 = (uint64_t)readU8(input);
  uint64_t p5 = (uint64_t)readU8(input);
  uint64_t p6 = (uint64_t)readU8(input);
  uint64_t p7 = (uint64_t)readU8(input);
  return (uint64_t)(p0|(p1<<8)|(p2<<16)|(p3<<24)|(p4<<32)|(p5<<40)|(p6<<48)|(p7<<56));
}

double libcdr::readDouble(WPXInputStream *input)
{
  union
  {
    uint64_t u;
    double d;
  } tmpUnion;

  tmpUnion.u = readU64(input);

  return tmpUnion.d;
}

::WPXString libcdr::readFourCC(WPXInputStream *input)
{
  ::WPXString tmpStr;
  for (unsigned char i = 0; i<4 && !input->atEOS(); i++)
    tmpStr.append((char)readU8(input));
  return tmpStr;
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
