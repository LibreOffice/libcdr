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
 * Copyright (C) 2011 Eilidh McAdam <tibbylickle@gmail.com>
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

#include "CDRTransforms.h"
#include "CDRPath.h"
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

libcdr::CDRTransform::CDRTransform(const CDRTransform &trafo)
  : m_v0(trafo.m_v0), m_v1(trafo.m_v1), m_x0(trafo.m_x0),
    m_v3(trafo.m_v3), m_v4(trafo.m_v4), m_y0(trafo.m_y0) {}


void libcdr::CDRTransform::applyToPoint(double &x, double &y) const
{
  double tmpX = m_v0*x + m_v1*y+m_x0;
  y = m_v3*x + m_v4*y+m_y0;
  x = tmpX;
}

void libcdr::CDRTransform::applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &x, double &y) const
{
  // First transform the end-point, which is the easiest
  applyToPoint(x, y);

  // represent ellipse as a transformed unit circle
  double v0 = m_v0*rx*cos(rotation) - m_v1*rx*sin(rotation);
  double v1 = m_v1*ry*cos(rotation) + m_v0*ry*sin(rotation);
  double v3 = m_v3*rx*cos(rotation) - m_v4*rx*sin(rotation);
  double v4 = m_v4*ry*cos(rotation) + m_v3*ry*sin(rotation);

  // centered implicit equation
  double A = v0*v0 + v1*v1;
  double C = v3*v3 + v4*v4;
  double B = 2.0*(v0*v3  +  v1*v4);

  double r1, r2;
  // convert implicit equation to angle and halfaxis:
  if (CDR_ALMOST_ZERO(B))
  {
    rotation = 0;
    r1 = A;
    r2 = C;
  }
  else
  {
    if (CDR_ALMOST_ZERO(A-C))
    {
      r1 = A + B / 2.0;
      r2 = A - B / 2.0;
      rotation = M_PI / 4.0;
    }
    else
    {
      double radical = 1.0 + B*B /((A-C)*(A-C));
      radical = radical < 0.0 ? 0.0 : sqrt (radical);

      r1 = (A+C + radical*(A-C)) / 2.0;
      r2 = (A+C - radical*(A-C)) / 2.0;
      rotation = atan2(B,(A-C)) / 2.0;
    }
  }

  // Prevent sqrt of a negative number, however small it might be.
  r1 = r1 < 0.0 ? 0.0 : sqrt(r1);
  r2 = r2 < 0.0 ? 0.0 : sqrt(r2);

  // now r1 and r2 are half-axis:
  if ((A-C) <= 0)
  {
    ry = r1;
    rx = r2;
  }
  else
  {
    ry = r2;
    rx = r1;
  }

  // sweep is inversed each time the arc is flipped
  if (v0 < 0)
    sweep = !sweep;
  if (v4 < 0)
    sweep = !sweep;
  
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

double libcdr::CDRTransform::getRotation() const
{
  double x0 = 0.0;
  double x1 = 1.0;
  double y0 = 0.0;
  double y1 = 0.0;
  applyToPoint(x0, y0);
  applyToPoint(x1, y1);
  double angle = atan2(y1-y0, x1-x0);
  if (angle < 0.0)
    angle += 2*M_PI;
  return angle;
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


libcdr::CDRTransforms::CDRTransforms()
  : m_trafos()
{
}

libcdr::CDRTransforms::CDRTransforms(const CDRTransforms &trafos)
  : m_trafos(trafos.m_trafos)
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
  for (std::vector<CDRTransform>::const_iterator iter = m_trafos.begin(); iter != m_trafos.end(); ++iter)
    iter->applyToPoint(x,y);
}

void libcdr::CDRTransforms::applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &x, double &y) const
{
  for (std::vector<CDRTransform>::const_iterator iter = m_trafos.begin(); iter != m_trafos.end(); ++iter)
    iter->applyToArc(rx, ry, rotation, sweep, x, y);
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

double libcdr::CDRTransforms::getRotation() const
{
  double x0 = 0.0;
  double x1 = 1.0;
  double y0 = 0.0;
  double y1 = 0.0;
  applyToPoint(x0, y0);
  applyToPoint(x1, y1);
  double angle = atan2(y1-y0, x1-x0);
  if (angle < 0.0)
    angle += 2*M_PI;
  return angle;
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
