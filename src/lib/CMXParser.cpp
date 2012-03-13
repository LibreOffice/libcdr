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
 * Copyright (C) 2011 Fridrich Strba <fridrich.strba@bluewin.ch>
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

#include <libwpd-stream/libwpd-stream.h>
#include <locale.h>
#include <math.h>
#include <set>
#include <string.h>
#include "libcdr_utils.h"
#include "CDRInternalStream.h"
#include "CMXParser.h"
#include "CDRCollector.h"

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

libcdr::CMXParser::CMXParser(WPXInputStream *input, libcdr::CDRCollector *collector)
  : m_input(input),
    m_collector(collector) {}

libcdr::CMXParser::~CMXParser()
{
}

bool libcdr::CMXParser::parseRecords(WPXInputStream *input, unsigned level)
{
  if (!input)
  {
    return false;
  }
  m_collector->collectLevel(level);
  while (!input->atEOS())
  {
    if (!parseRecord(input, level))
      return false;
  }
  return true;
}

bool libcdr::CMXParser::parseRecord(WPXInputStream *input, unsigned level)
{
  if (!input)
  {
    return false;
  }
  try
  {
    m_collector->collectLevel(level);
    while (!input->atEOS() && readU8(input) == 0)
    {
    }
    if (!input->atEOS())
      input->seek(-1, WPX_SEEK_CUR);
    else
      return true;
    WPXString fourCC = readFourCC(input);
    unsigned length = readU32(input);
    unsigned long position = input->tell();
    WPXString listType;

    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%i)\n", level, fourCC.cstr(), length, length));

    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      listType = readFourCC(input);
      CDR_DEBUG_MSG(("CMX listType: %s\n", listType.cstr()));
      unsigned dataSize = length-4;
      CDRInternalStream tmpStream(input, dataSize);
      if (!parseRecords(&tmpStream, level+1))
        return false;
    }
    else
      readRecord(fourCC, length, input);

    input->seek(position + length, WPX_SEEK_SET);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

void libcdr::CMXParser::readRecord(WPXString /* fourCC */, unsigned length, WPXInputStream *input)
{
  long recordStart = input->tell();
  input->seek(recordStart + length, WPX_SEEK_CUR);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
