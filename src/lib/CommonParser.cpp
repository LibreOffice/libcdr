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
#include "libcdr_utils.h"
#include "CommonParser.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

libcdr::CommonParser::CommonParser(libcdr::CDRCollector *collector)
  : m_collector(collector), m_precision(libcdr::PRECISION_UNKNOWN) {}

libcdr::CommonParser::~CommonParser()
{
}

double libcdr::CommonParser::readCoordinate(WPXInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (double)readS16(input, bigEndian) / 1000.0;
  return (double)readS32(input, bigEndian) / 254000.0;
}

unsigned libcdr::CommonParser::readUnsigned(WPXInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (unsigned)readU16(input, bigEndian);
  return readU32(input, bigEndian);
}

unsigned short libcdr::CommonParser::readUnsignedShort(WPXInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (unsigned short)readU8(input, bigEndian);
  return readU16(input, bigEndian);
}

int libcdr::CommonParser::readInteger(WPXInputStream *input, bool bigEndian)
{
  if (m_precision == PRECISION_UNKNOWN)
    throw UnknownPrecisionException();
  else if (m_precision == PRECISION_16BIT)
    return (int)readS16(input, bigEndian);
  return readS32(input, bigEndian);
}

double libcdr::CommonParser::readAngle(WPXInputStream *input, bool bigEndian)
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
  bool isClosedPath = false;
  std::vector<std::pair<double, double> >tmpPoints;
  for (unsigned k=0; k<points.size(); k++)
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
      tmpPoints.clear();
      m_collector->collectMoveTo(points[k].first, points[k].second);
    }
    else if ((type & 0x40) && !(type & 0x80))
    {
      tmpPoints.clear();
      m_collector->collectLineTo(points[k].first, points[k].second);
      if (isClosedPath)
        m_collector->collectClosePath();
    }
    else if (!(type & 0x40) && (type & 0x80))
    {
      if (tmpPoints.size() >= 2)
        m_collector->collectCubicBezier(tmpPoints[0].first, tmpPoints[0].second,
                                        tmpPoints[1].first, tmpPoints[1].second,
                                        points[k].first, points[k].second);
      else
        m_collector->collectLineTo(points[k].first, points[k].second);
      if (isClosedPath)
        m_collector->collectClosePath();
      tmpPoints.clear();
    }
    else if((type & 0x40) && (type & 0x80))
    {
      tmpPoints.push_back(points[k]);
    }
  }
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
