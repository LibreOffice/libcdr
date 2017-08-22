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

namespace
{

static void separateTabsAndInsertText(librevenge::RVNGDrawingInterface *iface, const librevenge::RVNGString &text)
{
  if (!iface || text.empty())
    return;
  librevenge::RVNGString tmpText;
  librevenge::RVNGString::Iter i(text);
  for (i.rewind(); i.next();)
  {
    if (*(i()) == '\t')
    {
      if (!tmpText.empty())
      {
        if (iface)
          iface->insertText(tmpText);
        tmpText.clear();
      }

      if (iface)
        iface->insertTab();
    }
    else if (*(i()) == '\n')
    {
      if (!tmpText.empty())
      {
        if (iface)
          iface->insertText(tmpText);
        tmpText.clear();
      }

      if (iface)
        iface->insertLineBreak();
    }
    else
    {
      tmpText.append(i());
    }
  }
  if (iface && !tmpText.empty())
    iface->insertText(tmpText);
}

static void separateSpacesAndInsertText(librevenge::RVNGDrawingInterface *iface, const librevenge::RVNGString &text)
{
  if (!iface)
    return;
  if (text.empty())
  {
    iface->insertText(text);
    return;
  }
  librevenge::RVNGString tmpText;
  int numConsecutiveSpaces = 0;
  librevenge::RVNGString::Iter i(text);
  for (i.rewind(); i.next();)
  {
    if (*(i()) == ' ')
      numConsecutiveSpaces++;
    else
      numConsecutiveSpaces = 0;

    if (numConsecutiveSpaces > 1)
    {
      if (!tmpText.empty())
      {
        separateTabsAndInsertText(iface, tmpText);
        tmpText.clear();
      }

      if (iface)
        iface->insertSpace();
    }
    else
    {
      tmpText.append(i());
    }
  }
  separateTabsAndInsertText(iface, tmpText);
}

} // anonymous namespace

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
  ~CDRStyleOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDRPathOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDRGraphicObjectOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDRStartTextObjectOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDROpenParagraphOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDROpenSpanOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDRInsertTextOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDRCloseSpanOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
  {
    return new CDRCloseSpanOutputElement();
  }
};


class CDRCloseParagraphOutputElement : public CDROutputElement
{
public:
  CDRCloseParagraphOutputElement();
  ~CDRCloseParagraphOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
  {
    return new CDRCloseParagraphOutputElement();
  }
};


class CDREndTextObjectOutputElement : public CDROutputElement
{
public:
  CDREndTextObjectOutputElement();
  ~CDREndTextObjectOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
  {
    return new CDREndTextObjectOutputElement();
  }
};

class CDRStartLayerOutputElement : public CDROutputElement
{
public:
  CDRStartLayerOutputElement(const librevenge::RVNGPropertyList &propList);
  ~CDRStartLayerOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
  ~CDREndLayerOutputElement() override {}
  void draw(librevenge::RVNGDrawingInterface *painter) override;
  CDROutputElement *clone() override
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
    separateSpacesAndInsertText(painter, m_text);
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
  for (auto &element : m_elements)
    delete element;

  m_elements.clear();

  for (auto element : elementList.m_elements)
    m_elements.push_back(element->clone());

  return *this;
}

libcdr::CDROutputElementList::~CDROutputElementList()
{
  for (auto &element : m_elements)
    delete element;
  m_elements.clear();
}

void libcdr::CDROutputElementList::draw(librevenge::RVNGDrawingInterface *painter) const
{
  for (auto element : m_elements)
    element->draw(painter);
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
