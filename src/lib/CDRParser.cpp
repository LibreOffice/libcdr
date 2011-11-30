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

#include <libwpd-stream/libwpd-stream.h>
#include <locale.h>
#include <sstream>
#include <string>
#include <cmath>
#include <set>
#include <string.h>
#include "libcdr_utils.h"
#include "CDRInternalStream.h"
#include "CDRParser.h"

libcdr::CDRParser::CDRParser(WPXInputStream *input, libwpg::WPGPaintInterface *painter)
  : m_input(input),
    m_painter(painter) {}

libcdr::CDRParser::~CDRParser()
{
}

bool libcdr::CDRParser::parseRecords(WPXInputStream *input)
{
  if (!input)
  {
    return false;
  }
  while (!input->atEOS())
  {
    if (!parseRecord(input))
      return false;
  }
  return true;
}

bool libcdr::CDRParser::parseRecord(WPXInputStream *input)
{
  if (!input)
  {
    return false;
  }
  try
  {
    WPXString fourCC = readFourCC(input);
    unsigned length = readU32(input);
    unsigned long position = input->tell();
    CDR_DEBUG_MSG(("Record: %s, length: 0x%.8x (%i)\n", fourCC.cstr(), length, length));
    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      WPXString listType = readFourCC(input);
      CDR_DEBUG_MSG(("CDR listType: %s\n", listType.cstr()));
      bool compressed = (listType == "cmpr" ? true : false);
      if (length > 4)
      {
        CDRInternalStream tmpStream(input, length - 4, compressed);
        if (!parseRecords(&tmpStream))
          return false;
      }
    }

    if (length)
      input->seek(position + length, WPX_SEEK_SET);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
