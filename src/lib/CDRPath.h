/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRPATH_H__
#define __CDRPATH_H__

#include <memory>
#include <utility>
#include <vector>
#include <librevenge/librevenge.h>

namespace libcdr
{

class CDRTransform;
class CDRTransforms;

class CDRPathElement
{
public:
  CDRPathElement() {}
  virtual ~CDRPathElement() {}
  virtual void writeOut(librevenge::RVNGPropertyListVector &vec) const = 0;
  virtual void transform(const CDRTransforms &trafos) = 0;
  virtual void transform(const CDRTransform &trafo) = 0;
  virtual std::unique_ptr<CDRPathElement> clone() = 0;
};


class CDRPath : public CDRPathElement
{
public:
  CDRPath() : m_elements(), m_isClosed(false) {}
  CDRPath(const CDRPath &path);
  ~CDRPath() override;

  CDRPath &operator=(const CDRPath &path);

  void appendMoveTo(double x, double y);
  void appendLineTo(double x, double y);
  void appendCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y);
  void appendQuadraticBezierTo(double x1, double y1, double x, double y);
  void appendSplineTo(const std::vector<std::pair<double, double> > &points);
  void appendArcTo(double rx, double ry, double rotation, bool longAngle, bool sweep, double x, double y);
  void appendClosePath();
  void appendPath(const CDRPath &path);

  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void writeOut(librevenge::RVNGString &path, librevenge::RVNGString &viewBox, double &width) const;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  std::unique_ptr<CDRPathElement> clone() override;

  void clear();
  bool empty() const;
  bool isClosed() const;

private:
  std::vector<std::unique_ptr<CDRPathElement>> m_elements;
  bool m_isClosed;
};

} // namespace libcdr

#endif /* __CDRPATH_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
