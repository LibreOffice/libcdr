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

#ifndef DUMP_PREVIEW_IMAGE
#define DUMP_PREVIEW_IMAGE 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

libcdr::CMXParser::CMXParser(WPXInputStream *input, libcdr::CDRCollector *collector)
  : m_input(input),
    m_collector(collector), m_bigEndian(false), m_coordSize(0), m_unit(0),
    m_scale(0.0), m_xmin(0.0), m_xmax(0.0), m_ymin(0.0), m_ymax(0.0) {}

libcdr::CMXParser::~CMXParser()
{
}

bool libcdr::CMXParser::parseRecords(WPXInputStream *input, unsigned level)
{
  if (!input)
  {
    return false;
  }
  m_collector->collectLevel(level);
  while (!input->atEOS())
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
    WPXString fourCC = readFourCC(input);
    unsigned length = readU32(input);
    unsigned long position = input->tell();
    WPXString listType;

    CDR_DEBUG_MSG(("Record: level %u %s, length: 0x%.8x (%i)\n", level, fourCC.cstr(), length, length));

    if (fourCC == "RIFF" || fourCC == "LIST")
    {
      listType = readFourCC(input);
      CDR_DEBUG_MSG(("CMX listType: %s\n", listType.cstr()));
      unsigned dataSize = length-4;
      CDRInternalStream tmpStream(input, dataSize);
      if (!parseRecords(&tmpStream, level+1))
        return false;
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

void libcdr::CMXParser::readRecord(WPXString fourCC, unsigned length, WPXInputStream *input)
{
  long recordStart = input->tell();
  if (fourCC == "cont")
    readCMXHeader(input);
  else if (fourCC == "DISP")
    readDisp(input, length);
  input->seek(recordStart + length, WPX_SEEK_CUR);
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
  m_coordSize = (unsigned short)atoi(tmpString.cstr());
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
  input->seek(24, WPX_SEEK_CUR);
  m_xmin = (double)readS32(input, m_bigEndian);
  m_ymax = (double)readS32(input, m_bigEndian);
  m_xmax = (double)readS32(input, m_bigEndian);
  m_ymin = (double)readS32(input, m_bigEndian);
  CDR_DEBUG_MSG(("CMX Bouding Box: xmin: %f, xmax: %f, ymin: %f, ymax: %f\n", m_xmin, m_xmax, m_ymin, m_ymax));
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


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
