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
#include <utility>
#include <vector>

#include <librevenge/librevenge.h>

#include "CDRTypes.h"
#include "CDRCollector.h"

namespace libcdr
{

class CDRPath;
class CDRTransforms;

class CDRStylesCollector : public CDRCollector
{
public:
  CDRStylesCollector(CDRParserState &ps);
  ~CDRStylesCollector() override;

  // collector functions
  void collectPage(unsigned level) override;
  void collectObject(unsigned) override {}
  void collectGroup(unsigned) override {}
  void collectVect(unsigned) override {}
  void collectOtherList() override {}
  void collectPath(const CDRPath &) override {}
  void collectLevel(unsigned) override {}
  void collectTransform(const CDRTransforms &, bool) override {}
  void collectFillStyle(unsigned id, const CDRFillStyle &fillStyle) override;
  void collectFillStyleId(unsigned) override {}
  void collectLineStyle(unsigned id, const CDRLineStyle &lineStyle) override;
  void collectLineStyleId(unsigned) override {}
  void collectRotate(double,double,double) override {}
  void collectFlags(unsigned, bool) override {}
  void collectPageSize(double width, double height, double offsetX, double offsetY) override;
  void collectPolygonTransform(unsigned, unsigned, double, double, double, double) override {}
  void collectBitmap(unsigned, double, double, double, double) override {}
  void collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> &palette, const std::vector<unsigned char> &bitmap) override;
  void collectBmp(unsigned imageId, const std::vector<unsigned char> &bitmap) override;
  void collectBmpf(unsigned patternId, unsigned width, unsigned height, const std::vector<unsigned char> &pattern) override;
  void collectPpdt(const std::vector<std::pair<double, double> > &, const std::vector<unsigned> &) override {}
  void collectFillTransform(const CDRTransforms &) override {}
  void collectFillOpacity(double) override {}
  void collectPolygon() override {}
  void collectSpline() override {}
  void collectColorProfile(const std::vector<unsigned char> &profile) override;
  void collectBBox(double, double, double, double) override {}
  void collectSpnd(unsigned) override {}
  void collectVectorPattern(unsigned, const librevenge::RVNGBinaryData &) override {}
  void collectPaletteEntry(unsigned colorId, unsigned userId, const CDRColor &color) override;
  void collectText(unsigned textId, unsigned styleId, const std::vector<unsigned char> &data,
                   const std::vector<unsigned char> &charDescriptions, const std::map<unsigned, CDRStyle> &styleOverrides) override;
  void collectArtisticText(double, double) override {}
  void collectParagraphText(double, double, double, double) override {}
  void collectStld(unsigned id, const CDRStyle &style) override;
  void collectStyleId(unsigned) override {}

private:
  CDRStylesCollector(const CDRStylesCollector &);
  CDRStylesCollector &operator=(const CDRStylesCollector &);

  CDRParserState &m_ps;
  CDRPage m_page;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
