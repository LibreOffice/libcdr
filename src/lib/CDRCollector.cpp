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
  m_currentFildId(0), m_currentOutlId(0),
  m_currentObjectLevel(0), m_currentPageLevel(0),
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
  if (!m_isPageStarted)
    return;
  if (m_painter)
    m_painter->endGraphics();
  m_isPageStarted = false;
}

void libcdr::CDRCollector::collectPage(unsigned level)
{
  m_isPageProperties = true;
  m_currentPageLevel = level;
}

void libcdr::CDRCollector::collectObject(unsigned level)
{
  m_isPageProperties = false;
  if (!m_isPageStarted)
    _startPage(m_pageWidth, m_pageHeight);
  m_currentObjectLevel = level;
  m_currentFildId = 0;
  m_currentOutlId = 0;
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
    bool firstPoint = true;
    double initialX = 0.0;
    double initialY = 0.0;
    double previousX = 0.0;
    double previousY = 0.0;
    double x = 0.0;
    double y = 0.0;
    WPXPropertyList style;
    _fillProperties(style);
    _lineProperties(style);
    m_painter->setStyle(style, WPXPropertyListVector());
    WPXPropertyList node;
    WPXPropertyListVector path;
    WPXPropertyListVector::Iter i(m_currentPath);
    for (i.rewind(); i.next();)
    {
      if (!i()["libwpg:path-action"])
        continue;
      if (i()["svg:x"] && i()["svg:y"])
      {
        x = i()["svg:x"]->getDouble();
        y = i()["svg:y"]->getDouble();
        m_currentTransform.apply(x,y);
        y = m_pageHeight - y;

        if (firstPoint)
        {
          initialX = x;
          initialY = y;
          firstPoint = false;
        }
        else if (i()["libwpg:path-action"] && i()["libwpg:path-action"]->getStr() == "M")
        {
          if (initialX == previousX && initialY == previousY)
          {
            node.insert("libwpg:path-action", "Z");
            path.append(node);
            node.clear();
          }
          initialX = x;
          initialY = y;
        }
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
      node.insert("libwpg:path-action", i()["libwpg:path-action"]->getStr());
      path.append(node);
      node.clear();
      previousX = x;
      previousY = y;

    }
    if (initialX == previousX && initialY == previousY)
    {
      node.insert("libwpg:path-action", "Z");
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
  if (level <= m_currentObjectLevel)
  {
    _flushCurrentPath();
    m_currentObjectLevel = 0;
  }
  if (level <= m_currentPageLevel)
  {
    _endPage();
    m_currentPageLevel = 0;
  }
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

unsigned libcdr::CDRCollector::_getRGBColor(unsigned short colorModel, unsigned colorValue)
{
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char col0 = colorValue & 0xff;
  unsigned char col1 = (colorValue & 0xff00) >> 8;
  unsigned char col2 = (colorValue & 0xff0000) >> 16;
  unsigned char col3 = (colorValue & 0xff000000) >> 24;
  if (colorModel == 0x02) // CMYK100
  {
    red = (100 - col0)*(100 - col3)*255/10000;
    green = (100 - col1)*(100 - col3)*255/10000;
    blue = (100 - col2)*(100 - col3)*255/10000;
  }
  else if ((colorModel == 0x03) || (colorModel == 0x11)) // CMYK255
  {
    red = (255 - col0)*(255 - col3)/255;
    green = (255 - col1)*(255 - col3)/255;
    blue = (255 - col2)*(255 - col3)/255;
  }
  else if (colorModel == 0x04) // CMY
  {
    red = 255 - col0;
    green = 255 - col1;
    blue = 255 - col2;
  }
  else if (colorModel == 0x05) // RGB
  {
    red = col2;
    green = col1;
    blue = col0;
  }
  else if (colorModel == 0x09) // Grayscale
  {
    red = col0;
    green = col0;
    blue = col0;
  }
  return (unsigned)((red << 16) | (green << 8) | blue);
}

WPXString libcdr::CDRCollector::_getRGBColorString(unsigned short colorModel, unsigned colorValue)
{
  WPXString tempString;
  tempString.sprintf("#%.6x", _getRGBColor(colorModel, colorValue));
  return tempString;
}

void libcdr::CDRCollector::_fillProperties(WPXPropertyList &propList)
{
  if (m_currentFildId == 0)
    propList.insert("draw:fill", "none");
  else
  {
    std::map<unsigned, CDRFillStyle>::iterator iter = m_fillStyles.find(m_currentFildId);
    if ((iter == m_fillStyles.end()) || !(iter->second.fillType))
      propList.insert("draw:fill", "none");
    else
    {
      propList.insert("draw:fill", "solid");
      propList.insert("draw:fill-color", _getRGBColorString(iter->second.colorModel, iter->second.color1));
    }
  }
}

void libcdr::CDRCollector::_lineProperties(WPXPropertyList &propList)
{
  if (m_currentOutlId == 0)
  {
    propList.insert("draw:stroke", "solid");
    propList.insert("svg:stroke-width", 0.0);
    propList.insert("svg:stroke-color", "#000000");
  }
  else
  {
    std::map<unsigned, CDRLineStyle>::iterator iter = m_lineStyles.find(m_currentOutlId);
    if (iter == m_lineStyles.end() || !(iter->second.lineType))
    {
      propList.insert("draw:stroke", "solid");
      propList.insert("svg:stroke-width", 0.0);
      propList.insert("svg:stroke-color", "#000000");
    }
    else
    {
      propList.insert("draw:stroke", "solid");
      propList.insert("svg:stroke-width", iter->second.lineWidth);
      propList.insert("svg:stroke-color", _getRGBColorString(iter->second.colorModel, iter->second.color));
    }
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
