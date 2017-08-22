/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <math.h>
#include <map>
#include "CDRPath.h"
#include "CDRTypes.h"

#ifndef DEBUG_SPLINES
#define DEBUG_SPLINES 0
#endif

namespace
{

static inline double getAngle(double bx, double by)
{
  return fmod(2*M_PI + (by > 0.0 ? 1.0 : -1.0) * acos(bx / sqrt(bx * bx + by * by)), 2*M_PI);
}

static void getEllipticalArcBBox(double x0, double y0,
                                 double rx, double ry, double phi, bool largeArc, bool sweep, double x, double y,
                                 double &xmin, double &ymin, double &xmax, double &ymax)
{
  phi *= M_PI/180;
  if (rx < 0.0)
    rx *= -1.0;
  if (ry < 0.0)
    ry *= -1.0;

  if (rx == 0.0 || ry == 0.0)
  {
    xmin = (x0 < x ? x0 : x);
    xmax = (x0 > x ? x0 : x);
    ymin = (y0 < y ? y0 : y);
    ymax = (y0 > y ? y0 : y);
    return;
  }

  const double x1prime = cos(phi)*(x0 - x)/2 + sin(phi)*(y0 - y)/2;
  const double y1prime = -sin(phi)*(x0 - x)/2 + cos(phi)*(y0 - y)/2;

  double radicant = (rx*rx*ry*ry - rx*rx*y1prime*y1prime - ry*ry*x1prime*x1prime)/(rx*rx*y1prime*y1prime + ry*ry*x1prime*x1prime);
  double cxprime = 0.0;
  double cyprime = 0.0;
  if (radicant < 0.0)
  {
    double ratio = rx/ry;
    radicant = y1prime*y1prime + x1prime*x1prime/(ratio*ratio);
    if (radicant < 0.0)
    {
      xmin = (x0 < x ? x0 : x);
      xmax = (x0 > x ? x0 : x);
      ymin = (y0 < y ? y0 : y);
      ymax = (y0 > y ? y0 : y);
      return;
    }
    ry=sqrt(radicant);
    rx=ratio*ry;
  }
  else
  {
    double factor = (largeArc==sweep ? -1.0 : 1.0)*sqrt(radicant);

    cxprime = factor*rx*y1prime/ry;
    cyprime = -factor*ry*x1prime/rx;
  }

  double cx = cxprime*cos(phi) - cyprime*sin(phi) + (x0 + x)/2;
  double cy = cxprime*sin(phi) + cyprime*cos(phi) + (y0 + y)/2;

  double txmin, txmax, tymin, tymax;

  if (phi == 0 || phi == M_PI)
  {
    xmin = cx - rx;
    txmin = getAngle(-rx, 0);
    xmax = cx + rx;
    txmax = getAngle(rx, 0);
    ymin = cy - ry;
    tymin = getAngle(0, -ry);
    ymax = cy + ry;
    tymax = getAngle(0, ry);
  }
  else if (phi == M_PI / 2.0 || phi == 3.0*M_PI/2.0)
  {
    xmin = cx - ry;
    txmin = getAngle(-ry, 0);
    xmax = cx + ry;
    txmax = getAngle(ry, 0);
    ymin = cy - rx;
    tymin = getAngle(0, -rx);
    ymax = cy + rx;
    tymax = getAngle(0, rx);
  }
  else
  {
    txmin = -atan(ry*tan(phi)/rx);
    txmax = M_PI - atan(ry*tan(phi)/rx);
    xmin = cx + rx*cos(txmin)*cos(phi) - ry*sin(txmin)*sin(phi);
    xmax = cx + rx*cos(txmax)*cos(phi) - ry*sin(txmax)*sin(phi);
    double tmpY = cy + rx*cos(txmin)*sin(phi) + ry*sin(txmin)*cos(phi);
    txmin = getAngle(xmin - cx, tmpY - cy);
    tmpY = cy + rx*cos(txmax)*sin(phi) + ry*sin(txmax)*cos(phi);
    txmax = getAngle(xmax - cx, tmpY - cy);

    tymin = atan(ry/(tan(phi)*rx));
    tymax = atan(ry/(tan(phi)*rx))+M_PI;
    ymin = cy + rx*cos(tymin)*sin(phi) + ry*sin(tymin)*cos(phi);
    ymax = cy + rx*cos(tymax)*sin(phi) + ry*sin(tymax)*cos(phi);
    double tmpX = cx + rx*cos(tymin)*cos(phi) - ry*sin(tymin)*sin(phi);
    tymin = getAngle(tmpX - cx, ymin - cy);
    tmpX = cx + rx*cos(tymax)*cos(phi) - ry*sin(tymax)*sin(phi);
    tymax = getAngle(tmpX - cx, ymax - cy);
  }
  if (xmin > xmax)
  {
    std::swap(xmin,xmax);
    std::swap(txmin,txmax);
  }
  if (ymin > ymax)
  {
    std::swap(ymin,ymax);
    std::swap(tymin,tymax);
  }
  double angle1 = getAngle(x0 - cx, y0 - cy);
  double angle2 = getAngle(x - cx, y - cy);

  if (!sweep)
    std::swap(angle1, angle2);

  bool otherArc = false;
  if (angle1 > angle2)
  {
    std::swap(angle1, angle2);
    otherArc = true;
  }

  if ((!otherArc && (angle1 > txmin || angle2 < txmin)) || (otherArc && !(angle1 > txmin || angle2 < txmin)))
    xmin = x0 < x ? x0 : x;
  if ((!otherArc && (angle1 > txmax || angle2 < txmax)) || (otherArc && !(angle1 > txmax || angle2 < txmax)))
    xmax = x0 > x ? x0 : x;
  if ((!otherArc && (angle1 > tymin || angle2 < tymin)) || (otherArc && !(angle1 > tymin || angle2 < tymin)))
    ymin = y0 < y ? y0 : y;
  if ((!otherArc && (angle1 > tymax || angle2 < tymax)) || (otherArc && !(angle1 > tymax || angle2 < tymax)))
    ymax = y0 > y ? y0 : y;
}

static inline double quadraticExtreme(double t, double a, double b, double c)
{
  return (1.0-t)*(1.0-t)*a + 2.0*(1.0-t)*t*b + t*t*c;
}

static inline double quadraticDerivative(double a, double b, double c)
{
  double denominator = a - 2.0*b + c;
  if (fabs(denominator) != 0.0)
    return (a - b)/denominator;
  return -1.0;
}

static void getQuadraticBezierBBox(double x0, double y0, double x1, double y1, double x, double y,
                                   double &xmin, double &ymin, double &xmax, double &ymax)
{
  xmin = x0 < x ? x0 : x;
  xmax = x0 > x ? x0 : x;
  ymin = y0 < y ? y0 : y;
  ymax = y0 > y ? y0 : y;

  double t = quadraticDerivative(x0, x1, x);
  if (t>=0 && t<=1)
  {
    double tmpx = quadraticExtreme(t, x0, x1, x);
    xmin = tmpx < xmin ? tmpx : xmin;
    xmax = tmpx > xmax ? tmpx : xmax;
  }

  t = quadraticDerivative(y0, y1, y);
  if (t>=0 && t<=1)
  {
    double tmpy = quadraticExtreme(t, y0, y1, y);
    ymin = tmpy < ymin ? tmpy : ymin;
    ymax = tmpy > ymax ? tmpy : ymax;
  }
}

static inline double cubicBase(double t, double a, double b, double c, double d)
{
  return (1.0-t)*(1.0-t)*(1.0-t)*a + 3.0*(1.0-t)*(1.0-t)*t*b + 3.0*(1.0-t)*t*t*c + t*t*t*d;
}

static void getCubicBezierBBox(double x0, double y0, double x1, double y1, double x2, double y2, double x, double y,
                               double &xmin, double &ymin, double &xmax, double &ymax)
{
  xmin = x0 < x ? x0 : x;
  xmax = x0 > x ? x0 : x;
  ymin = y0 < y ? y0 : y;
  ymax = y0 > y ? y0 : y;

  for (double t = 0.0; t <= 1.0; t+=0.01)
  {
    double tmpx = cubicBase(t, x0, x1, x2, x);
    xmin = tmpx < xmin ? tmpx : xmin;
    xmax = tmpx > xmax ? tmpx : xmax;
    double tmpy = cubicBase(t, y0, y1, y2, y);
    ymin = tmpy < ymin ? tmpy : ymin;
    ymax = tmpy > ymax ? tmpy : ymax;
  }
}

} // anonymous namespace

