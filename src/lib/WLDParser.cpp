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
#include <stdlib.h>
#include "libcdr_utils.h"
#include "WLDParser.h"
#include "CDRCollector.h"

libcdr::WLDParser::WLDParser(WPXInputStream *input, libcdr::CDRCollector *collector)
  : m_input(input),
    m_collector(collector) {}

libcdr::WLDParser::~WLDParser()
{
}

bool libcdr::WLDParser::parseDocument()
{
  try
  {
    m_input->seek(0, WPX_SEEK_SET);
    unsigned magic = readU32(m_input);
    if (magic != 0x6c4c57)
      return false;
    unsigned offset = readU32(m_input);
    m_input->seek(offset, WPX_SEEK_SET);
    readMcfg();
    m_input->seek(offset+275, WPX_SEEK_SET);
    for(unsigned i = 0; !m_input->atEOS(); ++i)
    {
      unsigned length = readU32(m_input);
      long startPosition = m_input->tell();
	  readRecord(i, length);
      m_input->seek(startPosition + (long)length, WPX_SEEK_SET);
    }
    return true;
  }
  catch (...)
  {
    return false;
  }
}

void libcdr::WLDParser::readMcfg()
{
  CDR_DEBUG_MSG(("WLDParser::readMcfg, offset 0x%x\n", (unsigned)m_input->tell()));
}

void libcdr::WLDParser::readRecord(unsigned id, unsigned length)
{
  CDR_DEBUG_MSG(("WLDParser::readRecord, offset 0x%x, id %x, length %u\n", (unsigned)m_input->tell(), id, length));
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
