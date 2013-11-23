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

#include "CDROutputElementList.h"

namespace libcdr
{

class CDROutputElement
{
public:
  CDROutputElement() {}
  virtual ~CDROutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter) = 0;
  virtual CDROutputElement *clone() = 0;
};


class CDRStyleOutputElement : public CDROutputElement
{
public:
  CDRStyleOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDRStyleOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStyleOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};


class CDRPathOutputElement : public CDROutputElement
{
public:
  CDRPathOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDRPathOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRPathOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};


class CDRGraphicObjectOutputElement : public CDROutputElement
{
public:
  CDRGraphicObjectOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDRGraphicObjectOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRGraphicObjectOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};


class CDRStartTextObjectOutputElement : public CDROutputElement
{
public:
  CDRStartTextObjectOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDRStartTextObjectOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStartTextObjectOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};


class CDROpenParagraphOutputElement : public CDROutputElement
{
public:
  CDROpenParagraphOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDROpenParagraphOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDROpenParagraphOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};


class CDROpenSpanOutputElement : public CDROutputElement
{
public:
  CDROpenSpanOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDROpenSpanOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDROpenSpanOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};


class CDRInsertTextOutputElement : public CDROutputElement
{
public:
  CDRInsertTextOutputElement(const librevenge::RVNGString &text);
  virtual ~CDRInsertTextOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRInsertTextOutputElement(m_text);
  }
private:
  librevenge::RVNGString m_text;
};


class CDRCloseSpanOutputElement : public CDROutputElement
{
public:
  CDRCloseSpanOutputElement();
  virtual ~CDRCloseSpanOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRCloseSpanOutputElement();
  }
};


class CDRCloseParagraphOutputElement : public CDROutputElement
{
public:
  CDRCloseParagraphOutputElement();
  virtual ~CDRCloseParagraphOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRCloseParagraphOutputElement();
  }
};


class CDREndTextObjectOutputElement : public CDROutputElement
{
public:
  CDREndTextObjectOutputElement();
  virtual ~CDREndTextObjectOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDREndTextObjectOutputElement();
  }
};

class CDRStartLayerOutputElement : public CDROutputElement
{
public:
  CDRStartLayerOutputElement(const librevenge::RVNGPropertyList &propList);
  virtual ~CDRStartLayerOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStartLayerOutputElement(m_propList);
  }
private:
  librevenge::RVNGPropertyList m_propList;
};

class CDREndLayerOutputElement : public CDROutputElement
{
public:
  CDREndLayerOutputElement();
  virtual ~CDREndLayerOutputElement() {}
  virtual void draw(librevenge::RVNGDrawingInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDREndLayerOutputElement();
  }
};

} // namespace libcdr

libcdr::CDRStyleOutputElement::CDRStyleOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRStyleOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->setStyle(m_propList);
}


libcdr::CDRPathOutputElement::CDRPathOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRPathOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->drawPath(m_propList);
}


libcdr::CDRGraphicObjectOutputElement::CDRGraphicObjectOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRGraphicObjectOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->drawGraphicObject(m_propList);
}


libcdr::CDRStartTextObjectOutputElement::CDRStartTextObjectOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRStartTextObjectOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->startTextObject(m_propList);
}

libcdr::CDROpenSpanOutputElement::CDROpenSpanOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDROpenSpanOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->openSpan(m_propList);
}


libcdr::CDROpenParagraphOutputElement::CDROpenParagraphOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDROpenParagraphOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->openParagraph(m_propList);
}


libcdr::CDRInsertTextOutputElement::CDRInsertTextOutputElement(const librevenge::RVNGString &text) :
  m_text(text) {}

void libcdr::CDRInsertTextOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->insertText(m_text);
}

libcdr::CDRCloseSpanOutputElement::CDRCloseSpanOutputElement() {}

void libcdr::CDRCloseSpanOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->closeSpan();
}


libcdr::CDRCloseParagraphOutputElement::CDRCloseParagraphOutputElement() {}

void libcdr::CDRCloseParagraphOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->closeParagraph();
}


libcdr::CDREndTextObjectOutputElement::CDREndTextObjectOutputElement() {}

void libcdr::CDREndTextObjectOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->endTextObject();
}


libcdr::CDREndLayerOutputElement::CDREndLayerOutputElement() {}

void libcdr::CDREndLayerOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->endLayer();
}


libcdr::CDRStartLayerOutputElement::CDRStartLayerOutputElement(const librevenge::RVNGPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRStartLayerOutputElement::draw(librevenge::RVNGDrawingInterface *painter)
{
  if (painter)
    painter->startLayer(m_propList);
}


libcdr::CDROutputElementList::CDROutputElementList()
  : m_elements()
{
}

libcdr::CDROutputElementList::CDROutputElementList(const libcdr::CDROutputElementList &elementList)
  : m_elements()
{
  std::vector<libcdr::CDROutputElement *>::const_iterator iter;
  for (iter = elementList.m_elements.begin(); iter != elementList.m_elements.end(); ++iter)
    m_elements.push_back((*iter)->clone());
}

libcdr::CDROutputElementList &libcdr::CDROutputElementList::operator=(const libcdr::CDROutputElementList &elementList)
{
  for (std::vector<CDROutputElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    delete (*iter);

  m_elements.clear();

  for (std::vector<CDROutputElement *>::const_iterator cstiter = elementList.m_elements.begin(); cstiter != elementList.m_elements.end(); ++cstiter)
    m_elements.push_back((*cstiter)->clone());

  return *this;
}

libcdr::CDROutputElementList::~CDROutputElementList()
{
  for (std::vector<CDROutputElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    delete (*iter);
  m_elements.clear();
}

void libcdr::CDROutputElementList::draw(librevenge::RVNGDrawingInterface *painter) const
{
  for (std::vector<CDROutputElement *>::const_iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->draw(painter);
}

void libcdr::CDROutputElementList::addStyle(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDRStyleOutputElement(propList));
}

void libcdr::CDROutputElementList::addPath(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDRPathOutputElement(propList));
}

void libcdr::CDROutputElementList::addGraphicObject(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDRGraphicObjectOutputElement(propList));
}

void libcdr::CDROutputElementList::addStartTextObject(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDRStartTextObjectOutputElement(propList));
}

void libcdr::CDROutputElementList::addOpenParagraph(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDROpenParagraphOutputElement(propList));
}

void libcdr::CDROutputElementList::addOpenSpan(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDROpenSpanOutputElement(propList));
}

void libcdr::CDROutputElementList::addInsertText(const librevenge::RVNGString &text)
{
  m_elements.push_back(new CDRInsertTextOutputElement(text));
}

void libcdr::CDROutputElementList::addCloseSpan()
{
  m_elements.push_back(new CDRCloseSpanOutputElement());
}

void libcdr::CDROutputElementList::addCloseParagraph()
{
  m_elements.push_back(new CDRCloseParagraphOutputElement());
}

void libcdr::CDROutputElementList::addEndTextObject()
{
  m_elements.push_back(new CDREndTextObjectOutputElement());
}

void libcdr::CDROutputElementList::addStartGroup(const librevenge::RVNGPropertyList &propList)
{
  m_elements.push_back(new CDRStartLayerOutputElement(propList));
}

void libcdr::CDROutputElementList::addEndGroup()
{
  m_elements.push_back(new CDREndLayerOutputElement());
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
