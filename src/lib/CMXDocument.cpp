/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string>
#include <string.h>
#include <libcdr/libcdr.h>
#include "CDRDocumentStructure.h"
#include "CMXParser.h"
#include "CDRSVGGenerator.h"
#include "CDRContentCollector.h"
#include "CDRStylesCollector.h"
#include "CDRZipStream.h"
#include "libcdr_utils.h"

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\return A value that indicates whether the content from the input
stream is a Corel Draw Document that libcdr is able to parse
*/
bool libcdr::CMXDocument::isSupported(WPXInputStream *input)
try
{
  input->seek(0, WPX_SEEK_SET);
  unsigned riff = readU32(input);
  if (riff != CDR_FOURCC_RIFF && riff != CDR_FOURCC_RIFX)
    return false;
  input->seek(4, WPX_SEEK_CUR);
  char signature_c = (char)readU8(input);
  if (signature_c != 'C' && signature_c != 'c')
    return false;
  char signature_d = (char)readU8(input);
  if (signature_d != 'M' && signature_d != 'm')
    return false;
  char signature_r = (char)readU8(input);
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
bool libcdr::CMXDocument::parse(::WPXInputStream *input, libwpg::WPGPaintInterface *painter)
{
  input->seek(0, WPX_SEEK_SET);
  CDRParserState ps;
  CDRStylesCollector stylesCollector(ps);
  CMXParser stylesParser(&stylesCollector);
  bool retVal = stylesParser.parseRecords(input);
  if (ps.m_pages.empty())
    retVal = false;
  if (retVal)
  {
    input->seek(0, WPX_SEEK_SET);
    CDRContentCollector contentCollector(ps, painter);
    CMXParser contentParser(&contentCollector);
    retVal = contentParser.parseRecords(input);
  }
  return retVal;
}

/**
Parses the input stream content and generates a valid Scalable Vector Graphics
Provided as a convenience function for applications that support SVG internally.
\param input The input stream
\param output The output string whose content is the resulting SVG
\return A value that indicates whether the SVG generation was successful.
*/
bool libcdr::CMXDocument::generateSVG(::WPXInputStream *input, libcdr::CDRStringVector &output)
{
  libcdr::CDRSVGGenerator generator(output);
  bool result = libcdr::CMXDocument::parse(input, &generator);
  return result;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
