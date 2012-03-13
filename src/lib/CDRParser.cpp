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
#include "CDRParser.h"
#include "CDRCollector.h"

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
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
    readDisp(input, length);
  else if (fourCC == "loda")
    readLoda(input);
  else if (fourCC == "vrsn")
    m_version = readU16(input);
  else if (fourCC == "trfd")
    readTrfd(input);
  else if (fourCC == "outl")
    readOutl(input);
  else if (fourCC == "fild")
    readFild(input);
  /*  else if (fourCC == "arrw")
      ; */
  else if (fourCC == "flgs")
    readFlags(input);
  else if (fourCC == "mcfg")
    readMcfg(input);
  else if (fourCC == "bmp ")
    readBmp(input);
  else if (fourCC == "bmpf")
    readBmpf(input, length);
  else if (fourCC == "ppdt")
    readPpdt(input);
  else if (fourCC == "ftil")
    readFtil(input);
  input->seek(recordStart + length, WPX_SEEK_CUR);
}

double libcdr::CDRParser::readCoordinate(WPXInputStream *input)
{
  if (m_version < 1500)
    return (double)readS32(input) / 254000.0;
  else
    return readDouble(input) / 254000.0;
}

double libcdr::CDRParser::readAngle(WPXInputStream *input)
{
  return M_PI * (double)readS32(input) / 180000000.0;
}

void libcdr::CDRParser::readRectangle(WPXInputStream *input)
{
  double x0 = readCoordinate(input);
  double y0 = readCoordinate(input);
  double r3 = readCoordinate(input);
  double r2 = m_version < 900 ? r3 : readCoordinate(input);
  double r1 = m_version < 900 ? r3 : readCoordinate(input);
  double r0 = m_version < 900 ? r3 : readCoordinate(input);
  if (r0 > 0.0)
    m_collector->collectMoveTo(0.0, -r0);
  else
    m_collector->collectMoveTo(0.0, 0.0);
  if (r1 > 0.0)
  {
    m_collector->collectLineTo(0.0, y0+r1);
    m_collector->collectQuadraticBezier(0.0, y0, r1, y0);
  }
  else
    m_collector->collectLineTo(0.0, y0);
  if (r2 > 0.0)
  {
    m_collector->collectLineTo(x0-r2, y0);
    m_collector->collectQuadraticBezier(x0, y0, x0, y0+r2);
  }
  else
    m_collector->collectLineTo(x0, y0);
  if (r3 > 0.0)
  {
    m_collector->collectLineTo(x0, -r3);
    m_collector->collectQuadraticBezier(x0, 0.0, x0-r3, 0.0);
  }
  else
    m_collector->collectLineTo(x0, 0.0);
  if (r0 > 0.0)
  {
    m_collector->collectLineTo(r0, 0.0);
    m_collector->collectQuadraticBezier(0.0, 0.0, 0.0, -r0);
  }
  else
    m_collector->collectLineTo(0.0, 0.0);
}

void libcdr::CDRParser::readEllipse(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readEllipse\n"));

  double x = (double)readS32(input) / 254000.0;
  double y = (double)readS32(input) / 254000.0;
  double angle1 = readAngle(input);
  double angle2 = readAngle(input);
  bool pie(0 != readU32(input));

  double cx = x/2.0;
  double cy = y/2.0;
  double rx = fabs(cx);
  double ry = fabs(cy);

  if (angle1 != angle2)
  {
    if (angle2 < angle1)
      angle2 += 2*M_PI;
    double x0 = cx + rx*cos(angle1);
    double y0 = cy - ry*sin(angle1);

    double x1 = cx + rx*cos(angle2);
    double y1 = cy - ry*sin(angle2);

    bool largeArc = (angle2 - angle1 > M_PI || angle2 - angle1 < -M_PI);

    m_collector->collectMoveTo(x0, y0);
    m_collector->collectArcTo(rx, ry, largeArc, true, x1, y1);
    if (pie)
    {
      m_collector->collectLineTo(cx, cy);
      m_collector->collectLineTo(x0, y0);
      m_collector->collectClosePath();
    }
  }
  else
  {
    double x0 = cx + rx;
    double y0 = cy;

    double x1 = cx;
    double y1 = cy - ry;

    m_collector->collectMoveTo(x0, y0);
    m_collector->collectArcTo(rx, ry, false, true, x1, y1);
    m_collector->collectArcTo(rx, ry, true, true, x0, y0);
  }
}

