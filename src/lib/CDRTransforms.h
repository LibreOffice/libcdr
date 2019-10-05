/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRTRANSFORMS_H__
#define __CDRTRANSFORMS_H__

#include <vector>
#include <librevenge/librevenge.h>

namespace libcdr
{

class CDRTransform
{
public:
  CDRTransform();
  CDRTransform(double v0, double v1, double x0, double v3, double v4, double y0);
  CDRTransform(const CDRTransform &trafo) = default;

  CDRTransform &operator=(const CDRTransform &trafo) = default;

  void applyToPoint(double &x, double &y) const;
  void applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &endx, double &endy) const;
  double getScaleX() const;
  double getScaleY() const;
  double getTranslateX() const;
  double getTranslateY() const;
  bool getFlipX() const;
  bool getFlipY() const;

  librevenge::RVNGString toString() const;

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
  CDRTransforms(const CDRTransforms &trafos) = default;
  ~CDRTransforms();

  CDRTransforms &operator=(const CDRTransforms &trafo) = default;

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
