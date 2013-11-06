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


/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
