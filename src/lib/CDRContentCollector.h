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

#ifndef __CDRCONTENTCOLLECTOR_H__
#define __CDRCONTENTCOLLECTOR_H__

#include <map>
#include <vector>
#include <stack>
#include <libwpg/libwpg.h>
#include <lcms2.h>
#include "CDRTypes.h"
#include "CDRPath.h"
#include "CDROutputElementList.h"
#include "CDRCollector.h"

namespace libcdr
{

class CDRContentCollector : public CDRCollector
{
public:
  CDRContentCollector(CDRParserState &ps, ::libwpg::WPGPaintInterface *painter);
  virtual ~CDRContentCollector();

  // collector functions
  void collectPage(unsigned level);
  void collectObject(unsigned level);
  void collectGroup(unsigned level);
  void collectVect(unsigned level);
  void collectOtherList();
  void collectCubicBezier(double x1, double y1, double x2, double y2, double x, double y);
  void collectQuadraticBezier(double x1, double y1, double x, double y);
  void collectMoveTo(double x, double y);
  void collectLineTo(double x, double y);
  void collectArcTo(double rx, double ry, bool largeArc, bool sweep, double x, double y);
  void collectClosePath();
  void collectLevel(unsigned level);
  void collectTransform(double v0, double v1, double x, double v3, double v4, double y, bool considerGroupTransform);
  void collectFildId(unsigned id);
  void collectOutlId(unsigned id);
  void collectFild(unsigned, unsigned short, const CDRColor &, const CDRColor &, const CDRGradient &, const CDRImageFill &) {}
  void collectOutl(unsigned, unsigned short, unsigned short, unsigned short, double, double, double, const CDRColor &,
                   const std::vector<unsigned short> &, unsigned, unsigned) {}
  void collectRotate(double angle);
  void collectFlags(unsigned flags);
  void collectPageSize(double, double) {}
  void collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy);
  void collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2);
  void collectBmp(unsigned, unsigned, unsigned, unsigned, unsigned, const std::vector<unsigned>&, const std::vector<unsigned char>&) {}
  void collectBmp(unsigned, const std::vector<unsigned char>&) {}
  void collectBmpf(unsigned, unsigned, unsigned, const std::vector<unsigned char> &) {}
  void collectPpdt(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned> &knotVector);
  void collectFillTransform(double v0, double v1, double x, double v3, double v4, double y);
  void collectFillOpacity(double opacity);
  void collectPolygon();
  void collectSpline();
  void collectColorProfile(const std::vector<unsigned char> &) {}
  void collectBBox(double width, double height, double offsetX, double offsetY);
  void collectSpnd(unsigned spnd);

private:
  CDRContentCollector(const CDRContentCollector &);
  CDRContentCollector &operator=(const CDRContentCollector &);

  // helper functions
  void _startPage(double width, double height);
  void _endPage();
  void _flushCurrentPath();

  void _fillProperties(WPXPropertyList &propList, WPXPropertyListVector &vec);
  void _lineProperties(WPXPropertyList &propList);
  void _generateBitmapFromPattern(WPXBinaryData &bitmap, const CDRPattern &pattern, const CDRColor &fgColor, const CDRColor &bgColor);

  libwpg::WPGPaintInterface *m_painter;

  bool m_isPageProperties;
  bool m_isPageStarted;

  double m_pageWidth, m_pageHeight, m_pageOffsetX, m_pageOffsetY;
  unsigned m_currentFildId, m_currentOutlId;
  unsigned m_spnd;
  unsigned m_currentObjectLevel, m_currentGroupLevel, m_currentVectLevel, m_currentPageLevel;
  CDRImage m_currentImage;

  CDRPath m_currentPath;
  CDRTransform m_currentTransform, m_fillTransform;
  CDRPolygon *m_polygon;
  bool m_isInPolygon;
  bool m_isInSpline;
  std::stack<CDROutputElementList> *m_outputElements;
  std::stack<CDROutputElementList> m_contentOutputElements;
  std::stack<CDROutputElementList> m_fillOutputElements;
  std::stack<unsigned> m_groupLevels;
  std::stack<CDRTransform> m_groupTransforms;
  CDRSplineData m_splineData;
  double m_fillOpacity;

  CDRParserState &m_ps;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
