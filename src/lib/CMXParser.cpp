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
#include "CDRInternalStream.h"
#include "CMXParser.h"
#include "CDRCollector.h"
#include "CDRDocumentStructure.h"
#include "CMXDocumentStructure.h"

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

libcdr::CMXParser::CMXParser(libcdr::CDRCollector *collector)
  : CommonParser(collector),
    m_bigEndian(false), m_unit(0),
    m_scale(0.0), m_xmin(0.0), m_xmax(0.0), m_ymin(0.0), m_ymax(0.0),
    m_indexSectionOffset(0), m_infoSectionOffset(0), m_thumbnailOffset(0),
    m_fillIndex(0), m_nextInstructionOffset(0) {}

libcdr::CMXParser::~CMXParser()
{
}

bool libcdr::CMXParser::parseRecords(WPXInputStream *input, long size, unsigned level)
{
  if (!input)
  {
    return false;
  }
  m_collector->collectLevel(level);
  long endPosition = -1;
  if (size > 0)
    endPosition = input->tell() + size;
  while (!input->atEOS() && (endPosition < 0 || input->tell() < endPosition))
  {
    if (!parseRecord(input, level))
      return false;
  }
  return true;
}

bool libcdr::CMXParser::parseRecord(WPXInputStream *input, unsigned level)
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
    unsigned fourCC = readU32(input);
    unsigned length = readU32(input);
    long endPosition = input->tell() + length;

    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%i)\n", level, toFourCC(fourCC), length, length));

    if (fourCC == FOURCC_RIFF || fourCC == FOURCC_RIFX || fourCC == FOURCC_LIST)
    {
#ifdef DEBUG
      unsigned listType = readU32(input);
#else
      input->seek(4, WPX_SEEK_CUR);
#endif
      CDR_DEBUG_MSG(("CMX listType: %s\n", toFourCC(listType)));
      unsigned dataSize = length-4;
      if (!parseRecords(input, dataSize, level+1))
        return false;
    }
    else
      readRecord(fourCC, length, input);

    if (input->tell() < endPosition)
      input->seek(endPosition, WPX_SEEK_SET);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

void libcdr::CMXParser::readRecord(unsigned fourCC, unsigned &length, WPXInputStream *input)
{
  long recordEnd = input->tell() + length;
  switch (fourCC)
  {
  case FOURCC_cont:
    readCMXHeader(input);
    break;
  case FOURCC_DISP:
    readDisp(input, length);
    break;
  case FOURCC_page:
    readPage(input, length);
    break;
  case FOURCC_ccmm:
    readCcmm(input, recordEnd);
    break;
  default:
    break;
  }
  if (input->tell() < recordEnd)
    input->seek(recordEnd, WPX_SEEK_SET);
}

