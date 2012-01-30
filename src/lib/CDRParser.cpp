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
#include <sstream>
#include <string>
#include <cmath>
#include <set>
#include <string.h>
#include "libcdr_utils.h"
#include "CDRInternalStream.h"
#include "CDRParser.h"
#include "CDRCollector.h"

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

libcdr::CDRParser::CDRParser(WPXInputStream *input, libcdr::CDRCollector *collector)
  : m_input(input),
    m_collector(collector),
    m_version(0) {}

libcdr::CDRParser::~CDRParser()
{
}

bool libcdr::CDRParser::parseRecords(WPXInputStream *input, unsigned *blockLengths, unsigned level)
{
  if (!input)
  {
    return false;
  }
  m_collector->collectLevel(level);
  while (!input->atEOS())
  {
    if (!parseRecord(input, blockLengths, level))
      return false;
  }
  return true;
}

bool libcdr::CDRParser::parseRecord(WPXInputStream *input, unsigned *blockLengths, unsigned level)
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
    if (blockLengths)
      length=blockLengths[length];
    unsigned long position = input->tell();
    WPXString listType;
    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      listType = readFourCC(input);
      if (listType == "stlt")
        fourCC = listType;
      else
        m_collector->collectOtherList();
    }
    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%i)\n", level, fourCC.cstr(), length, length));

    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      CDR_DEBUG_MSG(("CDR listType: %s\n", listType.cstr()));
      unsigned cmprsize = length-4;
      if (listType == "cmpr")
      {
        cmprsize  = readU32(input);
        input->seek(12, WPX_SEEK_CUR);
        if (readFourCC(input) != "CPng")
          return false;
        if (readU16(input) != 1)
          return false;
        if (readU16(input) != 4)
          return false;
      }
      else if (listType == "page")
        m_collector->collectPage(level);
      else if (listType == "obj ")
        m_collector->collectObject(level);

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
  long recordStart = input->tell();
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
    readLoda(input);
  else if (fourCC == "bbox")
    readBbox(input);
  else if (fourCC == "vrsn")
    m_version = readU16(input);
  else if (fourCC == "trfd")
    readTrfd(input);
  else if (fourCC == "outl")
    readOutl(input);
  else if (fourCC == "fild")
    readFild(input);
  else if (fourCC == "arrw")
    ;
  else if (fourCC == "obox")
    readObox(input);
  else if (fourCC == "flgs")
    readFlags(input);
  input->seek(recordStart + length, WPX_SEEK_CUR);
}

void libcdr::CDRParser::readRectangle(WPXInputStream *input)
{
  double x0 = (double)readS32(input) / 254000.0;
  double y0 = (double)readS32(input) / 254000.0;
  int R0 = readS32(input);
  int R1 = readS32(input);
  int R2 = readS32(input);
  int R3 = readS32(input);
  m_collector->collectMoveTo(0.0, 0.0);
  m_collector->collectLineTo(0.0, y0);
  m_collector->collectLineTo(x0, y0);
  m_collector->collectLineTo(x0, 0.0);
  m_collector->collectLineTo(0.0, 0.0);
  m_collector->collectClosePath();

}

void libcdr::CDRParser::readEllipse(WPXInputStream *input)
{
  int CX = readS32(input);
  int CY = readS32(input);
  int angle1 = readS32(input);
  int angle2 = readS32(input);
  int rotation = readS32(input);
}

void libcdr::CDRParser::readLineAndCurve(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readLineAndCurve\n"));

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
  std::vector<std::pair<double, double> >tmpPoints;
  for (unsigned i=0; i<pointNum; i++)
  {
    const unsigned char &type = pointTypes[i];
    bool isClosed = false;
    if (type & 0x08)
      isClosed = true;
    if (!(type & 0x10) && !(type & 0x20))
    {
      // cont angle
    }
    else if (type & 0x10)
    {
      // cont smooth
    }
    else if (type & 0x20)
    {
      // cont symmetrical
    }
    if (!(type & 0x40) && !(type & 0x80))
    {
      tmpPoints.clear();
      m_collector->collectMoveTo(points[i].first, points[i].second);
    }
    else if ((type & 0x40) && !(type & 0x80))
    {
      tmpPoints.clear();
      m_collector->collectLineTo(points[i].first, points[i].second);
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        m_collector->collectCubicBezier(tmpPoints[0].first, tmpPoints[0].second, tmpPoints[1].first, tmpPoints[1].second, points[i].first, points[i].second);
      else
        m_collector->collectLineTo(points[i].first, points[i].second);
      tmpPoints.clear();
    }
    else if((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[i]);
    }
  }
}

void libcdr::CDRParser::readText(WPXInputStream *input)
{
  int x0 = readS32(input);
  int y0 = readS32(input);
}

