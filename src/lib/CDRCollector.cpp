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

libcdr::CDRCollector::CDRCollector(libwpg::WPGPaintInterface *painter) :
  m_painter(painter),
  m_isPageProperties(false),
  m_isPageStarted(false),
  m_pageWidth(0.0),
  m_pageHeight(0.0)
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
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
