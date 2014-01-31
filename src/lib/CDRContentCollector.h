/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRCONTENTCOLLECTOR_H__
#define __CDRCONTENTCOLLECTOR_H__

#include <map>
#include <vector>
#include <stack>
#include <librevenge/librevenge.h>
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
  CDRContentCollector(CDRParserState &ps, ::librevenge::RVNGDrawingInterface *painter);
  virtual ~CDRContentCollector();

  // collector functions
  void collectPage(unsigned level);
  void collectObject(unsigned level);
  void collectGroup(unsigned level);
  void collectVect(unsigned level);
  void collectOtherList();
  void collectPath(const CDRPath &path);
  void collectLevel(unsigned level);
  void collectTransform(const CDRTransforms &transforms, bool considerGroupTransform);
  void collectFillStyle(unsigned short fillType, const CDRColor &color1, const CDRColor &color2, const CDRGradient &gradient, const CDRImageFill &imageFill);
  void collectLineStyle(unsigned short lineType, unsigned short capsType, unsigned short joinType, double lineWidth,
                        double stretch, double angle, const CDRColor &color, const std::vector<unsigned> &dashArray,
                        const CDRPath &startMarker, const CDRPath &endMarker);
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
  void collectVectorPattern(unsigned id, const librevenge::RVNGBinaryData &data);
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
  void _startDocument();
  void _endDocument();
  void _startPage(double width, double height);
  void _endPage();
  void _flushCurrentPath();

  void _fillProperties(librevenge::RVNGPropertyList &propList);
  void _lineProperties(librevenge::RVNGPropertyList &propList);
  void _generateBitmapFromPattern(librevenge::RVNGBinaryData &bitmap, const CDRPattern &pattern, const CDRColor &fgColor, const CDRColor &bgColor);

  librevenge::RVNGDrawingInterface *m_painter;

  bool m_isDocumentStarted;
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
