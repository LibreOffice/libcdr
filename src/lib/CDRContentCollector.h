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
  void collectTransform(const CDRTransforms &transforms, bool considerGroupTransform);
  void collectFillStyle(unsigned short fillType, const CDRColor &color1, const CDRColor &color2, const CDRGradient &gradient, const CDRImageFill &imageFill);
  void collectLineStyle(unsigned short lineType, unsigned short capsType, unsigned short joinType, double lineWidth,
                        double stretch, double angle, const CDRColor &color, const std::vector<unsigned> &dashArray,
                        unsigned startMarkerId, unsigned endMarkerId);
  void collectRotate(double angle, double cx, double cy);
  void collectFlags(unsigned flags, bool considerFlags);
  void collectPageSize(double, double, double, double) {}
  void collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy);
  void collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2);
  void collectBmp(unsigned, unsigned, unsigned, unsigned, unsigned, const std::vector<unsigned> &, const std::vector<unsigned char> &) {}
  void collectBmp(unsigned, const std::vector<unsigned char> &) {}
  void collectBmpf(unsigned, unsigned, unsigned, const std::vector<unsigned char> &) {}
  void collectPpdt(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned> &knotVector);
  void collectFillTransform(const CDRTransforms &fillTrafo);
  void collectFillOpacity(double opacity);
  void collectPolygon();
  void collectSpline();
  void collectColorProfile(const std::vector<unsigned char> &) {}
  void collectBBox(double x0, double y0, double x1, double y1);
  void collectSpnd(unsigned spnd);
  void collectVectorPattern(unsigned id, const WPXBinaryData &data);
  void collectPaletteEntry(unsigned, unsigned, const CDRColor &) {}
  void collectText(unsigned, unsigned, const std::vector<unsigned char> &,
                   const std::vector<unsigned char> &, const std::map<unsigned, CDRCharacterStyle> &) {}
  void collectArtisticText(double x, double y);
  void collectParagraphText(double x, double y, double width, double height);
  void collectStld(unsigned, const CDRCharacterStyle &) {}

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
  bool m_ignorePage;

  CDRPage m_page;
  unsigned m_pageIndex;
  CDRFillStyle m_currentFillStyle;
  CDRLineStyle m_currentLineStyle;
  unsigned m_spnd;
  unsigned m_currentObjectLevel, m_currentGroupLevel, m_currentVectLevel, m_currentPageLevel;
  CDRImage m_currentImage;
  const std::vector<CDRTextLine> *m_currentText;
  CDRBox m_currentBBox;
  CDRBox m_currentTextBox;

  CDRPath m_currentPath;
  CDRTransforms m_currentTransforms;
  CDRTransforms m_fillTransforms;
  CDRPolygon *m_polygon;
  bool m_isInPolygon;
  bool m_isInSpline;
  std::stack<CDROutputElementList> *m_outputElements;
  std::stack<CDROutputElementList> m_contentOutputElements;
  std::stack<CDROutputElementList> m_fillOutputElements;
  std::stack<unsigned> m_groupLevels;
  std::stack<CDRTransforms> m_groupTransforms;
  CDRSplineData m_splineData;
  double m_fillOpacity;

  CDRParserState &m_ps;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
