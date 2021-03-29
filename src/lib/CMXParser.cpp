/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CMXParser.h"

#include <librevenge-stream/librevenge-stream.h>

#include <math.h>
#include <stdlib.h>
#include <utility>
#include <cinttypes>

#include "libcdr_utils.h"
#include "CDRPath.h"
#include "CDRCollector.h"
#include "CDRDocumentStructure.h"
#include "CMXDocumentStructure.h"

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

#ifndef DUMP_IMAGE
#define DUMP_IMAGE 0
#endif

static const int MAX_RECORD_DEPTH = 1 << 10;

namespace
{

uint16_t readTagLength(librevenge::RVNGInputStream *const input, const bool bigEndian)
{
  uint16_t tagLength = libcdr::readU16(input, bigEndian);
  if (tagLength < 3)
  {
    CDR_DEBUG_MSG(("invalid tag length %" PRIu16 "\n", tagLength));
    tagLength = 3;
  }
  return tagLength;
}

void sanitizeNumRecords(
  unsigned long &numRecords,
  const libcdr::CoordinatePrecision precision, const unsigned size16, const unsigned size32,
  const unsigned long remainingLength)
{
  unsigned recordSize = 1;
  if (precision == libcdr::PRECISION_16BIT)
    recordSize = size16;
  else if (precision == libcdr::PRECISION_32BIT)
    recordSize = size32;
  if (numRecords > remainingLength / recordSize)
    numRecords = remainingLength / recordSize;
}

void sanitizeNumRecords(
  unsigned long &numRecords,
  const libcdr::CoordinatePrecision precision, const unsigned size16, const unsigned size32, const unsigned tags,
  const unsigned long remainingLength)
{
  sanitizeNumRecords(numRecords, precision, size16, size32 + 3 * tags + 1, remainingLength);
}

}

libcdr::CMXParser::CMXParser(libcdr::CDRCollector *collector, CMXParserState &parserState)
  : CommonParser(collector),
    m_bigEndian(false), m_unit(0),
    m_scale(0.0), m_xmin(0.0), m_xmax(0.0), m_ymin(0.0), m_ymax(0.0),
    m_fillIndex(0), m_nextInstructionOffset(0), m_parserState(parserState),
    m_currentImageInfo(), m_currentPattern(), m_currentBitmap() {}

libcdr::CMXParser::~CMXParser()
{
}