void libcdr::CDRParser::readDisp(WPXInputStream *input, unsigned length)
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

void libcdr::CDRParser::readLineAndCurve(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readLineAndCurve\n"));

  unsigned isClosedPath = false;
  unsigned short pointNum = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
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
    if (type & 0x08)
      isClosedPath = true;
    else
      isClosedPath = false;
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
      if (isClosedPath)
        m_collector->collectClosePath();
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        m_collector->collectCubicBezier(tmpPoints[0].first, tmpPoints[0].second, tmpPoints[1].first, tmpPoints[1].second, points[i].first, points[i].second);
      else
        m_collector->collectLineTo(points[i].first, points[i].second);
      if (isClosedPath)
        m_collector->collectClosePath();
      tmpPoints.clear();
    }
    else if((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[i]);
    }
  }
}

void libcdr::CDRParser::readPath(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readPath\n"));

  bool isClosedPath = false;
  input->seek(4, WPX_SEEK_CUR);
  unsigned short pointNum = readU16(input)+readU16(input);
  input->seek(16, WPX_SEEK_CUR);
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
    if (type & 0x08)
      isClosedPath = true;
    else
      isClosedPath = false;
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
      if (isClosedPath)
        m_collector->collectClosePath();
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        m_collector->collectCubicBezier(tmpPoints[0].first, tmpPoints[0].second, tmpPoints[1].first, tmpPoints[1].second, points[i].first, points[i].second);
      else
        m_collector->collectLineTo(points[i].first, points[i].second);
      if (isClosedPath)
        m_collector->collectClosePath();
      tmpPoints.clear();
    }
    else if((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[i]);
    }
  }
}


/*
void libcdr::CDRParser::readText(WPXInputStream *input)
{
  int x0 = readS32(input);
  int y0 = readS32(input);
}
*/

void libcdr::CDRParser::readBitmap(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readBitmap\n"));

  bool isClosedPath = false;
  double x1 = (double)readS32(input) / 254000.0;
  double y1 = (double)readS32(input) / 254000.0;
  double x2 = (double)readS32(input) / 254000.0;
  double y2 = (double)readS32(input) / 254000.0;
#if 0
  double x3 = (double)readS32(input) / 254000.0;
  double y3 = (double)readS32(input) / 254000.0;
  double x4 = (double)readS32(input) / 254000.0;
  double y4 = (double)readS32(input) / 254000.0;
#else
  input->seek(16, WPX_SEEK_CUR);
#endif

  input->seek(16, WPX_SEEK_CUR);
  unsigned imageId = readU32(input);
  if (m_version == 700)
    input->seek(8, WPX_SEEK_CUR);
  else if (m_version >= 800 && m_version < 900)
    input->seek(12, WPX_SEEK_CUR);
  else
    input->seek(20, WPX_SEEK_CUR);

  unsigned short pointNum = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
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
    if (type & 0x08)
      isClosedPath = true;
    else
      isClosedPath = false;
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
      if (isClosedPath)
        m_collector->collectClosePath();
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        m_collector->collectCubicBezier(tmpPoints[0].first, tmpPoints[0].second, tmpPoints[1].first, tmpPoints[1].second, points[i].first, points[i].second);
      else
        m_collector->collectLineTo(points[i].first, points[i].second);
      if (isClosedPath)
        m_collector->collectClosePath();
      tmpPoints.clear();
    }
    else if((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[i]);
    }
  }
  m_collector->collectBitmap(imageId, x1, x2, y1, y2);
}