namespace libcdr
{

class CDRMoveToElement : public CDRPathElement
{
public:
  CDRMoveToElement(double x, double y)
    : m_x(x),
      m_y(y) {}
  ~CDRMoveToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
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
  ~CDRLineToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
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
  ~CDRCubicBezierToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
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
  ~CDRQuadraticBezierToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
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
  ~CDRSplineToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
private:
  std::vector<std::pair<double, double> > m_points;
  unsigned knot(unsigned i) const;
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
  ~CDRArcToElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
private:
  double m_rx;
  double m_ry;
  double m_rotation;
  bool m_largeArc;
  bool m_sweep;
  double m_x;
  double m_y;
};

class CDRClosePathElement : public CDRPathElement
{
public:
  CDRClosePathElement() {}
  ~CDRClosePathElement() override {}
  void writeOut(librevenge::RVNGPropertyListVector &vec) const override;
  void transform(const CDRTransforms &trafos) override;
  void transform(const CDRTransform &trafo) override;
  CDRPathElement *clone() override;
};

} // namespace libcdr


void libcdr::CDRMoveToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "M");
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

void libcdr::CDRLineToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "L");
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

void libcdr::CDRCubicBezierToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "C");
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

void libcdr::CDRQuadraticBezierToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "Q");
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

