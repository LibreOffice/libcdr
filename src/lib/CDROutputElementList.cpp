/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDROutputElementList.h"

namespace libcdr
{

class CDROutputElement
{
public:
  CDROutputElement() {}
  virtual ~CDROutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter) = 0;
  virtual CDROutputElement *clone() = 0;
};


class CDRStyleOutputElement : public CDROutputElement
{
public:
  CDRStyleOutputElement(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec);
  virtual ~CDRStyleOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStyleOutputElement(m_propList, m_propListVec);
  }
private:
  WPXPropertyList m_propList;
  WPXPropertyListVector m_propListVec;
};


class CDRPathOutputElement : public CDROutputElement
{
public:
  CDRPathOutputElement(const WPXPropertyListVector &propListVec);
  virtual ~CDRPathOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRPathOutputElement(m_propListVec);
  }
private:
  WPXPropertyListVector m_propListVec;
};


class CDRGraphicObjectOutputElement : public CDROutputElement
{
public:
  CDRGraphicObjectOutputElement(const WPXPropertyList &propList, const ::WPXBinaryData &binaryData);
  virtual ~CDRGraphicObjectOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRGraphicObjectOutputElement(m_propList, m_binaryData);
  }
private:
  WPXPropertyList m_propList;
  WPXBinaryData m_binaryData;
};


class CDRStartTextObjectOutputElement : public CDROutputElement
{
public:
  CDRStartTextObjectOutputElement(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec);
  virtual ~CDRStartTextObjectOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStartTextObjectOutputElement(m_propList, m_propListVec);
  }
private:
  WPXPropertyList m_propList;
  WPXPropertyListVector m_propListVec;
};


class CDRStartTextLineOutputElement : public CDROutputElement
{
public:
  CDRStartTextLineOutputElement(const WPXPropertyList &propList);
  virtual ~CDRStartTextLineOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStartTextLineOutputElement(m_propList);
  }
private:
  WPXPropertyList m_propList;
};


class CDRStartTextSpanOutputElement : public CDROutputElement
{
public:
  CDRStartTextSpanOutputElement(const WPXPropertyList &propList);
  virtual ~CDRStartTextSpanOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStartTextSpanOutputElement(m_propList);
  }
private:
  WPXPropertyList m_propList;
};


class CDRInsertTextOutputElement : public CDROutputElement
{
public:
  CDRInsertTextOutputElement(const WPXString &text);
  virtual ~CDRInsertTextOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRInsertTextOutputElement(m_text);
  }
private:
  WPXString m_text;
};


class CDREndTextSpanOutputElement : public CDROutputElement
{
public:
  CDREndTextSpanOutputElement();
  virtual ~CDREndTextSpanOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDREndTextSpanOutputElement();
  }
};


class CDREndTextLineOutputElement : public CDROutputElement
{
public:
  CDREndTextLineOutputElement();
  virtual ~CDREndTextLineOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDREndTextLineOutputElement();
  }
};


class CDREndTextObjectOutputElement : public CDROutputElement
{
public:
  CDREndTextObjectOutputElement();
  virtual ~CDREndTextObjectOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDREndTextObjectOutputElement();
  }
};

class CDRStartLayerOutputElement : public CDROutputElement
{
public:
  CDRStartLayerOutputElement(const WPXPropertyList &propList);
  virtual ~CDRStartLayerOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDRStartLayerOutputElement(m_propList);
  }
private:
  WPXPropertyList m_propList;
};

class CDREndLayerOutputElement : public CDROutputElement
{
public:
  CDREndLayerOutputElement();
  virtual ~CDREndLayerOutputElement() {}
  virtual void draw(libwpg::WPGPaintInterface *painter);
  virtual CDROutputElement *clone()
  {
    return new CDREndLayerOutputElement();
  }
};

} // namespace libcdr

libcdr::CDRStyleOutputElement::CDRStyleOutputElement(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec) :
  m_propList(propList), m_propListVec(propListVec) {}

void libcdr::CDRStyleOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->setStyle(m_propList, m_propListVec);
}


libcdr::CDRPathOutputElement::CDRPathOutputElement(const WPXPropertyListVector &propListVec) :
  m_propListVec(propListVec) {}

void libcdr::CDRPathOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->drawPath(m_propListVec);
}


libcdr::CDRGraphicObjectOutputElement::CDRGraphicObjectOutputElement(const WPXPropertyList &propList, const ::WPXBinaryData &binaryData) :
  m_propList(propList), m_binaryData(binaryData) {}

void libcdr::CDRGraphicObjectOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->drawGraphicObject(m_propList, m_binaryData);
}


libcdr::CDRStartTextObjectOutputElement::CDRStartTextObjectOutputElement(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec) :
  m_propList(propList), m_propListVec(propListVec) {}

