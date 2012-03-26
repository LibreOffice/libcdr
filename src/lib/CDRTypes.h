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
#include <math.h>
#include <libwpd/libwpd.h>

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

struct CDRColor
{
  unsigned short m_colorModel;
  unsigned m_colorValue;
  CDRColor() : m_colorModel(0), m_colorValue(0) {}
  CDRColor(unsigned short colorModel, unsigned colorValue)
    : m_colorModel(colorModel), m_colorValue(colorValue) {}
  CDRColor(const CDRColor &color)
    : m_colorModel(color.m_colorModel), m_colorValue(color.m_colorValue) {}
};

struct CDRGradientStop
{
  CDRColor m_color;
  double m_offset;
  CDRGradientStop() : m_color(), m_offset(0.0) {}
  CDRGradientStop(const CDRColor &color, double offset)
    : m_color(color), m_offset(offset) {}
  CDRGradientStop(const CDRGradientStop &stop)
    : m_color(stop.m_color), m_offset(stop.m_offset) {}
};

struct CDRGradient
{
  unsigned char m_type;
  unsigned char m_mode;
  double m_angle;
  double m_midPoint;
  int m_edgeOffset;
  int m_centerXOffset;
  int m_centerYOffset;
  std::vector<CDRGradientStop> m_stops;
  CDRGradient()
    : m_type(0), m_mode(0), m_angle(0.0), m_midPoint(0.0), m_edgeOffset(0), m_centerXOffset(0), m_centerYOffset(0), m_stops() {}
  CDRGradient(const CDRGradient &gradient)
    : m_type(gradient.m_type), m_mode(gradient.m_mode), m_angle(gradient.m_angle), m_midPoint(gradient.m_midPoint),
      m_edgeOffset(gradient.m_edgeOffset), m_centerXOffset(gradient.m_centerXOffset), m_centerYOffset(gradient.m_centerYOffset),
      m_stops(gradient.m_stops) {}
};

struct CDRImageFill
{
  unsigned id;
  double width;
  double height;
  bool isRelative;
  double xOffset;
  double yOffset;
  double rcpOffset;
  unsigned char flags;
  bool refPoint;
  CDRImageFill() : id(0), width(0.0), height(0.0), isRelative(false), xOffset(0.0), yOffset(0.0), rcpOffset(0.0), flags(0), refPoint(false) {}
  CDRImageFill(unsigned i, double w, double h, bool r, double x, double y, double o, unsigned char f, bool rp)
    : id(i), width(w), height(h), isRelative(r), xOffset(x), yOffset(y), rcpOffset(o), flags(f), refPoint(rp) {}
};

struct CDRFillStyle
{
  unsigned short fillType;
  CDRColor color1, color2;
  CDRGradient gradient;
  CDRImageFill imageFill;
  CDRFillStyle()
    : fillType(0), color1(), color2(), gradient(), imageFill() {}
  CDRFillStyle(unsigned short ft, CDRColor c1, CDRColor c2, const CDRGradient &gr, const CDRImageFill &img)
    : fillType(ft), color1(c1), color2(c2), gradient(gr), imageFill(img) {}
  CDRFillStyle(const CDRFillStyle &style)
    : fillType(style.fillType), color1(style.color1), color2(style.color2), gradient(style.gradient), imageFill(style.imageFill) {}
};

struct CDRLineStyle
{
  unsigned short lineType;
  unsigned short capsType;
  unsigned short joinType;
  double lineWidth;
  double stretch;
  double angle;
  CDRColor color;
  std::vector<unsigned short> dashArray;
  unsigned startMarkerId;
  unsigned endMarkerId;
  CDRLineStyle()
    : lineType(0), capsType(0), joinType(0), lineWidth(0.0),
      stretch(0.0), angle(0.0), color(), dashArray(),
      startMarkerId(0), endMarkerId(0) {}
  CDRLineStyle(unsigned short lt, unsigned short ct, unsigned short jt,
               double lw, double st, double a, const CDRColor &c, const std::vector<unsigned short> &da,
               unsigned smi, unsigned emi)
    : lineType(lt), capsType(ct), joinType(jt), lineWidth(lw),
      stretch(st), angle(a), color(c), dashArray(da),
      startMarkerId(smi), endMarkerId(emi) {}
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

struct CDRImage
{
  WPXBinaryData m_image;
  double m_x1;
  double m_x2;
  double m_y1;
  double m_y2;
  CDRImage() : m_image(), m_x1(0.0), m_x2(0.0), m_y1(0.0), m_y2(0.0) {}
  CDRImage(const WPXBinaryData &image, double x1, double x2, double y1, double y2)
    : m_image(image), m_x1(x1), m_x2(x2), m_y1(y1), m_y2(y2) {}
  double getMiddleX() const
  {
    return (m_x1 + m_x2) / 2.0;
  }
  double getMiddleY() const
  {
    return (m_y1 + m_y2) / 2.0;
  }
  double getWidth() const
  {
    return fabs(m_x1 - m_x2);
  }
  double getHeight() const
  {
    return fabs(m_y1 - m_y2);
  }
  const WPXBinaryData &getImage() const
  {
    return m_image;
  }
};

struct CDRPattern
{
  unsigned width;
  unsigned height;
  std::vector<unsigned char> pattern;
  CDRPattern() : width(0), height(0), pattern() {}
  CDRPattern(unsigned w, unsigned h, const std::vector<unsigned char> &p)
    : width(w), height(h), pattern(p) {}
};

struct CDRSplineData
{
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned> knotVector;
  CDRSplineData() : points(), knotVector() {}
  CDRSplineData(const std::vector<std::pair<double, double> > &ps, const std::vector<unsigned> &kntv)
    : points(ps), knotVector(kntv) {}
  CDRSplineData(const CDRSplineData &data)
    : points(data.points), knotVector(data.knotVector) {}
  void clear()
  {
    points.clear();
    knotVector.clear();
  }
  bool empty()
  {
    return (points.empty() || knotVector.empty());
  }
  void create(CDRPath &path) const;
};

} // namespace libcdr

#endif /* __CDRTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
