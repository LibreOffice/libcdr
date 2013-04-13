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

#ifndef __CDRSTYLESCOLLECTOR_H__
#define __CDRSTYLESCOLLECTOR_H__

#include <map>
#include <vector>
#include <stack>
#include <libwpg/libwpg.h>
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
  void collectCubicBezier(double, double, double, double, double, double) {}
  void collectQuadraticBezier(double, double, double, double) {}
  void collectMoveTo(double, double) {}
  void collectLineTo(double, double) {}
  void collectArcTo(double, double, bool, bool, double, double) {}
  void collectClosePath() {}
  void collectLevel(unsigned) {}
  void collectTransform(const CDRTransforms &, bool) {}
  void collectFillStyle(unsigned short, const CDRColor &, const CDRColor &, const CDRGradient &, const CDRImageFill &) {}
  void collectLineStyle(unsigned short, unsigned short, unsigned short, double, double, double, const CDRColor &,
                        const std::vector<unsigned> &, unsigned, unsigned) {}
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
  void collectVectorPattern(unsigned, const WPXBinaryData &) {}
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