#define CDR_SPLINE_DEGREE 3

unsigned libcdr::CDRSplineToElement::knot(unsigned i) const
{
  /* Emulates knot vector of an uniform B-Spline of degree 3 */
  if (i < CDR_SPLINE_DEGREE)
    return 0;
  if (i > m_points.size())
    return (unsigned)(m_points.size() - CDR_SPLINE_DEGREE);
  return i - CDR_SPLINE_DEGREE;
}

void libcdr::CDRSplineToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;

#if DEBUG_SPLINES
  /* Code for visual debugging of the spline decomposition */

  node.insert("librevenge:path-action", "M");
  node.insert("svg:x", m_points[0].first);
  node.insert("svg:y", m_points[0].second);
  vec.append(node);

  for (unsigned j = 0; j < m_points.size(); ++j)
  {
    node.insert("librevenge:path-action", "L");
    node.insert("svg:x", m_points[j].first);
    node.insert("svg:y", m_points[j].second);
    vec.append(node);
  }
#endif

  node.insert("librevenge:path-action", "M");
  node.insert("svg:x", m_points[0].first);
  node.insert("svg:y", m_points[0].second);
  vec.append(node);

  /* Decomposition of a spline of 3rd degree into Bezier segments
   * adapted from the algorithm DecomposeCurve (Les Piegl, Wayne Tiller:
   * The NURBS Book, 2nd Edition, 1997
   */

  unsigned m = m_points.size() + CDR_SPLINE_DEGREE + 1;
  unsigned a = CDR_SPLINE_DEGREE;
  unsigned b = CDR_SPLINE_DEGREE + 1;
  std::vector< std::pair<double, double> > Qw(CDR_SPLINE_DEGREE+1), NextQw(CDR_SPLINE_DEGREE+1);
  unsigned i = 0;
  for (; i <= CDR_SPLINE_DEGREE; i++)
    Qw[i] = m_points[i];
  while (b < m)
  {
    i = b;
    while (b < m && knot(b+1) == knot(b))
      b++;
    unsigned mult = b - i + 1;
    if (mult < CDR_SPLINE_DEGREE)
    {
      double numer = (double)(knot(b) - knot(a));
      unsigned j = CDR_SPLINE_DEGREE;
      std::map<unsigned, double> alphas;
      for (; j >mult; j--)
        alphas[j-mult-1] = numer/double(knot(a+j)-knot(a));
      unsigned r = CDR_SPLINE_DEGREE - mult;
      for (j=1; j<=r; j++)
      {
        unsigned save = r - j;
        unsigned s = mult+j;
        for (unsigned k = CDR_SPLINE_DEGREE; k>=s; k--)
        {
          double alpha = alphas[k-s];
          Qw[k].first = alpha*Qw[k].first + (1.0-alpha)*Qw[k-1].first;
          Qw[k].second = alpha*Qw[k].second + (1.0-alpha)*Qw[k-1].second;
        }
        if (b < m)
        {
          NextQw[save].first = Qw[CDR_SPLINE_DEGREE].first;
          NextQw[save].second = Qw[CDR_SPLINE_DEGREE].second;
        }
      }
    }
    // Pass the segment to the path

    node.clear();
    node.insert("librevenge:path-action", "C");
    node.insert("svg:x1", Qw[1].first);
    node.insert("svg:y1", Qw[1].second);
    node.insert("svg:x2", Qw[2].first);
    node.insert("svg:y2", Qw[2].second);
    node.insert("svg:x", Qw[3].first);
    node.insert("svg:y", Qw[3].second);

    vec.append(node);

    std::swap(Qw, NextQw);

    if (b < m)
    {
      for (i=CDR_SPLINE_DEGREE-mult; i <= CDR_SPLINE_DEGREE; i++)
      {
        Qw[i].first = m_points[b-CDR_SPLINE_DEGREE+i].first;
        Qw[i].second = m_points[b-CDR_SPLINE_DEGREE+i].second;
      }
      a = b;
      b++;
    }
  }
}

