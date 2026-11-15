/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <memory>
#include <string>

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
  auto signature_c = (char)readU8(input);
  if (signature_c != 'C' && signature_c != 'c')
    return 0;
  auto signature_d = (char)readU8(input);
  if (signature_d != 'D' && signature_d != 'd')
    return 0;
  auto signature_r = (char)readU8(input);
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
\param input_ The input stream
\return A value that indicates whether the content from the input
stream is a Corel Draw Document that libcdr is able to parse
*/
CDRAPI bool libcdr::CDRDocument::isSupported(librevenge::RVNGInputStream *input_) try
{
  if (!input_)
    return false;

  input_->seek(0, librevenge::RVNG_SEEK_SET);
  if (getCDRVersion(input_))
    return true;
  if (!input_->isStructured())
    return false;
  std::unique_ptr<librevenge::RVNGInputStream> input(input_->getSubStreamByName("content/riffData.cdr"));
  if (!input)
    input.reset(input_->getSubStreamByName("content/root.dat"));
  input_->seek(0, librevenge::RVNG_SEEK_SET);
  if (!input)
    return false;
  input->seek(0, librevenge::RVNG_SEEK_SET);
  return getCDRVersion(input.get()) != 0;
}
catch (...)
{
  return false;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
CDRPaintInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input_ The input stream
\param painter A CDRPainterInterface implementation
\return A value that indicates whether the parsing was successful
*/
CDRAPI bool libcdr::CDRDocument::parse(librevenge::RVNGInputStream *input_, librevenge::RVNGDrawingInterface *painter)
{
  if (!input_ || !painter)
    return false;

  input_->seek(0, librevenge::RVNG_SEEK_SET);
  bool retVal = false;
  unsigned version = 0;
  try
  {
    version = getCDRVersion(input_);
    if (version)
    {
      input_->seek(0, librevenge::RVNG_SEEK_SET);
      CDRParserState ps;
      std::vector<std::unique_ptr<librevenge::RVNGInputStream>> dummyDataStreams;
      CDRStylesCollector stylesCollector(ps);
      CDRParser stylesParser(dummyDataStreams, &stylesCollector);
      if (version >= 300)
        retVal = stylesParser.parseRecords(input_);
      else
        retVal = stylesParser.parseWaldo(input_);
      if (ps.m_pages.empty())
        retVal = false;
      if (retVal)
      {
        input_->seek(0, librevenge::RVNG_SEEK_SET);
        CDRContentCollector contentCollector(ps, painter);
        CDRParser contentParser(dummyDataStreams, &contentCollector);
        if (version >= 300)
          retVal = contentParser.parseRecords(input_);
        else
          retVal = contentParser.parseWaldo(input_);
      }
      return retVal;
    }
  }
  catch (libcdr::EndOfStreamException const &)
  {
    // This can only happen if isSupported() has not been called before
    return false;
  }

  librevenge::RVNGInputStream *tmpInput = input_;
  std::shared_ptr<librevenge::RVNGInputStream> input;
  try
  {
    std::vector<std::string> dataFiles;
    if (tmpInput->isStructured())
    {
      tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
      input.reset(tmpInput->getSubStreamByName("content/riffData.cdr"));
      if (!input)
      {
        tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
        input.reset(tmpInput->getSubStreamByName("content/root.dat"));
        if (input)
        {
          std::unique_ptr<librevenge::RVNGInputStream> tmpStream(tmpInput->getSubStreamByName("content/dataFileList.dat"));
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
              dataFiles.push_back(std::move(dataFileName));
          }
        }
      }
    }
    std::vector<std::unique_ptr<librevenge::RVNGInputStream>> dataStreams;
    dataStreams.reserve(dataFiles.size());
    for (const auto &dataFile : dataFiles)
    {
      std::string streamName("content/data/");
      streamName += dataFile;
      CDR_DEBUG_MSG(("Extracting stream: %s\n", streamName.c_str()));
      tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
      std::unique_ptr<librevenge::RVNGInputStream> strm(tmpInput->getSubStreamByName(streamName.c_str()));
      dataStreams.push_back(std::move(strm));
    }
    if (!input)
      input = makeUnownedStreamPtr(tmpInput);
    CDRParserState ps;
    {
      // libcdr extension to the getSubStreamByName. Will extract the first stream in the
      // given directory
      tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
      std::unique_ptr<librevenge::RVNGInputStream> cmykProfile(tmpInput->getSubStreamByName("color/profiles/cmyk/"));
      if (cmykProfile)
        ps.setColorTransform(cmykProfile.get());
    }
    {
      tmpInput->seek(0, librevenge::RVNG_SEEK_SET);
      std::unique_ptr<librevenge::RVNGInputStream> rgbProfile(tmpInput->getSubStreamByName("color/profiles/rgb/"));
      if (rgbProfile)
        ps.setColorTransform(rgbProfile.get());
    }
    CDRStylesCollector stylesCollector(ps);
    CDRParser stylesParser(dataStreams, &stylesCollector);
    input->seek(0, librevenge::RVNG_SEEK_SET);
    retVal = stylesParser.parseRecords(input.get());
    if (ps.m_pages.empty())
      retVal = false;
    if (retVal)
    {
      input->seek(0, librevenge::RVNG_SEEK_SET);
      CDRContentCollector contentCollector(ps, painter);
      CDRParser contentParser(dataStreams, &contentCollector);
      retVal = contentParser.parseRecords(input.get());
    }
  }
  catch (libcdr::EndOfStreamException const &)
  {
    retVal = false;
  }
  return retVal;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
