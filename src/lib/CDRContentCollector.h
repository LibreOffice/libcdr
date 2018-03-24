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
#include <memory>
#include <utility>
#include <vector>
#include <stack>
#include <queue>

#include <librevenge/librevenge.h>

#include "CDROutputElementList.h"
#include "CDRTransforms.h"
#include "CDRTypes.h"
#include "CDRPath.h"
#include "CDRCollector.h"

namespace libcdr
{

class CDRContentCollector : public CDRCollector
{
public:
  CDRContentCollector(CDRParserState &ps, librevenge::RVNGDrawingInterface *painter, bool reverseOrder = true);
  ~CDRContentCollector() override;

  // collector functions
  void collectPage(unsigned level) override;
  void collectObject(unsigned level) override;
  void collectGroup(unsigned level) override;
  void collectVect(unsigned level) override;
  void collectOtherList() override;
  void collectPath(const CDRPath &path) override;
  void collectLevel(unsigned level) override;
  void collectTransform(const CDRTransforms &transforms, bool considerGroupTransform) override;
  void collectFillStyle(unsigned,const CDRFillStyle &) override {}
  void collectFillStyleId(unsigned id) override;
  void collectLineStyle(unsigned,const CDRLineStyle &) override {}
  void collectLineStyleId(unsigned id) override;
  void collectRotate(double angle, double cx, double cy) override;
  void collectFlags(unsigned flags, bool considerFlags) override;
  void collectPageSize(double, double, double, double) override {}
  void collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy) override;
  void collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2) override;
  void collectBmp(unsigned, unsigned, unsigned, unsigned, unsigned, const std::vector<unsigned> &, const std::vector<unsigned char> &) override {}
  void collectBmp(unsigned, const std::vector<unsigned char> &) override {}
  void collectBmpf(unsigned, unsigned, unsigned, const std::vector<unsigned char> &) override {}
  void collectPpdt(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned> &knotVector) override;
  void collectFillTransform(const CDRTransforms &fillTrafo) override;
  void collectFillOpacity(double opacity) override;
  void collectPolygon() override;
  void collectSpline() override;
  void collectColorProfile(const std::vector<unsigned char> &) override {}
  void collectBBox(double x0, double y0, double x1, double y1) override;
  void collectSpnd(unsigned spnd) override;
  void collectVectorPattern(unsigned id, const librevenge::RVNGBinaryData &data) override;
  void collectPaletteEntry(unsigned, unsigned, const CDRColor &) override {}
  void collectText(unsigned, unsigned, const std::vector<unsigned char> &,
                   const std::vector<unsigned char> &, const std::map<unsigned, CDRStyle> &) override {}
  void collectArtisticText(double x, double y) override;
  void collectParagraphText(double x, double y, double width, double height) override;
  void collectStld(unsigned, const CDRStyle &) override {}
  void collectStyleId(unsigned styleId) override;

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
  unsigned m_currentObjectLevel, m_currentGroupLevel, m_currentVectLevel, m_currentPageLevel, m_currentStyleId;
  CDRImage m_currentImage;
  const std::vector<CDRTextLine> *m_currentText;
  CDRBox m_currentBBox;
  CDRBox m_currentTextBox;

  CDRPath m_currentPath;
  CDRTransforms m_currentTransforms;
  CDRTransforms m_fillTransforms;
  std::unique_ptr<CDRPolygon> m_polygon;
  bool m_isInPolygon;
  bool m_isInSpline;
  std::stack<CDROutputElementList> *m_outputElementsStack;
  std::stack<CDROutputElementList> m_contentOutputElementsStack;
  std::stack<CDROutputElementList> m_fillOutputElementsStack;
  std::queue<CDROutputElementList> *m_outputElementsQueue;
  std::queue<CDROutputElementList> m_contentOutputElementsQueue;
  std::queue<CDROutputElementList> m_fillOutputElementsQueue;
  std::stack<unsigned> m_groupLevels;
  std::stack<CDRTransforms> m_groupTransforms;
  CDRSplineData m_splineData;
  double m_fillOpacity;
  bool m_reverseOrder;

  CDRParserState &m_ps;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
