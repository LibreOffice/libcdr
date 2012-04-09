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

#ifndef __CDROUTPUTELEMENTLIST_H__
#define __CDROUTPUTELEMENTLIST_H__

#include <map>
#include <list>
#include <vector>
#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>

namespace libcdr
{

class CDROutputElement;

class CDROutputElementList
{
public:
  CDROutputElementList();
  CDROutputElementList(const CDROutputElementList &elementList);
  CDROutputElementList &operator=(const CDROutputElementList &elementList);
  virtual ~CDROutputElementList();
  void append(const CDROutputElementList &elementList);
  void draw(libwpg::WPGPaintInterface *painter) const;
  void addStyle(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec);
  void addPath(const WPXPropertyListVector &propListVec);
  void addGraphicObject(const WPXPropertyList &propList, const ::WPXBinaryData &binaryData);
  void addStartTextObject(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec);
  void addStartTextLine(const WPXPropertyList &propList);
  void addStartTextSpan(const WPXPropertyList &propList);
  void addInsertText(const WPXString &text);
  void addEndTextSpan();
  void addEndTextLine();
  void addEndTextObject();
  void addStartGroup(const WPXPropertyList &propList);
  void addEndGroup();
  bool empty() const
  {
    return m_elements.empty();
  }
private:
  std::vector<CDROutputElement *> m_elements;
};


} // namespace libcdr

#endif // __CDROUTPUTELEMENTLIST_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