void libcdr::CDRStartTextObjectOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->startTextObject(m_propList, m_propListVec);
}

libcdr::CDRStartTextSpanOutputElement::CDRStartTextSpanOutputElement(const WPXPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRStartTextSpanOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->startTextSpan(m_propList);
}


libcdr::CDRStartTextLineOutputElement::CDRStartTextLineOutputElement(const WPXPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRStartTextLineOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->startTextLine(m_propList);
}


libcdr::CDRInsertTextOutputElement::CDRInsertTextOutputElement(const WPXString &text) :
  m_text(text) {}

void libcdr::CDRInsertTextOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->insertText(m_text);
}

libcdr::CDREndTextSpanOutputElement::CDREndTextSpanOutputElement() {}

void libcdr::CDREndTextSpanOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->endTextSpan();
}


libcdr::CDREndTextLineOutputElement::CDREndTextLineOutputElement() {}

void libcdr::CDREndTextLineOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->endTextLine();
}


libcdr::CDREndTextObjectOutputElement::CDREndTextObjectOutputElement() {}

void libcdr::CDREndTextObjectOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->endTextObject();
}


libcdr::CDREndLayerOutputElement::CDREndLayerOutputElement() {}

void libcdr::CDREndLayerOutputElement::draw(libwpg::WPGPaintInterface *painter)
{
  if (painter)
    painter->endLayer();
}


libcdr::CDRStartLayerOutputElement::CDRStartLayerOutputElement(const WPXPropertyList &propList) :
  m_propList(propList) {}

void libcdr::CDRStartLayerOutputElement::draw(libwpg::WPGPaintInterface *painter)
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
    delete(*iter);

  m_elements.clear();

  for (std::vector<CDROutputElement *>::const_iterator cstiter = elementList.m_elements.begin(); cstiter != elementList.m_elements.end(); ++cstiter)
    m_elements.push_back((*cstiter)->clone());

  return *this;
}

void libcdr::CDROutputElementList::append(const libcdr::CDROutputElementList &elementList)
{
  for (std::vector<CDROutputElement *>::const_iterator cstiter = elementList.m_elements.begin(); cstiter != elementList.m_elements.end(); ++cstiter)
    m_elements.push_back((*cstiter)->clone());
}

libcdr::CDROutputElementList::~CDROutputElementList()
{
  for (std::vector<CDROutputElement *>::iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    delete(*iter);
  m_elements.clear();
}

void libcdr::CDROutputElementList::draw(libwpg::WPGPaintInterface *painter) const
{
  for (std::vector<CDROutputElement *>::const_iterator iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    (*iter)->draw(painter);
}

void libcdr::CDROutputElementList::addStyle(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec)
{
  m_elements.push_back(new CDRStyleOutputElement(propList, propListVec));
}

void libcdr::CDROutputElementList::addPath(const WPXPropertyListVector &propListVec)
{
  m_elements.push_back(new CDRPathOutputElement(propListVec));
}

void libcdr::CDROutputElementList::addGraphicObject(const WPXPropertyList &propList, const ::WPXBinaryData &binaryData)
{
  m_elements.push_back(new CDRGraphicObjectOutputElement(propList, binaryData));
}

void libcdr::CDROutputElementList::addStartTextObject(const WPXPropertyList &propList, const WPXPropertyListVector &propListVec)
{
  m_elements.push_back(new CDRStartTextObjectOutputElement(propList, propListVec));
}

void libcdr::CDROutputElementList::addStartTextLine(const WPXPropertyList &propList)
{
  m_elements.push_back(new CDRStartTextLineOutputElement(propList));
}

void libcdr::CDROutputElementList::addStartTextSpan(const WPXPropertyList &propList)
{
  m_elements.push_back(new CDRStartTextSpanOutputElement(propList));
}

void libcdr::CDROutputElementList::addInsertText(const WPXString &text)
{
  m_elements.push_back(new CDRInsertTextOutputElement(text));
}

void libcdr::CDROutputElementList::addEndTextSpan()
{
  m_elements.push_back(new CDREndTextSpanOutputElement());
}

void libcdr::CDROutputElementList::addEndTextLine()
{
  m_elements.push_back(new CDREndTextLineOutputElement());
}

void libcdr::CDROutputElementList::addEndTextObject()
{
  m_elements.push_back(new CDREndTextObjectOutputElement());
}

void libcdr::CDROutputElementList::addStartGroup(const WPXPropertyList &propList)
{
  m_elements.push_back(new CDRStartLayerOutputElement(propList));
}

void libcdr::CDROutputElementList::addEndGroup()
{
  m_elements.push_back(new CDREndLayerOutputElement());
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