void libcdr::CDRSplineToElement::transform(const CDRTransforms &trafos)
{
  for (auto &point : m_points)
    trafos.applyToPoint(point.first, point.second);
}

void libcdr::CDRSplineToElement::transform(const CDRTransform &trafo)
{
  for (auto &point : m_points)
    trafo.applyToPoint(point.first, point.second);
}

libcdr::CDRPathElement *libcdr::CDRSplineToElement::clone()
{
  return new CDRSplineToElement(m_points);
}

void libcdr::CDRArcToElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "A");
  node.insert("svg:rx", m_rx);
  node.insert("svg:ry", m_ry);
  node.insert("librevenge:rotate", m_rotation * 180 / M_PI, librevenge::RVNG_GENERIC);
  node.insert("librevenge:large-arc", m_largeArc);
  node.insert("librevenge:sweep", m_sweep);
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

void libcdr::CDRClosePathElement::transform(const CDRTransforms &)
{
}

void libcdr::CDRClosePathElement::transform(const CDRTransform &)
{
}

libcdr::CDRPathElement *libcdr::CDRClosePathElement::clone()
{
  return new CDRClosePathElement();
}

void libcdr::CDRClosePathElement::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  librevenge::RVNGPropertyList node;
  node.insert("librevenge:path-action", "Z");
  vec.append(node);
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
  m_elements.push_back(new libcdr::CDRClosePathElement());
  m_isClosed = true;
}

libcdr::CDRPath::CDRPath(const libcdr::CDRPath &path) : m_elements(), m_isClosed(false)
{
  for (auto element : path.m_elements)
    m_elements.push_back(element->clone());
  m_isClosed = path.isClosed();
}

libcdr::CDRPath &libcdr::CDRPath::operator=(const libcdr::CDRPath &path)
{
  // Check for self-assignment
  if (this == &path)
    return *this;
  clear();
  for (auto element : path.m_elements)
    m_elements.push_back(element->clone());
  m_isClosed = path.isClosed();
  return *this;
}