bool libcdr::CMXParser::parseRecords(librevenge::RVNGInputStream *input, long size, unsigned level)
{
  if (!input || level > MAX_RECORD_DEPTH)
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
    while (!input->isEnd() && readU8(input, m_bigEndian) == 0)
    {
    }
    if (!input->isEnd())
      input->seek(-1, librevenge::RVNG_SEEK_CUR);
    else
      return true;
    unsigned fourCC = readU32(input, m_bigEndian);
    unsigned long length = readU32(input, m_bigEndian);
    const unsigned long maxLength = getRemainingLength(input);
    if (length > maxLength)
      length = maxLength;
    long endPosition = input->tell() + length;

    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8lx (%lu)\n", level, toFourCC(fourCC), length, length));

    if (fourCC == CDR_FOURCC_RIFF || fourCC == CDR_FOURCC_RIFX || fourCC == CDR_FOURCC_LIST)
    {
      if (length < 4)
        return false;
#ifdef DEBUG
      unsigned listType = readU32(input, m_bigEndian);
#else
      input->seek(4, librevenge::RVNG_SEEK_CUR);
#endif
      CDR_DEBUG_MSG(("CMX listType: %s\n", toFourCC(listType)));
      unsigned long dataSize = length-4;
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

void libcdr::CMXParser::parseImage(librevenge::RVNGInputStream *input)
{
  if (!input)
  {
    return;
  }
  try
  {
    while (!input->isEnd() && readU8(input, m_bigEndian) == 0)
    {
    }
    if (!input->isEnd())
      input->seek(-1, librevenge::RVNG_SEEK_CUR);
    else
      return;
    unsigned fourCC = readU32(input, m_bigEndian);
    unsigned long length = readU32(input, m_bigEndian);
    const unsigned long maxLength = getRemainingLength(input);
    if (length > maxLength)
      length = maxLength;
    long endPosition = input->tell() + length;

    if (fourCC != CDR_FOURCC_LIST)
      return;
    unsigned listType = readU32(input, m_bigEndian);
    CDR_DEBUG_MSG(("CMX listType: %s\n", toFourCC(listType)));
    if (listType != CDR_FOURCC_imag)
      return;
    unsigned long dataSize = length-4;
    if (!parseRecords(input, dataSize, (unsigned)-1))
      return;

    if (input->tell() < endPosition)
      input->seek(endPosition, librevenge::RVNG_SEEK_SET);
  }
  catch (...)
  {
  }
}

void libcdr::CMXParser::readRecord(unsigned fourCC, unsigned long length, librevenge::RVNGInputStream *input)
{
  long recordEnd = input->tell() + length;
  switch (fourCC)
  {
  case CDR_FOURCC_cont:
    readCMXHeader(input);
    return;
  case CDR_FOURCC_info:
    readInfo(input);
    break;
  case CDR_FOURCC_data:
    readData(input);
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
    tmpString.append((char)readU8(input, m_bigEndian));
  CDR_DEBUG_MSG(("CMX File ID: %s\n", tmpString.cstr()));
  tmpString.clear();
  for (i = 0; i < 16; i++)
    tmpString.append((char)readU8(input, m_bigEndian));
  CDR_DEBUG_MSG(("CMX Platform: %s\n", tmpString.cstr()));
  tmpString.clear();
  for (i = 0; i < 4; i++)
    tmpString.append((char)readU8(input, m_bigEndian));
  CDR_DEBUG_MSG(("CMX Byte Order: %s\n", tmpString.cstr()));
  if (4 == atoi(tmpString.cstr()))
    m_bigEndian = true;
  tmpString.clear();
  for (i = 0; i < 2; i++)
    tmpString.append((char)readU8(input, m_bigEndian));
  CDR_DEBUG_MSG(("CMX Coordinate Size: %s\n", tmpString.cstr()));
  auto coordSize = (unsigned short)atoi(tmpString.cstr());
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
    tmpString.append((char)readU8(input, m_bigEndian));
  CDR_DEBUG_MSG(("CMX Version Major: %s\n", tmpString.cstr()));
  tmpString.clear();
  for (i = 0; i < 4; i++)
    tmpString.append((char)readU8(input, m_bigEndian));
  CDR_DEBUG_MSG(("CMX Version Minor: %s\n", tmpString.cstr()));
  m_unit = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMX Base Units: %u\n", m_unit));
  m_scale = readDouble(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMX Units Scale: %.9f\n", m_scale));
  input->seek(12, librevenge::RVNG_SEEK_CUR);
  unsigned indexSectionOffset = readU32(input, m_bigEndian);
#ifdef DEBUG
  unsigned infoSectionOffset = readU32(input, m_bigEndian);
#else
  input->seek(4, librevenge::RVNG_SEEK_CUR);
#endif
  unsigned thumbnailOffset = readU32(input, m_bigEndian);
#ifdef DEBUG
  CDRBox box = readBBox(input);
#endif
  CDR_DEBUG_MSG(("CMX Offsets: index section 0x%.8x, info section: 0x%.8x, thumbnail: 0x%.8x\n",
                 indexSectionOffset, infoSectionOffset, thumbnailOffset));
  CDR_DEBUG_MSG(("CMX Bounding Box: x: %f, y: %f, w: %f, h: %f\n", box.m_x, box.m_y, box.m_w, box.m_h));
  if (thumbnailOffset != (unsigned)-1)
  {
    long oldOffset = input->tell();
    input->seek(thumbnailOffset, librevenge::RVNG_SEEK_SET);
    readDisp(input);
    input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
  }
  if (indexSectionOffset != (unsigned)-1)
  {
    long oldOffset = input->tell();
    input->seek(indexSectionOffset, librevenge::RVNG_SEEK_SET);
    readIxmr(input);
    input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
  }
}

void libcdr::CMXParser::readDisp(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_DISP != fourCC)
    return;
  unsigned long length = readU32(input, m_bigEndian);
  const unsigned long maxLength = getRemainingLength(input);
  if (length > maxLength)
    length = maxLength;

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
  unsigned long lengthX = length + 10 - readU32(input, m_bigEndian);
  input->seek(startPosition, librevenge::RVNG_SEEK_SET);

  previewImage.append((unsigned char)((lengthX) & 0x000000ff));
  previewImage.append((unsigned char)(((lengthX) & 0x0000ff00) >> 8));
  previewImage.append((unsigned char)(((lengthX) & 0x00ff0000) >> 16));
  previewImage.append((unsigned char)(((lengthX) & 0xff000000) >> 24));

  input->seek(4, librevenge::RVNG_SEEK_CUR);
  for (unsigned long i = 4; i<length; i++)
    previewImage.append(readU8(input, m_bigEndian));
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

const unsigned *libcdr::CMXParser::_getOffsetByType(unsigned short type, const std::map<unsigned short, unsigned> &offsets)
{
  auto iter = offsets.find(type);
  if (iter != offsets.end())
    return &(iter->second);
  return nullptr;
}

void libcdr::CMXParser::readIxmr(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_ixmr != fourCC)
    return;
  readU32(input, m_bigEndian); // Length

  readU16(input, m_bigEndian); // Master ID
  readU16(input, m_bigEndian); // Size
  unsigned long recordCount = readU16(input, m_bigEndian);
  if (recordCount > getRemainingLength(input) / 6)
    recordCount = getRemainingLength(input) / 6;
  std::map<unsigned short, unsigned> offsets;
  for (unsigned long i = 1; i <= recordCount; ++i)
  {
    unsigned short indexRecordId = readU16(input, m_bigEndian);
    unsigned offset = readU32(input, m_bigEndian);
    offsets[indexRecordId] = offset;
  }
  long oldOffset = input->tell();
  const unsigned *address = nullptr;
  if ((address = _getOffsetByType(CMX_COLOR_DESCRIPTION_SECTION, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readRclr(input);
  }
  if ((address = _getOffsetByType(CMX_DOT_DASH_DESCRIPTION_SECTION, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readRdot(input);
  }
  if ((address = _getOffsetByType(CMX_PEN_DESCRIPTION_SECTION, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readRpen(input);
  }
  if ((address = _getOffsetByType(CMX_LINE_STYLE_DESCRIPTION_SECTION, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readRott(input);
  }
  if ((address = _getOffsetByType(CMX_OUTLINE_DESCRIPTION_SECTION, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readRotl(input);
  }
  if ((address = _getOffsetByType(CMX_BITMAP_INDEX_TABLE, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readIxtl(input);
  }
  if ((address = _getOffsetByType(CMX_EMBEDDED_FILE_INDEX_TABLE, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readIxef(input);
  }
  if ((address = _getOffsetByType(CMX_PROCEDURE_INDEX_TABLE, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readIxpc(input);
  }
  if ((address = _getOffsetByType(CMX_PAGE_INDEX_TABLE, offsets)))
  {
    input->seek(*address, librevenge::RVNG_SEEK_SET);
    readIxpg(input);
  }
  input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
}

void libcdr::CMXParser::readCommands(librevenge::RVNGInputStream *input, unsigned length)
{
  long endPosition = length + input->tell();
  while (!input->isEnd() && endPosition > input->tell())
  {
    long startPosition = input->tell();
    int instructionSize = readS16(input, m_bigEndian);
    int minInstructionSize = 4;
    if (instructionSize < 0)
    {
      instructionSize = readS32(input, m_bigEndian);
      minInstructionSize += 4;
    }
    if (instructionSize < minInstructionSize)
    {
      CDR_DEBUG_MSG(("CMXParser::readCommands - invalid instructionSize %i\n", instructionSize));
      instructionSize = minInstructionSize;
    }
    m_nextInstructionOffset = startPosition+instructionSize;
    short instructionCode = abs(readS16(input, m_bigEndian));
    CDR_DEBUG_MSG(("CMXParser::readCommands - instructionSize %i, instructionCode %i\n", instructionSize, instructionCode));
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
    case CMX_Command_DrawImage:
      readDrawImage(input);
      break;
    default:
      break;
    }
    input->seek(m_nextInstructionOffset, librevenge::RVNG_SEEK_SET);
  }
}

void libcdr::CMXParser::readPage(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_page != fourCC)
    return;
  unsigned length = readU32(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readPage\n"));
  readCommands(input, length);
}

void libcdr::CMXParser::readProc(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_proc != fourCC)
    return;
  unsigned length = readU32(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readProc\n"));
  readCommands(input, length);
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
      tagLength = readTagLength(input, m_bigEndian);
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
void libcdr::CMXParser::readBeginGroup(librevenge::RVNGInputStream *input)
{
  CDRBox box;
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
        CDR_DEBUG_MSG(("  CMXParser::readBeginGroup - tagId %i\n", tagId));
        break;
      }
      tagLength = readTagLength(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readBeginGroup - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_BeginGroup_GroupSpecification:
      {
        box = readBBox(input);
        /* unsigned short groupCount = */ readU16(input, m_bigEndian);
        /* unsigned commandCount = */ readU32(input, m_bigEndian);
        /* unsigned endAddress = */ readU32(input, m_bigEndian);
        break;
      }
      default:
        break;
      }
      input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    box = readBBox(input);
    /* unsigned short groupCount = */ readU16(input, m_bigEndian);
    /* unsigned commandCount = */ readU32(input, m_bigEndian);
    /* unsigned endAddress = */ readU32(input, m_bigEndian);
  }
  else
    return;
  m_collector->collectBBox(box.getMinX(), box.getMinX() + box.getWidth(), box.getMinY(), box.getMinY() + box.getHeight());
}

void libcdr::CMXParser::readPolyCurve(librevenge::RVNGInputStream *input)
{
  m_collector->collectObject(1);
  unsigned long pointNum = 0;
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
      tagLength = readTagLength(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readPolyCurve - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_PolyCurve_RenderingAttr:
        readRenderingAttributes(input);
        break;
      case CMX_Tag_PolyCurve_PointList:
        pointNum = readU16(input, m_bigEndian);
        if (pointNum > getRemainingLength(input) / (2 * 4 + 1))
          pointNum = getRemainingLength(input) / (2 * 4 + 1);
        points.reserve(pointNum);
        pointTypes.reserve(pointNum);
        for (unsigned long i = 0; i < pointNum; ++i)
        {
          std::pair<double, double> point;
          point.first = readCoordinate(input, m_bigEndian);
          point.second = readCoordinate(input, m_bigEndian);
          points.push_back(point);
        }
        for (unsigned long j = 0; j < pointNum; ++j)
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
    CDR_DEBUG_MSG(("  CMXParser::readPolyCurve\n"));
    if (!readRenderingAttributes(input))
      return;
    pointNum = readU16(input, m_bigEndian);
    const unsigned long maxPoints = getRemainingLength(input) / (2 * 2 + 1);
    if (pointNum > maxPoints)
      pointNum = maxPoints;
    for (unsigned long i = 0; i < pointNum; ++i)
    {
      std::pair<double, double> point;
      point.first = readCoordinate(input, m_bigEndian);
      point.second = readCoordinate(input, m_bigEndian);
      points.push_back(point);
    }
    for (unsigned long j = 0; j < pointNum; ++j)
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
      tagLength = readTagLength(input, m_bigEndian);
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
    CDR_DEBUG_MSG(("  CMXParser::readEllipse\n"));
    if (!readRenderingAttributes(input))
      return;
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
  if (!CDR_ALMOST_EQUAL(angle1,angle2))
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

void libcdr::CMXParser::readDrawImage(librevenge::RVNGInputStream *input)
{
  m_collector->collectObject(1);
  CDRBox bBox;
  CDRTransforms trafos;
  unsigned short imageRef = 0;
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
        CDR_DEBUG_MSG(("  CMXParser::readDrawImage - tagId %i\n", tagId));
        break;
      }
      tagLength = readTagLength(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readDrawImage - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_DrawImage_RenderingAttr:
        readRenderingAttributes(input);
        break;
      case CMX_Tag_DrawImage_DrawImageSpecification:
      {
        bBox = readBBox(input);
        readBBox(input);
        trafos.append(readMatrix(input));
        /* unsigned short imageType = */ readU16(input, m_bigEndian);
        imageRef = readU16(input, m_bigEndian);
        break;
      }
      default:
        break;
      }
      input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    CDR_DEBUG_MSG(("  CMXParser::readDrawImage\n"));
    if (!readRenderingAttributes(input))
      return;
    bBox = readBBox(input);
    readBBox(input);
    trafos.append(readMatrix(input));
    /* unsigned short imageType = */ readU16(input, m_bigEndian);
    imageRef = readU16(input, m_bigEndian);
  }
  else
    return;

  m_collector->collectTransform(trafos, false);
  m_collector->collectBitmap(imageRef, bBox.m_x, bBox.m_x + bBox.m_w, bBox.m_y, bBox.m_y + bBox.m_h);
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
      tagLength = readTagLength(input, m_bigEndian);
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
    CDR_DEBUG_MSG(("  CMXParser::readRectangle\n"));
    if (!readRenderingAttributes(input))
      return;
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

void libcdr::CMXParser::readBeginProcedure(librevenge::RVNGInputStream *input)
{
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
        CDR_DEBUG_MSG(("  CMXParser::readBeginProcedure - tagId %i\n", tagId));
        break;
      }
      tagLength = readTagLength(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readBeginProcedure - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_BeginProcedure_ProcedureSpecification:
        /* unsigned flags = */
        readU32(input, m_bigEndian);
        /* CDRBox bBox = */ readBBox(input);
        /* unsigned endOffset = */ readU32(input, m_bigEndian);
        /* unsigned short groupCount = */ readU16(input, m_bigEndian);
        /* unsigned tally = */ readU32(input, m_bigEndian);
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
    CDR_DEBUG_MSG(("  CMXParser::readBeginProcedure\n"));
    /* unsigned flags = */ readU32(input, m_bigEndian);
    /* CDRBox bBox = */ readBBox(input);
    /* unsigned endOffset = */ readU32(input, m_bigEndian);
    /* unsigned short groupCount = */ readU16(input, m_bigEndian);
    /* unsigned tally = */ readU32(input, m_bigEndian);
  }
  else
    return;
}

libcdr::CDRTransform libcdr::CMXParser::readMatrix(librevenge::RVNGInputStream *input)
{
  CDRTransform matrix;
  unsigned short type = readU16(input, m_bigEndian);
  if (type > 1)
  {
    double v0 = readDouble(input, m_bigEndian);
    double v3 = readDouble(input, m_bigEndian);
    double v1 = readDouble(input, m_bigEndian);
    double v4 = readDouble(input, m_bigEndian);
    double x0 = readDouble(input, m_bigEndian) ;
    double y0 = readDouble(input, m_bigEndian);
    if (m_precision == PRECISION_32BIT)
    {
      x0 /= 254000.0;
      y0 /= 254000.0;
    }
    else if (m_precision == PRECISION_16BIT)
    {
      x0 /= 1000.0;
      y0 /= 1000.0;
    }
    else
      return CDRTransform();
    return libcdr::CDRTransform(v0, v1, x0, v3, v4, y0);
  }
  else
    return matrix;
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

librevenge::RVNGString libcdr::CMXParser::readString(librevenge::RVNGInputStream *input)
{
  unsigned long count = readU16(input, m_bigEndian);
  if (count > getRemainingLength(input))
    count = getRemainingLength(input);
  librevenge::RVNGString tmpString;
  for (unsigned long i = 0; i < count; ++i)
    tmpString.append((char)readU8(input, m_bigEndian));
  return tmpString;
}

bool libcdr::CMXParser::readLens(librevenge::RVNGInputStream *input)
{
  unsigned char lensType = readU8(input, m_bigEndian);
  switch (lensType)
  {
  case 1: // Glass
  {
    unsigned char tintMethod = readU8(input, m_bigEndian);
    unsigned short uniformRate = readU16(input, m_bigEndian);
    /* unsigned short colorRef = */ readU16(input, m_bigEndian);
    /* unsigned short rangeProcRef = */ readU16(input, m_bigEndian);
    if (tintMethod == 0)
      m_collector->collectFillOpacity((double)uniformRate / 1000.0);
    break;
  }
  case 2: // Magnifying
  {
    /* unsigned short uniformRate = */ readU16(input, m_bigEndian);
    /* unsigned short rangeProcRef = */ readU16(input, m_bigEndian);
    break;
  }
  case 3: // Fisheye
  {
    /* unsigned short uniformRate = */ readU16(input, m_bigEndian);
    /* unsigned short rangeProcRef = */ readU16(input, m_bigEndian);
    break;
  }
  case 4: // Wireframe
  {
    /* unsigned char outlineMethod = */ readU8(input, m_bigEndian);
    /* unsigned short outlineColorRef = */ readU16(input, m_bigEndian);
    /* unsigned char fillMethod = */ readU8(input, m_bigEndian);
    /* unsigned short fillColorRef = */ readU16(input, m_bigEndian);
    /* unsigned short rangeProcRef = */ readU16(input, m_bigEndian);
    break;
  }
  default:
    if (m_precision == libcdr::PRECISION_16BIT)
      return false;
    break;
  }
  return true;
}

bool libcdr::CMXParser::readFill(librevenge::RVNGInputStream *input)
{
  libcdr::CDRColor color1;
  libcdr::CDRColor color2;
  libcdr::CDRImageFill imageFill;
  libcdr::CDRGradient gradient;
  auto fillId = (unsigned)input->tell();
  unsigned fillType = readU16(input, m_bigEndian);
  switch (fillType)
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
          break;
        }
        tagLength = readTagLength(input, m_bigEndian);
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
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("    Solid fill\n"));
      unsigned colorRef = readU16(input, m_bigEndian);
      color1 = getPaletteColor(colorRef);
      readU16(input, m_bigEndian);
    }
    break;
  case 2:  // Fountain
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
          CDR_DEBUG_MSG(("    Fountain fill - tagId %i\n", tagId));
          break;
        }
        tagLength = readTagLength(input, m_bigEndian);
        CDR_DEBUG_MSG(("    Fountain fill - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_FillSpec_Fountain_Base:
        {
          gradient.m_type = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
          /* unsigned short screen = */ readU16(input, m_bigEndian);
          gradient.m_edgeOffset = readU16(input, m_bigEndian);
          gradient.m_angle = readAngle(input, m_bigEndian);
          gradient.m_centerXOffset = readS32(input, m_bigEndian);
          gradient.m_centerYOffset = readS32(input, m_bigEndian);
          /* unsigned short steps = */ readU16(input, m_bigEndian);
          gradient.m_mode = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
          /* unsigned short rateMethod = */ readU16(input, m_bigEndian);
          /* unsigned short rateValue = */ readU16(input, m_bigEndian);
          break;
        }
        case CMX_Tag_RenderAttr_FillSpec_Fountain_Color:
        {
          unsigned long colorCount = readU16(input, m_bigEndian);
          if (colorCount > getRemainingLength(input) / 4)
            colorCount = getRemainingLength(input) / 4;
          for (unsigned long i = 0; i < colorCount; ++i)
          {
            unsigned short colorRef = readU16(input, m_bigEndian);
            unsigned short offset = readU16(input, m_bigEndian);
            libcdr::CDRGradientStop stop;
            stop.m_color = getPaletteColor(colorRef);
            stop.m_offset = (double)offset / 100.0;
            gradient.m_stops.push_back(stop);
          }
          break;
        }
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("    Fountain fill\n"));
      gradient.m_type = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
      /* unsigned short screen = */ readU16(input, m_bigEndian);
      gradient.m_edgeOffset = readU16(input, m_bigEndian);
      gradient.m_angle = readAngle(input, m_bigEndian);
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      gradient.m_centerXOffset = readS16(input, m_bigEndian);
      gradient.m_centerYOffset = readS16(input, m_bigEndian);
      /* unsigned short steps = */ readU16(input, m_bigEndian);
      gradient.m_mode = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
      unsigned long colorCount = readU16(input, m_bigEndian);
      if (colorCount > getRemainingLength(input) / 4)
        colorCount = getRemainingLength(input) / 4;
      for (unsigned long i = 0; i < colorCount; ++i)
      {
        unsigned short colorRef = readU16(input, m_bigEndian);
        unsigned short offset = readU16(input, m_bigEndian);
        libcdr::CDRGradientStop stop;
        stop.m_color = getPaletteColor(colorRef);
        stop.m_offset = (double)offset / 100.0;
        gradient.m_stops.push_back(stop);
      }
    }
    break;
  case 6: // Postscript
    CDR_DEBUG_MSG(("    Postscript fill\n"));
    if (m_precision == libcdr::PRECISION_32BIT)
    {
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      /* unsigned atom = */ readU32(input, m_bigEndian);
      unsigned long count = readU16(input, m_bigEndian);
      if (count > getRemainingLength(input) / 2)
        count = getRemainingLength(input) / 2;
      for (unsigned long i = 0; i < count; ++i)
        readU16(input, m_bigEndian);
      readString(input);
    }
    break;
  case 7: // Two-Color Pattern
  case 8: // Monochrome with transparent bitmap
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
          CDR_DEBUG_MSG(("    %s fill - tagId %i\n", fillType == 7 ? "Two-Color Pattern" : "Monochrome with transparent bitmap", tagId));
          break;
        }
        tagLength = readTagLength(input, m_bigEndian);
        CDR_DEBUG_MSG(("    %s fill - tagId %i, tagLength %u\n", fillType == 7 ? "Two-Color Pattern" : "Monochrome with transparent bitmap", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_FillSpec_MonoBM:
        {
          unsigned short patternId = readU16(input, m_bigEndian);
          input->seek(3, librevenge::RVNG_SEEK_CUR); // CMX_Tag_Tiling
          int tmpWidth = readS32(input, m_bigEndian);
          int tmpHeight = readS32(input, m_bigEndian);
          double tileOffsetX = (double)readU16(input, m_bigEndian) / 100.0;
          double tileOffsetY = (double)readU16(input, m_bigEndian) / 100.0;
          double rcpOffset = (double)readU16(input, m_bigEndian) / 100.0;
          unsigned char flags = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
          input->seek(1, librevenge::RVNG_SEEK_CUR); // CMX_Tag_EndTag
          double patternWidth = (double)tmpWidth / 254000.0;
          double patternHeight = (double)tmpHeight / 254000.0;
          bool isRelative = false;
          if (flags & 0x04)
          {
            isRelative = true;
            patternWidth = (double)tmpWidth / 100.0;
            patternHeight = (double)tmpHeight / 100.0;
          }
          color1 = getPaletteColor(readU16(input, m_bigEndian));
          color2 = getPaletteColor(readU16(input, m_bigEndian));
          /* unsigned short screen = */ readU16(input, m_bigEndian);
          imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
          break;
        }
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("    %s fill\n", fillType == 7 ? "Two-Color Pattern" : "Monochrome with transparent bitmap"));
      unsigned short patternId = readU16(input, m_bigEndian);
      int tmpWidth = readU16(input, m_bigEndian);
      int tmpHeight = readU16(input, m_bigEndian);
      double tileOffsetX = (double)readU16(input, m_bigEndian) / 100.0;
      double tileOffsetY = (double)readU16(input, m_bigEndian) / 100.0;
      double rcpOffset = (double)readU16(input, m_bigEndian) / 100.0;
      unsigned char flags = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
      double patternWidth = (double)tmpWidth / 1000.0;
      double patternHeight = (double)tmpHeight / 1000.0;
      bool isRelative = false;
      if (flags & 0x04)
      {
        isRelative = true;
        patternWidth = (double)tmpWidth / 100.0;
        patternHeight = (double)tmpHeight / 100.0;
      }
      color1 = getPaletteColor(readU16(input, m_bigEndian));
      color2 = getPaletteColor(readU16(input, m_bigEndian));
      /* unsigned short screen = */ readU16(input, m_bigEndian);
      imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
    }
    break;
  case 9: // Imported Bitmap
  case 10: // Full Color Pattern
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
          CDR_DEBUG_MSG(("    %s fill - tagId %i\n", fillType == 9 ? "Imported Bitmap" : "Full Color Pattern", tagId));
          break;
        }
        tagLength = readTagLength(input, m_bigEndian);
        CDR_DEBUG_MSG(("    %s fill - tagId %i, tagLength %u\n", fillType == 9 ? "Imported Bitmap" : "Full Color Pattern", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_FillSpec_ColorBM:
        {
          unsigned short patternId = readU16(input, m_bigEndian);
          input->seek(3, librevenge::RVNG_SEEK_CUR); // CMX_Tag_Tiling
          int tmpWidth = readS32(input, m_bigEndian);
          int tmpHeight = readS32(input, m_bigEndian);
          double tileOffsetX = (double)readU16(input, m_bigEndian) / 100.0;
          double tileOffsetY = (double)readU16(input, m_bigEndian) / 100.0;
          double rcpOffset = (double)readU16(input, m_bigEndian) / 100.0;
          unsigned char flags = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
          input->seek(1, librevenge::RVNG_SEEK_CUR); // CMX_Tag_EndTag
          double patternWidth = (double)tmpWidth / 254000.0;
          double patternHeight = (double)tmpHeight / 254000.0;
          bool isRelative = false;
          if (flags & 0x04)
          {
            isRelative = true;
            patternWidth = (double)tmpWidth / 100.0;
            patternHeight = (double)tmpHeight / 100.0;
          }
          /* libcdr::CDRBox box = */ readBBox(input);
          imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
          break;
        }
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("    %s fill\n", fillType == 9 ? "Imported Bitmap" : "Full Color Pattern"));
      unsigned short patternId = readU16(input, m_bigEndian);
      int tmpWidth = readU16(input, m_bigEndian);
      int tmpHeight = readU16(input, m_bigEndian);
      double tileOffsetX = (double)readU16(input, m_bigEndian) / 100.0;
      double tileOffsetY = (double)readU16(input, m_bigEndian) / 100.0;
      double rcpOffset = (double)readU16(input, m_bigEndian) / 100.0;
      unsigned char flags = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
      double patternWidth = (double)tmpWidth / 1000.0;
      double patternHeight = (double)tmpHeight / 1000.0;
      bool isRelative = false;
      if (flags & 0x04)
      {
        isRelative = true;
        patternWidth = (double)tmpWidth / 100.0;
        patternHeight = (double)tmpHeight / 100.0;
      }
      /* libcdr::CDRBox box = */ readBBox(input);
      imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
    }
    break;
  case 11:
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
          CDR_DEBUG_MSG(("    Texture fill - tagId %i\n", tagId));
          break;
        }
        tagLength = readTagLength(input, m_bigEndian);
        CDR_DEBUG_MSG(("    Texture fill - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_FillSpec_Texture:
        {
          unsigned char subTagId = 0;
          unsigned short subTagLength = 0;
          do
          {
            long subStartOffset = input->tell();
            subTagId = readU8(input, m_bigEndian);
            if (subTagId == CMX_Tag_EndTag)
              break;
            subTagLength = readTagLength(input, m_bigEndian);
            switch (subTagId)
            {
            case CMX_Tag_RenderAttr_FillSpec_ColorBM:
            {
              unsigned short patternId = readU16(input, m_bigEndian);
              input->seek(3, librevenge::RVNG_SEEK_CUR); // CMX_Tag_Tiling
              int tmpWidth = readS32(input, m_bigEndian);
              int tmpHeight = readS32(input, m_bigEndian);
              double tileOffsetX = (double)readU16(input, m_bigEndian) / 100.0;
              double tileOffsetY = (double)readU16(input, m_bigEndian) / 100.0;
              double rcpOffset = (double)readU16(input, m_bigEndian) / 100.0;
              unsigned char flags = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
              input->seek(1, librevenge::RVNG_SEEK_CUR); // CMX_Tag_EndTag
              double patternWidth = (double)tmpWidth / 254000.0;
              double patternHeight = (double)tmpHeight / 254000.0;
              bool isRelative = false;
              if (flags & 0x04)
              {
                isRelative = true;
                patternWidth = (double)tmpWidth / 100.0;
                patternHeight = (double)tmpHeight / 100.0;
              }
              /* libcdr::CDRBox box = */ readBBox(input);
              imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
              break;
            }
            default:
              break;
            }
            input->seek(subStartOffset + subTagLength, librevenge::RVNG_SEEK_SET);
          }
          while (subTagId != CMX_Tag_EndTag);
        }
        default:
          break;
        }
        input->seek(startOffset + tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("    Texture fill\n"));
      unsigned short patternId = readU16(input, m_bigEndian);
      int tmpWidth = readU16(input, m_bigEndian);
      int tmpHeight = readU16(input, m_bigEndian);
      double tileOffsetX = (double)readU16(input, m_bigEndian) / 100.0;
      double tileOffsetY = (double)readU16(input, m_bigEndian) / 100.0;
      double rcpOffset = (double)readU16(input, m_bigEndian) / 100.0;
      unsigned char flags = (unsigned char)(readU16(input, m_bigEndian) & 0xff);
      double patternWidth = (double)tmpWidth / 1000.0;
      double patternHeight = (double)tmpHeight / 1000.0;
      bool isRelative = false;
      if (flags & 0x04)
      {
        isRelative = true;
        patternWidth = (double)tmpWidth / 100.0;
        patternHeight = (double)tmpHeight / 100.0;
      }
      /* libcdr::CDRBox box = */ readBBox(input);
      /* unsigned char reserved = */ readU8(input, m_bigEndian);
      /* unsigned res = */ readU32(input, m_bigEndian);
      /* unsigned short maxEdge = */ readU16(input, m_bigEndian);
      /* librevenge::RVNGString lib = */ readString(input);
      /* librevenge::RVNGString name = */ readString(input);
      /* librevenge::RVNGString stl = */ readString(input);
      unsigned long count = readU16(input, m_bigEndian);
      if (count > getRemainingLength(input) / 8)
        count = getRemainingLength(input) / 8;
      for (unsigned long i = 0; i < count; ++i)
      {
        readU16(input, m_bigEndian);
        readU16(input, m_bigEndian);
        readU16(input, m_bigEndian);
        readU16(input, m_bigEndian);
      }
      imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
    }
    break;
  default:
    if (m_precision == libcdr::PRECISION_16BIT)
      return false;
    break;
  }
  m_collector->collectFillStyle(fillId, CDRFillStyle((unsigned short)fillType, color1, color2, gradient, imageFill));
  m_collector->collectFillStyleId(fillId);
  return true;
}

bool libcdr::CMXParser::readRenderingAttributes(librevenge::RVNGInputStream *input)
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
        tagLength = readTagLength(input, m_bigEndian);
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
    {
      CDR_DEBUG_MSG(("  Fill specification\n"));
      if (!readFill(input))
        return false;
    }
  }
  if (bitMask & 0x02) // outline
  {
    CDRLineStyle lineStyle;
    auto lineStyleId = (unsigned)input->tell();
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
        tagLength = readTagLength(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Outline specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_OutlineSpec:
          lineStyle = getLineStyle(readU16(input, m_bigEndian));
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
      CDR_DEBUG_MSG(("  Outline specification\n"));
      lineStyle = getLineStyle(readU16(input, m_bigEndian));
    }
    m_collector->collectLineStyle(lineStyleId, lineStyle);
    m_collector->collectLineStyleId(lineStyleId);
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
        tagLength = readTagLength(input, m_bigEndian);
        CDR_DEBUG_MSG(("  Lens specification - tagId %i, tagLength %u\n", tagId, tagLength));
        switch (tagId)
        {
        case CMX_Tag_RenderAttr_LensSpec_Base:
          readLens(input);
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
      CDR_DEBUG_MSG(("  Lens specification\n"));
      if (!readLens(input))
        return false;
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
        tagLength = readTagLength(input, m_bigEndian);
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
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("  Canvas specification\n"));
      return false;
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
        tagLength = readTagLength(input, m_bigEndian);
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
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      CDR_DEBUG_MSG(("  Container specification\n"));
      return false;
    }
  }
  return true;
}

void libcdr::CMXParser::readJumpAbsolute(librevenge::RVNGInputStream *input)
{
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    unsigned char tagId = 0;
    unsigned short tagLength = 0;
    do
    {
      long offset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
      {
        CDR_DEBUG_MSG(("  CMXParser::readJumpAbsolute - tagId %i\n", tagId));
        break;
      }
      tagLength = readTagLength(input, m_bigEndian);
      CDR_DEBUG_MSG(("  CMXParser::readJumpAbsolute - tagId %i, tagLength %u\n", tagId, tagLength));
      switch (tagId)
      {
      case CMX_Tag_JumpAbsolute_Offset:
        m_nextInstructionOffset = readU32(input, m_bigEndian);
      default:
        break;
      }
      input->seek(offset + tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
    m_nextInstructionOffset = readU32(input, m_bigEndian);
  else
    return;
}

void libcdr::CMXParser::readRclr(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_rclr != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readRclr - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 2, 2 + 0, 2, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    CDR_DEBUG_MSG(("Color index %i\n", j));
    unsigned char colorModel = 0;
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      unsigned char tagId = 0;
      do
      {
        long offset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
          break;
        unsigned short tagLength = readTagLength(input, m_bigEndian);
        switch (tagId)
        {
        case CMX_Tag_DescrSection_Color_Base:
          colorModel = readU8(input, m_bigEndian);
          readU8(input, m_bigEndian);
          break;
        case CMX_Tag_DescrSection_Color_ColorDescr:
          m_parserState.m_colorPalette[j] = readColor(input, colorModel);
          break;
        default:
          break;
        }
        input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      colorModel = readU8(input, m_bigEndian);
      readU8(input, m_bigEndian);
      m_parserState.m_colorPalette[j] = readColor(input, colorModel);
    }
    else
      return;
  }
}

void libcdr::CMXParser::readRdot(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_rdot != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readRdot - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 2, 2, 1, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    std::vector<unsigned> dots;
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      unsigned char tagId = 0;
      do
      {
        long offset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
          break;
        unsigned short tagLength = readTagLength(input, m_bigEndian);
        switch (tagId)
        {
        case CMX_Tag_DescrSection_Dash:
        {
          unsigned long dotCount = readU16(input, m_bigEndian);
          if (dotCount > getRemainingLength(input) / 2)
            dotCount = getRemainingLength(input) / 2;
          for (unsigned long i = 0; i < dotCount; ++i)
            dots.push_back(readU16(input, m_bigEndian));
          break;
        }
        default:
          break;
        }
        input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      unsigned long dotCount = readU16(input, m_bigEndian);
      if (dotCount > getRemainingLength(input) / 2)
        dotCount = getRemainingLength(input) / 2;
      for (unsigned long i = 0; i < dotCount; ++i)
        dots.push_back(readU16(input, m_bigEndian));
    }
    else
      return;
    m_parserState.m_dashArrays[j] = dots;
  }
}

void libcdr::CMXParser::readRott(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_rott != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readRott - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 2, 2, 1, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    CMXLineStyle lineStyle;
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      unsigned char tagId = 0;
      do
      {
        long offset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
          break;
        unsigned short tagLength = readTagLength(input, m_bigEndian);
        switch (tagId)
        {
        case CMX_Tag_DescrSection_LineStyle:
        {
          lineStyle.m_spec = readU8(input, m_bigEndian);
          lineStyle.m_capAndJoin = readU8(input, m_bigEndian);
          break;
        }
        default:
          break;
        }
        input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      lineStyle.m_spec = readU8(input, m_bigEndian);
      lineStyle.m_capAndJoin = readU8(input, m_bigEndian);
    }
    else
      return;
    m_parserState.m_lineStyles[j] = lineStyle;
  }
}

void libcdr::CMXParser::readRotl(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_rotl != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readRotl - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 12, 12, 1, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    CMXOutline outline;
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      unsigned char tagId = 0;
      do
      {
        long offset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
          break;
        unsigned short tagLength = readTagLength(input, m_bigEndian);
        switch (tagId)
        {
        case CMX_Tag_DescrSection_Outline:
        {
          outline.m_lineStyle = readU16(input, m_bigEndian);
          outline.m_screen = readU16(input, m_bigEndian);
          outline.m_color = readU16(input, m_bigEndian);
          outline.m_arrowHeads = readU16(input, m_bigEndian);
          outline.m_pen = readU16(input, m_bigEndian);
          outline.m_dashArray = readU16(input, m_bigEndian);
          break;
        }
        default:
          break;
        }
        input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      outline.m_lineStyle = readU16(input, m_bigEndian);
      outline.m_screen = readU16(input, m_bigEndian);
      outline.m_color = readU16(input, m_bigEndian);
      outline.m_arrowHeads = readU16(input, m_bigEndian);
      outline.m_pen = readU16(input, m_bigEndian);
      outline.m_dashArray = readU16(input, m_bigEndian);
    }
    else
      return;
    m_parserState.m_outlines[j] = outline;
  }
}

void libcdr::CMXParser::readRpen(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_rpen != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readRpen - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 10, 12, 1, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    CMXPen pen;
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      unsigned char tagId = 0;
      do
      {
        long offset = input->tell();
        tagId = readU8(input, m_bigEndian);
        if (tagId == CMX_Tag_EndTag)
          break;
        unsigned short tagLength = readTagLength(input, m_bigEndian);
        switch (tagId)
        {
        case CMX_Tag_DescrSection_Pen:
        {
          pen.m_width = readCoordinate(input);
          pen.m_aspect = (double)readU16(input, m_bigEndian) / 100.0;
          pen.m_angle = readAngle(input);
          pen.m_matrix = readMatrix(input);
          break;
        }
        default:
          break;
        }
        input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
      }
      while (tagId != CMX_Tag_EndTag);
    }
    else if (m_precision == libcdr::PRECISION_16BIT)
    {
      pen.m_width = readCoordinate(input);
      pen.m_aspect = (double)readU16(input, m_bigEndian) / 100.0;
      pen.m_angle = readAngle(input);
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      pen.m_matrix = readMatrix(input);
    }
    else
      return;
    m_parserState.m_pens[j] = pen;
  }
}

void libcdr::CMXParser::readIxtl(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_ixtl != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readIxtl - numRecords %li\n", numRecords));
  int sizeInFile(0);
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    sizeInFile = readU16(input, m_bigEndian);
    if (sizeInFile < 4)
      return;
  }
  unsigned type = readU16(input, m_bigEndian);
  sanitizeNumRecords(numRecords, m_precision, 4, 4, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    switch (type)
    {
    case 5: // BMP
    {
      unsigned offset = readU32(input, m_bigEndian);
      long oldOffset = input->tell();
      input->seek(offset, librevenge::RVNG_SEEK_SET);
      parseImage(input);
      input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
      if (m_currentPattern && !m_currentPattern->pattern.empty())
        m_collector->collectBmpf(j, m_currentPattern->width, m_currentPattern->height, m_currentPattern->pattern);
      m_currentPattern = nullptr;
      break;
    }
    case 6:
      m_parserState.m_arrowOffsets[j] = readU32(input, m_bigEndian);
      break;
    default:
      break;
    }
    if (sizeInFile)
      input->seek(sizeInFile-4, librevenge::RVNG_SEEK_CUR);
  }
}

void libcdr::CMXParser::readIxef(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_ixef != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readIxef - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 6, 8, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    int sizeInFile(0);
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      sizeInFile = readU16(input, m_bigEndian);
      if (sizeInFile < 6)
        return;
    }
    unsigned offset = readU32(input, m_bigEndian);
    unsigned type = readU16(input, m_bigEndian);
    long oldOffset = input->tell();
    if (type == 0x11)
    {
      input->seek(offset, librevenge::RVNG_SEEK_SET);
      parseImage(input);
      input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
      if (m_currentBitmap && !(m_currentBitmap->bitmap.empty()))
        m_collector->collectBmp(j, m_currentBitmap->colorModel, m_currentBitmap->width, m_currentBitmap->height,
                                m_currentBitmap->bpp, m_currentBitmap->palette, m_currentBitmap->bitmap);
      m_currentBitmap = nullptr;
    }
    if (sizeInFile)
      input->seek(sizeInFile-6, librevenge::RVNG_SEEK_CUR);
  }
}