void libcdr::CDRParser::readTrfd(WPXInputStream *input)
{
  long startPosition = input->tell();
  unsigned chunkLength = readU32(input);
  unsigned numOfArgs = readU32(input);
  unsigned startOfArgs = readU32(input);
  input->seek(4, WPX_SEEK_CUR);
  std::vector<unsigned> argOffsets(numOfArgs, 0);
  unsigned i = 0;
  input->seek(startPosition+startOfArgs, WPX_SEEK_SET);
  while (i<numOfArgs)
    argOffsets[i++] = readU32(input);

  for (i=0; i < argOffsets.size(); i++)
  {
    input->seek(startPosition+argOffsets[i], WPX_SEEK_SET);
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    unsigned short tmpType = readU16(input);
    if (tmpType == 0x08) // trafo
    {
      input->seek(6, WPX_SEEK_CUR);
      double v0 = readDouble(input);
      double v1 = readDouble(input);
      double x0 = readDouble(input) / 254000.0;
      double v3 = readDouble(input);
      double v4 = readDouble(input);
      double y0 = readDouble(input) / 254000.0;
      m_collector->collectTransform(v0, v1, x0, v3, v4, y0);
    }
    else if (tmpType == 0x10)
    {
#if 0
      input->seek(6, WPX_SEEK_CUR);
      unsigned short type = readU16(input);
      double x = readS32(input) / 254000.0;
      double y = readS32(input) / 254000.0;
      unsigned subType = readU32(input);
      if (!subType)
      {
      }
      else
      {
        if (subType&1) // Smooth
        {
        }
        if (subType&2) // Random
        {
        }
        if (subType&4) // Local
        {
        }
      }
      unsigned opt1 = readU32(input);
      unsigned opt2 = readU32(input);
#endif
    }
  }
  input->seek(startPosition+chunkLength, WPX_SEEK_SET);
}

