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

#ifndef __CDRTYPES_H__
#define __CDRTYPES_H__

#include <vector>

namespace libcdr
{
class CDRPath;

struct CDRTransform
{
  double m_v0;
  double m_v1;
  double m_x0;
  double m_v3;
  double m_v4;
  double m_y0;
  CDRTransform()
    : m_v0(1.0), m_v1(0.0), m_x0(0.0),
      m_v3(0.0), m_v4(1.0), m_y0(0.0) {}
  CDRTransform(double v0, double v1, double x0, double v3, double v4, double y0)
    : m_v0(v0), m_v1(v1), m_x0(x0), m_v3(v3), m_v4(v4), m_y0(y0) {}

  void applyToPoint(double &x, double &y) const;
  void applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &x, double &y) const;
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

struct CDRPolygon
{
  unsigned m_numAngles;
  unsigned m_nextPoint;
  double m_rx;
  double m_ry;
  double m_cx;
  double m_cy;
  CDRPolygon() : m_numAngles(0), m_nextPoint(0), m_rx(0.0), m_ry(0.0), m_cx(0.0), m_cy(0.0) {}
  CDRPolygon(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy)
    : m_numAngles(numAngles), m_nextPoint(nextPoint), m_rx(rx), m_ry(ry), m_cx(cx), m_cy(cy) {}
  void create(CDRPath &path) const;
};

} // namespace libcdr

#endif /* __CDRTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
