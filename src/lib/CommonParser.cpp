/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommonParser.h"

#include <string.h>

#include "CDRCollector.h"
#include "CDRPath.h"
#include "libcdr_utils.h"

libcdr::CommonParser::CommonParser(libcdr::CDRCollector *collector)
  : m_collector(collector), m_precision(libcdr::PRECISION_UNKNOWN) {}

libcdr::CommonParser::~CommonParser()
{
}

double libcdr::CommonParser::readCoordinate(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (double)readS16(input, bigEndian) / 1000.0;
  return (double)readS32(input, bigEndian) / 254000.0;
}

unsigned libcdr::CommonParser::readUnsigned(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (unsigned)readU16(input, bigEndian);
  return readU32(input, bigEndian);
}

unsigned short libcdr::CommonParser::readUnsignedShort(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (unsigned short)readU8(input, bigEndian);
  return readU16(input, bigEndian);
}

int libcdr::CommonParser::readInteger(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (int)readS16(input, bigEndian);
  return readS32(input, bigEndian);
}

double libcdr::CommonParser::readAngle(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return M_PI * (double)readS16(input, bigEndian) / 1800.0;
  return M_PI * (double)readS32(input, bigEndian) / 180000000.0;
}

void libcdr::CommonParser::outputPath(const std::vector<std::pair<double, double> > &points,
                                      const std::vector<unsigned char> &types)
{
  CDRPath path;
  processPath(points, types, path);
  m_collector->collectPath(path);
}

void libcdr::CommonParser::processPath(const std::vector<std::pair<double, double> > &points,
                                       const std::vector<unsigned char> &types, CDRPath &path)
{
  bool isClosedPath = false;
  std::vector<std::pair<double, double> >tmpPoints;
  for (size_t k=0; k<points.size(); k++)
  {
    const unsigned char &type = types[k];
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
      if (isClosedPath)
        path.appendClosePath();
      tmpPoints.clear();
      path.appendMoveTo(points[k].first, points[k].second);
    }
    else if ((type & 0x40) && !(type & 0x80))
    {
      tmpPoints.clear();
      path.appendLineTo(points[k].first, points[k].second);
      if (isClosedPath)
        path.appendClosePath();
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        path.appendCubicBezierTo(tmpPoints[0].first, tmpPoints[0].second,
                                 tmpPoints[1].first, tmpPoints[1].second,
                                 points[k].first, points[k].second);
      else
        path.appendLineTo(points[k].first, points[k].second);
      if (isClosedPath)
        path.appendClosePath();
      tmpPoints.clear();
    }
    else if ((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[k]);
    }
  }
}

void libcdr::CommonParser::readRImage(unsigned &colorModel, unsigned &width, unsigned &height, unsigned &bpp,
                                      std::vector<unsigned> &palette, std::vector<unsigned char> &bitmap,
                                      librevenge::RVNGInputStream *input, bool bigEndian)
{
  colorModel = readU32(input, bigEndian);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  width = readU32(input, bigEndian);
  height = readU32(input, bigEndian);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  bpp = readU32(input, bigEndian);
  input->seek(4, librevenge::RVNG_SEEK_CUR);
  unsigned bmpsize = readU32(input, bigEndian);
  input->seek(32, librevenge::RVNG_SEEK_CUR);
  if (bpp < 24 && colorModel != 5 && colorModel != 6)
  {
    palette.clear();
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    unsigned long palettesize = readU16(input);
    if (palettesize > getRemainingLength(input) / 3)
      palettesize = getRemainingLength(input) / 3;
    palette.reserve(palettesize);
    for (unsigned long i = 0; i <palettesize; ++i)
    {
      unsigned b = readU8(input);
      unsigned g = readU8(input);
      unsigned r = readU8(input);
      palette.push_back(b | (g << 8) | (r << 16));
    }
  }
  if (bmpsize == 0)
    return;
  unsigned long tmpNumBytesRead = 0;
  const unsigned char *tmpBuffer = input->read(bmpsize, tmpNumBytesRead);
  if (bmpsize != tmpNumBytesRead)
    return;
  bitmap.clear();
  bitmap.resize(bmpsize);
  memcpy(&bitmap[0], tmpBuffer, bmpsize);
}

void libcdr::CommonParser::readBmpPattern(unsigned &width, unsigned &height, std::vector<unsigned char> &pattern,
                                          unsigned length, librevenge::RVNGInputStream *input, bool bigEndian)
{
  unsigned headerLength = readU32(input);
  if (headerLength != 40)
    return;
  width = readU32(input, bigEndian);
  height = readU32(input, bigEndian);
  input->seek(2, librevenge::RVNG_SEEK_CUR);
  unsigned bpp = readU16(input);
  if (bpp != 1)
    return;
  input->seek(4, librevenge::RVNG_SEEK_CUR); // compression
  unsigned dataSize = readU32(input, bigEndian);
  if (dataSize == 0)
    return;
  CDR_DEBUG_MSG(("CommonParser::readBmpPattern - offset of pixel data %i\n", length - dataSize - 24));
  input->seek(length - dataSize - 24, librevenge::RVNG_SEEK_CUR);
  unsigned long tmpNumBytesRead = 0;
  const unsigned char *tmpBuffer = input->read(dataSize, tmpNumBytesRead);
  if (dataSize != tmpNumBytesRead)
    return;
  pattern.clear();
  pattern.resize(dataSize);
  memcpy(&pattern[0], tmpBuffer, dataSize);
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
