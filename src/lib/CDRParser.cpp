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
#include "CDRDocumentStructure.h"
#include "CDRInternalStream.h"
#include "CDRParser.h"
#include "CDRCollector.h"
#include "CDRColorPalettes.h"

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{

unsigned getCDRVersion(char c)
{
  if (c == 0x20)
    return 300;
  else if (c < 0x31)
    return 0;
  else if (c < 0x3a)
    return 100 * ((unsigned char)c - 0x30);
  else if (c < 0x41)
    return 0;
  return 100 * ((unsigned char)c - 0x37);
}

} // anonymous namespace

libcdr::CDRParser::CDRParser(const std::vector<WPXInputStream *> &externalStreams, libcdr::CDRCollector *collector)
  : CommonParser(collector),
    m_externalStreams(externalStreams),
    m_version(0), m_fillId(0), m_outlId(0) {}

libcdr::CDRParser::~CDRParser()
{
  m_collector->collectLevel(0);
}

bool libcdr::CDRParser::parseWaldo(WPXInputStream *input)
{
  try
  {
    input->seek(0, WPX_SEEK_SET);
    unsigned short magic = readU16(input);
    if (magic != 0x4c57)
      return false;
    m_version = 200;
    m_precision = libcdr::PRECISION_16BIT;
    if ('e' >= readU8(input))
      m_version = 100;
    input->seek(1, WPX_SEEK_CUR);
    std::vector<unsigned> offsets;
    unsigned i = 0;
    for (i = 0; i < 8; ++i)
      offsets.push_back(readU32(input));
    /* unsigned char flag = */
    readU8(input);
    for (i = 0; i < 10; i++)
      offsets.push_back(readU32(input));
    input->seek(offsets[0], WPX_SEEK_SET);
    CDR_DEBUG_MSG(("CDRParser::parseWaldo, Mcfg offset 0x%x\n", (unsigned)input->tell()));
    readMcfg(input, 275);
    std::vector<WaldoRecordInfo> records;
    std::map<unsigned, WaldoRecordInfo> records2;
    std::map<unsigned, WaldoRecordInfo> records3;
    std::map<unsigned, WaldoRecordInfo> records4;
    std::map<unsigned, WaldoRecordInfo> records6;
    std::map<unsigned, WaldoRecordInfo> records8;
    std::map<unsigned, WaldoRecordInfo> records7;
    std::map<unsigned, WaldoRecordInfo> recordsOther;
    if (offsets[3])
    {
      input->seek(offsets[3], WPX_SEEK_SET);
      if (!gatherWaldoInformation(input, records, records2, records3, records4, records6, records7, records8, recordsOther))
        return false;
    }
    if (offsets[5])
    {
      input->seek(offsets[5], WPX_SEEK_SET);
      gatherWaldoInformation(input, records, records2, records3, records4, records6, records7, records8, recordsOther);
    }
    if (offsets[11])
    {
      input->seek(offsets[11], WPX_SEEK_SET);
      gatherWaldoInformation(input, records, records2, records3, records4, records6, records7, records8, recordsOther);
    }
    std::map<unsigned, WaldoRecordType1> records1;
    for (std::vector<WaldoRecordInfo>::iterator iterVec = records.begin(); iterVec != records.end(); ++iterVec)
    {
      input->seek(iterVec->offset, WPX_SEEK_SET);
      unsigned length = readU32(input);
      if (length != 0x18)
      {
        CDR_DEBUG_MSG(("Throwing GenericException\n"));
        throw GenericException();
      }
      unsigned short next = readU16(input);
      unsigned short previous = readU16(input);
      unsigned short child = readU16(input);
      unsigned short parent = readU16(input);
      input->seek(4, WPX_SEEK_CUR);
      unsigned short moreDataID = readU16(input);
      double x0 = readCoordinate(input);
      double y0 = readCoordinate(input);
      double x1 = readCoordinate(input);
      double y1 = readCoordinate(input);
      unsigned short flags = readU16(input);

      double width = fabs(x1-x0);
      double height = fabs(y1-y0);
      double offsetX = x1 < x0 ? x1 : x0;
      double offsetY = y1 < y0 ? y1 : y0;
      CDRTransform trafo;
      if (moreDataID)
      {
        std::map<unsigned, WaldoRecordInfo>::const_iterator iter7 = records7.find(moreDataID);
        if (iter7 != records7.end())
          input->seek(iter7->second.offset, WPX_SEEK_SET);
        input->seek(0x26, WPX_SEEK_CUR);
        trafo.m_v0 = readFixedPoint(input);
        trafo.m_v1 = readFixedPoint(input);
        trafo.m_x0 = readFixedPoint(input) / 1000.0;
        trafo.m_v3 = readFixedPoint(input);
        trafo.m_v4 = readFixedPoint(input);
        trafo.m_y0 = readFixedPoint(input) / 1000.0;
      }
      records1[iterVec->id] = WaldoRecordType1(iterVec->id, next, previous, child, parent, flags, width,  height, offsetX, offsetY, trafo);
    }
    std::map<unsigned, WaldoRecordInfo>::const_iterator iter;
    for (iter = records3.begin(); iter != records3.end(); ++iter)
      readWaldoRecord(input, iter->second);
    for (iter = records6.begin(); iter != records6.end(); ++iter)
      readWaldoRecord(input, iter->second);
    for (iter = records8.begin(); iter != records8.end(); ++iter)
      readWaldoRecord(input, iter->second);
    for (iter = recordsOther.begin(); iter != recordsOther.end(); ++iter)
      readWaldoRecord(input, iter->second);
    if (!records1.empty() && !records2.empty())
    {

      std::map<unsigned, WaldoRecordType1>::iterator iter1 = records1.find(1);
      std::stack<WaldoRecordType1> waldoStack;
      if (iter1 != records1.end())
      {
        waldoStack.push(iter1->second);
        m_collector->collectVect(waldoStack.size());
        parseWaldoStructure(input, waldoStack, records1, records2);
      }
      iter1 = records1.find(0);
      if (iter1 == records1.end())
        return false;
      waldoStack = std::stack<WaldoRecordType1>();
      waldoStack.push(iter1->second);
      m_collector->collectPage(waldoStack.size());
      if (!parseWaldoStructure(input, waldoStack, records1, records2))
        return false;
    }
    return true;
  }
  catch (...)
  {
    return false;
  }
}

