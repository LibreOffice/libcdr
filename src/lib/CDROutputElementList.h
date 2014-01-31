/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