void libcdr::CDRParser::readBitmap(WPXInputStream *input)
{
}

void libcdr::CDRParser::readTrfd(WPXInputStream *input)
{
  int offset = 32;
  if (m_version == 1300)
    offset = 40;
  if (m_version == 500)
    offset = 18;
  input->seek(offset, WPX_SEEK_CUR);
  double v0 = readDouble(input);
  double v1 = readDouble(input);
  double x0 = readDouble(input) / 254000.0;
  double v3 = readDouble(input);
  double v4 = readDouble(input);
  double y0 = readDouble(input) / 254000.0;
  m_collector->collectTransform(v0, v1, x0, v3, v4, y0);
}

void libcdr::CDRParser::readFild(WPXInputStream *input)
{
  unsigned fillId = readU32(input);
  unsigned short fillType = readU16(input);
  unsigned short colorModel = 0;
  unsigned color = 0;
  if (fillType)
  {
    if (m_version >= 1300)
      input->seek(19, WPX_SEEK_CUR);
    input->seek(2, WPX_SEEK_CUR);
    colorModel = readU16(input);
    input->seek(6, WPX_SEEK_CUR);
    color = readU32(input);
  }
  m_collector->collectFild(fillId, fillType, colorModel, color, 0);
}

void libcdr::CDRParser::readOutl(WPXInputStream *input)
{
  unsigned lineId = readU32(input);
  if (m_version >= 1300)
    input->seek(20, WPX_SEEK_CUR);
  unsigned short lineType = readU16(input);
  unsigned short capsType = readU16(input);
  unsigned short joinType = readU16(input);
  if (m_version < 1300)
    input->seek(2, WPX_SEEK_CUR);
  double lineWidth = (double)readS32(input) / 254000.0;
  if (m_version < 1300)
    input->seek(6, WPX_SEEK_CUR);
  input->seek(54, WPX_SEEK_CUR);
  unsigned short colorModel = readU16(input);
  input->seek(6, WPX_SEEK_CUR);
  unsigned color = readU32(input);
  input->seek(16, WPX_SEEK_CUR);
  unsigned short numDash = readU16(input);
  int fixPosition = input->tell();
  std::vector<unsigned short> dashArray;
  for (unsigned short i = 0; i < numDash; ++i)
    dashArray.push_back(readU16(input));
  input->seek(fixPosition + 22, WPX_SEEK_SET);
  unsigned startMarkerId = readU32(input);
  unsigned endMarkerId = readU32(input);
  m_collector->collectOutl(lineId, lineType, capsType, joinType, lineWidth, colorModel, color, dashArray, startMarkerId, endMarkerId);
}

void libcdr::CDRParser::readBbox(WPXInputStream *input)
{
  double x0 = (double)readS32(input) / 254000.0;
  double y0 = (double)readS32(input) / 254000.0;
  double x1 = (double)readS32(input) / 254000.0;
  double y1 = (double)readS32(input) / 254000.0;

  CDR_DEBUG_MSG(("CDRParser::readBbox x0/y0 = %f/%f, x1/y1 = %f/%f\n", x0, y0, x1, y1));
  m_collector->collectBbox(x0, y0, x1, y1);
}

void libcdr::CDRParser::readObox(WPXInputStream *input)
{
  int x0 = readS32(input);
  int y0 = readS32(input);
  int x1 = readS32(input);
  int y1 = readS32(input);
  int x2 = readS32(input);
  int y2 = readS32(input);
  int x3 = readS32(input);
  int y3 = readS32(input);
}

void libcdr::CDRParser::readLoda(WPXInputStream *input)
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
    input->seek(startPosition+argOffsets[i], WPX_SEEK_SET);
    if (argTypes[i] == 0x001e) // loda coords
    {
      if (chunkType == 0x01) // Rectangle
        readRectangle(input);
      else if (chunkType == 0x02) // Ellipse
        readEllipse(input);
      else if (chunkType == 0x03) // Line and curve
        readLineAndCurve(input);
      else if (chunkType == 0x04) // Text
        readText(input);
      else if (chunkType == 0x05)
        readBitmap(input);
    }
    else if (argTypes[i] == 0x14)
      m_collector->collectFildId(readU32(input));
    else if (argTypes[i] == 0x0a)
      m_collector->collectOutlId(readU32(input));
  }
  input->seek(startPosition+chunkLength, WPX_SEEK_SET);
}

void libcdr::CDRParser::readFlags(WPXInputStream *input)
{
  unsigned char b0 = readU8(input);
  unsigned char b1 = readU8(input);
  unsigned char b2 = readU8(input);
  unsigned char b3 = readU8(input);
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