bool libcdr::CDRParser::gatherWaldoInformation(WPXInputStream *input, std::vector<WaldoRecordInfo> &records, std::map<unsigned, WaldoRecordInfo> &records2,
    std::map<unsigned, WaldoRecordInfo> &records3, std::map<unsigned, WaldoRecordInfo> &records4,
    std::map<unsigned, WaldoRecordInfo> &records6, std::map<unsigned, WaldoRecordInfo> &records7,
    std::map<unsigned, WaldoRecordInfo> &records8, std::map<unsigned, WaldoRecordInfo> recordsOther)
{
  try
  {
    unsigned short numRecords = readU16(input);
    for (; numRecords > 0 && !input->atEOS(); --numRecords)
    {
      unsigned char recordType = readU8(input);
      unsigned recordId = readU32(input);
      unsigned recordOffset = readU32(input);
      switch (recordType)
      {
      case 1:
        records.push_back(WaldoRecordInfo(recordType, recordId, recordOffset));
        break;
      case 2:
        records2[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      case 3:
        records3[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      case 4:
        records4[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      case 6:
        records6[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      case 7:
        records7[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      case 8:
        records8[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      default:
        recordsOther[recordId]  = WaldoRecordInfo(recordType, recordId, recordOffset);
        break;
      }
    }
    return true;
  }
  catch (...)
  {
    CDR_DEBUG_MSG(("CDRParser::gatherWaldoInformation: something went wrong during information gathering\n"));
    return false;
  }
}


bool libcdr::CDRParser::parseWaldoStructure(WPXInputStream *input, std::stack<WaldoRecordType1> &waldoStack,
    const std::map<unsigned, WaldoRecordType1> &records1, std::map<unsigned, WaldoRecordInfo> &records2)
{
  while (!waldoStack.empty())
  {
    m_collector->collectBBox(waldoStack.top().width, waldoStack.top().height, waldoStack.top().offsetX, waldoStack.top().offsetY);
    std::map<unsigned, WaldoRecordType1>::const_iterator iter1;
    if (waldoStack.top().flags & 0x01)
    {
      if (waldoStack.size() > 1)
      {
        m_collector->collectGroup(waldoStack.size());
        m_collector->collectSpnd(waldoStack.top().id);
        m_collector->collectTransform(waldoStack.top().trafo.m_v0, waldoStack.top().trafo.m_v1, waldoStack.top().trafo.m_x0,
                                      waldoStack.top().trafo.m_v3, waldoStack.top().trafo.m_v4, waldoStack.top().trafo.m_y0,
                                      true);
      }
      iter1 = records1.find(waldoStack.top().child);
      if (iter1 == records1.end())
        return false;
      waldoStack.push(iter1->second);
      m_collector->collectLevel(waldoStack.size());
    }
    else
    {
      if (waldoStack.size() > 1)
        m_collector->collectObject(waldoStack.size());
      std::map<unsigned, WaldoRecordInfo>::const_iterator iter2 = records2.find(waldoStack.top().child);
      if (iter2 == records2.end())
        return false;
      readWaldoRecord(input, iter2->second);
      while (!waldoStack.empty() && !waldoStack.top().next)
        waldoStack.pop();
      m_collector->collectLevel(waldoStack.size());
      if (waldoStack.empty())
        return true;
      iter1 = records1.find(waldoStack.top().next);
      if (iter1 == records1.end())
        return false;
      waldoStack.top() = iter1->second;
    }
  }
  return true;
}

void libcdr::CDRParser::readWaldoRecord(WPXInputStream *input, const WaldoRecordInfo &info)
{
  CDR_DEBUG_MSG(("CDRParser::readWaldoRecord, type %i, id %x, offset %x\n", info.type, info.id, info.offset));
  input->seek(info.offset, WPX_SEEK_SET);
  switch (info.type)
  {
  case 2:
  {
    unsigned length = readU32(input);
    readWaldoLoda(input, length);
  }
  break;
  case 3:
  {
    unsigned length = readU32(input);
    readWaldoBmp(input, length, info.id);
  }
  break;
  case 6:
    readWaldoBmpf(input, info.id);
    break;
  default:
    break;
  }
}

void libcdr::CDRParser::readWaldoTrfd(WPXInputStream *input)
{
  if (m_version >= 400)
    return;
  double v0 = 0.0;
  double v1 = 0.0;
  double x0 = 0.0;
  double v3 = 0.0;
  double v4 = 0.0;
  double y0 = 0.0;
  if (m_version >= 300)
  {
    long startPosition = input->tell();
    input->seek(0x0a, WPX_SEEK_CUR);
    unsigned offset = readUnsigned(input);
    input->seek(startPosition+offset, WPX_SEEK_SET);
    v0 = readFixedPoint(input);
    v1 = readFixedPoint(input);
    x0 = (double)readS32(input) / 1000.0;
    v3 = readFixedPoint(input);
    v4 = readFixedPoint(input);
    y0 = (double)readS32(input) / 1000.0;
  }
  else
  {
    x0 = readCoordinate(input);
    y0 = readCoordinate(input);
    v0 = readFixedPoint(input);
    v1 = readFixedPoint(input);
    x0 += readFixedPoint(input) / 1000.0;
    v3 = readFixedPoint(input);
    v4 = readFixedPoint(input);
    y0 += readFixedPoint(input) / 1000.0;
  }
  CDR_DEBUG_MSG(("CDRParser::readWaldoTrfd %f %f %f %f %f %f %i\n", v0, v1, x0, v3, v4, y0, m_version));
  m_collector->collectTransform(v0, v1, x0, v3, v4, y0, m_version < 400);
}

void libcdr::CDRParser::readWaldoLoda(WPXInputStream *input, unsigned length)
{
  if (m_version >= 300)
    return;
  long startPosition = input->tell();
  readWaldoTrfd(input);
  unsigned chunkType = readU8(input);
  unsigned shapeOffset = readU16(input);
  unsigned outlOffset = readU16(input);
  unsigned fillOffset = readU16(input);
  if (outlOffset)
  {
    input->seek(startPosition + outlOffset, WPX_SEEK_SET);
    readWaldoOutl(input);
  }
  if (fillOffset)
  {
    input->seek(startPosition + fillOffset, WPX_SEEK_SET);
    readWaldoFill(input);
  }
  if (shapeOffset)
  {
    input->seek(startPosition + shapeOffset, WPX_SEEK_SET);
    if (chunkType == 0x00) // Rectangle
      readRectangle(input);
    else if (chunkType == 0x01) // Ellipse
      readEllipse(input);
    else if (chunkType == 0x02) // Line and curve
      readLineAndCurve(input);
    /* else if (chunkType == 0x03) // Text
            readText(input); */
    else if (chunkType == 0x04) // Bitmap
      readBitmap(input);
  }
  input->seek(startPosition + length, WPX_SEEK_SET);
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
    unsigned fourCC = readU32(input);
    unsigned length = readU32(input);
    if (blockLengths)
      length=blockLengths[length];
    unsigned long position = input->tell();
    unsigned listType(0);
    if (fourCC == FOURCC_RIFF || fourCC == FOURCC_LIST)
    {
      listType = readU32(input);
      if (listType == FOURCC_stlt && m_version >= 700)
        fourCC = listType;
      else
        m_collector->collectOtherList();
    }
    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%i)\n", level, toFourCC(fourCC), length, length));

    if (fourCC == FOURCC_RIFF || fourCC == FOURCC_LIST)
    {
      CDR_DEBUG_MSG(("CDR listType: %s\n", toFourCC(listType)));
      unsigned cmprsize = length-4;
      if (listType == FOURCC_cmpr)
      {
        cmprsize  = readU32(input);
        input->seek(12, WPX_SEEK_CUR);
        if (readU32(input) != FOURCC_CPng)
          return false;
        if (readU16(input) != 1)
          return false;
        if (readU16(input) != 4)
          return false;
      }
      else if (listType == FOURCC_page)
        m_collector->collectPage(level);
      else if (listType == FOURCC_obj)
        m_collector->collectObject(level);
      else if (listType == FOURCC_grp)
        m_collector->collectGroup(level);
      else if ((listType & 0xffffff) == FOURCC_CDR || (listType & 0xffffff) == FOURCC_cdr)
      {
        m_version = getCDRVersion((listType & 0xff000000) >> 24);
        if (m_version < 600)
          m_precision = libcdr::PRECISION_16BIT;
        else
          m_precision = libcdr::PRECISION_32BIT;
      }
      else if (listType == FOURCC_vect)
        m_collector->collectVect(level);

      bool compressed = (listType == FOURCC_cmpr ? true : false);
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

void libcdr::CDRParser::readRecord(unsigned fourCC, unsigned length, WPXInputStream *input)
{
  long recordStart = input->tell();
  switch (fourCC)
  {
  case FOURCC_DISP:
    readDisp(input, length);
    break;
  case FOURCC_loda:
  case FOURCC_lobj:
    readLoda(input, length);
    break;
  case FOURCC_vrsn:
    readVersion(input, length);
    break;
  case FOURCC_trfd:
    readTrfd(input, length);
    break;
  case FOURCC_outl:
    readOutl(input, length);
    break;
  case FOURCC_fild:
  case FOURCC_fill:
    readFild(input, length);
    break;
  case FOURCC_arrw:
    break;
  case FOURCC_flgs:
    readFlags(input, length);
    break;
  case FOURCC_mcfg:
    readMcfg(input, length);
    break;
  case FOURCC_bmp:
    readBmp(input, length);
    break;
  case FOURCC_bmpf:
    readBmpf(input, length);
    break;
  case FOURCC_ppdt:
    readPpdt(input, length);
    break;
  case FOURCC_ftil:
    readFtil(input, length);
    break;
  case FOURCC_iccd:
    readIccd(input, length);
    break;
  case FOURCC_bbox:
    readBBox(input, length);
    break;
  case FOURCC_spnd:
    readSpnd(input, length);
    break;
  case FOURCC_uidr:
    readUidr(input, length);
    break;
  case FOURCC_vpat:
    readVpat(input, length);
    break;
  case FOURCC_font:
    readFont(input, length);
    break;
  case FOURCC_stlt:
    readStlt(input, length);
    break;
  case FOURCC_txsm:
    readTxsm(input, length);
    break;
  default:
    break;
  }
  input->seek(recordStart + length, WPX_SEEK_CUR);
}

double libcdr::CDRParser::readRectCoord(WPXInputStream *input)
{
  if (m_version < 1500)
    return readCoordinate(input);
  return readDouble(input) / 254000.0;
}

libcdr::CDRColor libcdr::CDRParser::readColor(WPXInputStream *input)
{
  unsigned short colorModel = 0;
  unsigned colorValue = 0;
  if (m_version >= 500)
  {
    colorModel = readU16(input);
    if (colorModel == 0x19)
    {
      unsigned char r = 0;
      unsigned char g = 0;
      unsigned char b = 0;
      unsigned char c = 0;
      unsigned char m = 0;
      unsigned char y = 0;
      unsigned char k = 100;
      unsigned short paletteID = readU16(input);
      input->seek(4, WPX_SEEK_CUR);
      unsigned short ix = readU16(input);
      unsigned short tint = readU16(input);
      switch (paletteID)
      {
      case 0x08:
        if (ix > 0
            && ix <= sizeof(palette_19_08_C)/sizeof(palette_19_08_C[0])
            && ix <= sizeof(palette_19_08_M)/sizeof(palette_19_08_M[0])
            && ix <= sizeof(palette_19_08_Y)/sizeof(palette_19_08_Y[0])
            && ix <= sizeof(palette_19_08_K)/sizeof(palette_19_08_K[0]))
        {
          c = palette_19_08_C[ix-1];
          m = palette_19_08_M[ix-1];
          y = palette_19_08_Y[ix-1];
          k = palette_19_08_K[ix-1];
        }
        break;
      case 0x09:
        if (ix > 0
            && ix <= sizeof(palette_19_09_L)/sizeof(palette_19_09_L[0])
            && ix <= sizeof(palette_19_09_A)/sizeof(palette_19_09_A[0])
            && ix <= sizeof(palette_19_09_B)/sizeof(palette_19_09_B[0]))
        {
          colorValue = palette_19_09_B[ix-1];
          colorValue <<= 8;
          colorValue |= palette_19_09_A[ix-1];
          colorValue <<= 8;
          colorValue |= palette_19_09_L[ix-1];
        }
        break;
      case 0x0a:
        if (ix > 0
            && ix <= sizeof(palette_19_0A_C)/sizeof(palette_19_0A_C[0])
            && ix <= sizeof(palette_19_0A_M)/sizeof(palette_19_0A_M[0])
            && ix <= sizeof(palette_19_0A_Y)/sizeof(palette_19_0A_Y[0])
            && ix <= sizeof(palette_19_0A_K)/sizeof(palette_19_0A_K[0]))
        {
          c = palette_19_0A_C[ix-1];
          m = palette_19_0A_M[ix-1];
          y = palette_19_0A_Y[ix-1];
          k = palette_19_0A_K[ix-1];
        }
        break;
      case 0x0b:
        if (ix > 0
            && ix <= sizeof(palette_19_0B_C)/sizeof(palette_19_0B_C[0])
            && ix <= sizeof(palette_19_0B_M)/sizeof(palette_19_0B_M[0])
            && ix <= sizeof(palette_19_0B_Y)/sizeof(palette_19_0B_Y[0])
            && ix <= sizeof(palette_19_0B_K)/sizeof(palette_19_0B_K[0]))
        {
          c = palette_19_0B_C[ix-1];
          m = palette_19_0B_M[ix-1];
          y = palette_19_0B_Y[ix-1];
          k = palette_19_0B_K[ix-1];
        }
        break;
      case 0x11:
        if (ix > 0
            && ix <= sizeof(palette_19_11_C)/sizeof(palette_19_11_C[0])
            && ix <= sizeof(palette_19_11_M)/sizeof(palette_19_11_M[0])
            && ix <= sizeof(palette_19_11_Y)/sizeof(palette_19_11_Y[0])
            && ix <= sizeof(palette_19_11_K)/sizeof(palette_19_11_K[0]))
        {
          c = palette_19_11_C[ix-1];
          m = palette_19_11_M[ix-1];
          y = palette_19_11_Y[ix-1];
          k = palette_19_11_K[ix-1];
        }
        break;
      case 0x12:
        if (ix > 0
            && ix <= sizeof(palette_19_12_C)/sizeof(palette_19_12_C[0])
            && ix <= sizeof(palette_19_12_M)/sizeof(palette_19_12_M[0])
            && ix <= sizeof(palette_19_12_Y)/sizeof(palette_19_12_Y[0])
            && ix <= sizeof(palette_19_12_K)/sizeof(palette_19_12_K[0]))
        {
          c = palette_19_12_C[ix-1];
          m = palette_19_12_M[ix-1];
          y = palette_19_12_Y[ix-1];
          k = palette_19_12_K[ix-1];
        }
        break;
      case 0x14:
        if (ix > 0
            && ix <= sizeof(palette_19_14_C)/sizeof(palette_19_14_C[0])
            && ix <= sizeof(palette_19_14_M)/sizeof(palette_19_14_M[0])
            && ix <= sizeof(palette_19_14_Y)/sizeof(palette_19_14_Y[0])
            && ix <= sizeof(palette_19_14_K)/sizeof(palette_19_14_K[0]))
        {
          c = palette_19_14_C[ix-1];
          m = palette_19_14_M[ix-1];
          y = palette_19_14_Y[ix-1];
          k = palette_19_14_K[ix-1];
        }
        break;
      case 0x15:
        if (ix > 0
            && ix <= sizeof(palette_19_15_C)/sizeof(palette_19_15_C[0])
            && ix <= sizeof(palette_19_15_M)/sizeof(palette_19_15_M[0])
            && ix <= sizeof(palette_19_15_Y)/sizeof(palette_19_15_Y[0])
            && ix <= sizeof(palette_19_15_K)/sizeof(palette_19_15_K[0]))
        {
          c = palette_19_15_C[ix-1];
          m = palette_19_15_M[ix-1];
          y = palette_19_15_Y[ix-1];
          k = palette_19_15_K[ix-1];
        }
        break;
      case 0x16:
        if (ix > 0
            && ix <= sizeof(palette_19_16_C)/sizeof(palette_19_16_C[0])
            && ix <= sizeof(palette_19_16_M)/sizeof(palette_19_16_M[0])
            && ix <= sizeof(palette_19_16_Y)/sizeof(palette_19_16_Y[0])
            && ix <= sizeof(palette_19_16_K)/sizeof(palette_19_16_K[0]))
        {
          c = palette_19_16_C[ix-1];
          m = palette_19_16_M[ix-1];
          y = palette_19_16_Y[ix-1];
          k = palette_19_16_K[ix-1];
        }
        break;
      case 0x17:
        if (ix > 0
            && ix <= sizeof(palette_19_17_C)/sizeof(palette_19_17_C[0])
            && ix <= sizeof(palette_19_17_M)/sizeof(palette_19_17_M[0])
            && ix <= sizeof(palette_19_17_Y)/sizeof(palette_19_17_Y[0])
            && ix <= sizeof(palette_19_17_K)/sizeof(palette_19_17_K[0]))
        {
          c = palette_19_17_C[ix-1];
          m = palette_19_17_M[ix-1];
          y = palette_19_17_Y[ix-1];
          k = palette_19_17_K[ix-1];
        }
        break;
      case 0x1a:
        if (ix < sizeof(palette_19_1A_C)/sizeof(palette_19_1A_C[0])
            && ix < sizeof(palette_19_1A_M)/sizeof(palette_19_1A_M[0])
            && ix < sizeof(palette_19_1A_Y)/sizeof(palette_19_1A_Y[0])
            && ix < sizeof(palette_19_1A_K)/sizeof(palette_19_1A_K[0]))
        {
          c = palette_19_1A_C[ix];
          m = palette_19_1A_M[ix];
          y = palette_19_1A_Y[ix];
          k = palette_19_1A_K[ix];
        }
        break;
      case 0x1b:
        if (ix < sizeof(palette_19_1B_C)/sizeof(palette_19_1B_C[0])
            && ix < sizeof(palette_19_1B_M)/sizeof(palette_19_1B_M[0])
            && ix < sizeof(palette_19_1B_Y)/sizeof(palette_19_1B_Y[0])
            && ix < sizeof(palette_19_1B_K)/sizeof(palette_19_1B_K[0]))
        {
          c = palette_19_1B_C[ix];
          m = palette_19_1B_M[ix];
          y = palette_19_1B_Y[ix];
          k = palette_19_1B_K[ix];
        }
        break;
      case 0x1c:
        if (ix < sizeof(palette_19_1C_C)/sizeof(palette_19_1C_C[0])
            && ix < sizeof(palette_19_1C_M)/sizeof(palette_19_1C_M[0])
            && ix < sizeof(palette_19_1C_Y)/sizeof(palette_19_1C_Y[0])
            && ix < sizeof(palette_19_1C_K)/sizeof(palette_19_1C_K[0]))
        {
          c = palette_19_1C_C[ix];
          m = palette_19_1C_M[ix];
          y = palette_19_1C_Y[ix];
          k = palette_19_1C_K[ix];
        }
        break;
      case 0x1d:
        if (ix < sizeof(palette_19_1D_C)/sizeof(palette_19_1D_C[0])
            && ix < sizeof(palette_19_1D_M)/sizeof(palette_19_1D_M[0])
            && ix < sizeof(palette_19_1D_Y)/sizeof(palette_19_1D_Y[0])
            && ix < sizeof(palette_19_1D_K)/sizeof(palette_19_1D_K[0]))
        {
          c = palette_19_1D_C[ix];
          m = palette_19_1D_M[ix];
          y = palette_19_1D_Y[ix];
          k = palette_19_1D_K[ix];
        }
        break;
      case 0x1e:
        if (ix > 0
            && ix <= sizeof(palette_19_1E_R)/sizeof(palette_19_1E_R[0])
            && ix <= sizeof(palette_19_1E_G)/sizeof(palette_19_1E_G[0])
            && ix <= sizeof(palette_19_1E_B)/sizeof(palette_19_1E_B[0]))
        {
          r = palette_19_1E_R[ix-1];
          g = palette_19_1E_G[ix-1];
          b = palette_19_1E_B[ix-1];
        }
        break;
      case 0x1f:
        if (ix > 0
            && ix <= sizeof(palette_19_1F_R)/sizeof(palette_19_1F_R[0])
            && ix <= sizeof(palette_19_1F_G)/sizeof(palette_19_1F_G[0])
            && ix <= sizeof(palette_19_1F_B)/sizeof(palette_19_1F_B[0]))
        {
          r = palette_19_1F_R[ix-1];
          g = palette_19_1F_G[ix-1];
          b = palette_19_1F_B[ix-1];
        }
        break;
      case 0x20:
        if (ix > 0
            && ix <= sizeof(palette_19_20_R)/sizeof(palette_19_20_R[0])
            && ix <= sizeof(palette_19_20_G)/sizeof(palette_19_20_G[0])
            && ix <= sizeof(palette_19_20_B)/sizeof(palette_19_20_B[0]))
        {
          r = palette_19_20_R[ix-1];
          g = palette_19_20_G[ix-1];
          b = palette_19_20_B[ix-1];
        }
        break;
      case 0x23:
        if (ix > 0
            && ix <= sizeof(palette_19_23_C)/sizeof(palette_19_23_C[0])
            && ix <= sizeof(palette_19_23_M)/sizeof(palette_19_23_M[0])
            && ix <= sizeof(palette_19_23_Y)/sizeof(palette_19_23_Y[0])
            && ix <= sizeof(palette_19_23_K)/sizeof(palette_19_23_K[0]))
        {
          c = palette_19_23_C[ix-1];
          m = palette_19_23_M[ix-1];
          y = palette_19_23_Y[ix-1];
          k = palette_19_23_K[ix-1];
        }
        break;
      case 0x24:
        if (ix > 0
            && ix <= sizeof(palette_19_24_C)/sizeof(palette_19_24_C[0])
            && ix <= sizeof(palette_19_24_M)/sizeof(palette_19_24_M[0])
            && ix <= sizeof(palette_19_24_Y)/sizeof(palette_19_24_Y[0])
            && ix <= sizeof(palette_19_24_K)/sizeof(palette_19_24_K[0]))
        {
          c = palette_19_24_C[ix-1];
          m = palette_19_24_M[ix-1];
          y = palette_19_24_Y[ix-1];
          k = palette_19_24_K[ix-1];
        }
        break;
      case 0x25:
        if (ix > 0
            && ix <= sizeof(palette_19_25_C)/sizeof(palette_19_25_C[0])
            && ix <= sizeof(palette_19_25_M)/sizeof(palette_19_25_M[0])
            && ix <= sizeof(palette_19_25_Y)/sizeof(palette_19_25_Y[0])
            && ix <= sizeof(palette_19_25_K)/sizeof(palette_19_25_K[0]))
        {
          c = palette_19_25_C[ix-1];
          m = palette_19_25_M[ix-1];
          y = palette_19_25_Y[ix-1];
          k = palette_19_25_K[ix-1];
        }
        break;
      default:
        colorValue = tint;
        colorValue <<= 16;
        colorValue |= ix;
        break;
      }

      switch (paletteID)
      {
      case 0x08:
      case 0x0a:
      case 0x0b:
      case 0x11:
      case 0x12:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x23:
      case 0x24:
      case 0x25:
      {
        colorModel = 0x02; // CMYK100
        unsigned cyan = (unsigned)tint * (unsigned)c / 100;
        unsigned magenta = (unsigned)tint * (unsigned)m / 100;
        unsigned yellow = (unsigned)tint * (unsigned)y / 100;
        unsigned black = (unsigned)tint * (unsigned)k / 100;
        colorValue = (black & 0xff);
        colorValue <<= 8;
        colorValue |= (yellow & 0xff);
        colorValue <<= 8;
        colorValue |= (magenta & 0xff);
        colorValue <<= 8;
        colorValue |= (cyan & 0xff);
        break;
      }
      case 0x1e:
      case 0x1f:
      case 0x20:
      {
        colorModel = 0x05; // RGB
        unsigned red = (unsigned)tint * (unsigned)r + 255 * (100 - tint);
        unsigned green = (unsigned)tint * (unsigned)g + 255 * (100 - tint);
        unsigned blue = (unsigned )tint * (unsigned)b + 255 * (100 - tint);
        red /= 100;
        green /= 100;
        blue /= 100;
        colorValue = (blue & 0xff);
        colorValue <<= 8;
        colorValue |= (green & 0xff);
        colorValue <<= 8;
        colorValue |= (red & 0xff);
        break;
      }
      case 0x09:
        colorModel = 0x12; // L*a*b
        break;
      default:
        break;
      }
    }
    else if (colorModel == 0x0e)
    {
      unsigned short paletteID = readU16(input);
      input->seek(4, WPX_SEEK_CUR);
      unsigned short ix = readU16(input);
      unsigned short tint = readU16(input);
      switch (paletteID)
      {
      case 0x0c:
        if (ix > 0
            && ix <= sizeof(palette_0E_0C_L)/sizeof(palette_0E_0C_L[0])
            && ix <= sizeof(palette_0E_0C_A)/sizeof(palette_0E_0C_A[0])
            && ix <= sizeof(palette_0E_0C_B)/sizeof(palette_0E_0C_B[0]))
        {
          colorValue = palette_0E_0C_B[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_0C_A[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_0C_L[ix-1];
        }
        break;
      case 0x18:
        if (ix > 0
            && ix <= sizeof(palette_0E_18_L)/sizeof(palette_0E_18_L[0])
            && ix <= sizeof(palette_0E_18_A)/sizeof(palette_0E_18_A[0])
            && ix <= sizeof(palette_0E_18_B)/sizeof(palette_0E_18_B[0]))
        {
          colorValue = palette_0E_18_B[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_18_A[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_18_L[ix-1];
        }
        break;
      case 0x21:
        if (ix > 0
            && ix <= sizeof(palette_0E_21_L)/sizeof(palette_0E_21_L[0])
            && ix <= sizeof(palette_0E_21_A)/sizeof(palette_0E_21_A[0])
            && ix <= sizeof(palette_0E_21_B)/sizeof(palette_0E_21_B[0]))
        {
          colorValue = palette_0E_21_B[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_21_A[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_21_L[ix-1];
        }
        break;
      case 0x22:
        if (ix > 0
            && ix <= sizeof(palette_0E_22_L)/sizeof(palette_0E_22_L[0])
            && ix <= sizeof(palette_0E_22_A)/sizeof(palette_0E_22_A[0])
            && ix <= sizeof(palette_0E_22_B)/sizeof(palette_0E_22_B[0]))
        {
          colorValue = palette_0E_22_B[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_22_A[ix-1];
          colorValue <<= 8;
          colorValue |= palette_0E_22_L[ix-1];
        }
        break;
      default:
        colorValue = tint;
        colorValue <<= 16;
        colorValue |= ix;
        break;
      }

      switch (paletteID)
      {
      case 0x0c:
      case 0x18:
      case 0x21:
      case 0x22:
        colorModel = 0x12; // L*a*b
        break;
      default:
        break;
      }
    }
    else
    {
      input->seek(6, WPX_SEEK_CUR);
      colorValue = readU32(input);
    }
  }
  else if (m_version >= 400)
  {
    colorModel = readU16(input);
    unsigned short c = readU16(input);
    unsigned short m = readU16(input);
    unsigned short y = readU16(input);
    unsigned short k = readU16(input);
    colorValue = (k & 0xff);
    colorValue <<= 8;
    colorValue |= (y & 0xff);
    colorValue <<= 8;
    colorValue |= (m & 0xff);
    colorValue <<= 8;
    colorValue |= (c & 0xff);
    input->seek(2, WPX_SEEK_CUR);
  }
  else
  {
    colorModel = readU8(input);
    colorValue = readU32(input);
  }
  return libcdr::CDRColor(colorModel, colorValue);
}

void libcdr::CDRParser::readRectangle(WPXInputStream *input)
{
  double x0 = readRectCoord(input);
  double y0 = readRectCoord(input);
  double r3 = 0.0;
  double r2 = 0.0;
  double r1 = 0.0;
  double r0 = 0.0;
  unsigned int corner_type = 0;
  double scaleX = 1.0;
  double scaleY = 1.0;

  if (m_version < 1500)
  {
    r3 = readRectCoord(input);
    r2 = m_version < 900 ? r3 : readRectCoord(input);
    r1 = m_version < 900 ? r3 : readRectCoord(input);
    r0 = m_version < 900 ? r3 : readRectCoord(input);
  }
  else
  {
    scaleX = readDouble(input);
    scaleY = readDouble(input);
    unsigned int scale_with = readU8(input);
    input->seek(7, WPX_SEEK_CUR);
    if (scale_with == 0)
    {
      r3 = readDouble(input);
      corner_type = readU8(input);
      input->seek(15, WPX_SEEK_CUR);
      r2 = readDouble(input);
      input->seek(16, WPX_SEEK_CUR);
      r1 = readDouble(input);
      input->seek(16, WPX_SEEK_CUR);
      r0 = readDouble(input);

      double width = fabs(x0*scaleX / 2.0);
      double height = fabs(y0*scaleY / 2.0);
      r3 *= width < height ? width : height;
      r2 *= width < height ? width : height;
      r1 *= width < height ? width : height;
      r0 *= width < height ? width : height;
    }
    else
    {
      r3 = readRectCoord(input);
      corner_type = readU8(input);
      input->seek(15, WPX_SEEK_CUR);
      r2 = readRectCoord(input);
      input->seek(16, WPX_SEEK_CUR);
      r1 = readRectCoord(input);
      input->seek(16, WPX_SEEK_CUR);
      r0 = readRectCoord(input);
    }
  }
  if (r0 > 0.0)
    m_collector->collectMoveTo(0.0, -r0/scaleY);
  else
    m_collector->collectMoveTo(0.0, 0.0);
  if (r1 > 0.0)
  {
    m_collector->collectLineTo(0.0, y0+r1/scaleY);
    if (corner_type == 0)
      m_collector->collectQuadraticBezier(0.0, y0, r1/scaleX, y0);
    else if (corner_type == 1)
      m_collector->collectQuadraticBezier(r1/scaleX, y0+r1/scaleY, r1/scaleX, y0);
    else if (corner_type == 2)
      m_collector->collectLineTo(r1/scaleX, y0);
  }
  else
    m_collector->collectLineTo(0.0, y0);
  if (r2 > 0.0)
  {
    m_collector->collectLineTo(x0-r2/scaleX, y0);
    if (corner_type == 0)
      m_collector->collectQuadraticBezier(x0, y0, x0, y0+r2/scaleY);
    else if (corner_type == 1)
      m_collector->collectQuadraticBezier(x0-r2/scaleX, y0+r2/scaleY, x0, y0+r2/scaleY);
    else if (corner_type == 2)
      m_collector->collectLineTo(x0, y0+r2/scaleY);
  }
  else
    m_collector->collectLineTo(x0, y0);
  if (r3 > 0.0)
  {
    m_collector->collectLineTo(x0, -r3/scaleY);
    if (corner_type == 0)
      m_collector->collectQuadraticBezier(x0, 0.0, x0-r3/scaleX, 0.0);
    else if (corner_type == 1)
      m_collector->collectQuadraticBezier(x0-r3/scaleX, -r3/scaleY, x0-r3/scaleX, 0.0);
    else if (corner_type == 2)
      m_collector->collectLineTo(x0-r3/scaleX, 0.0);
  }
  else
    m_collector->collectLineTo(x0, 0.0);
  if (r0 > 0.0)
  {
    m_collector->collectLineTo(r0/scaleX, 0.0);
    if (corner_type == 0)
      m_collector->collectQuadraticBezier(0.0, 0.0, 0.0, -r0/scaleY);
    else if (corner_type == 1)
      m_collector->collectQuadraticBezier(r0/scaleX, -r0/scaleY, 0.0, -r0/scaleY);
    else if (corner_type == 2)
      m_collector->collectLineTo(0.0, -r0/scaleY);
  }
  else
    m_collector->collectLineTo(0.0, 0.0);
}

void libcdr::CDRParser::readEllipse(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readEllipse\n"));

  double x = readCoordinate(input);
  double y = readCoordinate(input);
  double angle1 = readAngle(input);
  double angle2 = readAngle(input);
  bool pie(0 != readUnsigned(input));

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
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
#if DUMP_PREVIEW_IMAGE
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

  unsigned short pointNum = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned char> pointTypes;
  for (unsigned j=0; j<pointNum; j++)
  {
    std::pair<double, double> point;
    point.first = (double)readCoordinate(input);
    point.second = (double)readCoordinate(input);
    points.push_back(point);
  }
  for (unsigned k=0; k<pointNum; k++)
    pointTypes.push_back(readU8(input));
  outputPath(points, pointTypes);
}

void libcdr::CDRParser::readPath(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readPath\n"));

  input->seek(4, WPX_SEEK_CUR);
  unsigned short pointNum = readU16(input)+readU16(input);
  input->seek(16, WPX_SEEK_CUR);
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned char> pointTypes;
  for (unsigned j=0; j<pointNum; j++)
  {
    std::pair<double, double> point;
    point.first = (double)readCoordinate(input);
    point.second = (double)readCoordinate(input);
    points.push_back(point);
  }
  for (unsigned k=0; k<pointNum; k++)
    pointTypes.push_back(readU8(input));
  outputPath(points, pointTypes);
}

void libcdr::CDRParser::readBitmap(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readBitmap\n"));

  double x1 = 0.0;
  double y1 = 0.0;
  double x2 = 0.0;
  double y2 = 0.0;
  unsigned imageId = 0;
  if (m_version < 600)
  {
    x1 = readCoordinate(input);
    y1 = readCoordinate(input);
    x2 = 0.0;
    y2 = 0.0;
    if (m_version < 400)
      input->seek(2, WPX_SEEK_CUR);
    input->seek(8, WPX_SEEK_CUR);
    imageId = readUnsigned(input);
    input->seek(20, WPX_SEEK_CUR);
    m_collector->collectMoveTo(x1, y1);
    m_collector->collectLineTo(x1, y2);
    m_collector->collectLineTo(x2, y2);
    m_collector->collectLineTo(x2, y1);
    m_collector->collectLineTo(x1, y1);
  }
  else
  {
    x1 = (double)readCoordinate(input);
    y1 = (double)readCoordinate(input);
    x2 = (double)readCoordinate(input);
    y2 = (double)readCoordinate(input);
    input->seek(16, WPX_SEEK_CUR);

    input->seek(16, WPX_SEEK_CUR);
    imageId = readUnsigned(input);
    if (m_version < 800)
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
      point.first = (double)readCoordinate(input);
      point.second = (double)readCoordinate(input);
      points.push_back(point);
    }
    for (unsigned k=0; k<pointNum; k++)
      pointTypes.push_back(readU8(input));
    outputPath(points, pointTypes);
  }
  m_collector->collectBitmap(imageId, x1, x2, y1, y2);
}

void libcdr::CDRParser::readWaldoOutl(WPXInputStream *input)
{
  if (m_version >= 400)
    return;
  unsigned short lineType = readU8(input);
  lineType <<= 1;
  double lineWidth = (double)readCoordinate(input);
  double stretch = (double)readU16(input) / 100.0;
  double angle = readAngle(input);
  libcdr::CDRColor color = readColor(input);
  input->seek(7, WPX_SEEK_CUR);
  unsigned short numDash = readU8(input);
  int fixPosition = input->tell();
  std::vector<unsigned short> dashArray;
  for (unsigned short i = 0; i < numDash; ++i)
    dashArray.push_back(readU8(input));
  input->seek(fixPosition + 10, WPX_SEEK_SET);
  unsigned short joinType = readU16(input);
  unsigned short capsType = readU16(input);
  unsigned startMarkerId = readU32(input);
  unsigned endMarkerId = readU32(input);
  m_collector->collectOutl(++m_outlId, lineType, capsType, joinType, lineWidth, stretch, angle, color, dashArray, startMarkerId, endMarkerId);
  m_collector->collectOutlId(m_outlId);
}

void libcdr::CDRParser::readWaldoFill(WPXInputStream *input)
{
  if (m_version >= 400)
    return;
  unsigned short fillType = readU8(input);
  libcdr::CDRColor color1;
  libcdr::CDRColor color2;
  libcdr::CDRImageFill imageFill;
  libcdr::CDRGradient gradient;
  switch (fillType)
  {
  case 1: // Solid
  {
    color1 = readColor(input);
  }
  break;
  case 2: // Linear Gradient
  {
    gradient.m_type = 1;
    gradient.m_angle = readAngle(input);
    color1 = readColor(input);
    color2 = readColor(input);
    if (m_version >= 200)
    {
      input->seek(7, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readS16(input);
      gradient.m_centerXOffset = readInteger(input);
      gradient.m_centerYOffset = readInteger(input);
    }
    libcdr::CDRGradientStop stop;
    stop.m_color = color1;
    stop.m_offset = 0.0;
    gradient.m_stops.push_back(stop);
    stop.m_color = color2;
    stop.m_offset = 1.0;
    gradient.m_stops.push_back(stop);
  }
  break;
  case 4: // Radial Gradient
  {
    gradient.m_type = 2;
    fillType = 2;
    gradient.m_angle = readAngle(input);
    color1 = readColor(input);
    color2 = readColor(input);
    if (m_version >= 200)
    {
      input->seek(7, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readS16(input);
      gradient.m_centerXOffset = readInteger(input);
      gradient.m_centerYOffset = readInteger(input);
    }
    libcdr::CDRGradientStop stop;
    stop.m_color = color1;
    stop.m_offset = 0.0;
    gradient.m_stops.push_back(stop);
    stop.m_color = color2;
    stop.m_offset = 1.0;
    gradient.m_stops.push_back(stop);
  }
  break;
  case 7: // Pattern
  {
    unsigned patternId = m_version < 300 ? readU16(input) : readU32(input);
    double patternWidth = readCoordinate(input);
    double patternHeight = readCoordinate(input);
    double tileOffsetX = (double)readU16(input) / 100.0;
    double tileOffsetY = (double)readU16(input) / 100.0;
    double rcpOffset = (double)readU16(input) / 100.0;
    input->seek(1, WPX_SEEK_CUR);
    color1 = readColor(input);
    color2 = readColor(input);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, false, tileOffsetX, tileOffsetY, rcpOffset, 0);
  }
  break;
  case 10: // Full color
  {
    unsigned patternId = readU16(input);
    double patternWidth = readCoordinate(input);
    double patternHeight = readCoordinate(input);
    double tileOffsetX = (double)readU16(input) / 100.0;
    double tileOffsetY = (double)readU16(input) / 100.0;
    double rcpOffset = (double)readU16(input) / 100.0;
    input->seek(1, WPX_SEEK_CUR);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, false, tileOffsetX, tileOffsetY, rcpOffset, 0);
  }
  break;
  default:
    break;
  }
  m_collector->collectFild(++m_fillId, fillType, color1, color2, gradient, imageFill);
  m_collector->collectFildId(m_fillId);
}

void libcdr::CDRParser::readTrfd(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  long startPosition = input->tell();
  unsigned chunkLength = readUnsigned(input);
  unsigned numOfArgs = readUnsigned(input);
  unsigned startOfArgs = readUnsigned(input);
  std::vector<unsigned> argOffsets(numOfArgs, 0);
  unsigned i = 0;
  input->seek(startPosition+startOfArgs, WPX_SEEK_SET);
  while (i<numOfArgs)
    argOffsets[i++] = readUnsigned(input);

  for (i=0; i < argOffsets.size(); i++)
  {
    input->seek(startPosition+argOffsets[i], WPX_SEEK_SET);
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    unsigned short tmpType = readU16(input);
    if (tmpType == 0x08) // trafo
    {
      double v0 = 0.0;
      double v1 = 0.0;
      double x0 = 0.0;
      double v3 = 0.0;
      double v4 = 0.0;
      double y0 = 0.0;
      if (m_version >= 600)
        input->seek(6, WPX_SEEK_CUR);
      if (m_version >= 500)
      {
        v0 = readDouble(input);
        v1 = readDouble(input);
        x0 = readDouble(input) / (m_version < 600 ? 1000.0 : 254000.0);
        v3 = readDouble(input);
        v4 = readDouble(input);
        y0 = readDouble(input) / (m_version < 600 ? 1000.0 : 254000.0);
      }
      else
      {
        v0 = readFixedPoint(input);
        v1 = readFixedPoint(input);
        x0 = (double)readS32(input) / 1000.0;
        v3 = readFixedPoint(input);
        v4 = readFixedPoint(input);
        y0 = (double)readS32(input) / 1000.0;
      }
      m_collector->collectTransform(v0, v1, x0, v3, v4, y0, m_version < 400);
    }
    else if (tmpType == 0x10)
    {
#if 0
      input->seek(6, WPX_SEEK_CUR);
      unsigned short type = readU16(input);
      double x = readCoordinate(input);
      double y = readCoordinate(input);
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

void libcdr::CDRParser::readFild(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
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
  switch (fillType)
  {
  case 1: // Solid
  {
    if (m_version >= 1300)
      input->seek(13, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    color1 = readColor(input);
  }
  break;
  case 2: // Gradient
  {
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    gradient.m_type = readU8(input);
    if (m_version >= 1300)
    {
      input->seek(17, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readS16(input);
    }
    else if (m_version >= 600)
    {
      input->seek(19, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readS32(input);
    }
    else
    {
      input->seek(11, WPX_SEEK_CUR);
      gradient.m_edgeOffset = readS16(input);
    }
    gradient.m_angle = readAngle(input);
    gradient.m_centerXOffset = readInteger(input);
    gradient.m_centerYOffset = readInteger(input);
    if (m_version >= 600)
      input->seek(2, WPX_SEEK_CUR);
    gradient.m_mode = (unsigned short)(readUnsigned(input) & 0xffff);
    gradient.m_midPoint = (double)readU8(input) / 100.0;
    input->seek(1, WPX_SEEK_CUR);
    unsigned short numStops = (unsigned short)(readUnsigned(input) & 0xffff);
    if (m_version >= 1300)
      input->seek(3, WPX_SEEK_CUR);
    for (unsigned short i = 0; i < numStops; ++i)
    {
      libcdr::CDRGradientStop stop;
      stop.m_color = readColor(input);
      if (m_version >= 1300)
      {
        if (v13flag == 0x9e || (m_version >= 1600 && v13flag == 0x96))
          input->seek(26, WPX_SEEK_CUR);
        else
          input->seek(5, WPX_SEEK_CUR);
      }
      stop.m_offset = (double)readUnsigned(input) / 100.0;
      if (m_version >= 1300)
        input->seek(3, WPX_SEEK_CUR);
      gradient.m_stops.push_back(stop);
    }
  }
  break;
  case 7: // Pattern
  {
    if (m_version >= 1300)
      input->seek(8, WPX_SEEK_CUR);
    else
      input->seek(2, WPX_SEEK_CUR);
    unsigned patternId = readU32(input);
    int tmpWidth = readInteger(input);
    int tmpHeight = readInteger(input);
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
    double patternWidth = (double)tmpWidth / (m_version < 600 ? 1000.0 : 254000.0);
    double patternHeight = (double)tmpHeight / (m_version < 600 ? 1000.0 : 254000.0);
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
    color1 = readColor(input);
    if (m_version >= 1300)
    {
      if (v13flag == 0x94 || (m_version >= 1600 && v13flag == 0x8c))
        input->seek(31, WPX_SEEK_CUR);
      else
        input->seek(10, WPX_SEEK_CUR);
    }
    color2 = readColor(input);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
  }
  break;
  case 9: // bitmap
  {
    if (m_version < 600)
      fillType = 10;
    input->seek(2, WPX_SEEK_CUR);
    unsigned patternId = readUnsigned(input);
    if (m_version >= 1600)
      input->seek(26, WPX_SEEK_CUR);
    else if (m_version >= 1300)
      input->seek(2, WPX_SEEK_CUR);
    int tmpWidth = readUnsigned(input);
    int tmpHeight = readUnsigned(input);
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
    double patternWidth = (double)tmpWidth / (m_version < 600 ? 1000.0 :254000.0);
    double patternHeight = (double)tmpHeight / (m_version < 600 ? 1000.0 :254000.0);
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
    if (m_version >= 600)
      patternId = readUnsigned(input);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
  }
  break;
  case 10: // Full color
  {
    if (m_version >= 1300)
    {
      if (v13flag == 0x4e)
        input->seek(28, WPX_SEEK_CUR);
      else
        input->seek(4, WPX_SEEK_CUR);
    }
    else
      input->seek(2, WPX_SEEK_CUR);
    unsigned patternId = readUnsigned(input);
    int tmpWidth = readUnsigned(input);
    int tmpHeight = readUnsigned(input);
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
    double patternWidth = (double)tmpWidth / (m_version < 600 ? 1000.0 :254000.0);
    double patternHeight = (double)tmpHeight / (m_version < 600 ? 1000.0 :254000.0);
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
    if (m_version >= 600)
      patternId = readUnsigned(input);
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
  }
  break;
  case 11: // Texture
  {
    if (m_version < 600)
      fillType = 10;
    if (m_version >= 1300)
    {
      if (v13flag == 0x18e)
        input->seek(36, WPX_SEEK_CUR);
      else
        input->seek(1, WPX_SEEK_CUR);
    }
    else
      input->seek(2, WPX_SEEK_CUR);
    unsigned patternId = readU32(input);
    double patternWidth = 1.0;
    double patternHeight = 1.0;
    bool isRelative = true;
    double tileOffsetX = 0.0;
    double tileOffsetY = 0.0;
    double rcpOffset = 0.0;
    unsigned char flags = 0;
    if (m_version >= 600)
    {
      int tmpWidth = readUnsigned(input);
      int tmpHeight = readUnsigned(input);
      if (m_version < 900)
      {
        tileOffsetX = (double)readU16(input) / 100.0;
        tileOffsetY = (double)readU16(input) / 100.0;
      }
      else
        input->seek(4, WPX_SEEK_CUR);
      rcpOffset = (double)readU16(input) / 100.0;
      flags = readU8(input);
      patternWidth = (double)tmpWidth / (m_version < 600 ? 1000.0 :254000.0);
      patternHeight = (double)tmpHeight / (m_version < 600 ? 1000.0 :254000.0);
      isRelative = false;
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
      if (m_version >= 600)
        patternId = readUnsigned(input);
    }
    imageFill = libcdr::CDRImageFill(patternId, patternWidth, patternHeight, isRelative, tileOffsetX, tileOffsetY, rcpOffset, flags);
  }
  break;
  default:
    break;
  }
  m_collector->collectFild(fillId, fillType, color1, color2, gradient, imageFill);
}

void libcdr::CDRParser::readOutl(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned lineId = readU32(input);
  if (m_version >= 1300)
  {
    unsigned id = 0;
    unsigned lngth = 0;
    while (id != 1)
    {
      input->seek(lngth, WPX_SEEK_CUR);
      id = readU32(input);
      lngth = readU32(input);
    }
  }
  unsigned short lineType = readU16(input);
  unsigned short capsType = readU16(input);
  unsigned short joinType = readU16(input);
  if (m_version < 1300 && m_version >= 600)
    input->seek(2, WPX_SEEK_CUR);
  double lineWidth = (double)readCoordinate(input);
  double stretch = (double)readU16(input) / 100.0;
  if (m_version >= 600)
    input->seek(2, WPX_SEEK_CUR);
  double angle = readAngle(input);
  if (m_version >= 1300)
    input->seek(46, WPX_SEEK_CUR);
  else if (m_version >= 600)
    input->seek(52, WPX_SEEK_CUR);
  libcdr::CDRColor color = readColor(input);
  if (m_version < 600)
    input->seek(10, WPX_SEEK_CUR);
  else
    input->seek(16, WPX_SEEK_CUR);
  unsigned short numDash = readU16(input);
  int fixPosition = input->tell();
  std::vector<unsigned short> dashArray;
  for (unsigned short i = 0; i < numDash; ++i)
    dashArray.push_back(readU16(input));
  if (m_version < 600)
    input->seek(fixPosition + 20, WPX_SEEK_SET);
  else
    input->seek(fixPosition + 22, WPX_SEEK_SET);
  unsigned startMarkerId = readU32(input);
  unsigned endMarkerId = readU32(input);
  m_collector->collectOutl(lineId, lineType, capsType, joinType, lineWidth, stretch, angle, color, dashArray, startMarkerId, endMarkerId);
}

void libcdr::CDRParser::readLoda(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  long startPosition = input->tell();
  unsigned chunkLength = readUnsigned(input);
  unsigned numOfArgs = readUnsigned(input);
  unsigned startOfArgs = readUnsigned(input);
  unsigned startOfArgTypes = readUnsigned(input);
  unsigned chunkType = readUnsigned(input);
  if (chunkType == 0x26)
    m_collector->collectSpline();
  std::vector<unsigned> argOffsets(numOfArgs, 0);
  std::vector<unsigned> argTypes(numOfArgs, 0);
  unsigned i = 0;
  input->seek(startPosition+startOfArgs, WPX_SEEK_SET);
  while (i<numOfArgs)
    argOffsets[i++] = readUnsigned(input);
  input->seek(startPosition+startOfArgTypes, WPX_SEEK_SET);
  while (i>0)
    argTypes[--i] = readUnsigned(input);

  for (i=0; i < argTypes.size(); i++)
  {
    input->seek(startPosition+argOffsets[i], WPX_SEEK_SET);
    if (argTypes[i] == 0x1e) // loda coords
    {
      if ((m_version >= 400 && chunkType == 0x01) || (m_version < 400 && chunkType == 0x00)) // Rectangle
        readRectangle(input);
      else if ((m_version >= 400 && chunkType == 0x02) || (m_version < 400 && chunkType == 0x01)) // Ellipse
        readEllipse(input);
      else if ((m_version >= 400 && chunkType == 0x03) || (m_version < 400 && chunkType == 0x02)) // Line and curve
        readLineAndCurve(input);
      else if (chunkType == 0x25) // Path
        readPath(input);
      else if ((m_version >= 400 && chunkType == 0x04) || (m_version < 400 && chunkType == 0x03)) // Artistic text
        m_collector->collectArtisticText();
      else if ((m_version >= 400 && chunkType == 0x05) || (m_version < 400 && chunkType == 0x04)) // Bitmap
        readBitmap(input);
      else if ((m_version >= 400 && chunkType == 0x06) || (m_version < 400 && chunkType == 0x05)) // Paragraph text
        m_collector->collectParagraphText();
      else if (chunkType == 0x14) // Polygon
        readPolygonCoords(input);
    }
    else if (argTypes[i] == 0x14)
    {
      if (m_version < 400)
        readWaldoFill(input);
      else
        m_collector->collectFildId(readU32(input));
    }
    else if (argTypes[i] == 0x0a)
    {
      if (m_version < 400)
        readWaldoOutl(input);
      else
        m_collector->collectOutlId(readU32(input));
    }
    else if (argTypes[i] == 0x2af8)
      readPolygonTransform(input);
    else if (argTypes[i] == 0x1f40)
      readOpacity(input, length);
    else if (argTypes[i] == 0x64)
    {
      if (m_version < 400)
        readWaldoTrfd(input);
    }
    else if (argTypes[i] == 0x4aba)
      readPageSize(input);
  }
  input->seek(startPosition+chunkLength, WPX_SEEK_SET);
}

void libcdr::CDRParser::readFlags(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned flags = readU32(input);
  m_collector->collectFlags(flags, m_version >= 400);
}

void libcdr::CDRParser::readMcfg(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  if (m_version >= 1300)
    input->seek(12, WPX_SEEK_CUR);
  else if (m_version >= 900)
    input->seek(4, WPX_SEEK_CUR);
  else if (m_version < 700 && m_version >= 600)
    input->seek(0x1c, WPX_SEEK_CUR);
  double width = 0.0;
  double height = 0.0;
  if (m_version < 400)
  {
    input->seek(2, WPX_SEEK_CUR);
    double x0 = readCoordinate(input);
    double y0 = readCoordinate(input);
    double x1 = readCoordinate(input);
    double y1 = readCoordinate(input);
    width = fabs(x1-x0);
    height = fabs(y1-y0);
  }
  else
  {
    width = readCoordinate(input);
    height = readCoordinate(input);
  }
  m_collector->collectPageSize(width, height, -width/2.0, -height/2.0);
}

void libcdr::CDRParser::readPolygonCoords(WPXInputStream *input)
{
  CDR_DEBUG_MSG(("CDRParser::readPolygonCoords\n"));

  unsigned short pointNum = readU16(input);
  input->seek(2, WPX_SEEK_CUR);
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned char> pointTypes;
  for (unsigned j=0; j<pointNum; j++)
  {
    std::pair<double, double> point;
    point.first = (double)readCoordinate(input);
    point.second = (double)readCoordinate(input);
    points.push_back(point);
  }
  for (unsigned k=0; k<pointNum; k++)
    pointTypes.push_back(readU8(input));
  outputPath(points, pointTypes);
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
  double cx = readCoordinate(input);
  double cy = readCoordinate(input);
  m_collector->collectPolygonTransform(numAngles, nextPoint, rx, ry, cx, cy);
}

void libcdr::CDRParser::readPageSize(WPXInputStream *input)
{
  double width = readCoordinate(input);
  double height = readCoordinate(input);
  m_collector->collectPageSize(width, height, -width/2.0, -height/2.0);
}

void libcdr::CDRParser::readWaldoBmp(WPXInputStream *input, unsigned length, unsigned id)
{
  if (m_version >= 400)
    return;
  if (readU8(input) != 0x42)
    return;
  if (readU8(input) != 0x4d)
    return;
  input->seek(-2, WPX_SEEK_CUR);
  unsigned long tmpNumBytesRead = 0;
  const unsigned char *tmpBuffer = input->read(length, tmpNumBytesRead);
  if (!tmpNumBytesRead || length != tmpNumBytesRead)
    return;
  std::vector<unsigned char> bitmap(tmpNumBytesRead);
  memcpy(&bitmap[0], tmpBuffer, tmpNumBytesRead);
  m_collector->collectBmp(id, bitmap);
  return;
}

void libcdr::CDRParser::readBmp(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned imageId = readUnsigned(input);
  if (m_version < 500)
  {
    if (readU8(input) != 0x42)
      return;
    if (readU8(input) != 0x4d)
      return;
    unsigned lngth = readU32(input);
    input->seek(-6, WPX_SEEK_CUR);
    unsigned long tmpNumBytesRead = 0;
    const unsigned char *tmpBuffer = input->read(lngth, tmpNumBytesRead);
    if (!tmpNumBytesRead || lngth != tmpNumBytesRead)
      return;
    std::vector<unsigned char> bitmap(tmpNumBytesRead);
    memcpy(&bitmap[0], tmpBuffer, tmpNumBytesRead);
    m_collector->collectBmp(imageId, bitmap);
    return;
  }
  if (m_version < 600)
    input->seek(14, WPX_SEEK_CUR);
  else if (m_version < 700)
    input->seek(46, WPX_SEEK_CUR);
  else
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

void libcdr::CDRParser::readOpacity(WPXInputStream *input, unsigned /* length */)
{
  if (m_version < 1300)
    input->seek(10, WPX_SEEK_CUR);
  else
    input->seek(14, WPX_SEEK_CUR);
  double opacity = (double)readU16(input) / 1000.0;
  m_collector->collectFillOpacity(opacity);
}

void libcdr::CDRParser::readBmpf(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
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

void libcdr::CDRParser::readWaldoBmpf(WPXInputStream *input, unsigned id)
{
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
  std::vector<unsigned char> pattern(dataSize);
  unsigned long tmpNumBytesRead = 0;
  input->seek(24, WPX_SEEK_CUR); // TODO: is this empirical experience universal???
  const unsigned char *tmpBuffer = input->read(dataSize, tmpNumBytesRead);
  if (dataSize != tmpNumBytesRead)
    return;
  memcpy(&pattern[0], tmpBuffer, dataSize);
  m_collector->collectBmpf(id, width, height, pattern);
}

void libcdr::CDRParser::readPpdt(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned short pointNum = readU16(input);
  input->seek(4, WPX_SEEK_CUR);
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned> knotVector;
  for (unsigned j=0; j<pointNum; j++)
  {
    std::pair<double, double> point;
    point.first = (double)readCoordinate(input);
    point.second = (double)readCoordinate(input);
    points.push_back(point);
  }
  for (unsigned k=0; k<pointNum; k++)
    knotVector.push_back(readU32(input));
  m_collector->collectPpdt(points, knotVector);
}

void libcdr::CDRParser::readFtil(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  double v0 = readDouble(input);
  double v1 = readDouble(input);
  double x0 = readDouble(input) / 254000.0;
  double v3 = readDouble(input);
  double v4 = readDouble(input);
  double y0 = readDouble(input) / 254000.0;
  m_collector->collectFillTransform(v0, v1, x0, v3, v4, y0);
}

void libcdr::CDRParser::readVersion(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  m_version = readU16(input);
  if (m_version < 600)
    m_precision = libcdr::PRECISION_16BIT;
  else
    m_precision = libcdr::PRECISION_32BIT;
}

bool libcdr::CDRParser::_redirectX6Chunk(WPXInputStream **input, unsigned &length)
{
  if (m_version >= 1600 && length == 0x10)
  {
    unsigned streamNumber = readU32(*input);
    length = readU32(*input);
    if (streamNumber < m_externalStreams.size())
    {
      unsigned streamOffset = readU32(*input);
      *input = m_externalStreams[streamNumber];
      (*input)->seek(streamOffset, WPX_SEEK_SET);
      return true;
    }
    else if (streamNumber == 0xffffffff)
      return true;
    return false;
  }
  return true;
}

void libcdr::CDRParser::readIccd(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned long numBytesRead = 0;
  const unsigned char *tmpProfile = input->read(length, numBytesRead);
  if (length != numBytesRead)
    throw EndOfStreamException();
  if (!numBytesRead)
    return;
  std::vector<unsigned char> profile(numBytesRead);
  memcpy(&profile[0], tmpProfile, numBytesRead);
  m_collector->collectColorProfile(profile);
}

void libcdr::CDRParser::readBBox(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  double x0 = readCoordinate(input);
  double y0 = readCoordinate(input);
  double x1 = readCoordinate(input);
  double y1 = readCoordinate(input);
  double width = fabs(x1-x0);
  double height = fabs(y1-y0);
  double offsetX = x1 < x0 ? x1 : x0;
  double offsetY = y1 < y0 ? y1 : y0;
  m_collector->collectBBox(width, height, offsetX, offsetY);
}

void libcdr::CDRParser::readSpnd(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned spnd = readUnsigned(input);
  m_collector->collectSpnd(spnd);
}

void libcdr::CDRParser::readVpat(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned fillId = readUnsigned(input);
  unsigned long numBytesRead = 0;
  const unsigned char *buffer = input->read(length-4, numBytesRead);
  if (numBytesRead)
  {
    WPXBinaryData data(buffer, numBytesRead);
    m_collector->collectVectorPattern(fillId, data);
  }
}

void libcdr::CDRParser::readUidr(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned colorId = readU32(input);
  unsigned userId = readU32(input);
  input->seek(36, WPX_SEEK_CUR);
  CDRColor color = readColor(input);
  m_collector->collectPaletteEntry(colorId, userId, color);
}

void libcdr::CDRParser::readFont(WPXInputStream *input, unsigned length)
{
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned fontId = readU16(input);
  unsigned short fontEncoding = readU16(input);
  WPXString name;
  if (m_version >= 1200)
  {
    unsigned short character = 0;
    while (true)
    {
      character = readU16(input);
      if (character)
        name.append((char)(character & 0xff));
      else
        break;
    }
  }
  else
  {
    unsigned char character = 0;
    while(true)
    {
      character = readU8(input);
      if (character)
        name.append((char)character);
      else
        break;
    }
  }
  m_collector->collectFont(fontId, fontEncoding, name);
}

void libcdr::CDRParser::readStlt(WPXInputStream *input, unsigned length)
{
  if (m_version < 700)
    return;
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  unsigned numRecords = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numRecords 0x%x\n", numRecords));
  if (!numRecords)
    return;
  unsigned numFills = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numFills 0x%x\n", numFills));
  std::map<unsigned,unsigned> fills;
  unsigned i = 0;
  for (i=0; i<numFills; ++i)
  {
    unsigned first = readU32(input);
    input->seek(4, WPX_SEEK_CUR);
    unsigned second = readU32(input);
    fills[first] = second;
    if (m_version >= 1300)
      input->seek(48, WPX_SEEK_CUR);
  }
  unsigned numOutls = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numOutls 0x%x\n", numOutls));
  std::map<unsigned,unsigned> outls;
  for (i=0; i<numOutls; ++i)
  {
    unsigned first = readU32(input);
    input->seek(4, WPX_SEEK_CUR);
    unsigned second = readU32(input);
    outls[first] = second;
  }
  unsigned numFonts = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numFonts 0x%x\n", numFonts));
  for (i=0; i<numFonts; ++i)
  {
    if (m_version >= 1000)
      input->seek(60, WPX_SEEK_CUR);
    else
      input->seek(44, WPX_SEEK_CUR);
  }
  unsigned numAligns = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numAligns 0x%x\n", numAligns));
  for (i=0; i<numAligns; ++i)
  {
    input->seek(12, WPX_SEEK_CUR);
  }
  unsigned numIntervals = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numIntervals 0x%x\n", numIntervals));
  for (i=0; i<numIntervals; ++i)
  {
    input->seek(52, WPX_SEEK_CUR);
  }
  unsigned numSet5s = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numSet5s 0x%x\n", numSet5s));
  for (i=0; i<numSet5s; ++i)
  {
    input->seek(152, WPX_SEEK_CUR);
  }
  unsigned numTabs = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numTabs 0x%x\n", numTabs));
  for (i=0; i<numTabs; ++i)
  {
    input->seek(784, WPX_SEEK_CUR);
  }
  unsigned numBullets = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numBullets 0x%x\n", numBullets));
  unsigned size = m_version < 1000 ? 72 : 80;
  unsigned shift = m_version < 1000 ? -8 :0;
  for (i=0; i<numBullets; ++i)
  {
    long position = input->tell();
    input->seek(68+shift, WPX_SEEK_CUR);
    unsigned inc = 0;
    if (readU32(input))
      inc = 8;
    input->seek(position, WPX_SEEK_SET);
    if (m_version >= 1300)
    {
      input->seek(8, WPX_SEEK_CUR);
      if (readU32(input))
        inc = 32;
      else
        inc = -24;
      input->seek(position, WPX_SEEK_SET);
    }
    input->seek(size+inc, WPX_SEEK_CUR);
  }
  unsigned numIndents = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numIndents 0x%x\n", numIndents));
  for (i=0; i<numIndents; ++i)
  {
    input->seek(28, WPX_SEEK_CUR);
  }
  unsigned numHypens = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numHypens 0x%x\n", numHypens));
  for (i=0; i<numHypens; ++i)
  {
    if (m_version >= 1300)
      input->seek(36, WPX_SEEK_CUR);
    else
      input->seek(32, WPX_SEEK_CUR);
  }
  unsigned numDropcaps = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numDropcaps 0x%x\n", numDropcaps));
  for (i=0; i<numDropcaps; ++i)
  {
    if (m_version < 900)
      input->seek(24, WPX_SEEK_CUR);
    else
      input->seek(28, WPX_SEEK_CUR);
  }
  unsigned numSet11s = readU32(input);
  CDR_DEBUG_MSG(("CDRParser::readStlt numSet11s 0x%x\n", numSet11s));
  for (i=0; i<numSet11s; ++i)
  {
    input->seek(12, WPX_SEEK_CUR);
  }
  if (m_version == 801) // cdr8bidi
    input->seek(16, WPX_SEEK_CUR);
  for (i=0; i<numRecords; ++i)
  {
    CDR_DEBUG_MSG(("CDRParser::readStlt parsing styles\n"));
    unsigned num = readU32(input);
    CDR_DEBUG_MSG(("CDRParser::readStlt parsing styles num 0x%x\n", num));
    unsigned asize = 0;
    switch (num)
    {
    case 3:
      asize = 48;
      break;
    case 2:
      asize = 28;
      break;
    case 1:
    default:
      asize = 8;
      break;
    }
    if (m_version <= 800 && num > 1)
      asize -= 4;
    input->seek(16, WPX_SEEK_CUR);
    unsigned namelen = readU32(input);
    CDR_DEBUG_MSG(("CDRParser::readStlt namelen 0x%.8x\n", namelen));
    if (m_version >= 1200)
      namelen *= 2;
    input->seek(namelen+asize, WPX_SEEK_CUR);
  }
}

void libcdr::CDRParser::readTxsm(WPXInputStream *input, unsigned length)
{
  if (m_version < 700)
    return;
  if (!_redirectX6Chunk(&input, length))
    throw GenericException();
  if (m_version >= 1600)
    return;
  if (m_version >= 1500)
    input->seek(0x25, WPX_SEEK_CUR);
  else
    input->seek(0x24, WPX_SEEK_CUR);
  if (readU32(input))
  {
    if (m_version < 800)
      input->seek(32, WPX_SEEK_CUR);
  }
  if (m_version < 800)
    input->seek(4, WPX_SEEK_CUR);
  unsigned textId = readU32(input);
  input->seek(48, WPX_SEEK_CUR);
  if (m_version >= 800)
  {
    if (readU32(input))
      input->seek(32, WPX_SEEK_CUR);
  }
  if (m_version >= 1500)
    input->seek(12, WPX_SEEK_CUR);
  unsigned num = readU32(input);
  if (!num)
  {
    if (m_version >= 800)
      input->seek(4, WPX_SEEK_CUR);
    if (m_version > 800)
      input->seek(2, WPX_SEEK_CUR);
    if (m_version >= 1400)
      input->seek(2, WPX_SEEK_CUR);
    input->seek(24, WPX_SEEK_CUR);
    if (m_version < 800)
      input->seek(8, WPX_SEEK_CUR);
    input->seek(4, WPX_SEEK_CUR);
  }

  unsigned stlId = readU32(input);
  if (m_version >= 1300 && num)
    input->seek(1, WPX_SEEK_CUR);
  input->seek(1, WPX_SEEK_CUR);
  unsigned numRecords = readU32(input);
  std::map<unsigned, CDRCharacterStyle> charStyles;
  unsigned i = 0;
  for (i=0; i<numRecords; ++i)
  {
    readU8(input);
    readU8(input);
    unsigned char fl2 = readU8(input);
    unsigned char fl3 = 0;
    if (m_version >= 800)
      fl3 = readU8(input);

    CDRCharacterStyle charStyle;
    // Read more information depending on the flags
    if (fl2&1) // Font
    {
      unsigned flag = readU32(input);
      charStyle.m_charSet = (flag >> 16);
    }
    if (fl2&2) // Bold/Italic, etc.
      input->seek(4, WPX_SEEK_CUR);
    if (fl2&4) // Font Size
      input->seek(4, WPX_SEEK_CUR);
    if (fl2&8) // assumption
      input->seek(4, WPX_SEEK_CUR);
    if (fl2&0x10) // Offset X
      input->seek(4, WPX_SEEK_CUR);
    if (fl2&0x20) // Offset Y
      input->seek(4, WPX_SEEK_CUR);
    if (fl2&0x40) // Font Colour
    {
      input->seek(4, WPX_SEEK_CUR);
      if (m_version >= 1500)
        input->seek(48, WPX_SEEK_CUR);
    }
    if (fl2&0x80) // Font Outl Colour
      input->seek(4, WPX_SEEK_CUR);

    if (fl3&8) // Encoding
    {
      if (m_version >= 1300)
      {
        unsigned tlen = readU32(input);
        input->seek(tlen*2, WPX_SEEK_CUR);
      }
      else
        input->seek(4, WPX_SEEK_CUR);
    }
    if (fl3&0x20) // Something
    {
      unsigned flag = readU8(input);
      if (flag)
        input->seek(52, WPX_SEEK_CUR);
    }

    charStyles[2*i] = charStyle;
  }
  unsigned numChars = readU32(input);
  std::vector<uint64_t> charDescriptions(numChars);
  for (i=0; i<numChars; ++i)
  {
    if (m_version >= 1200)
      charDescriptions[i] = readU64(input);
    else
      charDescriptions[i] = readU32(input);
  }
  unsigned numBytes = numChars;
  if (m_version >= 1200)
    numBytes = readU32(input);
  unsigned long numBytesRead = 0;
  const unsigned char *buffer = input->read(numBytes, numBytesRead);
  if (numBytesRead != numBytes)
    throw GenericException();
  std::vector<unsigned char> textData(numBytesRead);
  if (numBytesRead)
    memcpy(&textData[0], buffer, numBytesRead);

  m_collector->collectText(textId, stlId, textData, charDescriptions, charStyles);

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
