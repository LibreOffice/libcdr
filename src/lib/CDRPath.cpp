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

#include <math.h>
#include "CDRPath.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace libcdr
{

class CDRMoveToElement : public CDRPathElement
{
public:
  CDRMoveToElement(double x, double y)
    : m_x(x),
      m_y(y) {}
  ~CDRMoveToElement() {}
  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();
private:
  double m_x;
  double m_y;
};

class CDRLineToElement : public CDRPathElement
{
public:
  CDRLineToElement(double x, double y)
    : m_x(x),
      m_y(y) {}
  ~CDRLineToElement() {}
  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();
private:
  double m_x;
  double m_y;
};

class CDRCubicBezierToElement : public CDRPathElement
{
public:
  CDRCubicBezierToElement(double x1, double y1, double x2, double y2, double x, double y)
    : m_x1(x1),
      m_y1(y1),
      m_x2(x2),
      m_y2(y2),
      m_x(x),
      m_y(y) {}
  ~CDRCubicBezierToElement() {}
  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();
private:
  double m_x1;
  double m_y1;
  double m_x2;
  double m_y2;
  double m_x;
  double m_y;
};

class CDRQuadraticBezierToElement : public CDRPathElement
{
public:
  CDRQuadraticBezierToElement(double x1, double y1, double x, double y)
    : m_x1(x1),
      m_y1(y1),
      m_x(x),
      m_y(y) {}
  ~CDRQuadraticBezierToElement() {}
  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();
private:
  double m_x1;
  double m_y1;
  double m_x;
  double m_y;
};

class CDRSplineToElement : public CDRPathElement
{
public:
  CDRSplineToElement(const std::vector<std::pair<double, double> > &points)
    : m_points(points) {}
  ~CDRSplineToElement() {}
  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();
private:
  std::vector<std::pair<double, double> > m_points;
};

class CDRArcToElement : public CDRPathElement
{
public:
  CDRArcToElement(double rx, double ry, double rotation, bool largeArc, bool sweep, double x, double y)
    : m_rx(rx),
      m_ry(ry),
      m_rotation(rotation),
      m_largeArc(largeArc),
      m_sweep(sweep),
      m_x(x),
      m_y(y) {}
  ~CDRArcToElement() {}
  void writeOut(WPXPropertyListVector &vec) const;
  void transform(const CDRTransforms &trafos);
  void transform(const CDRTransform &trafo);
  CDRPathElement *clone();
private:
  double m_rx;
  double m_ry;
  double m_rotation;
  bool m_largeArc;
  bool m_sweep;
  double m_x;
  double m_y;
};

} // namespace libcdr


