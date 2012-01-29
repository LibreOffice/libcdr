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

#include "CDRCollector.h"
#include "libcdr_utils.h"

libcdr::CDRCollector::CDRCollector(libwpg::WPGPaintInterface *painter) :
  m_painter(painter),
  m_isPageProperties(false),
  m_isPageStarted(false),
  m_pageOffsetX(0.0), m_pageOffsetY(0.0),
  m_pageWidth(0.0), m_pageHeight(0.0),
  m_currentFildId(0), m_currentOutlId(0.0),
  m_currentPath(), m_currentTransform(),
  m_fillStyles(), m_lineStyles()
{
}

libcdr::CDRCollector::~CDRCollector()
{
  if (m_isPageStarted)
    _endPage();
}

void libcdr::CDRCollector::_startPage(double width, double height)
{
  WPXPropertyList propList;
  propList.insert("svg:width", width);
  propList.insert("svg:height", height);
  if (m_painter)
  {
    m_painter->startGraphics(propList);
    m_isPageStarted = true;
  }
}

void libcdr::CDRCollector::_endPage()
{
  if (m_painter)
    m_painter->endGraphics();
}

void libcdr::CDRCollector::collectPage()
{
  if (m_isPageStarted)
    _endPage();
  m_isPageStarted = false;
  m_isPageProperties = true;
}

void libcdr::CDRCollector::collectObject()
{
  m_isPageProperties = false;
  if (!m_isPageStarted)
    _startPage(m_pageWidth, m_pageHeight);
}

void libcdr::CDRCollector::collectOtherList()
{
  m_isPageProperties = false;
}

void libcdr::CDRCollector::collectBbox(double x0, double y0, double x1, double y1)
{
  double width = (x0 < x1 ? x1 - x0 : x0 - x1);
  double height = (y0 < y1 ? y1 - y0 : y0 - y1);
  if (m_isPageProperties)
  {
    m_pageWidth = width;
    m_pageHeight = height;
    m_pageOffsetX = x0;
    m_pageOffsetY = y1;
  }
}

void libcdr::CDRCollector::collectCubicBezier(double x1, double y1, double x2, double y2, double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectCubicBezier(%f, %f, %f, %f, %f, %f)\n", x1, y1, x2, y2, x, y));
  WPXPropertyList node;
  node.insert("svg:x1", x1);
  node.insert("svg:y1", y1);
  node.insert("svg:x2", x2);
  node.insert("svg:y2", y2);
  node.insert("svg:x", x);
  node.insert("svg:y", y);
  node.insert("libwpg:path-action", "C");
  m_currentPath.append(node);
}

void libcdr::CDRCollector::collectMoveTo(double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectMoveTo(%f, %f)\n", x, y));
  WPXPropertyList node;
  node.insert("svg:x", x);
  node.insert("svg:y", y);
  node.insert("libwpg:path-action", "M");
  m_currentPath.append(node);
}

void libcdr::CDRCollector::collectLineTo(double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectLineTo(%f, %f)\n", x, y));
  WPXPropertyList node;
  node.insert("svg:x", x);
  node.insert("svg:y", y);
  node.insert("libwpg:path-action", "L");
  m_currentPath.append(node);
}

void libcdr::CDRCollector::collectClosePath()
{
  CDR_DEBUG_MSG(("CDRCollector::collectClosePath\n"));
  WPXPropertyList node;
  node.insert("libwpg:path-action", "Z");
  m_currentPath.append(node);
}

void libcdr::CDRCollector::_flushCurrentPath()
{
  CDR_DEBUG_MSG(("CDRCollector::collectFlushPath\n"));
  if (m_currentPath.count())
  {
    WPXPropertyList style;
    style.insert("draw:stroke", "solid");
    style.insert("svg:stroke-width", 1.0 / 72.0);
    style.insert("svg:stroke-color", "#000000");
    style.insert("draw:fill", "none");
    m_painter->setStyle(style, WPXPropertyListVector());
    WPXPropertyListVector path;
    WPXPropertyListVector::Iter i(m_currentPath);
    for (i.rewind(); i.next();)
    {
      if (!i()["libwpg:path-action"])
        continue;
      WPXPropertyList node;
      node.insert("libwpg:path-action", i()["libwpg:path-action"]->getStr());
      if (i()["svg:x"] && i()["svg:y"])
      {
        double x = i()["svg:x"]->getDouble();
        double y = i()["svg:y"]->getDouble();
        m_currentTransform.apply(x,y);
        y = m_pageHeight - y;
        node.insert("svg:x", x);
        node.insert("svg:y", y);
      }
      if (i()["svg:x1"] && i()["svg:y1"])
      {
        double x = i()["svg:x1"]->getDouble();
        double y = i()["svg:y1"]->getDouble();
        m_currentTransform.apply(x,y);
        y = m_pageHeight - y;
        node.insert("svg:x1", x);
        node.insert("svg:y1", y);
      }
      if (i()["svg:x2"] && i()["svg:y2"])
      {
        double x = i()["svg:x2"]->getDouble();
        double y = i()["svg:y2"]->getDouble();
        m_currentTransform.apply(x,y);
        y = m_pageHeight - y;
        node.insert("svg:x2", x);
        node.insert("svg:y2", y);
      }
      path.append(node);
    }

    m_painter->drawPath(path);
    m_currentPath = WPXPropertyListVector();
  }
}

void libcdr::CDRCollector::collectTransform(double v0, double v1, double x0, double v3, double v4, double y0)
{
  m_currentTransform.v0 = v0;
  m_currentTransform.v1 = v1;
  m_currentTransform.x0 = x0 - m_pageOffsetX;
  m_currentTransform.v3 = v3;
  m_currentTransform.v4 = v4;
  m_currentTransform.y0 = y0 - m_pageOffsetY;
}

void libcdr::CDRCollector::collectLevel(unsigned level)
{
  if (level <= 4)
    _flushCurrentPath();
}

void libcdr::CDRCollector::collectFildId(unsigned id)
{
  m_currentFildId = id;
}

void libcdr::CDRCollector::collectOutlId(unsigned id)
{
  m_currentOutlId = id;
}

void libcdr::CDRCollector::collectFild(unsigned id, unsigned short fillType, unsigned short colorModel, unsigned color1, unsigned color2)
{
  m_fillStyles[id] = CDRFillStyle(fillType, colorModel, color1, color2);
}

void libcdr::CDRCollector::collectOutl(unsigned id, unsigned short lineType, unsigned short capsType, unsigned short joinType,
                                       double lineWidth, unsigned short colorModel, unsigned color,
                                       const std::vector<unsigned short> &dashArray, unsigned startMarkerId, unsigned endMarkerId)
{
  m_lineStyles[id] = CDRLineStyle(lineType, capsType, joinType, lineWidth, colorModel, color, dashArray, startMarkerId, endMarkerId);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
