/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRCOLLECTOR_H__
#define __CDRCOLLECTOR_H__

#include <map>
#include <vector>
#include <stack>
#include <librevenge/librevenge.h>
#include <librevenge-stream/librevenge-stream.h>
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
  std::map<unsigned, librevenge::RVNGBinaryData> m_bmps;
  std::map<unsigned, CDRPattern> m_patterns;
  std::map<unsigned, librevenge::RVNGBinaryData> m_vects;
  std::vector<CDRPage> m_pages;
  std::map<unsigned, CDRColor> m_documentPalette;
  std::map<unsigned, std::vector<CDRTextLine> > m_texts;

  unsigned _getRGBColor(const CDRColor &color);
  unsigned getBMPColor(const CDRColor &color);
  librevenge::RVNGString getRGBColorString(const CDRColor &color);
  cmsHTRANSFORM m_colorTransformCMYK2RGB;
  cmsHTRANSFORM m_colorTransformLab2RGB;
  cmsHTRANSFORM m_colorTransformRGB2RGB;

  void setColorTransform(const std::vector<unsigned char> &profile);
  void setColorTransform(librevenge::RVNGInputStream *input);

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
  virtual void collectPath(const CDRPath &path) = 0;
  virtual void collectLevel(unsigned level) = 0;
  virtual void collectTransform(const CDRTransforms &transforms, bool considerGroupTransform) = 0;
  virtual void collectFillStyle(unsigned short fillType, const CDRColor &color1, const CDRColor &color2, const CDRGradient &gradient, const CDRImageFill &imageFill) = 0;
  virtual void collectLineStyle(unsigned short lineType, unsigned short capsType, unsigned short joinType, double lineWidth,
                                double stretch, double angle, const CDRColor &color, const std::vector<unsigned> &dashArray,
                                const CDRPath &startMarker, const CDRPath &endMarker) = 0;
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
  virtual void collectVectorPattern(unsigned id, const librevenge::RVNGBinaryData &data) = 0;
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
