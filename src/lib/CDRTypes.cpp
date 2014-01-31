/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDRTypes.h"
#include "CDRPath.h"
#include "libcdr_utils.h"

void libcdr::CDRPolygon::create(libcdr::CDRPath &path) const
{
  libcdr::CDRPath tmpPath(path);
  double step = 2*M_PI / (double)m_numAngles;
  if (m_numAngles % m_nextPoint)
  {
    libcdr::CDRTransform tmpTrafo(cos(m_nextPoint*step), sin(m_nextPoint*step), 0.0, -sin(m_nextPoint*step), cos(m_nextPoint*step), 0.0);
    for (unsigned i = 1; i < m_numAngles; ++i)
    {
      tmpPath.transform(tmpTrafo);
      path.appendPath(tmpPath);
    }
  }
  else
  {
    libcdr::CDRTransform tmpTrafo(cos(m_nextPoint*step), sin(m_nextPoint*step), 0.0, -sin(m_nextPoint*step), cos(m_nextPoint*step), 0.0);
    libcdr::CDRTransform tmpShift(cos(step), sin(step), 0.0, -sin(step), cos(step), 0.0);
    for (unsigned i = 0; i < m_nextPoint; ++i)
    {
      if (i)
      {
        tmpPath.transform(tmpShift);
        path.appendPath(tmpPath);
      }
      for (unsigned j=1; j < m_numAngles / m_nextPoint; ++j)
      {
        tmpPath.transform(tmpTrafo);
        path.appendPath(tmpPath);
      }
      path.appendClosePath();
    }
  }
  path.appendClosePath();
  libcdr::CDRTransform trafo(m_rx, 0.0, m_cx, 0.0, m_ry, m_cy);
  path.transform(trafo);
}

void libcdr::CDRSplineData::create(libcdr::CDRPath &path) const
{
  if (points.empty() || knotVector.empty())
    return;
  path.appendMoveTo(points[0].first, points[0].second);
  std::vector<std::pair<double, double> > tmpPoints;
  tmpPoints.push_back(points[0]);
  for (unsigned i = 1; i<points.size() && i<knotVector.size(); ++i)
  {
    tmpPoints.push_back(points[i]);
    if (knotVector[i])
    {
      if (tmpPoints.size() == 2)
        path.appendLineTo(tmpPoints[1].first, tmpPoints[1].second);
      else if (tmpPoints.size() == 3)
        path.appendQuadraticBezierTo(tmpPoints[1].first, tmpPoints[1].second,
                                     tmpPoints[2].first, tmpPoints[3].second);
      else
        path.appendSplineTo(tmpPoints);
      tmpPoints.clear();
      tmpPoints.push_back(points[i]);
    }
  }
  if (tmpPoints.size() == 2)
    path.appendLineTo(tmpPoints[1].first, tmpPoints[1].second);
  else if (tmpPoints.size() == 3)
    path.appendQuadraticBezierTo(tmpPoints[1].first, tmpPoints[1].second,
                                 tmpPoints[2].first, tmpPoints[3].second);
  else if (tmpPoints.size() > 3)
    path.appendSplineTo(tmpPoints);
}

libcdr::CDRCMYKColor::CDRCMYKColor(unsigned colorValue, bool percentage)
  : c((double)(colorValue & 0xff) / (percentage ? 100.0 : 255.0)),
    m((double)((colorValue & 0xff00) >> 8) / (percentage ? 100.0 : 255.0)),
    y((double)((colorValue & 0xff0000) >> 16) / (percentage ? 100.0 : 255.0)),
    k((double)((colorValue & 0xff000000) >> 24) / (percentage ? 100.0 : 255.0))
{
}

void libcdr::CDRCMYKColor::applyTint(double tint)
{
  if (tint < 0.0)
    tint = 0.0;
  if (tint > 1.0)
    tint = 1.0;
  c *= tint;
  m *= tint;
  y *= tint;
  k *= tint;
}

unsigned libcdr::CDRCMYKColor::getColorValue() const
{
  return 0;
}


libcdr::CDRRGBColor::CDRRGBColor(unsigned colorValue)
  : r((double)(colorValue & 0xff) / 255.0),
    g((double)((colorValue & 0xff00) >> 8) / 255.0),
    b((double)((colorValue & 0xff0000) >> 16) / 255.0)
{
}

void libcdr::CDRRGBColor::applyTint(double tint)
{
  if (tint < 0.0)
    tint = 0.0;
  if (tint > 1.0)
    tint = 1.0;
  r = 1.0 + r*tint - tint;
  g = 1.0 + g*tint - tint;
  b = 1.0 + b*tint - tint;
}

unsigned libcdr::CDRRGBColor::getColorValue() const
{
  return 0;
}


libcdr::CDRLab2Color::CDRLab2Color(unsigned colorValue)
  : L((double)(colorValue & 0xff)*100.0/255.0),
    a((double)(signed char)((colorValue & 0xff00) >> 8)),
    b((double)(signed char)((colorValue & 0xff0000) >> 16))
{
}

void libcdr::CDRLab2Color::applyTint(double tint)
{
  if (tint < 0.0)
    tint = 0.0;
  if (tint > 1.0)
    tint = 1.0;
  L = (1.0 - tint)*100.0 + tint*L;
  a *= tint;
  b *= tint;
}

unsigned libcdr::CDRLab2Color::getColorValue() const
{
  return 0;
}


libcdr::CDRLab4Color::CDRLab4Color(unsigned colorValue)
  : L((double)(colorValue & 0xff)*100.0/255.0),
    a((double)((signed char)(((colorValue & 0xff00) >> 8) - 0x80))),
    b((double)((signed char)(((colorValue & 0xff0000) >> 16) - 0x80)))
{
}

void libcdr::CDRLab4Color::applyTint(double tint)
{
  if (tint < 0.0)
    tint = 0.0;
  if (tint > 1.0)
    tint = 1.0;
  L = (1.0 - tint)*100.0 + tint*L;
  a *= tint;
  b *= tint;
}

unsigned libcdr::CDRLab4Color::getColorValue() const
{
  return 0;
}


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
