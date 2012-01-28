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

#ifndef __CDRCOLLECTOR_H__
#define __CDRCOLLECTOR_H__

#include <libwpg/libwpg.h>

namespace libcdr
{

class CDRCollector
{
public:
  CDRCollector(::libwpg::WPGPaintInterface *painter);
  virtual ~CDRCollector();

  // collector functions
  void collectPage();
  void collectObject();
  void collectOtherList();
  void collectBbox(double x0, double y0, double x1, double y1);
  void collectCubicBezier(double x1, double y1, double x2, double y2, double x, double y);
  void collectMoveTo(double x, double y);
  void collectLineTo(double x, double y);
  void collectClosePath();
  void collectFlushPath();

private:
  CDRCollector(const CDRCollector &);
  CDRCollector &operator=(const CDRCollector &);

  // helper functions
  void _startPage(double width, double height);
  void _endPage();

  libwpg::WPGPaintInterface *m_painter;

  bool m_isPageProperties;
  bool m_isPageStarted;

  double m_pageWidth, m_pageHeight;

  WPXPropertyListVector m_currentPath;
};

} // namespace libcdr

#endif /* __CDRCOLLECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
