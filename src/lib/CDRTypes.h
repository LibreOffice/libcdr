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

#ifndef CDRTYPES_H
#define CDRTYPES_H

#include <vector>

namespace libcdr
{
struct CDRTransform
{
  double v0;
  double v1;
  double x0;
  double v3;
  double v4;
  double y0;
  CDRTransform()
    : v0(0.0), v1(0.0), x0(0.0),
      v3(0.0), v4(0.0), y0(0.0) {}
  void applyToPoint(double &x, double &y) const;
  void applyToAngle(double &radians) const;
  void applyToLength(double &width, double &height) const;
};

struct CDRFillStyle
{
  unsigned short fillType;
  unsigned short colorModel;
  unsigned color1, color2;
  CDRFillStyle()
    : fillType(0), colorModel(0), color1(0), color2(0) {}
  CDRFillStyle(unsigned short ft, unsigned short cm, unsigned c1, unsigned c2)
    : fillType(ft), colorModel(cm), color1(c1), color2(c2) {}
};

struct CDRLineStyle
{
  unsigned short lineType;
  unsigned short capsType;
  unsigned short joinType;
  double lineWidth;
  unsigned short colorModel;
  unsigned color;
  std::vector<unsigned short> dashArray;
  unsigned startMarkerId;
  unsigned endMarkerId;
  CDRLineStyle()
    : lineType(0), capsType(0), joinType(0),
      lineWidth(0.0), colorModel(0), color(0),
      dashArray(), startMarkerId(0), endMarkerId(0) {}
  CDRLineStyle(unsigned short lt, unsigned short ct, unsigned short jt,
               double lw, unsigned short cm, unsigned c,
               const std::vector<unsigned short> &da, unsigned smi, unsigned emi)
    : lineType(lt), capsType(ct), joinType(jt),
      lineWidth(lw), colorModel(cm), color(c),
      dashArray(da), startMarkerId(smi), endMarkerId(emi) {}
};

} // namespace libcdr

#endif /* CDRTYPES_H */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
