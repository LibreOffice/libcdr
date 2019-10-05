/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDRTransforms.h"

#include "libcdr_utils.h"


libcdr::CDRTransform::CDRTransform()
  : m_v0(1.0), m_v1(0.0), m_x0(0.0),
    m_v3(0.0), m_v4(1.0), m_y0(0.0)
{
}

libcdr::CDRTransform::CDRTransform(double v0, double v1, double x0, double v3, double v4, double y0)
  : m_v0(v0), m_v1(v1), m_x0(x0), m_v3(v3), m_v4(v4), m_y0(y0)
{
}

void libcdr::CDRTransform::applyToPoint(double &x, double &y) const
{
  double tmpX = m_v0*x + m_v1*y+m_x0;
  y = m_v3*x + m_v4*y+m_y0;
  x = tmpX;
}

void libcdr::CDRTransform::applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &endx, double &endy) const
{
  // Transform the end-point, which is the easiest
  applyToPoint(endx, endy);

  // Determine whether sweep should change
  double determinant = m_v0*m_v4 - m_v1*m_v3;
  if (determinant < 0.0)
    sweep = !sweep;

  // Transform a centered ellipse

  // rx = 0 and ry = 0
  if (CDR_ALMOST_ZERO(rx) && CDR_ALMOST_ZERO(ry))
  {
    rotation = rx = ry = 0.0;
    return;
  }

  // rx > 0, ry = 0
  if (CDR_ALMOST_ZERO(ry))
  {
    double x = m_v0*cos(rotation) + m_v1*sin(rotation);
    double y = m_v3*cos(rotation) + m_v4*sin(rotation);
    rx *= sqrt(x*x + y*y);
    if (CDR_ALMOST_ZERO(rx))
    {
      rotation = rx = ry = 0;
      return;
    }
    rotation = atan2(y, x);
    return;
  }

  // rx = 0, ry > 0
  if (CDR_ALMOST_ZERO(rx))
  {
    double x = -m_v0*sin(rotation) + m_v1*cos(rotation);
    double y = -m_v3*sin(rotation) + m_v4*cos(rotation);
    ry *= sqrt(x*x + y*y);
    if (CDR_ALMOST_ZERO(ry))
    {
      rotation = rx = ry = 0;
      return;
    }
    rotation = atan2(y, x) - M_PI/2.0;
    return;
  }

  double v0, v1, v2, v3;
  if (!CDR_ALMOST_ZERO(determinant))
  {
    v0 = ry*(m_v4*cos(rotation)- m_v3*sin(rotation));
    v1 = ry*(m_v0*sin(rotation)- m_v1*cos(rotation));
    v2 = -rx*(m_v4*sin(rotation) + m_v3*cos(rotation));
    v3 = rx*(m_v1*sin(rotation) + m_v0*cos(rotation));

    // Transformed ellipse
    double A = v0*v0 + v2*v2;
    double B = 2.0*(v0*v1 + v2*v3);
    double C = v1*v1 + v3*v3;

    // Rotate the transformed ellipse
    if (CDR_ALMOST_ZERO(B))
      rotation = 0;
    else
    {
      rotation = atan2(B, A-C) / 2.0;
      double c = cos(rotation);
      double s = sin(rotation);
      double cc = c*c;
      double ss = s*s;
      double sc = B*s*c;
      B = A;
      A = B*cc + sc + C*ss;
      C = B*ss - sc + C*cc;
    }

    // Compute rx and ry from A and C
    if (!CDR_ALMOST_ZERO(A) && !CDR_ALMOST_ZERO(C))
    {
      double abdet = fabs(rx*ry*determinant);
      A = sqrt(fabs(A));
      C = sqrt(fabs(C));
      rx = abdet / A;
      ry = abdet / C;
      return;
    }
  }

  // Special case of a close to singular transformation
  v0 = ry*(m_v4*cos(rotation) - m_v3*sin(rotation));
  v1 = ry*(m_v1*cos(rotation) - m_v0*sin(rotation));
  v2 = rx*(m_v3*cos(rotation) + m_v4*sin(rotation));
  v3 = rx*(m_v0*cos(rotation) + m_v1*sin(rotation));

  // The result of transformation is a point
  if (CDR_ALMOST_ZERO(v3*v3 + v1*v1) && CDR_ALMOST_ZERO(v2*v2 + v0*v0))
  {
    rotation = rx = ry = 0;
    return;
  }
  else // The result of transformation is a line
  {
    double x = sqrt(v3*v3 + v1*v1);
    double y = sqrt(v2*v2 + v0*v0);
    if (v3*v3 + v1*v1 >= v2*v2 + v0*v0)
      y = (v2*v2 + v0*v0) / x;
    else
      x = (v3*v3 + v1*v1) / y;
    rx = sqrt(x*x + y*y);
    ry = 0;
    rotation = atan2(y, x);
  }
}

