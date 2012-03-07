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

#ifndef __CDRCOLLECTOR_H__
#define __CDRCOLLECTOR_H__

#include <map>
#include <vector>
#include <stack>
#include <libwpg/libwpg.h>
#include <lcms2.h>
#include "CDRTypes.h"
#include "CDRPath.h"
#include "CDROutputElementList.h"

namespace libcdr
{

class CDRCollector
{
public:
  CDRCollector(::libwpg::WPGPaintInterface *painter);
  virtual ~CDRCollector();

  // collector functions
  void collectPage(unsigned level);
  void collectObject(unsigned level);
  void collectOtherList();
  void collectCubicBezier(double x1, double y1, double x2, double y2, double x, double y);
  void collectQuadraticBezier(double x1, double y1, double x, double y);
  void collectMoveTo(double x, double y);
  void collectLineTo(double x, double y);
  void collectArcTo(double rx, double ry, bool largeArc, bool sweep, double x, double y);
  void collectClosePath();
  void collectLevel(unsigned level);
  void collectTransform(double v0, double v1, double x, double v3, double v4, double y);
  void collectFildId(unsigned id);
  void collectOutlId(unsigned id);
  void collectFild(unsigned id, unsigned short fillType, const CDRColor &color1, const CDRColor &color2, const CDRGradient &gradient, unsigned patternId);
  void collectOutl(unsigned id, unsigned short lineType, unsigned short capsType, unsigned short joinType,
                   double lineWidth, const CDRColor &color, const std::vector<unsigned short> &dashArray,
                   unsigned startMarkerId, unsigned endMarkerId);
  void collectRotate(double angle);
  void collectFlags(unsigned flags);
  void collectPageSize(double width, double height);
  void collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy);
  void collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2);
  void collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> palette, const std::vector<unsigned char> bitmap);
  void collectBmpf(unsigned patternId, unsigned width, unsigned height, const std::vector<unsigned char> &pattern);
  void collectPpdt(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned> &knotVector);
  void collectPolygon();

private:
  CDRCollector(const CDRCollector &);
  CDRCollector &operator=(const CDRCollector &);

  // helper functions
  void _startPage(double width, double height);
  void _endPage();
  void _flushCurrentPath();

  unsigned _getRGBColor(const CDRColor &color);
  unsigned _getBMPColor(const CDRColor &color);
  WPXString _getRGBColorString(const CDRColor &color);

  void _fillProperties(WPXPropertyList &propList, WPXPropertyListVector &vec);
  void _lineProperties(WPXPropertyList &propList);
  void _generateBitmapFromPattern(WPXBinaryData &bitmap, const CDRPattern &pattern, const CDRColor &fgColor, const CDRColor &bgColor);

  libwpg::WPGPaintInterface *m_painter;

  bool m_isPageProperties;
  bool m_isPageStarted;

  double m_pageOffsetX, m_pageOffsetY;
  double m_pageWidth, m_pageHeight;
  unsigned m_currentFildId, m_currentOutlId;
  unsigned m_currentObjectLevel, m_currentPageLevel;
  CDRImage m_currentImage;

  CDRPath m_currentPath;
  CDRTransform m_currentTransform;
  std::map<unsigned, CDRFillStyle> m_fillStyles;
  std::map<unsigned, CDRLineStyle> m_lineStyles;
  CDRPolygon *m_polygon;
  std::map<unsigned, WPXBinaryData> m_bmps;
  std::map<unsigned, CDRPattern> m_patterns;
  bool m_isInPolygon;
  std::stack<CDROutputElementList> m_outputElements;
  CDRBSplineData m_bSplineData;

  cmsHTRANSFORM m_colorTransformCMYK2RGB;
  cmsHTRANSFORM m_colorTransformLab2RGB;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
