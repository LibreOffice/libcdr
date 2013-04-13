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
#include <libwpd-stream/libwpd-stream.h>
#include <lcms2.h>
#include "CDRTypes.h"
#include "CDRPath.h"
#include "CDROutputElementList.h"
#include "libcdr_utils.h"

namespace
{
#include "CDRColorProfiles.h"
}

namespace libcdr
{

class CDRParserState
{
public:
  CDRParserState();
  ~CDRParserState();
  std::map<unsigned, WPXBinaryData> m_bmps;
  std::map<unsigned, CDRPattern> m_patterns;
  std::map<unsigned, WPXBinaryData> m_vects;
  std::vector<CDRPage> m_pages;
  std::map<unsigned, CDRColor> m_documentPalette;
  std::map<unsigned, std::vector<CDRTextLine> > m_texts;

  unsigned _getRGBColor(const CDRColor &color);
  unsigned getBMPColor(const CDRColor &color);
  WPXString getRGBColorString(const CDRColor &color);
  cmsHTRANSFORM m_colorTransformCMYK2RGB;
  cmsHTRANSFORM m_colorTransformLab2RGB;
  cmsHTRANSFORM m_colorTransformRGB2RGB;

  void setColorTransform(const std::vector<unsigned char> &profile);
  void setColorTransform(WPXInputStream *input);

private:
  CDRParserState(const CDRParserState &);
  CDRParserState &operator=(const CDRParserState &);
};

class CDRCollector
{
public:
  CDRCollector() {}
  virtual ~CDRCollector() {}

  // collector functions
  virtual void collectPage(unsigned level) = 0;
  virtual void collectObject(unsigned level) = 0;
  virtual void collectGroup(unsigned level) = 0;
  virtual void collectVect(unsigned level) = 0;
  virtual void collectOtherList() = 0;
  virtual void collectCubicBezier(double x1, double y1, double x2, double y2, double x, double y) = 0;
  virtual void collectQuadraticBezier(double x1, double y1, double x, double y) = 0;
  virtual void collectMoveTo(double x, double y) = 0;
  virtual void collectLineTo(double x, double y) = 0;
  virtual void collectArcTo(double rx, double ry, bool largeArc, bool sweep, double x, double y) = 0;
  virtual void collectClosePath() = 0;
  virtual void collectLevel(unsigned level) = 0;
  virtual void collectTransform(const CDRTransforms &transforms, bool considerGroupTransform) = 0;
  virtual void collectFillStyle(unsigned short fillType, const CDRColor &color1, const CDRColor &color2, const CDRGradient &gradient, const CDRImageFill &imageFill) = 0;
  virtual void collectLineStyle(unsigned short lineType, unsigned short capsType, unsigned short joinType, double lineWidth,
                                double stretch, double angle, const CDRColor &color, const std::vector<unsigned> &dashArray,
                                unsigned startMarkerId, unsigned endMarkerId) = 0;
  virtual void collectRotate(double angle, double cx, double cy) = 0;
  virtual void collectFlags(unsigned flags, bool considerFlags) = 0;
  virtual void collectPageSize(double width, double height, double offsetX, double offsetY) = 0;
  virtual void collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy) = 0;
  virtual void collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2) = 0;
  virtual void collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> &palette, const std::vector<unsigned char> &bitmap) = 0;
  virtual void collectBmp(unsigned imageId, const std::vector<unsigned char> &bitmap) = 0;
  virtual void collectBmpf(unsigned patternId, unsigned width, unsigned height, const std::vector<unsigned char> &pattern) = 0;
  virtual void collectPpdt(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned> &knotVector) = 0;
  virtual void collectFillTransform(const CDRTransforms &fillTrafos) = 0;
  virtual void collectFillOpacity(double opacity) = 0;
  virtual void collectPolygon() = 0;
  virtual void collectSpline() = 0;
  virtual void collectColorProfile(const std::vector<unsigned char> &profile) = 0;
  virtual void collectBBox(double x0, double y0, double x1, double y1) = 0;
  virtual void collectSpnd(unsigned spnd) = 0;
  virtual void collectVectorPattern(unsigned id, const WPXBinaryData &data) = 0;
  virtual void collectPaletteEntry(unsigned colorId, unsigned userId, const CDRColor &color) = 0;
  virtual void collectText(unsigned textId, unsigned styleId, const std::vector<unsigned char> &data,
                           const std::vector<unsigned char> &charDescriptions, const std::map<unsigned, CDRCharacterStyle> &styleOverrides) = 0;
  virtual void collectArtisticText(double x, double y) = 0;
  virtual void collectParagraphText(double x, double y, double width, double height) = 0;
  virtual void collectStld(unsigned id, const CDRCharacterStyle &charStyle) = 0;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
