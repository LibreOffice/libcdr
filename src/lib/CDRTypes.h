/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRTYPES_H__
#define __CDRTYPES_H__

#include <utility>
#include <vector>
#include <math.h>
#include <librevenge/librevenge.h>
#include "CDRTransforms.h"
#include "CDRPath.h"
#include "libcdr_utils.h"

namespace libcdr
{

struct CDRBox
{
  double m_x;
  double m_y;
  double m_w;
  double m_h;
  CDRBox()
    : m_x(0.0), m_y(0.0), m_w(0.0), m_h(0.0) {}
  CDRBox(double x0, double y0, double x1, double y1)
    : m_x(x0 < x1 ? x0 : x1), m_y(y0 < y1 ? y0 : y1), m_w(fabs(x1-x0)), m_h(fabs(y1-y0)) {}
  double getWidth() const
  {
    return m_w;
  }
  double getHeight() const
  {
    return m_h;
  }
  double getMinX() const
  {
    return m_x;
  }
  double getMinY() const
  {
    return m_y;
  }

};

struct CDRColor
{
  unsigned short m_colorModel;
  unsigned short m_colorPalette;
  unsigned m_colorValue;
  CDRColor(unsigned short colorModel, unsigned short colorPalette, unsigned colorValue)
    : m_colorModel(colorModel), m_colorPalette(colorPalette), m_colorValue(colorValue) {}
  CDRColor()
    : m_colorModel(0), m_colorPalette(0), m_colorValue(0) {}
  CDRColor(unsigned short colorModel, unsigned colorValue)
    : m_colorModel(colorModel), m_colorPalette(0), m_colorValue(colorValue) {}
};

struct CDRGradientStop
{
  CDRColor m_color;
  double m_offset;
  CDRGradientStop() : m_color(), m_offset(0.0) {}
  CDRGradientStop(const CDRColor &color, double offset)
    : m_color(color), m_offset(offset) {}
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
  CDRImageFill() : id(0), width(0.0), height(0.0), isRelative(false), xOffset(0.0), yOffset(0.0), rcpOffset(0.0), flags(0)
  {}
  CDRImageFill(unsigned i, double w, double h, bool r, double x, double y, double o, unsigned char f)
    : id(i), width(w), height(h), isRelative(r), xOffset(x), yOffset(y), rcpOffset(o), flags(f) {}
};

struct CDRFillStyle
{
  unsigned short fillType;
  CDRColor color1, color2;
  CDRGradient gradient;
  CDRImageFill imageFill;
  CDRFillStyle()
    : fillType((unsigned short)-1), color1(), color2(), gradient(), imageFill() {}
  CDRFillStyle(unsigned short ft, CDRColor c1, CDRColor c2, const CDRGradient &gr, const CDRImageFill &img)
    : fillType(ft), color1(c1), color2(c2), gradient(gr), imageFill(img) {}
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
  std::vector<unsigned> dashArray;
  CDRPath startMarker;
  CDRPath endMarker;
  CDRLineStyle()
    : lineType((unsigned short)-1), capsType(0), joinType(0), lineWidth(0.0),
      stretch(0.0), angle(0.0), color(), dashArray(),
      startMarker(), endMarker() {}
  CDRLineStyle(unsigned short lt, unsigned short ct, unsigned short jt,
               double lw, double st, double a, const CDRColor &c, const std::vector<unsigned> &da,
               const CDRPath &sm, const CDRPath &em)
    : lineType(lt), capsType(ct), joinType(jt), lineWidth(lw),
      stretch(st), angle(a), color(c), dashArray(da),
      startMarker(sm), endMarker(em) {}
};

struct CDRStyle
{
  unsigned short m_charSet;
  librevenge::RVNGString m_fontName;
  double m_fontSize;
  unsigned m_align;
  double m_leftIndent, m_firstIndent, m_rightIndent;
  CDRLineStyle m_lineStyle;
  CDRFillStyle m_fillStyle;
  unsigned m_parentId;
  CDRStyle()
    : m_charSet((unsigned short)-1), m_fontName(),
      m_fontSize(0.0), m_align(0), m_leftIndent(0.0), m_firstIndent(0.0),
      m_rightIndent(0.0), m_lineStyle(), m_fillStyle(), m_parentId(0)
  {
    m_fontName.clear();
  }
  void overrideStyle(const CDRStyle &override)
  {
    if (override.m_charSet != (unsigned short)-1 || override.m_fontName.len())
    {
      m_charSet = override.m_charSet;
      m_fontName = override.m_fontName;
    }
    if (!CDR_ALMOST_ZERO(override.m_fontSize))
      m_fontSize = override.m_fontSize;
    if (override.m_align)
      m_align = override.m_align;
    if (!CDR_ALMOST_ZERO(override.m_leftIndent) && !CDR_ALMOST_ZERO(override.m_firstIndent) && !CDR_ALMOST_ZERO(override.m_rightIndent))
    {
      m_leftIndent = override.m_leftIndent;
      m_firstIndent = override.m_firstIndent;
      m_rightIndent = override.m_rightIndent;
    }
    if (override.m_lineStyle.lineType != (unsigned short)-1)
      m_lineStyle = override.m_lineStyle;
    if (override.m_fillStyle.fillType != (unsigned short)-1)
      m_fillStyle = override.m_fillStyle;
  }
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
  librevenge::RVNGBinaryData m_image;
  double m_x1;
  double m_x2;
  double m_y1;
  double m_y2;
  CDRImage() : m_image(), m_x1(0.0), m_x2(0.0), m_y1(0.0), m_y2(0.0) {}
  CDRImage(const librevenge::RVNGBinaryData &image, double x1, double x2, double y1, double y2)
    : m_image(image), m_x1(x1), m_x2(x2), m_y1(y1), m_y2(y2) {}
  double getMiddleX() const
  {
    return (m_x1 + m_x2) / 2.0;
  }
  double getMiddleY() const
  {
    return (m_y1 + m_y2) / 2.0;
  }
  const librevenge::RVNGBinaryData &getImage() const
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

struct CDRBitmap
{
  unsigned colorModel;
  unsigned width;
  unsigned height;
  unsigned bpp;
  std::vector<unsigned> palette;
  std::vector<unsigned char> bitmap;
  CDRBitmap() : colorModel(0), width(0), height(0), bpp(0), palette(), bitmap() {}
  CDRBitmap(unsigned cm, unsigned w, unsigned h, unsigned b, const std::vector<unsigned> &p, const std::vector<unsigned char> &bmp)
    : colorModel(cm), width(w), height(h), bpp(b), palette(p), bitmap(bmp) {}
};

struct CDRPage
{
  double width;
  double height;
  double offsetX;
  double offsetY;
  CDRPage() : width(0.0), height(0.0), offsetX(0.0), offsetY(0.0) {}
  CDRPage(double w, double h, double ox, double oy)
    : width(w), height(h), offsetX(ox), offsetY(oy) {}
};

struct CDRSplineData
{
  std::vector<std::pair<double, double> > points;
  std::vector<unsigned> knotVector;
  CDRSplineData() : points(), knotVector() {}
  CDRSplineData(const std::vector<std::pair<double, double> > &ps, const std::vector<unsigned> &kntv)
    : points(ps), knotVector(kntv) {}
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

struct WaldoRecordInfo
{
  WaldoRecordInfo(unsigned char t, unsigned i, unsigned o)
    : type(t), id(i), offset(o) {}
  WaldoRecordInfo() : type(0), id(0), offset(0) {}
  unsigned char type;
  unsigned id;
  unsigned offset;
};

struct WaldoRecordType1
{
  WaldoRecordType1(unsigned id, unsigned short next, unsigned short previous,
                   unsigned short child, unsigned short parent, unsigned short flags,
                   double x0, double y0, double x1, double y1, const CDRTransform &trafo)
    : m_id(id), m_next(next), m_previous(previous), m_child(child), m_parent(parent),
      m_flags(flags), m_x0(x0), m_y0(y0), m_x1(x1), m_y1(y1), m_trafo(trafo) {}
  WaldoRecordType1()
    : m_id(0), m_next(0), m_previous(0), m_child(0), m_parent(0), m_flags(0),
      m_x0(0.0), m_y0(0.0), m_x1(0.0), m_y1(0.0), m_trafo() {}
  unsigned m_id;
  unsigned short m_next;
  unsigned short m_previous;
  unsigned short m_child;
  unsigned short m_parent;
  unsigned short m_flags;
  double m_x0;
  double m_y0;
  double m_x1;
  double m_y1;
  CDRTransform m_trafo;
};

struct CDRCMYKColor
{
  CDRCMYKColor(double cyan, double magenta, double yellow, double black)
    : c(cyan), m(magenta), y(yellow), k(black) {}
  ~CDRCMYKColor() {}
  double c;
  double m;
  double y;
  double k;
};

struct CDRRGBColor
{
  CDRRGBColor(double red, double green, double blue)
    : r(red), g(green), b(blue) {}
  ~CDRRGBColor() {}
  double r;
  double g;
  double b;
};

struct CDRLab2Color
{
  CDRLab2Color(double l, double A, double B)
    : L(l), a(A), b(B) {}
  ~CDRLab2Color() {}
  double L;
  double a;
  double b;
};

struct CDRLab4Color
{
  CDRLab4Color(double l, double A, double B)
    : L(l), a(A), b(B) {}
  ~CDRLab4Color() {}
  double L;
  double a;
  double b;
};

struct CDRText
{
  CDRText() : m_text(), m_style() {}
  CDRText(const librevenge::RVNGString &text, const CDRStyle &style)
    : m_text(text), m_style(style) {}
  librevenge::RVNGString m_text;
  CDRStyle m_style;
};

struct CDRTextLine
{
  CDRTextLine() : m_line() {}
  CDRTextLine(const CDRTextLine &line) : m_line(line.m_line) {}
  void append(const CDRText &text)
  {
    m_line.push_back(text);
  }
  void clear()
  {
    m_line.clear();
  }
  std::vector<CDRText> m_line;
};

struct CDRFont
{
  CDRFont() : m_name(), m_encoding(0) {}
  CDRFont(const librevenge::RVNGString &name, unsigned short encoding)
    : m_name(name), m_encoding(encoding) {}
  CDRFont(const CDRFont &font) = default;
  CDRFont &operator=(const CDRFont &font) = default;
  librevenge::RVNGString m_name;
  unsigned short m_encoding;
};

} // namespace libcdr

#endif /* __CDRTYPES_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