void libcdr::CDRParser::readFild(WPXInputStream *input)
{
  unsigned fillId = readU32(input);
  unsigned short v13flag = 0;
  if (m_version >= 1300)
  {
    input->seek(4, WPX_SEEK_CUR);
    v13flag = readU16(input);
    input->seek(2, WPX_SEEK_CUR);
  }
  unsigned short fillType = readU16(input);
  libcdr::CDRColor color1;
  libcdr::CDRColor color2;
  libcdr::CDRImageFill imageFill;
  libcdr::CDRGradient gradient;
  if (fillType == 1) // Solid
  {
    if (m_version >= 1300)
      input->seek(13, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    unsigned short colorModel = readU16(input);
    input->seek(6, WPX_SEEK_CUR);
    unsigned colorValue = readU32(input);
    color1 = libcdr::CDRColor(colorModel, colorValue);
  }
  else if (fillType == 2) // Gradient
  {
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    gradient.m_type = readU8(input);
    if (m_version >= 1300)
    {
      input->seek(17, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readU16(input);
    }
    else
    {
      input->seek(19, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readS32(input);
    }
    gradient.m_angle = (double)readS32(input) / 1000000.0;
    gradient.m_centerXOffset = readS32(input);
    gradient.m_centerYOffset = readS32(input);
    input->seek(2, WPX_SEEK_CUR);
    gradient.m_mode = readU16(input);
    input->seek(2, WPX_SEEK_CUR);
    gradient.m_midPoint = (double)readU8(input) / 100.0;
    input->seek(1, WPX_SEEK_CUR);
    unsigned short numStops = readU16(input);
    if (m_version >= 1300)
      input->seek(5, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    for (unsigned short i = 0; i < numStops; ++i)
    {
      libcdr::CDRGradientStop stop;
      unsigned short colorModel = readU16(input);
      input->seek(6, WPX_SEEK_CUR);
      unsigned colorValue = readU32(input);
      stop.m_color = libcdr::CDRColor(colorModel, colorValue);
      if (m_version >= 1300)
      {
        if (v13flag == 0x9e)
          input->seek(26, WPX_SEEK_CUR);
        else
          input->seek(5, WPX_SEEK_CUR);
      }
      stop.m_offset = (double)readU16(input) / 100.0;
      if (m_version >= 1300)
        input->seek(5, WPX_SEEK_CUR);
      else
        input->seek(2, WPX_SEEK_CUR);
      gradient.m_stops.push_back(stop);
    }
  }
  else if (fillType == 7) // Pattern
  {
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    unsigned patternId = readU32(input);
    int tmpWidth = readS32(input);
    int tmpHeight = readS32(input);
    double tileOffsetX = 0.0;
    double tileOffsetY = 0.0;
    if (m_version < 900)
    {
      tileOffsetX = (double)readU16(input) / 100.0;
      tileOffsetY = (double)readU16(input) / 100.0;
    }
    else
      input->seek(4, WPX_SEEK_CUR);
    double rcpOffset = (double)readU16(input) / 100.0;
    unsigned char flags = readU8(input);
    double patternWidth = (double)tmpWidth / 254000.0;
    double patternHeight = (double)tmpWidth / 254000.0;
    bool isRelative = false;
    if ((flags & 0x04) && (m_version < 900))
    {
      isRelative = true;
      patternWidth = (double)tmpWidth / 100.0;
      patternHeight = (double)tmpHeight / 100.0;
    }
    if (m_version >= 1300)
      input->seek(6, WPX_SEEK_CUR);
    else
      input->seek(1, WPX_SEEK_CUR);
    unsigned short colorModel = readU16(input);
    input->seek(6, WPX_SEEK_CUR);
    unsigned colorValue = readU32(input);
    color1 = libcdr::CDRColor(colorModel, colorValue);
    if (m_version >= 1300)
    {
      if (v13flag == 0x94)
        input->seek(31, WPX_SEEK_CUR);
      else
        input->seek(10, WPX_SEEK_CUR);
    }
    colorModel = readU16(input);
    input->seek(6, WPX_SEEK_CUR);
    colorValue = readU32(input);
    color2 = libcdr::CDRColor(colorModel, colorValue);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags, m_version < 900 ? true : false);
  }
  else if (fillType == 9) // bitmap
  {
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    else
      input->seek(6, WPX_SEEK_CUR);
    int tmpWidth = readS32(input);
    int tmpHeight = readS32(input);
    double tileOffsetX = 0.0;
    double tileOffsetY = 0.0;
    if (m_version < 900)
    {
      tileOffsetX = (double)readU16(input) / 100.0;
      tileOffsetY = (double)readU16(input) / 100.0;
    }
    else
      input->seek(4, WPX_SEEK_CUR);
    double rcpOffset = (double)readU16(input) / 100.0;
    unsigned char flags = readU8(input);
    double patternWidth = (double)tmpWidth / 254000.0;
    double patternHeight = (double)tmpWidth / 254000.0;
    bool isRelative = false;
    if ((flags & 0x04) && (m_version < 900))
    {
      isRelative = true;
      patternWidth = (double)tmpWidth / 100.0;
      patternHeight = (double)tmpHeight / 100.0;
    }
    if (m_version >= 1300)
      input->seek(17, WPX_SEEK_CUR);
    else
      input->seek(21, WPX_SEEK_CUR);
    unsigned patternId = readU32(input);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags, m_version < 900 ? true : false);
  }
  m_collector->collectFild(fillId, fillType, color1, color2, gradient, imageFill);
}

void libcdr::CDRParser::readOutl(WPXInputStream *input)
{
  unsigned lineId = readU32(input);
  if (m_version >= 1300)
  {
    unsigned flag = readU32(input);
    if (flag == 5)
      input->seek(123, WPX_SEEK_CUR);
    else
      input->seek(16, WPX_SEEK_CUR);
  }
  unsigned short lineType = readU16(input);
  unsigned short capsType = readU16(input);
  unsigned short joinType = readU16(input);
  if (m_version < 1300)
    input->seek(2, WPX_SEEK_CUR);
  double lineWidth = (double)readS32(input) / 254000.0;
  double stretch = (double)readU16(input) / 100.0;
  input->seek(2, WPX_SEEK_CUR);
  double angle = readAngle(input);
  if (m_version < 1300)
    input->seek(52, WPX_SEEK_CUR);
  else
    input->seek(46, WPX_SEEK_CUR);
  unsigned short colorModel = readU16(input);
  input->seek(6, WPX_SEEK_CUR);
  unsigned colorValue = readU32(input);
  libcdr::CDRColor color(colorModel, colorValue);
  input->seek(16, WPX_SEEK_CUR);
  unsigned short numDash = readU16(input);
  int fixPosition = input->tell();
  std::vector<unsigned short> dashArray;
  for (unsigned short i = 0; i < numDash; ++i)
    dashArray.push_back(readU16(input));
  input->seek(fixPosition + 22, WPX_SEEK_SET);
  unsigned startMarkerId = readU32(input);
  unsigned endMarkerId = readU32(input);
  m_collector->collectOutl(lineId, lineType, capsType, joinType, lineWidth, stretch, angle, color, dashArray, startMarkerId, endMarkerId);
}

void libcdr::CDRParser::readLoda(WPXInputStream *input)
{
  long startPosition = input->tell();
  unsigned chunkLength = readU32(input);
  unsigned numOfArgs = readU32(input);
  unsigned startOfArgs = readU32(input);
  unsigned startOfArgTypes = readU32(input);
  unsigned chunkType = readU32(input);
  if (chunkType == 0x26)
    m_collector->collectSpline();
  std::vector<unsigned> argOffsets(numOfArgs, 0);
  std::vector<unsigned> argTypes(numOfArgs, 0);
  unsigned i = 0;
  input->seek(startPosition+startOfArgs, WPX_SEEK_SET);
  while (i<numOfArgs)
    argOffsets[i++] = readU32(input);
  input->seek(startPosition+startOfArgTypes, WPX_SEEK_SET);
  while (i>0)
    argTypes[--i] = readU32(input);

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
      else if (chunkType == 0x25) // Path
        readPath(input);
      /*      else if (chunkType == 0x04) // Text
              readText(input); */
      else if (chunkType == 0x05)
        readBitmap(input);
      else if (chunkType == 0x14) // Polygon
        readPolygonCoords(input);
    }
    else if (argTypes[i] == 0x14)
      m_collector->collectFildId(readU32(input));
    else if (argTypes[i] == 0x0a)
      m_collector->collectOutlId(readU32(input));
    else if (argTypes[i] == 0x2efe)
      m_collector->collectRotate((double)readU32(input)*M_PI / 180000000.0);
    else if (argTypes[i] == 0x2af8)
      readPolygonTransform(input);
  }
  input->seek(startPosition+chunkLength, WPX_SEEK_SET);
}