void libcdr::CMXParser::readCMXHeader(WPXInputStream *input)
{
  WPXString tmpString;
  unsigned i = 0;
  for (i = 0; i < 32; i++)
    tmpString.append((char)readU8(input));
  CDR_DEBUG_MSG(("CMX File ID: %s\n", tmpString.cstr()));
  tmpString.clear();
  for (i = 0; i < 16; i++)
    tmpString.append((char)readU8(input));
  CDR_DEBUG_MSG(("CMX Platform: %s\n", tmpString.cstr()));
  tmpString.clear();
  for (i = 0; i < 4; i++)
    tmpString.append((char)readU8(input));
  CDR_DEBUG_MSG(("CMX Byte Order: %s\n", tmpString.cstr()));
  if (4 == atoi(tmpString.cstr()))
    m_bigEndian = true;
  tmpString.clear();
  for (i = 0; i < 2; i++)
    tmpString.append((char)readU8(input));
  CDR_DEBUG_MSG(("CMX Coordinate Size: %s\n", tmpString.cstr()));
  unsigned short coordSize = (unsigned short)atoi(tmpString.cstr());
  switch (coordSize)
  {
  case 2:
    m_precision = libcdr::PRECISION_16BIT;
    break;
  case 4:
    m_precision = libcdr::PRECISION_32BIT;
    break;
  default:
    m_precision = libcdr::PRECISION_UNKNOWN;
    break;
  }
  tmpString.clear();
  for (i = 0; i < 4; i++)
    tmpString.append((char)readU8(input));
  CDR_DEBUG_MSG(("CMX Version Major: %s\n", tmpString.cstr()));
  tmpString.clear();
  for (i = 0; i < 4; i++)
    tmpString.append((char)readU8(input));
  CDR_DEBUG_MSG(("CMX Version Minor: %s\n", tmpString.cstr()));
  m_unit = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMX Base Units: %i\n", m_unit));
  m_scale = readDouble(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMX Units Scale: %.9f\n", m_scale));
  input->seek(12, WPX_SEEK_CUR);
  m_indexSectionOffset = readU32(input, m_bigEndian);
  m_infoSectionOffset = readU32(input, m_bigEndian);
  m_thumbnailOffset = readU32(input, m_bigEndian);
#ifdef DEBUG
  CDRBBox box = readBBox(input);
#endif
  CDR_DEBUG_MSG(("CMX Offsets: index section 0x%.8x, info section: 0x%.8x, thumbnail: 0x%.8x\n",
                 m_indexSectionOffset, m_infoSectionOffset, m_thumbnailOffset));
  CDR_DEBUG_MSG(("CMX Bounding Box: x0: %f, y0: %f, x1: %f, y1: %f\n", box.m_x0, box.m_y0, box.m_x1, box.m_y1));
}

void libcdr::CMXParser::readDisp(WPXInputStream *input, unsigned length)
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

void libcdr::CMXParser::readCcmm(WPXInputStream *input, long &recordEnd)
{
  if (m_thumbnailOffset == (unsigned)-1)
    recordEnd += 0x10;
}

void libcdr::CMXParser::readPage(WPXInputStream *input, unsigned length)
{
  long endPosition = length + input->tell();
  while (!input->atEOS() && endPosition > input->tell())
  {
    long startPosition = input->tell();
    int instructionSize = readS16(input, m_bigEndian);
    if (instructionSize < 0)
      instructionSize = readS32(input, m_bigEndian);
    m_nextInstructionOffset = startPosition+instructionSize;
    short instructionCode = abs(readS16(input, m_bigEndian));
    CDR_DEBUG_MSG(("CMXParser::readPage - instructionSize %i, instructionCode %i\n", instructionSize, instructionCode));
    switch (instructionCode)
    {
    case CMX_Command_BeginPage:
      readBeginPage(input);
      break;
    case CMX_Command_BeginLayer:
      readBeginLayer(input);
      break;
    case CMX_Command_BeginGroup:
      readBeginGroup(input);
      break;
    case CMX_Command_PolyCurve:
      readPolyCurve(input);
      break;
    case CMX_Command_Ellipse:
      readEllipse(input);
      break;
    case CMX_Command_Rectangle:
      readRectangle(input);
      break;
    case CMX_Command_JumpAbsolute:
      readJumpAbsolute(input);
      break;
    default:
      break;
    }
    input->seek(m_nextInstructionOffset, WPX_SEEK_SET);
  }
}

void libcdr::CMXParser::readBeginPage(WPXInputStream *input)
{
  unsigned char tagId = 0;
  unsigned short tagLength = 0;
  CDRBBox box;
  CDRTransform matrix;
  unsigned flags = 0;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    do
    {
      long startOffset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
      {
        CDR_DEBUG_MSG(("  CMXParser::readBeginPage - tagId %i\n", tagId));
        break;
      }
      tagLength = readU16(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readBeginPage - tagId %i, tagLength %i\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_BeginPage_PageSpecification:
        input->seek(2, WPX_SEEK_CUR);
        flags = readU32(input, m_bigEndian);
        box = readBBox(input);
        break;
      case CMX_Tag_BeginPage_Matrix:
        matrix = readMatrix(input);
        break;
      default:
        break;
      }
      input->seek(startOffset + tagLength, WPX_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    input->seek(2, WPX_SEEK_CUR);
    flags = readU32(input, m_bigEndian);
    box = readBBox(input);
  }
  else
    return;
  m_collector->collectPage(0);
  m_collector->collectFlags(flags, true);
  m_collector->collectPageSize(box.getWidth(), box.getHeight(), box.getMinX(), box.getMinY());
}
void libcdr::CMXParser::readBeginLayer(WPXInputStream *input)
{
}
void libcdr::CMXParser::readBeginGroup(WPXInputStream *input)
{
}

void libcdr::CMXParser::readPolyCurve(WPXInputStream *input)
{
  unsigned char tagId = 0;
  unsigned short tagLength = 0;
  unsigned pointNum = 0;
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned char> pointTypes;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    do
    {
      long startOffset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
      {
        CDR_DEBUG_MSG(("  CMXParser::readPolyCurve - tagId %i\n", tagId));
        break;
      }
      tagLength = readU16(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readPolyCurve - tagId %i, tagLength %i\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_PolyCurve_RenderingAttr:
        readRenderingAttributes(input);
        break;
      case CMX_Tag_PolyCurve_PointList:
        pointNum = readU16(input);
        for (unsigned i = 0; i < pointNum; ++i)
        {
          std::pair<double, double> point;
          point.first = readCoordinate(input, m_bigEndian);
          point.second = readCoordinate(input, m_bigEndian);
          points.push_back(point);
        }
        for (unsigned j = 0; j < pointNum; ++j)
          pointTypes.push_back(readU8(input, m_bigEndian));
      default:
        break;
      }
      input->seek(startOffset + tagLength, WPX_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    readRenderingAttributes(input);
    pointNum = readU16(input);
    for (unsigned i = 0; i < pointNum; ++i)
    {
      std::pair<double, double> point;
      point.first = readCoordinate(input, m_bigEndian);
      point.second = readCoordinate(input, m_bigEndian);
      points.push_back(point);
    }
    for (unsigned j = 0; j < pointNum; ++j)
      pointTypes.push_back(readU8(input, m_bigEndian));
  }
  else
    return;

  m_collector->collectObject(1);
  outputPath(points, pointTypes);
  m_collector->collectLevel(1);
}

void libcdr::CMXParser::readEllipse(WPXInputStream *input)
{
  unsigned char tagId = 0;
  unsigned short tagLength = 0;
  double angle1 = 0.0;
  double angle2 = 0.0;
  double rotation = 0.0;
  bool pie = false;

  double cx = 0.0;
  double cy = 0.0;
  double rx = 0.0;
  double ry = 0.0;

  if (m_precision == libcdr::PRECISION_32BIT)
  {
    do
    {
      long startOffset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
      {
        CDR_DEBUG_MSG(("  CMXParser::readEllipse - tagId %i\n", tagId));
        break;
      }
      tagLength = readU16(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readEllipse - tagId %i, tagLength %i\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_Ellips_RenderingAttr:
        readRenderingAttributes(input);
        break;
      case CMX_Tag_Ellips_EllipsSpecification:
        cx = readCoordinate(input, m_bigEndian);
        cy = readCoordinate(input, m_bigEndian);
        rx = readCoordinate(input, m_bigEndian) / 2.0;
        ry = readCoordinate(input, m_bigEndian) / 2.0;
        angle1 = readAngle(input, m_bigEndian);
        angle2 = readAngle(input, m_bigEndian);
        rotation = readAngle(input, m_bigEndian);
        pie = (0 != readU8(input, m_bigEndian));
      default:
        break;
      }
      input->seek(startOffset + tagLength, WPX_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    cx = readCoordinate(input, m_bigEndian);
    cy = readCoordinate(input, m_bigEndian);
    rx = readCoordinate(input, m_bigEndian) / 2.0;
    ry = readCoordinate(input, m_bigEndian) / 2.0;
    angle1 = readAngle(input, m_bigEndian);
    angle2 = readAngle(input, m_bigEndian);
    rotation = readAngle(input, m_bigEndian);
    pie = (0 != readU8(input, m_bigEndian));
  }
  else
    return;

  m_collector->collectObject(1);
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
  m_collector->collectRotate(rotation, cx, cy);
  m_collector->collectLevel(1);
}

void libcdr::CMXParser::readRectangle(WPXInputStream *input)
{
  unsigned char tagId = 0;
  unsigned short tagLength = 0;
  double cx = 0.0;
  double cy = 0.0;
  double width = 0.0;
  double height = 0.0;
  double radius = 0.0;
  double angle = 0.0;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    do
    {
      long startOffset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
      {
        CDR_DEBUG_MSG(("  CMXParser::readRectangle - tagId %i\n", tagId));
        break;
      }
      tagLength = readU16(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readRectangle - tagId %i, tagLength %i\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_Rectangle_RenderingAttr:
        readRenderingAttributes(input);
        break;
      case CMX_Tag_Rectangle_RectangleSpecification:
        cx = readCoordinate(input, m_bigEndian);
        cy = readCoordinate(input, m_bigEndian);
        width = readCoordinate(input, m_bigEndian);
        height = readCoordinate(input, m_bigEndian);
        radius = readCoordinate(input, m_bigEndian);
        angle = readAngle(input, m_bigEndian);
        break;
      default:
        break;
      }
      input->seek(startOffset + tagLength, WPX_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    input->seek(3, WPX_SEEK_CUR);
    cx = readCoordinate(input, m_bigEndian);
    cy = readCoordinate(input, m_bigEndian);
    width = readCoordinate(input, m_bigEndian);
    height = readCoordinate(input, m_bigEndian);
    radius = readCoordinate(input, m_bigEndian);
    angle = readAngle(input, m_bigEndian);
  }
  else
    return;

  m_collector->collectObject(1);
  double x0 = cx - width / 2.0;
  double y0 = cy - height / 2.0;
  double x1 = cx + width / 2.0;
  double y1 = cy + height / 2.0;
  if (radius > 0.0)
  {
    m_collector->collectMoveTo(x0, y0-radius);
    m_collector->collectLineTo(x0, y1+radius);
    m_collector->collectQuadraticBezier(x0, y1, x0+radius, y1);
    m_collector->collectLineTo(x1-radius, y1);
    m_collector->collectQuadraticBezier(x1, y1, x1, y1+radius);
    m_collector->collectLineTo(x1, y0-radius);
    m_collector->collectQuadraticBezier(x1, y0, x1-radius, y0);
    m_collector->collectLineTo(x0+radius, y0);
    m_collector->collectQuadraticBezier(x0, y0, x0, y0-radius);
  }
  else
  {
    m_collector->collectMoveTo(x0, y0);
    m_collector->collectLineTo(x0, y1);
    m_collector->collectLineTo(x1, y1);
    m_collector->collectLineTo(x1, y0);
    m_collector->collectLineTo(x0, y0);
  }
  m_collector->collectRotate(angle, cx, cy);
  m_collector->collectLevel(1);
}

libcdr::CDRTransform libcdr::CMXParser::readMatrix(WPXInputStream *input)
{
  CDRTransform matrix;
  unsigned short type = readU16(input, m_bigEndian);
  switch (type)
  {
  case 2: // general matrix
    matrix.m_v0 = readDouble(input, m_bigEndian);
    matrix.m_v3 = readDouble(input, m_bigEndian);
    matrix.m_v1 = readDouble(input, m_bigEndian);
    matrix.m_v4 = readDouble(input, m_bigEndian);
    matrix.m_x0 = readDouble(input, m_bigEndian);
    matrix.m_y0 = readDouble(input, m_bigEndian);
    return matrix;
  default: // identity matrix for the while
    return matrix;
  }
}

libcdr::CDRBBox libcdr::CMXParser::readBBox(WPXInputStream *input)
{
  CDRBBox box;
  box.m_x0 = readCoordinate(input, m_bigEndian);
  box.m_y0 = readCoordinate(input, m_bigEndian);
  box.m_x1 = readCoordinate(input, m_bigEndian);
  box.m_y1 = readCoordinate(input, m_bigEndian);
  return box;
}

void libcdr::CMXParser::readRenderingAttributes(WPXInputStream *input)
{
  unsigned char tagId = 0;
  unsigned short tagLength = 0;
  unsigned char bitMask = readU8(input, m_bigEndian);
  if (bitMask & 0x01) // fill
  {
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      do
      {
        long startOffset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
        {
          CDR_DEBUG_MSG(("  Fill specification - tagId %i\n", tagId));
          break;
        }
        tagLength = readU16(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Fill specification - tagId %i, tagLength %i\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, WPX_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
  }
  if (bitMask & 0x02) // outline
  {
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      do
      {
        long startOffset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
        {
          CDR_DEBUG_MSG(("  Outline specification - tagId %i\n", tagId));
          break;
        }
        tagLength = readU16(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Outline specification - tagId %i, tagLength %i\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_OutlineSpec:
          m_collector->collectOutlId(readU16(input, m_bigEndian));
          break;
        default:
          break;
        }
        input->seek(startOffset + tagLength, WPX_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
      m_collector->collectOutlId(readU16(input, m_bigEndian));
  }
  if (bitMask & 0x04) // lens
  {
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      do
      {
        long startOffset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
        {
          CDR_DEBUG_MSG(("  Lens specification - tagId %i\n", tagId));
          break;
        }
        tagLength = readU16(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Lens specification - tagId %i, tagLength %i\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, WPX_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
  }
  if (bitMask & 0x08) // canvas
  {
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      do
      {
        long startOffset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
        {
          CDR_DEBUG_MSG(("  Canvas specification - tagId %i\n", tagId));
          break;
        }
        tagLength = readU16(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Canvas specification - tagId %i, tagLength %i\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, WPX_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
  }
  if (bitMask & 0x10) // container
  {
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      do
      {
        long startOffset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
        {
          CDR_DEBUG_MSG(("  Container specification - tagId %i\n", tagId));
          break;
        }
        tagLength = readU16(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Container specification - tagId %i, tagLength %i\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, WPX_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
  }
}

void libcdr::CMXParser::readJumpAbsolute(WPXInputStream *input)
{
  unsigned char tagId = 0;
  unsigned short tagLength = 0;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    do
    {
      long endOffset = input->tell() + tagLength;
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
      {
        CDR_DEBUG_MSG(("  CMXParser::readJumpAbsolute - tagId %i\n", tagId));
        break;
      }
      tagLength = readU16(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readJumpAbsolute - tagId %i, tagLength %i\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_JumpAbsolute_Offset:
        m_nextInstructionOffset = readU32(input, m_bigEndian);
      default:
        break;
      }
      input->seek(endOffset, WPX_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
    m_nextInstructionOffset = readU32(input, m_bigEndian);
  else
    return;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
