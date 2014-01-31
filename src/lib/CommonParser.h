/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __COMMONPARSER_H__
#define __COMMONPARSER_H__

#include "CDRCollector.h"
#include "CDRPath.h"

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
  double readRectCoord(librevenge::RVNGInputStream *input, bool bigEndian = false);
  double readCoordinate(librevenge::RVNGInputStream *input, bool bigEndian = false);
  unsigned readUnsigned(librevenge::RVNGInputStream *input, bool bigEndian = false);
  unsigned short readUnsignedShort(librevenge::RVNGInputStream *input, bool bigEndian = false);
  int readInteger(librevenge::RVNGInputStream *input, bool bigEndian = false);
  double readAngle(librevenge::RVNGInputStream *input, bool bigEndian = false);

  void processPath(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned char> &types, CDRPath &path);
  void outputPath(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned char> &types);

  CDRCollector *m_collector;
  CoordinatePrecision m_precision;
};
} // namespace libcdr

#endif // __COMMONPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