void libcdr::CDRParser::readFlags(WPXInputStream *input)
{
  unsigned flags = readU32(input);
  m_collector->collectFlags(flags);
}

void libcdr::CDRParser::readMcfg(WPXInputStream *input)
{
  if (m_version >= 1300)
    input->seek(12, WPX_SEEK_CUR);
  else if (m_version >= 900)
    input->seek(4, WPX_SEEK_CUR);
  double width = (double)readS32(input) / 254000.0;
  double height = (double)readS32(input) / 254000.0;
  m_collector->collectPageSize(width, height);
}

void libcdr::CDRParser::readPolygonCoords(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readPolygonCoords\n"));

  bool isClosedPath = false;
  unsigned short pointNum = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
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
    if (type & 0x08)
      isClosedPath = true;
    else
      isClosedPath = false;
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
      if (isClosedPath)
        m_collector->collectClosePath();
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        m_collector->collectCubicBezier(tmpPoints[0].first, tmpPoints[0].second, tmpPoints[1].first, tmpPoints[1].second, points[i].first, points[i].second);
      else
        m_collector->collectLineTo(points[i].first, points[i].second);
      if (isClosedPath)
        m_collector->collectClosePath();
      tmpPoints.clear();
    }
    else if((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[i]);
    }
  }
  m_collector->collectPolygon();
}

