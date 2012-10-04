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

#ifndef __CDRPATH_H__
#define __CDRPATH_H__

#include <vector>
#include <libwpd/libwpd.h>

#include "CDRTypes.h"

namespace libcdr
{

class CDRTransform;

class CDRPathElement
{
public:
  CDRPathElement() {}
  virtual ~CDRPathElement() {}
  virtual void writeOut(WPXPropertyListVector &vec) const = 0;
  virtual void transform(const CDRTransforms &trafos) = 0;
  virtual void transform(const CDRTransform &trafo) = 0;
  virtual CDRPathElement *clone() = 0;
};


class CDRPath : public CDRPathElement
{
public:
  CDRPath() : m_elements(), m_isClosed(false) {}
  CDRPath(const CDRPath &path);
  ~CDRPath();

  void appendMoveTo(double x, double y);
  void appendLineTo(double x, double y);
  void appendCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y);
  void appendQuadraticBezierTo(double x1, double y1, double x, double y);
  void appendSplineTo(std::vector<std::pair<double, double> > &points);
  void appendArcTo(double rx, double ry, double rotation, bool longAngle, bool sweep, double x, double y);
  void appendClosePath();
  void appendPath(const CDRPath &path);

  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();

  void clear();
  bool empty() const;
  bool isClosed() const;

private:
  CDRPath &operator=(const CDRPath &path);

private:
  std::vector<CDRPathElement *> m_elements;
  bool m_isClosed;
};

} // namespace libcdr

#endif /* __CDRPATH_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
