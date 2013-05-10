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

#include <string>
#include <string.h>
#include <libcdr/libcdr.h>
#include "CDRParser.h"
#include "CDRSVGGenerator.h"
#include "CDRContentCollector.h"
#include "CDRStylesCollector.h"
#include "CDRZipStream.h"
#include "libcdr_utils.h"
#include "CDRDocumentStructure.h"

using namespace libcdr;

namespace
{

static unsigned getCDRVersion(WPXInputStream *input)
{
  unsigned riff = readU32(input);
  if ((riff & 0xffff) == 0x4c57) // "WL<micro>\0"
    return 200;
  if (riff != CDR_FOURCC_RIFF)
    return 0;
  input->seek(4, WPX_SEEK_CUR);
  char signature_c = (char)readU8(input);
  if (signature_c != 'C' && signature_c != 'c')
    return 0;
  char signature_d = (char)readU8(input);
  if (signature_d != 'D' && signature_d != 'd')
    return 0;
  char signature_r = (char)readU8(input);
  if (signature_r != 'R' && signature_r != 'r')
    return 0;
  unsigned char c = readU8(input);
  if (c == 0x20)
    return 300;
  else if (c < 0x31)
    return 0;
  else if (c < 0x3a)
    return 100 * (c - 0x30);
  else if (c < 0x41)
    return 0;
  return 100 * (c - 0x37);
}

} // anonymous namespace

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\return A value that indicates whether the content from the input
stream is a Corel Draw Document that libcdr is able to parse
*/
bool libcdr::CDRDocument::isSupported(WPXInputStream *input)
{
  WPXInputStream *tmpInput = input;
  try
  {
    input->seek(0, WPX_SEEK_SET);
    unsigned version = getCDRVersion(input);
    if (version)
      return true;
    CDRZipStream zinput(input);
    // Yes, we are kidnapping here the OLE document API and extending
    // it to support also zip files.
    if (zinput.isOLEStream())
    {
      input = zinput.getDocumentOLEStream("content/riffData.cdr");
      if (!input)
        input = zinput.getDocumentOLEStream("content/root.dat");
    }
    if (!input)
      return false;
    input->seek(0, WPX_SEEK_SET);
    version = getCDRVersion(input);
    if (input != tmpInput)
      delete input;
    input = tmpInput;
    if (!version)
      return false;
    return true;
  }
  catch (...)
  {
    if (input != tmpInput)
      delete input;
    return false;
  }
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
CDRPaintInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param painter A CDRPainterInterface implementation
\return A value that indicates whether the parsing was successful
*/
bool libcdr::CDRDocument::parse(::WPXInputStream *input, libwpg::WPGPaintInterface *painter)
{
  input->seek(0, WPX_SEEK_SET);
  bool retVal = false;
  unsigned version = 0;
  try
  {
    version = getCDRVersion(input);
    if (version)
    {
      input->seek(0, WPX_SEEK_SET);
      CDRParserState ps;
      CDRStylesCollector stylesCollector(ps);
      CDRParser stylesParser(std::vector<WPXInputStream *>(), &stylesCollector);
      if (version >= 300)
        retVal = stylesParser.parseRecords(input);
      else
        retVal = stylesParser.parseWaldo(input);
      if (ps.m_pages.empty())
        retVal = false;
      if (retVal)
      {
        input->seek(0, WPX_SEEK_SET);
        CDRContentCollector contentCollector(ps, painter);
        CDRParser contentParser(std::vector<WPXInputStream *>(), &contentCollector);
        if (version >= 300)
          retVal = contentParser.parseRecords(input);
        else
          retVal = contentParser.parseWaldo(input);
      }
      return retVal;
    }
  }
  catch (libcdr::EndOfStreamException const &)
  {
    // This can only happen if isSupported() has not been called before
    return false;
  }

  WPXInputStream *tmpInput = input;
  std::vector<WPXInputStream *> dataStreams;
  try
  {
    CDRZipStream zinput(input);
    bool isZipDocument = zinput.isOLEStream();
    std::vector<std::string> dataFiles;
    if (isZipDocument)
    {
      input = zinput.getDocumentOLEStream("content/riffData.cdr");
      if (!input)
      {
        input = zinput.getDocumentOLEStream("content/root.dat");
        if (input)
        {
          WPXInputStream *tmpStream = zinput.getDocumentOLEStream("content/dataFileList.dat");
          if (tmpStream)
          {
            std::string dataFileName;
            while (!tmpStream->atEOS())
            {
              unsigned char character = readU8(tmpStream);
              if (character == 0x0a)
              {
                dataFiles.push_back(dataFileName);
                dataFileName.clear();
              }
              else
                dataFileName += (char)character;
            }
            if (!dataFileName.empty())
              dataFiles.push_back(dataFileName);
          }
        }
      }
    }
    dataStreams.reserve(dataFiles.size());
    for (unsigned i=0; i<dataFiles.size(); i++)
    {
      std::string streamName("content/data/");
      streamName += dataFiles[i];
      CDR_DEBUG_MSG(("Extracting stream: %s\n", streamName.c_str()));
      dataStreams.push_back(zinput.getDocumentOLEStream(streamName.c_str()));
    }
    if (!input)
      input = tmpInput;
    input->seek(0, WPX_SEEK_SET);
    CDRParserState ps;
    // libcdr extension to the getDocumentOLEStream. Will extract the first stream in the
    // given directory
    WPXInputStream *cmykProfile = zinput.getDocumentOLEStream("color/profiles/cmyk/");
    if (cmykProfile)
    {
      ps.setColorTransform(cmykProfile);
      delete cmykProfile;
    }
    WPXInputStream *rgbProfile = zinput.getDocumentOLEStream("color/profiles/rgb/");
    if (rgbProfile)
    {
      ps.setColorTransform(rgbProfile);
      delete rgbProfile;
    }
    CDRStylesCollector stylesCollector(ps);
    CDRParser stylesParser(dataStreams, &stylesCollector);
    retVal = stylesParser.parseRecords(input);
    if (ps.m_pages.empty())
      retVal = false;
    if (retVal)
    {
      input->seek(0, WPX_SEEK_SET);
      CDRContentCollector contentCollector(ps, painter);
      CDRParser contentParser(dataStreams, &contentCollector);
      retVal = contentParser.parseRecords(input);
    }
  }
  catch (libcdr::EndOfStreamException const &)
  {
    retVal = false;
  }
  if (input != tmpInput)
    delete input;
  input = tmpInput;
  for (std::vector<WPXInputStream *>::iterator iter = dataStreams.begin(); iter != dataStreams.end(); ++iter)
    delete *iter;
  return retVal;
}

/**
Parses the input stream content and generates a valid Scalable Vector Graphics
Provided as a convenience function for applications that support SVG internally.
\param input The input stream
\param output The output string whose content is the resulting SVG
\return A value that indicates whether the SVG generation was successful.
*/
bool libcdr::CDRDocument::generateSVG(::WPXInputStream *input, libcdr::CDRStringVector &output)
{
  libcdr::CDRSVGGenerator generator(output);
  bool result = libcdr::CDRDocument::parse(input, &generator);
  return result;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
