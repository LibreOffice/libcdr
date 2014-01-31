/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRSTYLESCOLLECTOR_H__
#define __CDRSTYLESCOLLECTOR_H__

#include <map>
#include <vector>
#include <stack>
#include <librevenge/librevenge.h>
#include <lcms2.h>
#include "CDRTypes.h"
#include "CDRPath.h"
#include "CDROutputElementList.h"
#include "CDRCollector.h"
#include "libcdr_utils.h"

namespace libcdr
{

class CDRStylesCollector : public CDRCollector
{
public:
  CDRStylesCollector(CDRParserState &ps);
  virtual ~CDRStylesCollector();

  // collector functions
  void collectPage(unsigned level);
  void collectObject(unsigned) {}
  void collectGroup(unsigned) {}
  void collectVect(unsigned) {}
  void collectOtherList() {}
  void collectPath(const CDRPath &) {}
  void collectLevel(unsigned) {}
  void collectTransform(const CDRTransforms &, bool) {}
  void collectFillStyle(unsigned short, const CDRColor &, const CDRColor &, const CDRGradient &, const CDRImageFill &) {}
  void collectLineStyle(unsigned short, unsigned short, unsigned short, double, double, double, const CDRColor &,
                        const std::vector<unsigned> &, const CDRPath &, const CDRPath &) {}
  void collectRotate(double,double,double) {}
  void collectFlags(unsigned, bool) {}
  void collectPageSize(double width, double height, double offsetX, double offsetY);
  void collectPolygonTransform(unsigned, unsigned, double, double, double, double) {}
  void collectBitmap(unsigned, double, double, double, double) {}
  void collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> &palette, const std::vector<unsigned char> &bitmap);
  void collectBmp(unsigned imageId, const std::vector<unsigned char> &bitmap);
  void collectBmpf(unsigned patternId, unsigned width, unsigned height, const std::vector<unsigned char> &pattern);
  void collectPpdt(const std::vector<std::pair<double, double> > &, const std::vector<unsigned> &) {}
  void collectFillTransform(const CDRTransforms &) {}
  void collectFillOpacity(double) {}
  void collectPolygon() {}
  void collectSpline() {}
  void collectColorProfile(const std::vector<unsigned char> &profile);
  void collectBBox(double, double, double, double) {}
  void collectSpnd(unsigned) {}
  void collectVectorPattern(unsigned, const librevenge::RVNGBinaryData &) {}
  void collectPaletteEntry(unsigned colorId, unsigned userId, const CDRColor &color);
  void collectText(unsigned textId, unsigned styleId, const std::vector<unsigned char> &data,
                   const std::vector<unsigned char> &charDescriptions, const std::map<unsigned, CDRCharacterStyle> &styleOverrides);
  void collectArtisticText(double, double) {}
  void collectParagraphText(double, double, double, double) {}
  void collectStld(unsigned id, const CDRCharacterStyle &charStyle);

private:
  CDRStylesCollector(const CDRStylesCollector &);
  CDRStylesCollector &operator=(const CDRStylesCollector &);

  void getRecursedStyle(CDRCharacterStyle &charStyle, unsigned styleId);

  CDRParserState &m_ps;
  CDRPage m_page;
  std::map<unsigned, CDRCharacterStyle> m_charStyles;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
