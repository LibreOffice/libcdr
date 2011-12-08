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

#define DUMP_PREVIEW_IMAGE 0

libcdr::CDRParser::CDRParser(WPXInputStream *input, libwpg::WPGPaintInterface *painter)
  : m_input(input),
    m_painter(painter),
    m_isListTypePage(false),
    m_isPageOpened(false) {}

libcdr::CDRParser::~CDRParser()
{
  _closePage();
}

bool libcdr::CDRParser::parseRecords(WPXInputStream *input, unsigned *blockLengths, unsigned level)
{
  if (!input)
  {
    return false;
  }
  while (!input->atEOS())
  {
    if (!parseRecord(input, blockLengths, level))
      return false;
  }
  return true;
}

void libcdr::CDRParser::_closePage()
{
  if (m_isPageOpened)
  {
    m_painter->endGraphics();
    m_isPageOpened = false;
  }
}

bool libcdr::CDRParser::parseRecord(WPXInputStream *input, unsigned *blockLengths, unsigned level)
{
  if (!input)
  {
    return false;
  }
  try
  {
    while (readU8(input) == 0);
    if (!input->atEOS())
      input->seek(-1, WPX_SEEK_CUR);
    WPXString fourCC = readFourCC(input);
    unsigned length = readU32(input);
    if (blockLengths)
      length=blockLengths[length];
    unsigned long position = input->tell();
    WPXString listType;
    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      listType = readFourCC(input);
      if (listType == "stlt")
        fourCC = listType;
      if (listType == "page")
      {
        _closePage();
        m_isListTypePage = true;
      }
      else
        m_isListTypePage = false;

    }
    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%i)\n", level, fourCC.cstr(), length, length));

    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      CDR_DEBUG_MSG(("CDR listType: %s\n", listType.cstr()));
      unsigned cmprsize = length-4;
      unsigned ucmprsize = length-4;
      unsigned blcks = 0;
      if (listType == "cmpr")
      {
        cmprsize = readU32(input);
        ucmprsize = readU32(input);
        blcks = readU32(input);
        input->seek(4, WPX_SEEK_CUR);
        if (readFourCC(input) != "CPng")
          return false;
        if (readU16(input) != 1)
          return false;
        if (readU16(input) != 4)
          return false;
      }

      bool compressed = (listType == "cmpr" ? true : false);
      CDRInternalStream tmpStream(input, cmprsize, compressed);
      if (!compressed)
      {
        if (!parseRecords(&tmpStream, blockLengths, level+1))
          return false;
      }
      else
      {
        std::vector<unsigned> tmpBlockLengths;
        unsigned blocksLength = length + position - input->tell();
        CDRInternalStream tmpBlocksStream(input, blocksLength, compressed);
        while (!tmpBlocksStream.atEOS())
          tmpBlockLengths.push_back(readU32(&tmpBlocksStream));
        if (!parseRecords(&tmpStream, tmpBlockLengths.size() ? &tmpBlockLengths[0] : 0, level+1))
          return false;
      }
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

