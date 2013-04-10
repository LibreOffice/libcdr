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
 * Copyright (C) 2012 Fridrich Strba <fridrich.strba@bluewin.ch>
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

#ifndef __COMMONPARSER_H__
#define __COMMONPARSER_H__

#include "CDRCollector.h"

class WPXInputSTream;

namespace libcdr
{

enum CoordinatePrecision
{ PRECISION_UNKNOWN = 0, PRECISION_16BIT, PRECISION_32BIT };

class CommonParser
{
public:
  CommonParser(CDRCollector *collector);
  virtual ~CommonParser();

private:
  CommonParser();
  CommonParser(const CommonParser &);
  CommonParser &operator=(const CommonParser &);


protected:
  double readRectCoord(WPXInputStream *input, bool bigEndian = false);
  double readCoordinate(WPXInputStream *input, bool bigEndian = false);
  unsigned readUnsigned(WPXInputStream *input, bool bigEndian = false);
  unsigned short readUnsignedShort(WPXInputStream *input, bool bigEndian = false);
  int readInteger(WPXInputStream *input, bool bigEndian = false);
  double readAngle(WPXInputStream *input, bool bigEndian = false);

  void outputPath(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned char> &types);

  CDRCollector *m_collector;
  CoordinatePrecision m_precision;
};
} // namespace libcdr

#endif // __COMMONPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