libcdr::CDRPath::~CDRPath()
{
  clear();
}

void libcdr::CDRPath::appendPath(const CDRPath &path)
{
  for (auto element : path.m_elements)
    m_elements.push_back(element->clone());
}

void libcdr::CDRPath::writeOut(librevenge::RVNGPropertyListVector &vec) const
{
  bool wasZ = true;
  for (auto element : m_elements)
  {
    if (dynamic_cast<CDRClosePathElement *>(element))
    {
      if (!wasZ)
      {
        element->writeOut(vec);
        wasZ = true;
      }
    }
    else
    {
      element->writeOut(vec);
      wasZ = false;
    }
  }
}

void libcdr::CDRPath::writeOut(librevenge::RVNGString &path, librevenge::RVNGString &viewBox, double &width) const
{
  librevenge::RVNGPropertyListVector vec;
  writeOut(vec);
  if (vec.count() == 0)
    return;
  // This must be a mistake and we do not want to crash lower
  if (vec[0]["librevenge:path-action"]->getStr() == "Z")
    return;

  // try to find the bounding box
  bool isFirstPoint = true;

  double px = 0.0, py = 0.0, qx = 0.0, qy = 0.0;
  double lastX = 0.0;
  double lastY = 0.0;

  for (unsigned k = 0; k < vec.count(); ++k)
  {
    if (!vec[k]["svg:x"] || !vec[k]["svg:y"])
      continue;
    if (isFirstPoint)
    {
      px = vec[k]["svg:x"]->getDouble();
      py = vec[k]["svg:y"]->getDouble();
      qx = px;
      qy = py;
      lastX = px;
      lastY = py;
      isFirstPoint = false;
    }
    px = (px > vec[k]["svg:x"]->getDouble()) ? vec[k]["svg:x"]->getDouble() : px;
    py = (py > vec[k]["svg:y"]->getDouble()) ? vec[k]["svg:y"]->getDouble() : py;
    qx = (qx < vec[k]["svg:x"]->getDouble()) ? vec[k]["svg:x"]->getDouble() : qx;
    qy = (qy < vec[k]["svg:y"]->getDouble()) ? vec[k]["svg:y"]->getDouble() : qy;

    double xmin, xmax, ymin, ymax;

    if (vec[k]["librevenge:path-action"]->getStr() == "C")
    {
      getCubicBezierBBox(lastX, lastY, vec[k]["svg:x1"]->getDouble(), vec[k]["svg:y1"]->getDouble(),
                         vec[k]["svg:x2"]->getDouble(), vec[k]["svg:y2"]->getDouble(),
                         vec[k]["svg:x"]->getDouble(), vec[k]["svg:y"]->getDouble(), xmin, ymin, xmax, ymax);

      px = (px > xmin ? xmin : px);
      py = (py > ymin ? ymin : py);
      qx = (qx < xmax ? xmax : qx);
      qy = (qy < ymax ? ymax : qy);
    }
    if (vec[k]["librevenge:path-action"]->getStr() == "Q")
    {
      getQuadraticBezierBBox(lastX, lastY, vec[k]["svg:x1"]->getDouble(), vec[k]["svg:y1"]->getDouble(),
                             vec[k]["svg:x"]->getDouble(), vec[k]["svg:y"]->getDouble(), xmin, ymin, xmax, ymax);

      px = (px > xmin ? xmin : px);
      py = (py > ymin ? ymin : py);
      qx = (qx < xmax ? xmax : qx);
      qy = (qy < ymax ? ymax : qy);
    }
    if (vec[k]["librevenge:path-action"]->getStr() == "A")
    {
      getEllipticalArcBBox(lastX, lastY, vec[k]["svg:rx"]->getDouble(), vec[k]["svg:ry"]->getDouble(),
                           vec[k]["librevenge:rotate"] ? vec[k]["librevenge:rotate"]->getDouble() : 0.0,
                           vec[k]["librevenge:large-arc"] ? vec[k]["librevenge:large-arc"]->getInt() : 1,
                           vec[k]["librevenge:sweep"] ? vec[k]["librevenge:sweep"]->getInt() : 1,
                           vec[k]["svg:x"]->getDouble(), vec[k]["svg:y"]->getDouble(), xmin, ymin, xmax, ymax);

      px = (px > xmin ? xmin : px);
      py = (py > ymin ? ymin : py);
      qx = (qx < xmax ? xmax : qx);
      qy = (qy < ymax ? ymax : qy);
    }
    lastX = vec[k]["svg:x"]->getDouble();
    lastY = vec[k]["svg:y"]->getDouble();
  }


  width = qy - py;
  viewBox.sprintf("%i %i %i %i", 0, 0, (unsigned)(2540*(qx - px)), (unsigned)(2540*(qy - py)));

  for (unsigned i = 0; i < vec.count(); ++i)
  {
    librevenge::RVNGString sElement;
    if (vec[i]["librevenge:path-action"]->getStr() == "M")
    {
      // 2540 is 2.54*1000, 2.54 in = 1 inch
      sElement.sprintf("M%i %i", (unsigned)((vec[i]["svg:x"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y"]->getDouble()-py)*2540));
      path.append(sElement);
    }
    else if (vec[i]["librevenge:path-action"]->getStr() == "L")
    {
      sElement.sprintf("L%i %i", (unsigned)((vec[i]["svg:x"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y"]->getDouble()-py)*2540));
      path.append(sElement);
    }
    else if (vec[i]["librevenge:path-action"]->getStr() == "C")
    {
      sElement.sprintf("C%i %i %i %i %i %i", (unsigned)((vec[i]["svg:x1"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y1"]->getDouble()-py)*2540), (unsigned)((vec[i]["svg:x2"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y2"]->getDouble()-py)*2540), (unsigned)((vec[i]["svg:x"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y"]->getDouble()-py)*2540));
      path.append(sElement);
    }
    else if (vec[i]["librevenge:path-action"]->getStr() == "Q")
    {
      sElement.sprintf("Q%i %i %i %i", (unsigned)((vec[i]["svg:x1"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y1"]->getDouble()-py)*2540), (unsigned)((vec[i]["svg:x"]->getDouble()-px)*2540),
                       (unsigned)((vec[i]["svg:y"]->getDouble()-py)*2540));
      path.append(sElement);
    }
    else if (vec[i]["librevenge:path-action"]->getStr() == "A")
    {
      sElement.sprintf("A%i %i %i %i %i %i %i", (unsigned)((vec[i]["svg:rx"]->getDouble())*2540),
                       (unsigned)((vec[i]["svg:ry"]->getDouble())*2540), (vec[i]["librevenge:rotate"] ? vec[i]["librevenge:rotate"]->getInt() : 0),
                       (vec[i]["librevenge:large-arc"] ? vec[i]["librevenge:large-arc"]->getInt() : 1),
                       (vec[i]["librevenge:sweep"] ? vec[i]["librevenge:sweep"]->getInt() : 1),
                       (unsigned)((vec[i]["svg:x"]->getDouble()-px)*2540), (unsigned)((vec[i]["svg:y"]->getDouble()-py)*2540));
      path.append(sElement);
    }
    else if (vec[i]["librevenge:path-action"]->getStr() == "Z")
    {
      path.append(" Z");
    }
  }
}

void libcdr::CDRPath::transform(const CDRTransforms &trafos)
{
  for (auto &element : m_elements)
    element->transform(trafos);
}

void libcdr::CDRPath::transform(const CDRTransform &trafo)
{
  for (auto &element : m_elements)
    element->transform(trafo);
}

libcdr::CDRPathElement *libcdr::CDRPath::clone()
{
  return new CDRPath(*this);
}

void libcdr::CDRPath::clear()
{
  for (auto &element : m_elements)
    if (element)
      delete element;
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