double libcdr::CDRTransform::_getScaleX() const
{
  double x0 = 0.0;
  double x1 = 1.0;
  double y0 = 0.0;
  double y1 = 0.0;
  applyToPoint(x0, y0);
  applyToPoint(x1, y1);
  return x1 - x0;
}

double libcdr::CDRTransform::getScaleX() const
{
  return fabs(_getScaleX());
}

bool libcdr::CDRTransform::getFlipX() const
{
  return (0 > _getScaleX());
}

double libcdr::CDRTransform::_getScaleY() const
{
  double x0 = 0.0;
  double x1 = 0.0;
  double y0 = 0.0;
  double y1 = 1.0;
  applyToPoint(x0, y0);
  applyToPoint(x1, y1);
  return y1 - y0;
}

double libcdr::CDRTransform::getScaleY() const
{
  return fabs(_getScaleY());
}

bool libcdr::CDRTransform::getFlipY() const
{
  return (0 > _getScaleY());
}

double libcdr::CDRTransform::getTranslateX() const
{
  double x = 0.0;
  double y = 0.0;
  applyToPoint(x, y);
  return x;
}

double libcdr::CDRTransform::getTranslateY() const
{
  double x = 0.0;
  double y = 0.0;
  applyToPoint(x, y);
  return y;
}

librevenge::RVNGString libcdr::CDRTransform::toString() const
{
  librevenge::RVNGString output;
  output.sprintf("[%f %f %f | %f %f %f]", m_v0, m_v1, m_x0, m_v3, m_v4, m_y0);
  return output;
}


libcdr::CDRTransforms::CDRTransforms()
  : m_trafos()
{
}

libcdr::CDRTransforms::~CDRTransforms()
{
}

void libcdr::CDRTransforms::append(double v0, double v1, double x0, double v3, double v4, double y0)
{
  append(CDRTransform(v0, v1, x0, v3, v4, y0));
}

void libcdr::CDRTransforms::append(const CDRTransform &trafo)
{
  m_trafos.push_back(trafo);
}

void libcdr::CDRTransforms::clear()
{
  m_trafos.clear();
}

bool libcdr::CDRTransforms::empty() const
{
  return m_trafos.empty();
}

void libcdr::CDRTransforms::applyToPoint(double &x, double &y) const
{
  for (const auto &trafo : m_trafos)
    trafo.applyToPoint(x,y);
}

void libcdr::CDRTransforms::applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &x, double &y) const
{
  for (const auto &trafo : m_trafos)
    trafo.applyToArc(rx, ry, rotation, sweep, x, y);
}

double libcdr::CDRTransforms::_getScaleX() const
{
  double x0 = 0.0;
  double x1 = 1.0;
  double y0 = 0.0;
  double y1 = 0.0;
  applyToPoint(x0, y0);
  applyToPoint(x1, y1);
  return x1 - x0;
}

double libcdr::CDRTransforms::getScaleX() const
{
  return fabs(_getScaleX());
}

bool libcdr::CDRTransforms::getFlipX() const
{
  return (0 > _getScaleX());
}

double libcdr::CDRTransforms::_getScaleY() const
{
  double x0 = 0.0;
  double x1 = 0.0;
  double y0 = 0.0;
  double y1 = 1.0;
  applyToPoint(x0, y0);
  applyToPoint(x1, y1);
  return y1 - y0;
}

double libcdr::CDRTransforms::getScaleY() const
{
  return fabs(_getScaleY());
}

bool libcdr::CDRTransforms::getFlipY() const
{
  return (0 > _getScaleY());
}

double libcdr::CDRTransforms::getTranslateX() const
{
  double x = 0.0;
  double y = 0.0;
  applyToPoint(x, y);
  return x;
}

double libcdr::CDRTransforms::getTranslateY() const
{
  double x = 0.0;
  double y = 0.0;
  applyToPoint(x, y);
  return y;
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
