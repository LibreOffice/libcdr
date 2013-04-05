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

#ifndef __CDRTRANSFORMS_H__
#define __CDRTRANSFORMS_H__

#include <vector>
#include <math.h>

namespace libcdr
{
class CDRPath;

class CDRTransform
{
public:
  CDRTransform();
  CDRTransform(double v0, double v1, double x0, double v3, double v4, double y0);
  CDRTransform(const CDRTransform &trafo);

  void applyToPoint(double &x, double &y) const;
  void applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &endx, double &endy) const;
  double getScaleX() const;
  double getScaleY() const;
  double getTranslateX() const;
  double getTranslateY() const;
  bool getFlipX() const;
  bool getFlipY() const;

private:
  double _getScaleX() const;
  double _getScaleY() const;
  double m_v0;
  double m_v1;
  double m_x0;
  double m_v3;
  double m_v4;
  double m_y0;
};

class CDRTransforms
{
public:
  CDRTransforms();
  CDRTransforms(const CDRTransforms &trafos);
  ~CDRTransforms();

  void append(double v0, double v1, double x0, double v3, double v4, double y0);
  void append(const CDRTransform &trafo);
  void clear();
  bool empty() const;

  void applyToPoint(double &x, double &y) const;
  void applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &x, double &y) const;
  double getScaleX() const;
  double getScaleY() const;
  double getTranslateX() const;
  double getTranslateY() const;
  bool getFlipX() const;
  bool getFlipY() const;

private:
  double _getScaleX() const;
  double _getScaleY() const;
  std::vector<CDRTransform> m_trafos;
};

} // namespace libcdr

#endif /* __CDRTRANSFORMS_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