void libcdr::CDRParser::readRecord(WPXString fourCC, unsigned length, WPXInputStream *input)
{
  if (fourCC == "DISP")
  {
    WPXBinaryData previewImage;
    previewImage.append(0x42);
    previewImage.append(0x4d);

    previewImage.append((unsigned char)((length+8) & 0x000000ff));
    previewImage.append((unsigned char)(((length+8) & 0x0000ff00) >> 8));
    previewImage.append((unsigned char)(((length+8) & 0x00ff0000) >> 16));
    previewImage.append((unsigned char)(((length+8) & 0xff000000) >> 24));

    previewImage.append(0x00);
    previewImage.append(0x00);
    previewImage.append(0x00);
    previewImage.append(0x00);

    long startPosition = input->tell();
    input->seek(0x18, WPX_SEEK_CUR);
    int lengthX = length + 10 - readU32(input);
    input->seek(startPosition, WPX_SEEK_SET);

    previewImage.append((unsigned char)((lengthX) & 0x000000ff));
    previewImage.append((unsigned char)(((lengthX) & 0x0000ff00) >> 8));
    previewImage.append((unsigned char)(((lengthX) & 0x00ff0000) >> 16));
    previewImage.append((unsigned char)(((lengthX) & 0xff000000) >> 24));

    input->seek(4, WPX_SEEK_CUR);
    for (unsigned i = 4; i<length; i++)
      previewImage.append(readU8(input));
#if DUMP_PREVIEW_IMAGE
    FILE *f = fopen("previewImage.bmp", "wb");
    if (f)
    {
      const unsigned char *tmpBuffer = previewImage.getDataBuffer();
      for (unsigned long k = 0; k < previewImage.size(); k++)
        fprintf(f, "%c",tmpBuffer[k]);
      fclose(f);
    }
#endif
  }
  else if (fourCC == "loda")
  {
    long startPosition = input->tell();
    unsigned chunkLength = readU32(input);
    unsigned numOfArgs = readU32(input);
    unsigned startOfArgs = readU32(input);
    unsigned startOfArgTypes = readU32(input);
    unsigned chunkType = readU32(input);
    std::vector<unsigned> argOffsets(numOfArgs, 0);
    std::vector<unsigned> argTypes(numOfArgs, 0);
    unsigned i = 0;
    input->seek(startPosition+startOfArgs, WPX_SEEK_SET);
    while (i<numOfArgs)
      argOffsets[i++] = readU32(input);
    input->seek(startPosition+startOfArgTypes, WPX_SEEK_SET);
    while (i>0)
      argTypes[--i] = readU32(input);
#if 0
    printf("loda %lu, %u, %u, %u, %u, %u, %lu, %lu\n", startPosition, chunkLength, numOfArgs, startOfArgs, startOfArgTypes, chunkType, argOffsets.size(), argTypes.size());
    printf("--> argOffsets --> ");
    for (std::vector<unsigned>::iterator iter1 = argOffsets.begin(); iter1 != argOffsets.end(); iter1++)
      printf("0x%.8x, ", *iter1);
    printf("\n");
    printf("--> argTypes --> ");
    for (std::vector<unsigned>::iterator iter2 = argTypes.begin(); iter2 != argTypes.end(); iter2++)
      printf("0x%.8x, ", *iter2);
    printf("\n");
#endif
    for (i=0; i < argTypes.size(); i++)
    {
      if ((argTypes[i] == 0x001e) && (chunkType = 0x03))
      {
        input->seek(startPosition+argOffsets[i], WPX_SEEK_SET);
        unsigned pointNum = readU32(input);
        std::vector<std::pair<double, double> > points;
        std::vector<unsigned char> pointTypes;
        for (unsigned j=0; j<pointNum; j++)
        {
          std::pair<double, double> point;
          point.first = (double)readS32(input) / 254000.0;
          point.second = (double)readS32(input) / 254000.0;
          points.push_back(point);
        }
        for (unsigned k=0; k<pointNum; k++)
          pointTypes.push_back(readU8(input));
      }
      else if ((argTypes[i] == 0x001e) && (chunkType < 0x05))
      {
      }
    }
    input->seek(startPosition+chunkLength, WPX_SEEK_SET);
  }
  else if (fourCC == "bbox")
  {
    int X0 = readS32(input);
    int Y0 = readS32(input);
    int X1 = readS32(input);
    int Y1 = readS32(input);
    double width = (double)(X0 < X1 ? X1 - X0 : X0 - X1) / 254000.0;
    double height = (double)(Y0 < Y1 ? Y1 - Y0 : Y0 - Y1) / 254000.0;
    if (X0 != X1 && Y0 != Y1 && m_isListTypePage)
    {
      WPXPropertyList propList;
      propList.insert("svg:width", width);
      propList.insert("svg:height", height);
      m_painter->startGraphics(propList);
      m_isPageOpened = true;
    }
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
