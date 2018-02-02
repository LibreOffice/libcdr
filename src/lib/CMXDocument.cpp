/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <libcdr/libcdr.h>
#include "CDRDocumentStructure.h"
#include "CMXParser.h"
#include "CDRContentCollector.h"
#include "CDRStylesCollector.h"
#include "libcdr_utils.h"

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\return A value that indicates whether the content from the input
stream is a Corel Draw Document that libcdr is able to parse
*/
CDRAPI bool libcdr::CMXDocument::isSupported(librevenge::RVNGInputStream *input)
try
{
  if (!input)
    return false;

  input->seek(0, librevenge::RVNG_SEEK_SET);
  unsigned riff = readU32(input);
  if (riff != CDR_FOURCC_RIFF && riff != CDR_FOURCC_RIFX)
    return false;
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  auto signature_c = (char)readU8(input);
  if (signature_c != 'C' && signature_c != 'c')
    return false;
  auto signature_d = (char)readU8(input);
  if (signature_d != 'M' && signature_d != 'm')
    return false;
  auto signature_r = (char)readU8(input);
  if (signature_r != 'X' && signature_r != 'x')
    return false;
  return true;
}
catch (...)
{
  return false;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
CDRPaintInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param painter A CDRPainterInterface implementation
\return A value that indicates whether the parsing was successful
*/
CDRAPI bool libcdr::CMXDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter)
{
  if (!input || !painter)
    return false;

  input->seek(0, librevenge::RVNG_SEEK_SET);
  CDRParserState ps;
  CDRStylesCollector stylesCollector(ps);
  CMXParserState parserState;
  CMXParser stylesParser(&stylesCollector, parserState);
  bool retVal = stylesParser.parseRecords(input);
  if (ps.m_pages.empty())
    retVal = false;
  if (retVal)
  {
    input->seek(0, librevenge::RVNG_SEEK_SET);
    CDRContentCollector contentCollector(ps, painter, false);
    CMXParser contentParser(&contentCollector, parserState);
    retVal = contentParser.parseRecords(input);
  }
  return retVal;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
