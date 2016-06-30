/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <librevenge-stream/librevenge-stream.h>
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

libcdr::CMXParser::CMXParser(libcdr::CDRCollector *collector, std::map<unsigned, libcdr::CDRColor> &colorPalette)
  : CommonParser(collector),
    m_bigEndian(false), m_unit(0),
    m_scale(0.0), m_xmin(0.0), m_xmax(0.0), m_ymin(0.0), m_ymax(0.0),
    m_indexSectionOffset(0), m_infoSectionOffset(0), m_thumbnailOffset(0),
    m_fillIndex(0), m_nextInstructionOffset(0), m_colorPalette(colorPalette) {}

libcdr::CMXParser::~CMXParser()
{
}

bool libcdr::CMXParser::parseRecords(librevenge::RVNGInputStream *input, long size, unsigned level)
{
  if (!input)
  {
    return false;
  }
  m_collector->collectLevel(level);
  long endPosition = -1;
  if (size > 0)
    endPosition = input->tell() + size;
  while (!input->isEnd() && (endPosition < 0 || input->tell() < endPosition))
  {
    if (!parseRecord(input, level))
      return false;
  }
  return true;
}

bool libcdr::CMXParser::parseRecord(librevenge::RVNGInputStream *input, unsigned level)
{
  if (!input)
  {
    return false;
  }
  try
  {
    m_collector->collectLevel(level);
    while (!input->isEnd() && readU8(input) == 0)
    {
    }
    if (!input->isEnd())
      input->seek(-1, librevenge::RVNG_SEEK_CUR);
    else
      return true;
    unsigned fourCC = readU32(input);
    unsigned length = readU32(input);
    const unsigned long maxLength = getRemainingLength(input);
    if (length > maxLength)
      length = maxLength;
    long endPosition = input->tell() + length;

    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%u)\n", level, toFourCC(fourCC), length, length));

    if (fourCC == CDR_FOURCC_RIFF || fourCC == CDR_FOURCC_RIFX || fourCC == CDR_FOURCC_LIST)
    {
#ifdef DEBUG
      unsigned listType = readU32(input);
#else
      input->seek(4, librevenge::RVNG_SEEK_CUR);
#endif
      CDR_DEBUG_MSG(("CMX listType: %s\n", toFourCC(listType)));
      unsigned dataSize = length-4;
      if (!parseRecords(input, dataSize, level+1))
        return false;
    }
    else
      readRecord(fourCC, length, input);

    if (input->tell() < endPosition)
      input->seek(endPosition, librevenge::RVNG_SEEK_SET);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

void libcdr::CMXParser::readRecord(unsigned fourCC, unsigned &length, librevenge::RVNGInputStream *input)
{
  long recordEnd = input->tell() + length;
  switch (fourCC)
  {
  case CDR_FOURCC_cont:
    readCMXHeader(input);
    break;
  case CDR_FOURCC_DISP:
    readDisp(input, length);
    break;
  case CDR_FOURCC_page:
    readPage(input, length);
    break;
  case CDR_FOURCC_ccmm:
    readCcmm(input, recordEnd);
    break;
  case CDR_FOURCC_rclr:
    readRclr(input, length);
    break;
  default:
    break;
  }
  if (input->tell() < recordEnd)
    input->seek(recordEnd, librevenge::RVNG_SEEK_SET);
}

void libcdr::CMXParser::readCMXHeader(librevenge::RVNGInputStream *input)
{
  librevenge::RVNGString tmpString;
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
  CDR_DEBUG_MSG(("CMX Base Units: %u\n", m_unit));
  m_scale = readDouble(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMX Units Scale: %.9f\n", m_scale));
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  m_indexSectionOffset = readU32(input, m_bigEndian);
  m_infoSectionOffset = readU32(input, m_bigEndian);
  m_thumbnailOffset = readU32(input, m_bigEndian);
#ifdef DEBUG
  CDRBox box = readBBox(input);
#endif
  CDR_DEBUG_MSG(("CMX Offsets: index section 0x%.8x, info section: 0x%.8x, thumbnail: 0x%.8x\n",
                 m_indexSectionOffset, m_infoSectionOffset, m_thumbnailOffset));
  CDR_DEBUG_MSG(("CMX Bounding Box: x: %f, y: %f, w: %f, h: %f\n", box.m_x, box.m_y, box.m_w, box.m_h));
}

void libcdr::CMXParser::readDisp(librevenge::RVNGInputStream *input, unsigned length)
{
  librevenge::RVNGBinaryData previewImage;
  previewImage.append((unsigned char)0x42);
  previewImage.append((unsigned char)0x4d);

  previewImage.append((unsigned char)((length+8) & 0x000000ff));
  previewImage.append((unsigned char)(((length+8) & 0x0000ff00) >> 8));
  previewImage.append((unsigned char)(((length+8) & 0x00ff0000) >> 16));
  previewImage.append((unsigned char)(((length+8) & 0xff000000) >> 24));

  previewImage.append((unsigned char)0x00);
  previewImage.append((unsigned char)0x00);
  previewImage.append((unsigned char)0x00);
  previewImage.append((unsigned char)0x00);

  long startPosition = input->tell();
  input->seek(0x18, librevenge::RVNG_SEEK_CUR);
  int lengthX = length + 10 - readU32(input);
  input->seek(startPosition, librevenge::RVNG_SEEK_SET);

  previewImage.append((unsigned char)((lengthX) & 0x000000ff));
  previewImage.append((unsigned char)(((lengthX) & 0x0000ff00) >> 8));
  previewImage.append((unsigned char)(((lengthX) & 0x00ff0000) >> 16));
  previewImage.append((unsigned char)(((lengthX) & 0xff000000) >> 24));

  input->seek(4, librevenge::RVNG_SEEK_CUR);
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

void libcdr::CMXParser::readCcmm(librevenge::RVNGInputStream * /* input */, long &recordEnd)
{
  if (m_thumbnailOffset == (unsigned)-1)
    recordEnd += 0x10;
}

void libcdr::CMXParser::readPage(librevenge::RVNGInputStream *input, unsigned length)
{
  long endPosition = length + input->tell();
  while (!input->isEnd() && endPosition > input->tell())
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
    input->seek(m_nextInstructionOffset, librevenge::RVNG_SEEK_SET);
  }
}

void libcdr::CMXParser::readBeginPage(librevenge::RVNGInputStream *input)
{
  CDRBox box;
  CDRTransform matrix;
  unsigned flags = 0;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    unsigned char tagId = 0;
    unsigned short tagLength = 0;
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
      CDR_DEBUG_MSG(("  CMXParser::readBeginPage - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_BeginPage_PageSpecification:
        input->seek(2, librevenge::RVNG_SEEK_CUR);
        flags = readU32(input, m_bigEndian);
        box = readBBox(input);
        break;
      case CMX_Tag_BeginPage_Matrix:
        matrix = readMatrix(input);
        break;
      default:
        break;
      }
      input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    flags = readU32(input, m_bigEndian);
    box = readBBox(input);
  }
  else
    return;
  m_collector->collectPage(0);
  m_collector->collectFlags(flags, true);
  m_collector->collectPageSize(box.getWidth(), box.getHeight(), box.getMinX(), box.getMinY());
}
void libcdr::CMXParser::readBeginLayer(librevenge::RVNGInputStream * /* input */)
{
}
void libcdr::CMXParser::readBeginGroup(librevenge::RVNGInputStream * /* input */)
{
}

void libcdr::CMXParser::readPolyCurve(librevenge::RVNGInputStream *input)
{
  m_collector->collectObject(1);
  unsigned pointNum = 0;
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned char> pointTypes;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    unsigned char tagId = 0;
    unsigned short tagLength = 0;
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
      CDR_DEBUG_MSG(("  CMXParser::readPolyCurve - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_PolyCurve_RenderingAttr:
        readRenderingAttributes(input);
        break;
      case CMX_Tag_PolyCurve_PointList:
        pointNum = readU16(input);
        if (pointNum > getRemainingLength(input) / (2 * 4 + 1))
          pointNum = getRemainingLength(input) / (2 * 4 + 1);
        points.reserve(pointNum);
        pointTypes.reserve(pointNum);
        for (unsigned i = 0; i < pointNum; ++i)
        {
          std::pair<double, double> point;
          point.first = readCoordinate(input, m_bigEndian);
          point.second = readCoordinate(input, m_bigEndian);
          points.push_back(point);
        }
        for (unsigned j = 0; j < pointNum; ++j)
          pointTypes.push_back(readU8(input, m_bigEndian));
        break;
      default:
        break;
      }
      input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    readRenderingAttributes(input);
    pointNum = readU16(input);
    const unsigned long maxPoints = getRemainingLength(input) / (2 * 2 + 1);
    if (pointNum > maxPoints)
      pointNum = maxPoints;
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

  outputPath(points, pointTypes);
  m_collector->collectLevel(1);
}

void libcdr::CMXParser::readEllipse(librevenge::RVNGInputStream *input)
{
  m_collector->collectObject(1);
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
    unsigned char tagId = 0;
    unsigned short tagLength = 0;
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
      CDR_DEBUG_MSG(("  CMXParser::readEllipse - tagId %i, tagLength %u\n", tagId, tagLength));
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
      input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
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

  CDRPath path;
  if (angle1 != angle2)
  {
    if (angle2 < angle1)
      angle2 += 2*M_PI;
    double x0 = cx + rx*cos(angle1);
    double y0 = cy - ry*sin(angle1);

    double x1 = cx + rx*cos(angle2);
    double y1 = cy - ry*sin(angle2);

    bool largeArc = (angle2 - angle1 > M_PI || angle2 - angle1 < -M_PI);

    path.appendMoveTo(x0, y0);
    path.appendArcTo(rx, ry, 0.0, largeArc, true, x1, y1);
    if (pie)
    {
      path.appendLineTo(cx, cy);
      path.appendLineTo(x0, y0);
      path.appendClosePath();
    }
  }
  else
  {
    double x0 = cx + rx;
    double y0 = cy;

    double x1 = cx;
    double y1 = cy - ry;

    path.appendMoveTo(x0, y0);
    path.appendArcTo(rx, ry, 0.0, false, true, x1, y1);
    path.appendArcTo(rx, ry, 0.0, true, true, x0, y0);
  }
  m_collector->collectPath(path);
  m_collector->collectRotate(rotation, cx, cy);
  m_collector->collectLevel(1);
}

void libcdr::CMXParser::readRectangle(librevenge::RVNGInputStream *input)
{
  m_collector->collectObject(1);
  double cx = 0.0;
  double cy = 0.0;
  double width = 0.0;
  double height = 0.0;
  double radius = 0.0;
  double angle = 0.0;
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    unsigned char tagId = 0;
    unsigned short tagLength = 0;
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
      CDR_DEBUG_MSG(("  CMXParser::readRectangle - tagId %i, tagLength %u\n", tagId, tagLength));
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
      input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    input->seek(3, librevenge::RVNG_SEEK_CUR);
    cx = readCoordinate(input, m_bigEndian);
    cy = readCoordinate(input, m_bigEndian);
    width = readCoordinate(input, m_bigEndian);
    height = readCoordinate(input, m_bigEndian);
    radius = readCoordinate(input, m_bigEndian);
    angle = readAngle(input, m_bigEndian);
  }
  else
    return;

  double x0 = cx - width / 2.0;
  double y0 = cy - height / 2.0;
  double x1 = cx + width / 2.0;
  double y1 = cy + height / 2.0;
  CDRPath path;
  if (radius > 0.0)
  {
    path.appendMoveTo(x0, y0-radius);
    path.appendLineTo(x0, y1+radius);
    path.appendQuadraticBezierTo(x0, y1, x0+radius, y1);
    path.appendLineTo(x1-radius, y1);
    path.appendQuadraticBezierTo(x1, y1, x1, y1+radius);
    path.appendLineTo(x1, y0-radius);
    path.appendQuadraticBezierTo(x1, y0, x1-radius, y0);
    path.appendLineTo(x0+radius, y0);
    path.appendQuadraticBezierTo(x0, y0, x0, y0-radius);
  }
  else
  {
    path.appendMoveTo(x0, y0);
    path.appendLineTo(x0, y1);
    path.appendLineTo(x1, y1);
    path.appendLineTo(x1, y0);
    path.appendLineTo(x0, y0);
  }
  m_collector->collectPath(path);
  m_collector->collectRotate(angle, cx, cy);
  m_collector->collectLevel(1);
}

libcdr::CDRTransform libcdr::CMXParser::readMatrix(librevenge::RVNGInputStream *input)
{
  CDRTransform matrix;
  unsigned short type = readU16(input, m_bigEndian);
  switch (type)
  {
  case 2: // general matrix
  {
    double v0 = readDouble(input, m_bigEndian);
    double v3 = readDouble(input, m_bigEndian);
    double v1 = readDouble(input, m_bigEndian);
    double v4 = readDouble(input, m_bigEndian);
    double x0 = readDouble(input, m_bigEndian);
    double y0 = readDouble(input, m_bigEndian);
    return libcdr::CDRTransform(v0, v1, x0, v3, v4, y0);
  }
  default: // identity matrix for the while
    return matrix;
  }
}

libcdr::CDRBox libcdr::CMXParser::readBBox(librevenge::RVNGInputStream *input)
{
  double x0 = readCoordinate(input, m_bigEndian);
  double y0 = readCoordinate(input, m_bigEndian);
  double x1 = readCoordinate(input, m_bigEndian);
  double y1 = readCoordinate(input, m_bigEndian);
  CDRBox box(x0, y0, x1, y1);
  return box;
}

void libcdr::CMXParser::readFill(librevenge::RVNGInputStream *input)
{
  libcdr::CDRColor color1;
  libcdr::CDRColor color2;
  libcdr::CDRImageFill imageFill;
  libcdr::CDRGradient gradient;
  unsigned fillIdentifier = readU16(input, m_bigEndian);
  switch (fillIdentifier)
  {
  case 1: // Uniform
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      unsigned char tagId = 0;
      unsigned short tagLength = 0;
      do
      {
        long startOffset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
        {
          CDR_DEBUG_MSG(("    Solid fill - tagId %i\n", tagId));
        }
        tagLength = readU16(input, m_bigEndian);
        CDR_DEBUG_MSG(("    Solid fill - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_FillSpec_Uniform:
        {
          unsigned colorRef = readU16(input, m_bigEndian);
          color1 = getPaletteColor(colorRef);
          readU16(input, m_bigEndian);
          break;
        }
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
      m_collector->collectFillStyle(fillIdentifier, color1, color2, gradient, imageFill);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      readU16(input, m_bigEndian);
      readU16(input, m_bigEndian);
    }
    break;
  case 2:
    break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    break;
  case 9:
    break;
  case 10:
    break;
  case 11:
    break;
  default:
    break;
  }
}

void libcdr::CMXParser::readRenderingAttributes(librevenge::RVNGInputStream *input)
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
        CDR_DEBUG_MSG(("  Fill specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_FillSpec:
          readFill(input);
          break;
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
      readFill(input);
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
        CDR_DEBUG_MSG(("  Outline specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_OutlineSpec:
          break;
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
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
        CDR_DEBUG_MSG(("  Lens specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
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
        CDR_DEBUG_MSG(("  Canvas specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
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
        CDR_DEBUG_MSG(("  Container specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
  }
}

void libcdr::CMXParser::readJumpAbsolute(librevenge::RVNGInputStream *input)
{
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    unsigned char tagId = 0;
    unsigned short tagLength = 0;
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
      CDR_DEBUG_MSG(("  CMXParser::readJumpAbsolute - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_JumpAbsolute_Offset:
        m_nextInstructionOffset = readU32(input, m_bigEndian);
      default:
        break;
      }
      input->seek(endOffset, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
    m_nextInstructionOffset = readU32(input, m_bigEndian);
  else
    return;
}

void libcdr::CMXParser::readRclr(librevenge::RVNGInputStream *input, unsigned /* length */)
{
  unsigned numRecords = readU16(input, m_bigEndian);
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    for (unsigned j = 1; j < numRecords+1; ++j)
    {
      unsigned char colorModel = 0;
      unsigned char tagId = 0;
      CDRColor color;
      do
      {
        long offset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
          break;
        unsigned short tagLength = readU16(input, m_bigEndian);
        switch (tagId)
        {
        case CMX_Tag_DescrSection_Color_Base:
          colorModel = readU8(input, m_bigEndian);
          readU8(input, m_bigEndian);
          break;
        case CMX_Tag_DescrSection_Color_ColorDescr:
        {
          switch (colorModel)
          {
          case 0: // Invalid
            break;
          case 1: // Pantone
          {
            unsigned short pantoneId = readU16(input, m_bigEndian);
            unsigned short pantoneDensity = readU16(input, m_bigEndian);
            CDR_DEBUG_MSG(("Pantone color: id 0x%x, density 0x%x\n", pantoneId, pantoneDensity));
            color.m_colorValue = ((unsigned)pantoneId & 0xff) | ((unsigned)pantoneId & 0xff00) | ((unsigned)pantoneDensity & 0xff) << 16 | ((unsigned)pantoneDensity & 0xff00) << 16;
            color.m_colorModel = 0;
            m_colorPalette[j] = color;
            break;
          }
          case 2: // CMYK
          case 3: // CMYK255
          {
            unsigned char c = readU8(input, m_bigEndian);
            unsigned char m = readU8(input, m_bigEndian);
            unsigned char y = readU8(input, m_bigEndian);
            unsigned char k = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("CMYK%s color: c 0x%x, m 0x%x, y 0x%x, k 0x%x\n", colorModel == 2 ? "" : "255", c, m, y, k));
            color.m_colorValue = c | (unsigned)m << 8 | (unsigned)y << 16 | (unsigned)k << 24;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          case 4: // CMY
          {
            unsigned char c = readU8(input, m_bigEndian);
            unsigned char m = readU8(input, m_bigEndian);
            unsigned char y = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("CMY color: c 0x%x, m 0x%x, y 0x%x\n", c, m, y));
            color.m_colorValue = c | (unsigned)m << 8 | (unsigned)y << 16;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          case 5: // RGB
          {
            unsigned char r = readU8(input, m_bigEndian);
            unsigned char g = readU8(input, m_bigEndian);
            unsigned char b = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("RGB color: r 0x%x, g 0x%x, b 0x%x\n", r, g, b));
            color.m_colorValue = b | (unsigned)g << 8 | (unsigned)r << 16;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          case 6: // HSB
          {
            unsigned short h = readU16(input, m_bigEndian);
            unsigned char s = readU8(input, m_bigEndian);
            unsigned char b = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("HSB color: h 0x%x, s 0x%x, b 0x%x\n", h, s, b));
            color.m_colorValue = (h & 0xff) | ((unsigned)(h & 0xff00) >> 8) << 8 | (unsigned)s << 16 | (unsigned)b << 24;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          case 7: // HLS
          {
            unsigned short h = readU16(input, m_bigEndian);
            unsigned char l = readU8(input, m_bigEndian);
            unsigned char s = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("HLS color: h 0x%x, l 0x%x, s 0x%x\n", h, l, s));
            color.m_colorValue = (h & 0xff) | ((unsigned)(h & 0xff00) >> 8) << 8 | (unsigned)l << 16 | (unsigned)s << 24;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          case 8: // BW
          case 9: // Gray
          {
            unsigned value = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("%s color: %s 0x%x\n", colorModel == 8 ? "BW" : "Gray", colorModel == 8 ? "black" : "gray", value));
            color.m_colorValue = value;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          case 10: // YIQ255
          {
            unsigned char y = readU8(input, m_bigEndian);
            unsigned char i = readU8(input, m_bigEndian);
            unsigned char q = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("YIQ255 color: y 0x%x, i 0x%x, q 0x%x\n", y, i, q));
            color.m_colorValue = (unsigned)y << 8 | (unsigned)i << 16 | (unsigned)q << 24 ;
            color.m_colorModel = colorModel+1;
            m_colorPalette[j] = color;
            break;
          }
          case 11: // LAB
          {
            unsigned char l = readU8(input, m_bigEndian);
            unsigned char a = readU8(input, m_bigEndian);
            unsigned char b = readU8(input, m_bigEndian);
            CDR_DEBUG_MSG(("LAB color: L 0x%x, green2magenta 0x%x, blue2yellow 0x%x\n", l, a, b));
            color.m_colorValue =l | (unsigned)a << 8 | (unsigned)b << 16;
            color.m_colorModel = colorModel;
            m_colorPalette[j] = color;
            break;
          }
          default:
            break;
          }
        }
        default:
          break;
        }
        input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
    ;
  else
    return;
}

libcdr::CDRColor libcdr::CMXParser::getPaletteColor(unsigned id)
{
  const std::map<unsigned, libcdr::CDRColor>::const_iterator iter = m_colorPalette.find(id);
  if (iter != m_colorPalette.end())
    return iter->second;
  return libcdr::CDRColor();
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
