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
#include <boost/scoped_ptr.hpp>
#include <libcdr/libcdr.h>
#include "CDRParser.h"
#include "CDRContentCollector.h"
#include "CDRStylesCollector.h"
#include "libcdr_utils.h"
#include "CDRDocumentStructure.h"

using namespace libcdr;

namespace
{

static unsigned getCDRVersion(librevenge::RVNGInputStream *input)
{
  unsigned riff = readU32(input);
  if ((riff & 0xffff) == 0x4c57) // "WL<micro>\0"
    return 200;
  if (riff != CDR_FOURCC_RIFF)
    return 0;
  input->seek(4, librevenge::RVNG_SEEK_CUR);
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
CDRAPI bool libcdr::CDRDocument::isSupported(librevenge::RVNGInputStream *input)
{
  if (!input)
    return false;

  librevenge::RVNGInputStream *tmpInput = input;
  try
  {
    input->seek(0, librevenge::RVNG_SEEK_SET);
    unsigned version = getCDRVersion(input);
    if (version)
      return true;
    if (tmpInput->isStructured())
    {
      input = tmpInput->getSubStreamByName("content/riffData.cdr");
      if (!input)
        input = tmpInput->getSubStreamByName("content/root.dat");
    }
    tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
    if (!input)
      return false;
    input->seek(0, librevenge::RVNG_SEEK_SET);
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
CDRAPI bool libcdr::CDRDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter)
{
  if (!input || !painter)
    return false;

  input->seek(0, librevenge::RVNG_SEEK_SET);
  bool retVal = false;
  unsigned version = 0;
  try
  {
    version = getCDRVersion(input);
    if (version)
    {
      input->seek(0, librevenge::RVNG_SEEK_SET);
      CDRParserState ps;
      CDRStylesCollector stylesCollector(ps);
      CDRParser stylesParser(std::vector<librevenge::RVNGInputStream *>(), &stylesCollector);
      if (version >= 300)
        retVal = stylesParser.parseRecords(input);
      else
        retVal = stylesParser.parseWaldo(input);
      if (ps.m_pages.empty())
        retVal = false;
      if (retVal)
      {
        input->seek(0, librevenge::RVNG_SEEK_SET);
        CDRContentCollector contentCollector(ps, painter);
        CDRParser contentParser(std::vector<librevenge::RVNGInputStream *>(), &contentCollector);
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

  librevenge::RVNGInputStream *tmpInput = input;
  std::vector<librevenge::RVNGInputStream *> dataStreams;
  try
  {
    std::vector<std::string> dataFiles;
    if (tmpInput->isStructured())
    {
      tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
      input = tmpInput->getSubStreamByName("content/riffData.cdr");
      if (!input)
      {
        tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
        input = tmpInput->getSubStreamByName("content/root.dat");
        if (input)
        {
          boost::scoped_ptr<librevenge::RVNGInputStream> tmpStream(tmpInput->getSubStreamByName("content/dataFileList.dat"));
          if (bool(tmpStream))
          {
            std::string dataFileName;
            while (!tmpStream->isEnd())
            {
              unsigned char character = readU8(tmpStream.get());
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
      tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
      dataStreams.push_back(tmpInput->getSubStreamByName(streamName.c_str()));
    }
    if (!input)
      input = tmpInput;
    CDRParserState ps;
    // libcdr extension to the getSubStreamByName. Will extract the first stream in the
    // given directory
    tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
    librevenge::RVNGInputStream *cmykProfile = tmpInput->getSubStreamByName("color/profiles/cmyk/");
    if (cmykProfile)
    {
      ps.setColorTransform(cmykProfile);
      delete cmykProfile;
    }
    tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
    librevenge::RVNGInputStream *rgbProfile = tmpInput->getSubStreamByName("color/profiles/rgb/");
    if (rgbProfile)
    {
      ps.setColorTransform(rgbProfile);
      delete rgbProfile;
    }
    CDRStylesCollector stylesCollector(ps);
    CDRParser stylesParser(dataStreams, &stylesCollector);
    input->seek(0, librevenge::RVNG_SEEK_SET);
    retVal = stylesParser.parseRecords(input);
    if (ps.m_pages.empty())
      retVal = false;
    if (retVal)
    {
      input->seek(0, librevenge::RVNG_SEEK_SET);
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
  for (std::vector<librevenge::RVNGInputStream *>::iterator iter = dataStreams.begin(); iter != dataStreams.end(); ++iter)
    delete *iter;
  return retVal;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
