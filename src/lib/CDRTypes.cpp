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
 * in which case the procdrns of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
 */

#include <math.h>
#include "CDRTypes.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void libcdr::CDRTransform::applyToPoint(double &x, double &y) const
{
  x = m_v0*x + m_v1*y+m_x0;
  y = m_v3*x + m_v4*y+m_y0;
}

#define EPSILON 0.00001

void libcdr::CDRTransform::applyToArc(double &rx, double &ry, double &rotation, bool &sweep, double &x, double &y) const
{
  // First transform the end-point, which is the easiest
  applyToPoint(x, y);

  // represent ellipse as a transformed unit circle
  double v0 = m_v0*rx*cos(rotation) + m_v1*rx*sin(rotation);
  double v1 = m_v1*ry*cos(rotation) - m_v0*ry*sin(rotation);
  double v3 = m_v3*rx*cos(rotation) + m_v4*rx*sin(rotation);
  double v4 = m_v4*ry*cos(rotation) - m_v3*ry*sin(rotation);

  // centered implicit equation
  double A = v0*v0 + v1*v1;
  double C = v3*v3 + v4*v4;
  double B = 2.0*(v0*v3  +  v1*v4);

  double r1, r2;
  // convert implicit equation to angle and halfaxis:
  if (fabs(B) <= EPSILON)
  {
    rotation = 0;
    r1 = A;
    r2 = C;
  }
  else
  {
    if (fabs(A-C) <= EPSILON)
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
  sweep = (m_v0*m_v4 < m_v3*m_v1);
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
