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

void libcdr::CDRPolygon::create(libcdr::CDRPath &path) const
{
  if (m_numAngles == 0)
    return;

  libcdr::CDRPath tmpPath(path);
  double step = 2*M_PI / (double)m_numAngles;
  if (m_nextPoint && m_numAngles % m_nextPoint)
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


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