void libcdr::CMXParser::readIxpg(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_ixpg != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned long numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readIxpg - numRecords %li\n", numRecords));
  sanitizeNumRecords(numRecords, m_precision, 16, 18, getRemainingLength(input));
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    int sizeInFile(0);
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      sizeInFile = readU16(input, m_bigEndian);
      if (sizeInFile < 16)
        return;
    }
    unsigned pageOffset = readU32(input, m_bigEndian);
    /* unsigned layerTableOffset = */ readU32(input, m_bigEndian);
    /* unsigned thumbnailOffset = */ readU32(input, m_bigEndian);
    /* unsigned refListOffset = */ readU32(input, m_bigEndian);
    if (pageOffset && pageOffset != (unsigned)-1)
    {
      long oldOffset = input->tell();
      input->seek(pageOffset, librevenge::RVNG_SEEK_SET);
      readPage(input);
      input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
    }
    if (sizeInFile)
      input->seek(sizeInFile-16, librevenge::RVNG_SEEK_CUR);
  }
}

void libcdr::CMXParser::readIxpc(librevenge::RVNGInputStream *input)
{
  unsigned fourCC = readU32(input, m_bigEndian);
  if (CDR_FOURCC_ixpc != fourCC)
    return;
  /* unsigned length = */ readU32(input, m_bigEndian);

  unsigned numRecords = readU16(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMXParser::readIxpc - numRecords %i\n", numRecords));

  /* Don't really parse it for the while */
  return;
  for (unsigned j = 1; j <= numRecords; ++j)
  {
    int sizeInFile(0);
    if (m_precision == libcdr::PRECISION_32BIT)
    {
      sizeInFile = readU16(input, m_bigEndian);
      if (sizeInFile < 8)
        return;
    }
    /* unsigned refListOffset = */ readU32(input, m_bigEndian);
    unsigned procOffset = readU32(input, m_bigEndian);
    if (procOffset && procOffset != (unsigned)-1)
    {
      long oldOffset = input->tell();
      input->seek(procOffset, librevenge::RVNG_SEEK_SET);
      m_collector->collectVect(0);
      m_collector->collectSpnd(j);
      readProc(input);
      input->seek(oldOffset, librevenge::RVNG_SEEK_SET);
    }
    if (sizeInFile)
      input->seek(sizeInFile-8, librevenge::RVNG_SEEK_CUR);
  }
}

void libcdr::CMXParser::readInfo(librevenge::RVNGInputStream *input)
{
  m_currentImageInfo = libcdr::CMXImageInfo();
  CDR_DEBUG_MSG(("CMXParser::readInfo\n"));
  if (m_precision == libcdr::PRECISION_32BIT)
  {
    unsigned char tagId = 0;
    do
    {
      long offset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
        break;
      unsigned short tagLength = readTagLength(input, m_bigEndian);
      switch (tagId)
      {
      case CMX_Tag_DescrSection_Image_ImageInfo:
      {
        m_currentImageInfo.m_type = readU16(input, m_bigEndian);
        m_currentImageInfo.m_compression = readU16(input, m_bigEndian);
        m_currentImageInfo.m_size = readU32(input, m_bigEndian);
        m_currentImageInfo.m_compressedSize = readU32(input, m_bigEndian);
      }
      default:
        break;
      }
      input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT)
  {
    m_currentImageInfo.m_type = readU16(input, m_bigEndian);
    m_currentImageInfo.m_compression = readU16(input, m_bigEndian);
    m_currentImageInfo.m_size = readU32(input, m_bigEndian);
    m_currentImageInfo.m_compressedSize = readU32(input, m_bigEndian);
  }
  else
    return;
}

void libcdr::CMXParser::readData(librevenge::RVNGInputStream *input)
{
  CDR_DEBUG_MSG(("CMXParser::readData\n"));
  if (m_precision == libcdr::PRECISION_32BIT && m_currentImageInfo.m_type == 0x10)
  {
    unsigned char tagId = 0;
    do
    {
      long offset = input->tell();
      tagId = readU8(input, m_bigEndian);
      if (tagId == CMX_Tag_EndTag)
        break;
      unsigned tagLength = readU32(input, m_bigEndian);
      switch (tagId)
      {
      case CMX_Tag_DescrSection_Image_ImageData:
      {
        unsigned char first = readU8(input, m_bigEndian);
        unsigned char second = readU8(input, m_bigEndian);
        if (0x42 == first && 0x4d == second) // BM
        {
          unsigned fileSize = readU32(input, m_bigEndian);
          input->seek(8, librevenge::RVNG_SEEK_CUR);
          m_currentPattern.reset(new libcdr::CDRPattern());
          readBmpPattern(m_currentPattern->width, m_currentPattern->height, m_currentPattern->pattern,
                         fileSize - 14, input, m_bigEndian);
        }
        else if (0x52 == first && 0x49 == second) // RI
        {
          input->seek(12, librevenge::RVNG_SEEK_CUR);
          m_currentBitmap.reset(new libcdr::CDRBitmap());
          readRImage(m_currentBitmap->colorModel, m_currentBitmap->width, m_currentBitmap->height,
                     m_currentBitmap->bpp, m_currentBitmap->palette, m_currentBitmap->bitmap,
                     input, m_bigEndian);
        }
        break;
      }
      default:
        break;
      }
      input->seek(offset+tagLength, librevenge::RVNG_SEEK_SET);
    }
    while (tagId != CMX_Tag_EndTag);
  }
  else if (m_precision == libcdr::PRECISION_16BIT || m_currentImageInfo.m_type != 0x10)
  {
    unsigned char first = readU8(input, m_bigEndian);
    unsigned char second = readU8(input, m_bigEndian);
    if (0x42 == first && 0x4d == second) // RI
    {
      unsigned fileSize = readU32(input, m_bigEndian);
      input->seek(8, librevenge::RVNG_SEEK_CUR);
      m_currentPattern.reset(new libcdr::CDRPattern());
      readBmpPattern(m_currentPattern->width, m_currentPattern->height, m_currentPattern->pattern, fileSize - 14, input);
    }
    else if (0x52 == first && 0x49 == second)
    {
      input->seek(12, librevenge::RVNG_SEEK_CUR); // RI
      m_currentBitmap.reset(new libcdr::CDRBitmap());
      readRImage(m_currentBitmap->colorModel, m_currentBitmap->width, m_currentBitmap->height,
                 m_currentBitmap->bpp, m_currentBitmap->palette, m_currentBitmap->bitmap,
                 input, m_bigEndian);
    }
  }
  else
    return;
}

libcdr::CDRColor libcdr::CMXParser::getPaletteColor(unsigned id)
{
  const std::map<unsigned, libcdr::CDRColor>::const_iterator iter = m_parserState.m_colorPalette.find(id);
  if (iter != m_parserState.m_colorPalette.end())
    return iter->second;
  return libcdr::CDRColor();
}

libcdr::CDRColor libcdr::CMXParser::readColor(librevenge::RVNGInputStream *input, unsigned char colorModel)
{
  libcdr::CDRColor color;
  switch (colorModel)
  {
  case 0: // Invalid
    CDR_DEBUG_MSG(("Invalid color model\n"));
    break;
  case 1: // Pantone
  {
    unsigned short pantoneId = readU16(input, m_bigEndian);
    unsigned short pantoneDensity = readU16(input, m_bigEndian);
    CDR_DEBUG_MSG(("Pantone color: id 0x%x, density 0x%x\n", pantoneId, pantoneDensity));
    color.m_colorValue = ((unsigned)pantoneId & 0xff) | ((unsigned)pantoneId & 0xff00) | ((unsigned)pantoneDensity & 0xff) << 16 | ((unsigned)pantoneDensity & 0xff00) << 16;
    color.m_colorModel = 0;
    break;
  }
  case 2: // CMYK
  {
    unsigned char c = readU8(input, m_bigEndian);
    unsigned char m = readU8(input, m_bigEndian);
    unsigned char y = readU8(input, m_bigEndian);
    unsigned char k = readU8(input, m_bigEndian);
    CDR_DEBUG_MSG(("CMYK color: c 0x%x, m 0x%x, y 0x%x, k 0x%x\n", c, m, y, k));
    color.m_colorValue = c | (unsigned)m << 8 | (unsigned)y << 16 | (unsigned)k << 24;
    color.m_colorModel = colorModel;
    break;
  }
  case 3: // CMYK255
  {
    unsigned char c = readU8(input, m_bigEndian);
    unsigned char m = readU8(input, m_bigEndian);
    unsigned char y = readU8(input, m_bigEndian);
    unsigned char k = readU8(input, m_bigEndian);
    CDR_DEBUG_MSG(("CMYK255 color: c 0x%x, m 0x%x, y 0x%x, k 0x%x\n", c, m, y, k));
    color.m_colorValue = c | (unsigned)m << 8 | (unsigned)y << 16 | (unsigned)k << 24;
    color.m_colorModel = colorModel;
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
    break;
  }
  case 8: // BW
  case 9: // Gray
  {
    unsigned value = readU8(input, m_bigEndian);
    CDR_DEBUG_MSG(("%s color: %s 0x%x\n", colorModel == 8 ? "BW" : "Gray", colorModel == 8 ? "black" : "gray", value));
    color.m_colorValue = value;
    color.m_colorModel = colorModel;
    break;
  }
  case 10: // YIQ255
  {
    unsigned char y = readU8(input, m_bigEndian);
    unsigned char i = readU8(input, m_bigEndian);
    unsigned char q = readU8(input, m_bigEndian);
    CDR_DEBUG_MSG(("YIQ255 color: y 0x%x, i 0x%x, q 0x%x\n", y, i, q));
    color.m_colorValue = (unsigned)y << 8 | (unsigned)i << 16 | (unsigned)q << 24 ;
    color.m_colorModel = 11;
    break;
  }
  case 11: // LAB
  case 12: // LAB ???
  {
    unsigned char l = readU8(input, m_bigEndian);
    unsigned char a = readU8(input, m_bigEndian);
    unsigned char b = readU8(input, m_bigEndian);
    CDR_DEBUG_MSG(("LAB color: L 0x%x, green2magenta 0x%x, blue2yellow 0x%x\n", l, a, b));
    color.m_colorValue =l | (unsigned)a << 8 | (unsigned)b << 16;
    color.m_colorModel = 12;
    break;
  }
  case 0xff: // something funny here
    input->seek(4, librevenge::RVNG_SEEK_CUR);
    CDR_FALLTHROUGH;
  default:
    CDR_DEBUG_MSG(("Unknown color model %i\n", colorModel));
    break;
  }
  return color;
}

libcdr::CDRLineStyle libcdr::CMXParser::getLineStyle(unsigned id)
{
  libcdr::CDRLineStyle tmpLineStyle;
  std::map<unsigned, CMXOutline>::const_iterator iterOutline = m_parserState.m_outlines.find(id);
  if (iterOutline == m_parserState.m_outlines.end())
    return tmpLineStyle;
  unsigned lineStyleId = iterOutline->second.m_lineStyle;
  unsigned colorId = iterOutline->second.m_color;
  unsigned penId = iterOutline->second.m_pen;
  unsigned dashArrayId = iterOutline->second.m_dashArray;
  tmpLineStyle.color = getPaletteColor(colorId);
  std::map<unsigned, CMXLineStyle>::const_iterator iterLineStyle = m_parserState.m_lineStyles.find(lineStyleId);
  if (iterLineStyle != m_parserState.m_lineStyles.end())
  {
    tmpLineStyle.lineType = iterLineStyle->second.m_spec;
    tmpLineStyle.capsType = (unsigned short)((iterLineStyle->second.m_capAndJoin) & 0xf);
    tmpLineStyle.joinType = (unsigned short)(((iterLineStyle->second.m_capAndJoin) & 0xf0) >> 4);
  }
  std::map<unsigned, CMXPen>::const_iterator iterPen = m_parserState.m_pens.find(penId);
  if (iterPen != m_parserState.m_pens.end())
  {
    tmpLineStyle.lineWidth = iterPen->second.m_width * (iterPen->second.m_matrix.getScaleX()+iterPen->second.m_matrix.getScaleY())/ 2.0;
    if (!CDR_ALMOST_ZERO(iterPen->second.m_matrix.getScaleY()))
      tmpLineStyle.stretch = iterPen->second.m_matrix.getScaleX()/iterPen->second.m_matrix.getScaleY();
    else
      tmpLineStyle.stretch = 1.0;
    tmpLineStyle.stretch *= iterPen->second.m_aspect;
    tmpLineStyle.angle = iterPen->second.m_angle;
  }
  std::map<unsigned, std::vector<unsigned> >::const_iterator iterDash = m_parserState.m_dashArrays.find(dashArrayId);
  if (iterDash != m_parserState.m_dashArrays.end())
  {
    tmpLineStyle.dashArray = iterDash->second;
  }
  return tmpLineStyle;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
