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

#include <memory>
#include <vector>

#include <librevenge/librevenge.h>

namespace libcdr
{

class CDROutputElement;

class CDROutputElementList
{
public:
  CDROutputElementList();
  ~CDROutputElementList();
  void draw(librevenge::RVNGDrawingInterface *painter) const;
  void addStyle(const librevenge::RVNGPropertyList &propList);
  void addPath(const librevenge::RVNGPropertyList &propList);
  void addGraphicObject(const librevenge::RVNGPropertyList &propList);
  void addStartTextObject(const librevenge::RVNGPropertyList &propList);
  void addOpenParagraph(const librevenge::RVNGPropertyList &propList);
  void addOpenSpan(const librevenge::RVNGPropertyList &propList);
  void addInsertText(const librevenge::RVNGString &text);
  void addCloseSpan();
  void addCloseParagraph();
  void addEndTextObject();
  void addStartGroup(const librevenge::RVNGPropertyList &propList);
  void addEndGroup();
  bool empty() const
  {
    return m_elements.empty();
  }
private:
  std::vector<std::shared_ptr<CDROutputElement>> m_elements;
};


} // namespace libcdr

#endif // __CDROUTPUTELEMENTLIST_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