void libcdr::CDRMoveToElement::writeOut(WPXPropertyListVector &vec) const
{
  WPXPropertyList node;
  node.insert("libwpg:path-action", "M");
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libcdr::CDRMoveToElement::transform(const CDRTransforms &trafos)
{
  trafos.applyToPoint(m_x,m_y);
}

void libcdr::CDRMoveToElement::transform(const CDRTransform &trafo)
{
  trafo.applyToPoint(m_x,m_y);
}

libcdr::CDRPathElement *libcdr::CDRMoveToElement::clone()
{
  return new CDRMoveToElement(m_x, m_y);
}

void libcdr::CDRLineToElement::writeOut(WPXPropertyListVector &vec) const
{
  WPXPropertyList node;
  node.insert("libwpg:path-action", "L");
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libcdr::CDRLineToElement::transform(const CDRTransforms &trafos)
{
  trafos.applyToPoint(m_x,m_y);
}

void libcdr::CDRLineToElement::transform(const CDRTransform &trafo)
{
  trafo.applyToPoint(m_x,m_y);
}

libcdr::CDRPathElement *libcdr::CDRLineToElement::clone()
{
  return new CDRLineToElement(m_x, m_y);
}

void libcdr::CDRCubicBezierToElement::writeOut(WPXPropertyListVector &vec) const
{
  WPXPropertyList node;
  node.insert("libwpg:path-action", "C");
  node.insert("svg:x1", m_x1);
  node.insert("svg:y1", m_y1);
  node.insert("svg:x2", m_x2);
  node.insert("svg:y2", m_y2);
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libcdr::CDRCubicBezierToElement::transform(const CDRTransforms &trafos)
{
  trafos.applyToPoint(m_x1,m_y1);
  trafos.applyToPoint(m_x2,m_y2);
  trafos.applyToPoint(m_x,m_y);
}

void libcdr::CDRCubicBezierToElement::transform(const CDRTransform &trafo)
{
  trafo.applyToPoint(m_x1,m_y1);
  trafo.applyToPoint(m_x2,m_y2);
  trafo.applyToPoint(m_x,m_y);
}

libcdr::CDRPathElement *libcdr::CDRCubicBezierToElement::clone()
{
  return new CDRCubicBezierToElement(m_x1, m_y1, m_x2, m_y2, m_x, m_y);
}

void libcdr::CDRQuadraticBezierToElement::writeOut(WPXPropertyListVector &vec) const
{
  WPXPropertyList node;
  node.insert("libwpg:path-action", "Q");
  node.insert("svg:x1", m_x1);
  node.insert("svg:y1", m_y1);
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libcdr::CDRQuadraticBezierToElement::transform(const CDRTransforms &trafos)
{
  trafos.applyToPoint(m_x1,m_y1);
  trafos.applyToPoint(m_x,m_y);
}

void libcdr::CDRQuadraticBezierToElement::transform(const CDRTransform &trafo)
{
  trafo.applyToPoint(m_x1,m_y1);
  trafo.applyToPoint(m_x,m_y);
}

libcdr::CDRPathElement *libcdr::CDRQuadraticBezierToElement::clone()
{
  return new CDRQuadraticBezierToElement(m_x1, m_y1, m_x, m_y);
}

void libcdr::CDRSplineToElement::writeOut(WPXPropertyListVector &vec) const
{
  WPXPropertyList node;

#if 0
  node.insert("libwpg:path-action", "M");
  node.insert("svg:x", m_points[0].first);
  node.insert("svg:y", m_points[0].second);
  vec.append(node);

  for (unsigned j = 0; j < m_points.size(); ++j)
  {
    node.clear();
    node.insert("libwpg:path-action", "L");
    node.insert("svg:x", m_points[j].first);
    node.insert("svg:y", m_points[j].second);
    vec.append(node);
  }

  node.clear();
  node.insert("libwpg:path-action", "M");
  node.insert("svg:x", m_points[0].first);
  node.insert("svg:y", m_points[0].second);
  vec.append(node);
#endif
  for (unsigned i = 1; i < m_points.size()-1; i++)
  {
    node.clear();
    node.insert("libwpg:path-action", "Q");
    node.insert("svg:x1", m_points[i].first);
    node.insert("svg:y1", m_points[i].second);
    if (i < m_points.size() - 2)
    {
      node.insert("svg:x", (m_points[i].first+m_points[i+1].first)/2.0);
      node.insert("svg:y", (m_points[i].second+m_points[i+1].second)/2.0);
    }
    else
    {
      node.insert("svg:x", m_points[i+1].first);
      node.insert("svg:y", m_points[i+1].second);
    }
    vec.append(node);
  }

  // For the while, just move to the end point
  node.clear();
  node.insert("libwpg:path-action", "L");
  node.insert("svg:x", m_points.back().first);
  node.insert("svg:y", m_points.back().second);
  vec.append(node);
}

void libcdr::CDRSplineToElement::transform(const CDRTransforms &trafos)
{
  for (std::vector<std::pair<double, double> >::iterator iter = m_points.begin();
       iter != m_points.end(); ++iter)
    trafos.applyToPoint(iter->first, iter->second);
}

void libcdr::CDRSplineToElement::transform(const CDRTransform &trafo)
{
  for (std::vector<std::pair<double, double> >::iterator iter = m_points.begin();
       iter != m_points.end(); ++iter)
    trafo.applyToPoint(iter->first, iter->second);
}

libcdr::CDRPathElement *libcdr::CDRSplineToElement::clone()
{
  return new CDRSplineToElement(m_points);
}

void libcdr::CDRArcToElement::writeOut(WPXPropertyListVector &vec) const
{
  WPXPropertyList node;
  node.insert("libwpg:path-action", "A");
  node.insert("svg:rx", m_rx);
  node.insert("svg:ry", m_ry);
  node.insert("libwpg:rotate", m_rotation * 180 / M_PI, WPX_GENERIC);
  node.insert("libwpg:large-arc", m_largeArc);
  node.insert("libwpg:sweep", m_sweep);
  node.insert("svg:x", m_x);
  node.insert("svg:y", m_y);
  vec.append(node);
}

void libcdr::CDRArcToElement::transform(const CDRTransforms &trafos)
{
  trafos.applyToArc(m_rx, m_ry, m_rotation, m_sweep, m_x, m_y);
}

void libcdr::CDRArcToElement::transform(const CDRTransform &trafo)
{
  trafo.applyToArc(m_rx, m_ry, m_rotation, m_sweep, m_x, m_y);
}

libcdr::CDRPathElement *libcdr::CDRArcToElement::clone()
{
  return new CDRArcToElement(m_rx, m_ry, m_rotation, m_largeArc, m_sweep, m_x, m_y);
}

void libcdr::CDRPath::appendMoveTo(double x, double y)
{
  m_elements.push_back(new libcdr::CDRMoveToElement(x, y));
}

void libcdr::CDRPath::appendLineTo(double x, double y)
{
  m_elements.push_back(new libcdr::CDRLineToElement(x, y));
}

void libcdr::CDRPath::appendCubicBezierTo(double x1, double y1, double x2, double y2, double x, double y)
{
  m_elements.push_back(new libcdr::CDRCubicBezierToElement(x1, y1, x2, y2, x, y));
}

void libcdr::CDRPath::appendQuadraticBezierTo(double x1, double y1, double x, double y)
{
  m_elements.push_back(new libcdr::CDRQuadraticBezierToElement(x1, y1, x, y));
}

void libcdr::CDRPath::appendArcTo(double rx, double ry, double rotation, bool longAngle, bool sweep, double x, double y)
{
  m_elements.push_back(new libcdr::CDRArcToElement(rx, ry, rotation, longAngle, sweep, x, y));
}

void libcdr::CDRPath::appendSplineTo(std::vector<std::pair<double, double> > &points)
{
  m_elements.push_back(new libcdr::CDRSplineToElement(points));
}

void libcdr::CDRPath::appendClosePath()
{
  m_isClosed = true;
}

libcdr::CDRPath::CDRPath(const libcdr::CDRPath &path) : m_elements(), m_isClosed(false)
{
  for (std::vector<CDRPathElement *>::const_iterator iter = path.m_elements.begin(); iter != path.m_elements.end(); ++iter)
    m_elements.push_back((*iter)->clone());
  m_isClosed = path.isClosed();
}

libcdr::CDRPath::~CDRPath()
{
  clear();
}

void libcdr::CDRPath::appendPath(const CDRPath &path)
{
  for (std::vector<CDRPathElement *>::const_iterator iter = path.m_elements.begin(); iter != path.m_elements.end(); ++iter)
    m_elements.push_back((*iter)->clone());
}

void libcdr::CDRPath::writeOut(WPXPropertyListVector &vec) const
{
  for (std::vector<CDRPathElement *>::const_iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->writeOut(vec);
}

void libcdr::CDRPath::transform(const CDRTransforms &trafos)
{
  for (std::vector<CDRPathElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->transform(trafos);
}

void libcdr::CDRPath::transform(const CDRTransform &trafo)
{
  for (std::vector<CDRPathElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->transform(trafo);
}

libcdr::CDRPathElement *libcdr::CDRPath::clone()
{
  return new CDRPath(*this);
}

void libcdr::CDRPath::clear()
{
  for (std::vector<CDRPathElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    if (*iter)
      delete(*iter);
  m_elements.clear();
  m_isClosed = false;
}

bool libcdr::CDRPath::empty() const
{
  return m_elements.empty();
}

bool libcdr::CDRPath::isClosed() const
{
  return m_isClosed;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