void libcdr::CDRParser::readPolygonTransform(WPXInputStream *input)
{
  if (m_version < 1300)
    input->seek(4, WPX_SEEK_CUR);
  unsigned numAngles = readU32(input);
  unsigned nextPoint = readU32(input);
  if (nextPoint <= 1)
    nextPoint = readU32(input);
  else
    input->seek(4, WPX_SEEK_CUR);
  if (m_version >= 1300)
    input->seek(4, WPX_SEEK_CUR);
  double rx = readDouble(input);
  double ry = readDouble(input);
  double cx = (double)readS32(input) / 254000.0;
  double cy = (double)readS32(input) / 254000.0;
  m_collector->collectPolygonTransform(numAngles, nextPoint, rx, ry, cx, cy);
}

void libcdr::CDRParser::readBmp(WPXInputStream *input)
{
  unsigned imageId = readU32(input);
  input->seek(50, WPX_SEEK_CUR);
  unsigned colorModel = readU32(input);
  input->seek(4, WPX_SEEK_CUR);
  unsigned width = readU32(input);
  unsigned height = readU32(input);
  input->seek(4, WPX_SEEK_CUR);
  unsigned bpp = readU32(input);
  input->seek(4, WPX_SEEK_CUR);
  unsigned bmpsize = readU32(input);
  input->seek(32, WPX_SEEK_CUR);
  std::vector<unsigned> palette;
  if (bpp < 24 && colorModel != 5 && colorModel != 6)
  {
    input->seek(2, WPX_SEEK_CUR);
    unsigned short palettesize = readU16(input);
    for (unsigned short i = 0; i <palettesize; ++i)
      palette.push_back(readU8(input) | (readU8(input) << 8) | (readU8(input) << 16));
  }
  std::vector<unsigned char> bitmap(bmpsize);
  unsigned long tmpNumBytesRead = 0;
  const unsigned char *tmpBuffer = input->read(bmpsize, tmpNumBytesRead);
  if (bmpsize != tmpNumBytesRead)
    return;
  memcpy(&bitmap[0], tmpBuffer, bmpsize);
  m_collector->collectBmp(imageId, colorModel, width, height, bpp, palette, bitmap);
}

void libcdr::CDRParser::readBmpf(WPXInputStream *input, unsigned length)
{
  unsigned patternId = readU32(input);
  unsigned headerLength = readU32(input);
  if (headerLength != 40)
    return;
  unsigned width = readU32(input);
  unsigned height = readU32(input);
  input->seek(2, WPX_SEEK_CUR);
  unsigned bpp = readU16(input);
  if (bpp != 1)
    return;
  input->seek(4, WPX_SEEK_CUR);
  unsigned dataSize = readU32(input);
  input->seek(length - dataSize - 28, WPX_SEEK_CUR);
  std::vector<unsigned char> pattern(dataSize);
  unsigned long tmpNumBytesRead = 0;
  const unsigned char *tmpBuffer = input->read(dataSize, tmpNumBytesRead);
  if (dataSize != tmpNumBytesRead)
    return;
  memcpy(&pattern[0], tmpBuffer, dataSize);
  m_collector->collectBmpf(patternId, width, height, pattern);
}

void libcdr::CDRParser::readPpdt(WPXInputStream *input)
{
  unsigned short pointNum = readU16(input);
  input->seek(4, WPX_SEEK_CUR);
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned> knotVector;
  for (unsigned j=0; j<pointNum; j++)
  {
    std::pair<double, double> point;
    point.first = (double)readS32(input) / 254000.0;
    point.second = (double)readS32(input) / 254000.0;
    points.push_back(point);
  }
  for (unsigned k=0; k<pointNum; k++)
    knotVector.push_back(readU32(input));
  m_collector->collectPpdt(points, knotVector);
}

void libcdr::CDRParser::readFtil(WPXInputStream *input)
{
  double v0 = readDouble(input);
  double v1 = readDouble(input);
  double x0 = readDouble(input) / 254000.0;
  double v3 = readDouble(input);
  double v4 = readDouble(input);
  double y0 = readDouble(input) / 254000.0;
  m_collector->collectFillTransform(v0, v1, x0, v3, v4, y0);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
