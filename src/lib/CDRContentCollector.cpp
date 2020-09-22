/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDRContentCollector.h"

#include <math.h>
#include <string.h>
#include <librevenge/librevenge.h>
#include <libcdr/libcdr.h>
#include "CDROutputElementList.h"
#include "libcdr_utils.h"

#ifndef DUMP_PATTERN
#define DUMP_PATTERN 0
#endif

#ifndef DUMP_VECT
#define DUMP_VECT 0
#endif

namespace libcdr
{
namespace
{

/// Move the number into [0;1] range.
void normalize(double &d)
{
  if (d < 0.0)
  {
    const double n = d + static_cast<unsigned long>(-d) + 1.0;
    if ((n < 0.0) || (n > 1.0))
      d = 0.0; // The number was too big, thus rounded. Just pick a value.
    else
      d = n;
  }
  if (d > 1.0)
  {
    const double n = d - static_cast<unsigned long>(d);
    if ((n < 0.0) || (n > 1.0))
      d = 0.0; // The number was too big, thus rounded. Just pick a value.
    else
      d = n;
  }
}

void normalizeAngle(double &angle)
{
  angle = fmod(angle, 360);
  if (angle < 0)
    angle += 360;
}

}
}

libcdr::CDRContentCollector::CDRContentCollector(libcdr::CDRParserState &ps, librevenge::RVNGDrawingInterface *painter,
                                                 bool reverseOrder)
  : m_painter(painter), m_isDocumentStarted(false), m_isPageProperties(false), m_isPageStarted(false),
    m_ignorePage(false), m_page(ps.m_pages[0]), m_pageIndex(0), m_currentFillStyle(), m_currentLineStyle(),
    m_spnd(0), m_currentObjectLevel(0), m_currentGroupLevel(0), m_currentVectLevel(0), m_currentPageLevel(0),
    m_currentStyleId(0), m_currentImage(), m_currentText(nullptr), m_currentBBox(), m_currentTextBox(), m_currentPath(),
    m_currentTransforms(), m_fillTransforms(), m_polygon(), m_isInPolygon(false), m_isInSpline(false),
    m_outputElementsStack(nullptr), m_contentOutputElementsStack(), m_fillOutputElementsStack(),
    m_outputElementsQueue(nullptr), m_contentOutputElementsQueue(), m_fillOutputElementsQueue(),
    m_groupLevels(), m_groupTransforms(), m_splineData(), m_fillOpacity(1.0), m_reverseOrder(reverseOrder),
    m_ps(ps)
{
  m_outputElementsStack = &m_contentOutputElementsStack;
  m_outputElementsQueue = &m_contentOutputElementsQueue;
}

libcdr::CDRContentCollector::~CDRContentCollector()
{
  if (m_isPageStarted)
    _endPage();
  if (m_isDocumentStarted)
    _endDocument();
}

void libcdr::CDRContentCollector::_startDocument()
{
  if (m_isDocumentStarted)
    return;
  librevenge::RVNGPropertyList propList;
  if (m_painter)
    m_painter->startDocument(propList);
  m_isDocumentStarted = true;
}

void libcdr::CDRContentCollector::_endDocument()
{
  if (!m_isDocumentStarted)
    return;
  if (m_isPageStarted)
    _endPage();
  if (m_painter)
    m_painter->endDocument();
  m_isDocumentStarted = false;
}

void libcdr::CDRContentCollector::_startPage(double width, double height)
{
  if (m_ignorePage)
    return;
  if (!m_isDocumentStarted)
    _startDocument();
  librevenge::RVNGPropertyList propList;
  propList.insert("svg:width", width);
  propList.insert("svg:height", height);
  if (m_painter)
    m_painter->startPage(propList);
  m_isPageStarted = true;
}

void libcdr::CDRContentCollector::_endPage()
{
  if (!m_isPageStarted)
    return;
  while (!m_contentOutputElementsStack.empty())
  {
    m_contentOutputElementsStack.top().draw(m_painter);
    m_contentOutputElementsStack.pop();
  }
  while (!m_contentOutputElementsQueue.empty())
  {
    m_contentOutputElementsQueue.front().draw(m_painter);
    m_contentOutputElementsQueue.pop();
  }
  if (m_painter)
    m_painter->endPage();
  m_isPageStarted = false;
}

void libcdr::CDRContentCollector::collectPage(unsigned level)
{
  m_isPageProperties = true;
  m_ignorePage = false;
  m_currentPageLevel = level;
  m_page = m_ps.m_pages[m_pageIndex++];

}

void libcdr::CDRContentCollector::collectObject(unsigned level)
{
  if (!m_isPageStarted && !m_currentVectLevel && !m_ignorePage)
    _startPage(m_page.width, m_page.height);
  m_currentObjectLevel = level;
  m_currentFillStyle = CDRFillStyle();
  m_currentLineStyle = CDRLineStyle();
  m_currentStyleId = 0;
  m_currentBBox = CDRBox();
}

void libcdr::CDRContentCollector::collectGroup(unsigned level)
{
  if (!m_isPageStarted && !m_currentVectLevel && !m_ignorePage)
    _startPage(m_page.width, m_page.height);
  CDROutputElementList outputElement;
  if (m_reverseOrder)
  {
    // Since the CDR objects are drawn in reverse order, reverse the logic of groups too
    outputElement.addEndGroup();
    m_outputElementsStack->push(outputElement);
  }
  else
  {
    librevenge::RVNGPropertyList propList;
    outputElement.addStartGroup(propList);
    m_outputElementsQueue->push(outputElement);
  }
  m_groupLevels.push(level);
  m_groupTransforms.push(CDRTransforms());
}

void libcdr::CDRContentCollector::collectVect(unsigned level)
{
  m_currentVectLevel = level;
  m_outputElementsStack = &m_fillOutputElementsStack;
  m_outputElementsQueue = &m_fillOutputElementsQueue;
  m_page.width = 0.0;
  m_page.height = 0.0;
  m_page.offsetX = 0.0;
  m_page.offsetY = 0.0;
}

void libcdr::CDRContentCollector::collectFlags(unsigned flags, bool considerFlags)
{
  if (m_isPageProperties && !(flags & 0x00ff0000))
  {
    if (!m_isPageStarted)
      _startPage(m_page.width, m_page.height);
  }
  else if (m_isPageProperties && considerFlags)
    m_ignorePage = true;
  m_isPageProperties = false;
}

void libcdr::CDRContentCollector::collectOtherList()
{
//  m_isPageProperties = false;
}

void libcdr::CDRContentCollector::collectPath(const CDRPath &path)
{
  CDR_DEBUG_MSG(("CDRContentCollector::collectPath\n"));
  m_currentPath.appendPath(path);
}

void libcdr::CDRContentCollector::_flushCurrentPath()
{
  CDR_DEBUG_MSG(("CDRContentCollector::_flushCurrentPath\n"));
  CDROutputElementList outputElement;
  if (!m_currentPath.empty() || (!m_splineData.empty() && m_isInSpline))
  {
    if (m_polygon && m_isInPolygon)
      m_polygon->create(m_currentPath);
    m_polygon.reset();
    m_isInPolygon = false;
    if (!m_splineData.empty() && m_isInSpline)
      m_splineData.create(m_currentPath);
    m_splineData.clear();
    m_isInSpline = false;
    bool firstPoint = true;
    bool wasMove = false;
    double initialX = 0.0;
    double initialY = 0.0;
    double previousX = 0.0;
    double previousY = 0.0;
    double x = 0.0;
    double y = 0.0;
    librevenge::RVNGPropertyList style;
    _fillProperties(style);
    _lineProperties(style);
    outputElement.addStyle(style);
    m_currentPath.transform(m_currentTransforms);
    if (!m_groupTransforms.empty())
      m_currentPath.transform(m_groupTransforms.top());
    CDRTransform tmpTrafo(1.0, 0.0, -m_page.offsetX, 0.0, 1.0, -m_page.offsetY);
    m_currentPath.transform(tmpTrafo);
    tmpTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, m_page.height);
    m_currentPath.transform(tmpTrafo);

    std::vector<librevenge::RVNGPropertyList> tmpPath;

    librevenge::RVNGPropertyListVector path;
    m_currentPath.writeOut(path);

    bool isPathClosed = m_currentPath.isClosed();

    librevenge::RVNGPropertyListVector::Iter i(path);
    for (i.rewind(); i.next();)
    {
      if (!i()["librevenge:path-action"])
        continue;
      if (i()["svg:x"] && i()["svg:y"])
      {
        bool ignoreM = false;
        x = i()["svg:x"]->getDouble();
        y = i()["svg:y"]->getDouble();
        if (firstPoint)
        {
          initialX = x;
          initialY = y;
          firstPoint = false;
          wasMove = true;
        }
        else if (i()["librevenge:path-action"]->getStr() == "M")
        {
          // This is needed for a good generation of path from polygon
          if (CDR_ALMOST_ZERO(previousX - x) && CDR_ALMOST_ZERO(previousY - y))
            ignoreM = true;
          else
          {
            if (!tmpPath.empty())
            {
              if (!wasMove)
              {
                if ((CDR_ALMOST_ZERO(initialX - previousX) && CDR_ALMOST_ZERO(initialY - previousY)) || isPathClosed)
                {
                  librevenge::RVNGPropertyList node;
                  node.insert("librevenge:path-action", "Z");
                  tmpPath.push_back(node);
                }
              }
              else
                tmpPath.pop_back();
            }
          }

          if (!ignoreM)
          {
            initialX = x;
            initialY = y;
            wasMove = true;
          }

        }
        else
          wasMove = false;

        if (!ignoreM)
        {
          tmpPath.push_back(i());
          previousX = x;
          previousY = y;
        }

      }
      else if (i()["librevenge:path-action"]->getStr() == "Z")
      {
        if (tmpPath.back()["librevenge:path-action"] && tmpPath.back()["librevenge:path-action"]->getStr() != "Z")
          tmpPath.push_back(i());
      }
    }
    if (!tmpPath.empty())
    {
      if (!wasMove)
      {
        if ((CDR_ALMOST_ZERO(initialX - previousX) && CDR_ALMOST_ZERO(initialY - previousY)) || isPathClosed)
        {
          if (tmpPath.back()["librevenge:path-action"] && tmpPath.back()["librevenge:path-action"]->getStr() != "Z")
          {
            librevenge::RVNGPropertyList closedPath;
            closedPath.insert("librevenge:path-action", "Z");
            tmpPath.push_back(closedPath);
          }
        }
      }
      else
        tmpPath.pop_back();
    }
    if (!tmpPath.empty())
    {
      librevenge::RVNGPropertyListVector outputPath;
      for (std::vector<librevenge::RVNGPropertyList>::const_iterator iter = tmpPath.begin(); iter != tmpPath.end(); ++iter)
        outputPath.append(*iter);
      librevenge::RVNGPropertyList propList;
      propList.insert("svg:d", outputPath);
      outputElement.addPath(propList);

    }
    m_currentPath.clear();
  }

  if (m_currentImage.getImage().size())
  {
    double cx = m_currentImage.getMiddleX();
    double cy = m_currentImage.getMiddleY();
    double corner1x = m_currentImage.m_x1;
    double corner1y = m_currentImage.m_y1;
    double corner2x = m_currentImage.m_x1;
    double corner2y = m_currentImage.m_y2;
    double corner3x = m_currentImage.m_x2;
    double corner3y = m_currentImage.m_y2;
    m_currentTransforms.applyToPoint(cx, cy);
    m_currentTransforms.applyToPoint(corner1x, corner1y);
    m_currentTransforms.applyToPoint(corner2x, corner2y);
    m_currentTransforms.applyToPoint(corner3x, corner3y);
    if (!m_groupTransforms.empty())
    {
      m_groupTransforms.top().applyToPoint(cx, cy);
      m_groupTransforms.top().applyToPoint(corner1x, corner1y);
      m_groupTransforms.top().applyToPoint(corner2x, corner2y);
      m_groupTransforms.top().applyToPoint(corner3x, corner3y);
    }
    CDRTransform tmpTrafo(1.0, 0.0, -m_page.offsetX, 0.0, 1.0, -m_page.offsetY);
    tmpTrafo.applyToPoint(cx, cy);
    tmpTrafo.applyToPoint(corner1x, corner1y);
    tmpTrafo.applyToPoint(corner2x, corner2y);
    tmpTrafo.applyToPoint(corner3x, corner3y);
    tmpTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, m_page.height);
    tmpTrafo.applyToPoint(cx, cy);
    tmpTrafo.applyToPoint(corner1x, corner1y);
    tmpTrafo.applyToPoint(corner2x, corner2y);
    tmpTrafo.applyToPoint(corner3x, corner3y);
    bool flipX(m_currentTransforms.getFlipX());
    bool flipY(m_currentTransforms.getFlipY());
    double width = sqrt((corner2x - corner3x)*(corner2x - corner3x) + (corner2y - corner3y)*(corner2y - corner3y));
    double height = sqrt((corner2x - corner1x)*(corner2x - corner1x) + (corner2y - corner1y)*(corner2y - corner1y));
    double rotate = atan2(corner3y-corner2y, corner3x-corner2x);

    librevenge::RVNGPropertyList propList;

    propList.insert("svg:x", cx - width / 2.0);
    propList.insert("svg:width", width);
    propList.insert("svg:y", cy - height / 2.0);
    propList.insert("svg:height", height);

    if (flipX)
    {
      propList.insert("draw:mirror-horizontal", true);
      rotate = M_PI - rotate;
    }
    if (flipY)
    {
      propList.insert("draw:mirror-vertical", true);
      rotate *= -1.0;
    }

    while (rotate < 0.0)
      rotate += 2.0*M_PI;
    while (rotate > 2.0*M_PI)
      rotate -= 2.0*M_PI;

    if (!CDR_ALMOST_ZERO(rotate))
      propList.insert("librevenge:rotate", rotate * 180 / M_PI, librevenge::RVNG_GENERIC);

    propList.insert("librevenge:mime-type", "image/bmp");
    propList.insert("office:binary-data", m_currentImage.getImage());
    outputElement.addGraphicObject(propList);
  }
  if (m_currentText && !m_currentText->empty())
  {
    double x1 = m_currentTextBox.m_x;
    double y1 = m_currentTextBox.m_y;
    double x2 = m_currentTextBox.m_x + m_currentTextBox.m_w;
    double y2 = m_currentTextBox.m_y - m_currentTextBox.m_h;
    if (!CDR_ALMOST_ZERO(m_currentTextBox.m_h) && !CDR_ALMOST_ZERO(m_currentTextBox.m_w))
    {
      m_currentTransforms.applyToPoint(x1, y1);
      m_currentTransforms.applyToPoint(x2, y2);
      if (!m_groupTransforms.empty())
      {
        m_groupTransforms.top().applyToPoint(x1, y1);
        m_groupTransforms.top().applyToPoint(x2, y2);
      }
    }
    else if (!CDR_ALMOST_ZERO(m_currentBBox.getWidth()) && !CDR_ALMOST_ZERO(m_currentBBox.getHeight()))
    {
      y1 = m_currentBBox.getMinY();
      y2 = m_currentBBox.getMinY() + m_currentBBox.getHeight();
      if ((*m_currentText)[0].m_line[0].m_style.m_align == 2) // Center
      {
        x1 = m_currentBBox.getMinX() - m_currentBBox.getWidth() / 4.0;
        x2 = m_currentBBox.getMinX() + (3.0 * m_currentBBox.getWidth() / 4.0);
      }
      else if ((*m_currentText)[0].m_line[0].m_style.m_align == 3) // Right
      {
        x1 = m_currentBBox.getMinX() - m_currentBBox.getWidth() / 2.0;
        x2 = m_currentBBox.getMinX() + m_currentBBox.getWidth() / 2.0;
      }
      else
      {
        x1 = m_currentBBox.getMinX();
        x2 = m_currentBBox.getMinX() + m_currentBBox.getWidth();
      }
    }
    else if (!m_currentTransforms.empty())
    {
      x1 = m_currentTransforms.getTranslateX();
      y1 = m_currentTransforms.getTranslateY();
      x2 = x1;
      y2 = y1;
    }

    CDRTransform tmpTrafo(1.0, 0.0, -m_page.offsetX, 0.0, 1.0, -m_page.offsetY);
    tmpTrafo.applyToPoint(x1, y1);
    tmpTrafo.applyToPoint(x2, y2);
    tmpTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, m_page.height);
    tmpTrafo.applyToPoint(x1, y1);
    tmpTrafo.applyToPoint(x2, y2);
    if (x1 > x2)
      std::swap(x1, x2);
    if (y1 > y2)
      std::swap(y1, y2);

    librevenge::RVNGPropertyList textFrameProps;
    textFrameProps.insert("svg:width", fabs(x2-x1));
    textFrameProps.insert("svg:height", fabs(y2-y1));
    textFrameProps.insert("svg:x", x1);
    textFrameProps.insert("svg:y", y1);
    textFrameProps.insert("fo:padding-top", 0.0);
    textFrameProps.insert("fo:padding-bottom", 0.0);
    textFrameProps.insert("fo:padding-left", 0.0);
    textFrameProps.insert("fo:padding-right", 0.0);
    outputElement.addStartTextObject(textFrameProps);
    for (const auto &i : *m_currentText)
    {
      const std::vector<CDRText> &currentLine = i.m_line;
      if (currentLine.empty())
        continue;
      librevenge::RVNGPropertyList paraProps;
      bool rtl = false;
      switch (currentLine[0].m_style.m_align)
      {
      case 1:  // Left
        if (!rtl)
          paraProps.insert("fo:text-align", "left");
        else
          paraProps.insert("fo:text-align", "end");
        break;
      case 2:  // Center
        paraProps.insert("fo:text-align", "center");
        break;
      case 3:  // Right
        if (!rtl)
          paraProps.insert("fo:text-align", "end");
        else
          paraProps.insert("fo:text-align", "left");
        break;
      case 4:  // Full justify
        paraProps.insert("fo:text-align", "justify");
        break;
      case 5:  // Force justify
        paraProps.insert("fo:text-align", "full");
        break;
      case 0:  // None
      default:
        break;
      }
      outputElement.addOpenParagraph(paraProps);
      for (const auto &j : currentLine)
      {
        if (!j.m_text.empty())
        {
          librevenge::RVNGPropertyList spanProps;
          double fontSize = (double)cdr_round(144.0*j.m_style.m_fontSize) / 2.0;
          spanProps.insert("fo:font-size", fontSize, librevenge::RVNG_POINT);
          if (j.m_style.m_fontName.len())
            spanProps.insert("style:font-name", j.m_style.m_fontName);
          if (j.m_style.m_fillStyle.fillType != (unsigned short)-1)
            spanProps.insert("fo:color", m_ps.getRGBColorString(j.m_style.m_fillStyle.color1));
          outputElement.addOpenSpan(spanProps);
          outputElement.addInsertText(j.m_text);
          outputElement.addCloseSpan();
        }
      }
      outputElement.addCloseParagraph();
    }
    outputElement.addEndTextObject();
  }
  m_currentImage = libcdr::CDRImage();
  if (!outputElement.empty())
  {
    if (m_reverseOrder)
      m_outputElementsStack->push(outputElement);
    else
      m_outputElementsQueue->push(outputElement);
  }
  m_currentTransforms.clear();
  m_fillTransforms = libcdr::CDRTransforms();
  m_fillOpacity = 1.0;
  m_currentText = nullptr;
}

void libcdr::CDRContentCollector::collectTransform(const CDRTransforms &transforms, bool considerGroupTransform)
{
  if (m_currentObjectLevel)
    m_currentTransforms = transforms;
  else if (!m_groupLevels.empty() && considerGroupTransform)
    m_groupTransforms.top() = transforms;
}

void libcdr::CDRContentCollector::collectFillTransform(const CDRTransforms &fillTrafos)
{
  m_fillTransforms = fillTrafos;
}

void libcdr::CDRContentCollector::collectLevel(unsigned level)
{
  if (level <= m_currentObjectLevel)
  {
    _flushCurrentPath();
    m_currentObjectLevel = 0;
  }
  while (!m_groupLevels.empty() && level <= m_groupLevels.top())
  {
    CDROutputElementList outputElement;
    // since the CDR objects are drawn in reverse order, reverse group marks too
    if (m_reverseOrder)
    {
      librevenge::RVNGPropertyList propList;
      outputElement.addStartGroup(propList);
      m_outputElementsStack->push(outputElement);
    }
    else
    {
      outputElement.addEndGroup();
      m_outputElementsQueue->push(outputElement);
    }
    m_groupLevels.pop();
    m_groupTransforms.pop();
  }
  if (m_currentVectLevel && m_spnd && m_groupLevels.empty() && (!m_fillOutputElementsStack.empty() || !m_fillOutputElementsQueue.empty()))
  {
    librevenge::RVNGStringVector svgOutput;
    librevenge::RVNGSVGDrawingGenerator generator(svgOutput, "");
    librevenge::RVNGPropertyList propList;
    propList.insert("svg:width", m_page.width);
    propList.insert("svg:height", m_page.height);
    generator.startPage(propList);
    while (!m_fillOutputElementsStack.empty())
    {
      m_fillOutputElementsStack.top().draw(&generator);
      m_fillOutputElementsStack.pop();
    }
    while (!m_fillOutputElementsQueue.empty())
    {
      m_fillOutputElementsQueue.front().draw(&generator);
      m_fillOutputElementsQueue.pop();
    }

    generator.endPage();
    if (!svgOutput.empty())
    {
      const char *header =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
      librevenge::RVNGBinaryData output((const unsigned char *)header, strlen(header));
      output.append((const unsigned char *)svgOutput[0].cstr(), strlen(svgOutput[0].cstr()));
      m_ps.m_vects[m_spnd] = output;
    }
#if DUMP_VECT
    librevenge::RVNGString filename;
    filename.sprintf("vect%.8x.svg", m_spnd);
    FILE *f = fopen(filename.cstr(), "wb");
    if (f)
    {
      const unsigned char *tmpBuffer = m_ps.m_vects[m_spnd].getDataBuffer();
      for (unsigned long k = 0; k < m_ps.m_vects[m_spnd].size(); k++)
        fprintf(f, "%c",tmpBuffer[k]);
      fclose(f);
    }
#endif
    m_spnd = 0;
    m_page.width = 0.0;
    m_page.height = 0.0;
    m_page.offsetX = 0.0;
    m_page.offsetY = 0.0;
  }
  if (level <= m_currentVectLevel)
  {
    m_currentVectLevel = 0;
    m_outputElementsStack = &m_contentOutputElementsStack;
    m_outputElementsQueue = &m_contentOutputElementsQueue;
    m_page = m_ps.m_pages[m_pageIndex ? m_pageIndex-1 : 0];
  }
  if (level <= m_currentPageLevel)
  {
    _endPage();
    m_currentPageLevel = 0;
  }
}

void libcdr::CDRContentCollector::collectFillStyleId(unsigned id)
{
  std::map<unsigned, CDRFillStyle>::const_iterator iter = m_ps.m_fillStyles.find(id);
  if (iter != m_ps.m_fillStyles.end())
    m_currentFillStyle = iter->second;
}

void libcdr::CDRContentCollector::collectLineStyleId(unsigned id)
{
  std::map<unsigned, CDRLineStyle>::const_iterator iter = m_ps.m_lineStyles.find(id);
  if (iter != m_ps.m_lineStyles.end())
    m_currentLineStyle = iter->second;
}

void libcdr::CDRContentCollector::collectRotate(double angle, double cx, double cy)
{
  CDRTransform trafo1(1.0, 0.0, -cx, 0.0, 1.0, -cy);
  m_currentPath.transform(trafo1);
  CDRTransform trafo2(cos(angle), -sin(angle), 0, sin(angle), cos(angle), 0);
  m_currentPath.transform(trafo2);
  CDRTransform trafo3(1.0, 0.0, cx, 0.0, 1.0, cy);
  m_currentPath.transform(trafo3);
}

void libcdr::CDRContentCollector::collectPolygon()
{
  m_isInPolygon = true;
}

void libcdr::CDRContentCollector::collectSpline()
{
  m_isInSpline = true;
}

void libcdr::CDRContentCollector::collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy)
{
  m_polygon.reset(new CDRPolygon(numAngles, nextPoint, rx, ry, cx, cy));
}

void libcdr::CDRContentCollector::_fillProperties(librevenge::RVNGPropertyList &propList)
{
  if (m_currentFillStyle.fillType == (unsigned short)-1 && m_currentStyleId)
  {
    CDRStyle tmpStyle;
    m_ps.getRecursedStyle(tmpStyle, m_currentStyleId);
    m_currentFillStyle = tmpStyle.m_fillStyle;
  }

  if (m_fillOpacity < 1.0)
    propList.insert("draw:opacity", m_fillOpacity, librevenge::RVNG_PERCENT);
  if (m_currentFillStyle.fillType == 0)
    propList.insert("draw:fill", "none");
  else
  {
    if (m_currentFillStyle.fillType == (unsigned short)-1)
      propList.insert("draw:fill", "none");
    else
    {
      switch (m_currentFillStyle.fillType)
      {
      case 1: // Solid
        propList.insert("draw:fill", "solid");
        propList.insert("draw:fill-color", m_ps.getRGBColorString(m_currentFillStyle.color1));
        propList.insert("svg:fill-rule", "evenodd");
        break;
      case 2: // Gradient
        if (m_currentFillStyle.gradient.m_stops.empty())
          propList.insert("draw:fill", "none");
        else if (m_currentFillStyle.gradient.m_stops.size() == 1)
        {
          propList.insert("draw:fill", "solid");
          propList.insert("draw:fill-color", m_ps.getRGBColorString(m_currentFillStyle.gradient.m_stops[0].m_color));
          propList.insert("svg:fill-rule", "evenodd");
        }
        else if (m_currentFillStyle.gradient.m_stops.size() == 2)
        {
          double angle = m_currentFillStyle.gradient.m_angle * 180 / M_PI;
          normalizeAngle(angle);
          propList.insert("draw:fill", "gradient");
          propList.insert("draw:start-color", m_ps.getRGBColorString(m_currentFillStyle.gradient.m_stops[0].m_color));
          propList.insert("draw:end-color", m_ps.getRGBColorString(m_currentFillStyle.gradient.m_stops[1].m_color));
          propList.insert("draw:angle", (int)angle);
          switch (m_currentFillStyle.gradient.m_type)
          {
          case 1: // linear
          case 3: // conical
            propList.insert("draw:style", "linear");
            angle += 90.0;
            normalizeAngle(angle);
            propList.insert("draw:angle", (int)angle);
            propList.insert("draw:border", (double)(m_currentFillStyle.gradient.m_edgeOffset)/100.0, librevenge::RVNG_PERCENT);
            break;
          case 2: // radial
            propList.insert("draw:border", (2.0 * (double)(m_currentFillStyle.gradient.m_edgeOffset)/100.0), librevenge::RVNG_PERCENT);
            propList.insert("draw:style", "radial");
            propList.insert("svg:cx", (double)(0.5 + m_currentFillStyle.gradient.m_centerXOffset/200.0), librevenge::RVNG_PERCENT);
            propList.insert("svg:cy", (double)(0.5 + m_currentFillStyle.gradient.m_centerXOffset/200.0), librevenge::RVNG_PERCENT);
            break;
          case 4: // square
            propList.insert("draw:border", (2.0 * (double)(m_currentFillStyle.gradient.m_edgeOffset)/100.0), librevenge::RVNG_PERCENT);
            propList.insert("draw:style", "square");
            propList.insert("svg:cx", (double)(0.5 + m_currentFillStyle.gradient.m_centerXOffset/200.0), librevenge::RVNG_PERCENT);
            propList.insert("svg:cy", (double)(0.5 + m_currentFillStyle.gradient.m_centerXOffset/200.0), librevenge::RVNG_PERCENT);
            break;
          default:
            propList.insert("draw:style", "linear");
            angle += 90.0;
            normalizeAngle(angle);
            propList.insert("draw:angle", (int)angle);
            librevenge::RVNGPropertyListVector vec;
            for (auto &gradStop : m_currentFillStyle.gradient.m_stops)
            {
              librevenge::RVNGPropertyList stopElement;
              stopElement.insert("svg:offset", gradStop.m_offset, librevenge::RVNG_PERCENT);
              stopElement.insert("svg:stop-color", m_ps.getRGBColorString(gradStop.m_color));
              stopElement.insert("svg:stop-opacity", m_fillOpacity, librevenge::RVNG_PERCENT);
              vec.append(stopElement);
            }
            propList.insert("svg:linearGradient", vec);
            break;
          }
        }
        else // output svg gradient as a hail mary pass towards ODG that does not really support it
        {
          propList.insert("draw:fill", "gradient");
          propList.insert("draw:style", "linear");
          double angle = m_currentFillStyle.gradient.m_angle * 180 / M_PI;
          angle += 90.0;
          normalizeAngle(angle);
          propList.insert("draw:angle", (int)angle);
          librevenge::RVNGPropertyListVector vec;
          for (auto &gradStop : m_currentFillStyle.gradient.m_stops)
          {
            librevenge::RVNGPropertyList stopElement;
            stopElement.insert("svg:offset", gradStop.m_offset, librevenge::RVNG_PERCENT);
            stopElement.insert("svg:stop-color", m_ps.getRGBColorString(gradStop.m_color));
            stopElement.insert("svg:stop-opacity", m_fillOpacity, librevenge::RVNG_PERCENT);
            vec.append(stopElement);
          }
          propList.insert("svg:linearGradient", vec);
        }
        break;
      case 7: // Pattern
      case 8: // Pattern
      {
        auto iterPattern = m_ps.m_patterns.find(m_currentFillStyle.imageFill.id);
        if (iterPattern != m_ps.m_patterns.end())
        {
          propList.insert("draw:fill", "bitmap");
          librevenge::RVNGBinaryData image;
          _generateBitmapFromPattern(image, iterPattern->second, m_currentFillStyle.color1, m_currentFillStyle.color2);
#if DUMP_PATTERN
          librevenge::RVNGString filename;
          filename.sprintf("pattern%.8x.bmp", m_currentFillStyle.imageFill.id);
          FILE *f = fopen(filename.cstr(), "wb");
          if (f)
          {
            const unsigned char *tmpBuffer = image.getDataBuffer();
            for (unsigned long k = 0; k < image.size(); k++)
              fprintf(f, "%c",tmpBuffer[k]);
            fclose(f);
          }
#endif
          propList.insert("draw:fill-image", image);
          propList.insert("librevenge:mime-type", "image/bmp");
          propList.insert("style:repeat", "repeat");
          if (m_currentFillStyle.imageFill.isRelative)
          {
            propList.insert("svg:width", m_currentFillStyle.imageFill.width, librevenge::RVNG_PERCENT);
            propList.insert("svg:height", m_currentFillStyle.imageFill.height, librevenge::RVNG_PERCENT);
          }
          else
          {
            double scaleX = 1.0;
            double scaleY = 1.0;
            if (m_currentFillStyle.imageFill.flags & 0x04) // scale fill with image
            {
              scaleX = m_currentTransforms.getScaleX();
              scaleY = m_currentTransforms.getScaleY();
            }
            propList.insert("svg:width", m_currentFillStyle.imageFill.width * scaleX);
            propList.insert("svg:height", m_currentFillStyle.imageFill.height * scaleY);
          }
          propList.insert("draw:fill-image-ref-point", "bottom-left");
          if (m_currentFillStyle.imageFill.isRelative)
          {
            if (!CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.xOffset) && !CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.xOffset))
              propList.insert("draw:fill-image-ref-point-x", m_currentFillStyle.imageFill.xOffset, librevenge::RVNG_PERCENT);
            if (!CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.yOffset) && !CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.yOffset))
              propList.insert("draw:fill-image-ref-point-y", m_currentFillStyle.imageFill.yOffset, librevenge::RVNG_PERCENT);
          }
          else
          {
            if (!CDR_ALMOST_ZERO(m_fillTransforms.getTranslateX()))
            {
              double xOffset = m_fillTransforms.getTranslateX() / m_currentFillStyle.imageFill.width;
              normalize(xOffset);
              propList.insert("draw:fill-image-ref-point-x", xOffset, librevenge::RVNG_PERCENT);
            }
            if (!CDR_ALMOST_ZERO(m_fillTransforms.getTranslateY()))
            {
              double yOffset = m_fillTransforms.getTranslateY() / m_currentFillStyle.imageFill.width;
              normalize(yOffset);
              propList.insert("draw:fill-image-ref-point-y", 1.0 - yOffset, librevenge::RVNG_PERCENT);
            }
          }
        }
        else
        {
          // We did not find the pattern, so fill solid with the background colour
          propList.insert("draw:fill", "solid");
          propList.insert("draw:fill-color", m_ps.getRGBColorString(m_currentFillStyle.color2));
          propList.insert("svg:fill-rule", "evenodd");
        }
      }
      break;
      case 9: // Bitmap
      case 11: // Texture
      {
        auto iterBmp = m_ps.m_bmps.find(m_currentFillStyle.imageFill.id);
        if (iterBmp != m_ps.m_bmps.end())
        {
          propList.insert("librevenge:mime-type", "image/bmp");
          propList.insert("draw:fill", "bitmap");
          propList.insert("draw:fill-image", iterBmp->second);
          propList.insert("style:repeat", "repeat");
          if (m_currentFillStyle.imageFill.isRelative)
          {
            propList.insert("svg:width", m_currentFillStyle.imageFill.width, librevenge::RVNG_PERCENT);
            propList.insert("svg:height", m_currentFillStyle.imageFill.height, librevenge::RVNG_PERCENT);
          }
          else
          {
            double scaleX = 1.0;
            double scaleY = 1.0;
            if (m_currentFillStyle.imageFill.flags & 0x04) // scale fill with image
            {
              scaleX = m_currentTransforms.getScaleX();
              scaleY = m_currentTransforms.getScaleY();
            }
            propList.insert("svg:width", m_currentFillStyle.imageFill.width * scaleX);
            propList.insert("svg:height", m_currentFillStyle.imageFill.height * scaleY);
          }
          propList.insert("draw:fill-image-ref-point", "bottom-left");
          if (m_currentFillStyle.imageFill.isRelative)
          {
            if (!CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.xOffset) && !CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.xOffset))
              propList.insert("draw:fill-image-ref-point-x", m_currentFillStyle.imageFill.xOffset, librevenge::RVNG_PERCENT);
            if (!CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.yOffset) && !CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.yOffset))
              propList.insert("draw:fill-image-ref-point-y", m_currentFillStyle.imageFill.yOffset, librevenge::RVNG_PERCENT);
          }
          else
          {
            if (!CDR_ALMOST_ZERO(m_fillTransforms.getTranslateX()))
            {
              double xOffset = m_fillTransforms.getTranslateX() / m_currentFillStyle.imageFill.width;
              while (xOffset < 0.0)
                xOffset += 1.0;
              while (xOffset > 1.0)
                xOffset -= 1.0;
              propList.insert("draw:fill-image-ref-point-x", xOffset, librevenge::RVNG_PERCENT);
            }
            if (!CDR_ALMOST_ZERO(m_fillTransforms.getTranslateY()))
            {
              double yOffset = m_fillTransforms.getTranslateY() / m_currentFillStyle.imageFill.width;
              while (yOffset < 0.0)
                yOffset += 1.0;
              while (yOffset > 1.0)
                yOffset -= 1.0;
              propList.insert("draw:fill-image-ref-point-y", 1.0 - yOffset, librevenge::RVNG_PERCENT);
            }
          }
        }
        else
          propList.insert("draw:fill", "none");
      }
      break;
      case 10: // Full color
      {
        auto iterVect = m_ps.m_vects.find(m_currentFillStyle.imageFill.id);
        if (iterVect != m_ps.m_vects.end())
        {
          propList.insert("draw:fill", "bitmap");
          propList.insert("librevenge:mime-type", "image/svg+xml");
          propList.insert("draw:fill-image", iterVect->second);
          propList.insert("style:repeat", "repeat");
          if (m_currentFillStyle.imageFill.isRelative)
          {
            propList.insert("svg:width", m_currentFillStyle.imageFill.width, librevenge::RVNG_PERCENT);
            propList.insert("svg:height", m_currentFillStyle.imageFill.height, librevenge::RVNG_PERCENT);
          }
          else
          {
            double scaleX = 1.0;
            double scaleY = 1.0;
            if (m_currentFillStyle.imageFill.flags & 0x04) // scale fill with image
            {
              scaleX = m_currentTransforms.getScaleX();
              scaleY = m_currentTransforms.getScaleY();
            }
            propList.insert("svg:width", m_currentFillStyle.imageFill.width * scaleX);
            propList.insert("svg:height", m_currentFillStyle.imageFill.height * scaleY);
          }
          propList.insert("draw:fill-image-ref-point", "bottom-left");
          if (m_currentFillStyle.imageFill.isRelative)
          {
            if (!CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.xOffset) && !CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.xOffset))
              propList.insert("draw:fill-image-ref-point-x", m_currentFillStyle.imageFill.xOffset, librevenge::RVNG_PERCENT);
            if (!CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.yOffset) && !CDR_ALMOST_ZERO(m_currentFillStyle.imageFill.yOffset))
              propList.insert("draw:fill-image-ref-point-y", m_currentFillStyle.imageFill.yOffset, librevenge::RVNG_PERCENT);
          }
          else
          {
            if (!CDR_ALMOST_ZERO(m_fillTransforms.getTranslateX()))
            {
              double xOffset = m_fillTransforms.getTranslateX() / m_currentFillStyle.imageFill.width;
              while (xOffset < 0.0)
                xOffset += 1.0;
              while (xOffset > 1.0)
                xOffset -= 1.0;
              propList.insert("draw:fill-image-ref-point-x", xOffset, librevenge::RVNG_PERCENT);
            }
            if (!CDR_ALMOST_ZERO(m_fillTransforms.getTranslateY()))
            {
              double yOffset = m_fillTransforms.getTranslateY() / m_currentFillStyle.imageFill.width;
              while (yOffset < 0.0)
                yOffset += 1.0;
              while (yOffset > 1.0)
                yOffset -= 1.0;
              propList.insert("draw:fill-image-ref-point-y", 1.0 - yOffset, librevenge::RVNG_PERCENT);
            }
          }
        }
        else
          propList.insert("draw:fill", "none");
      }
      break;
      default:
        propList.insert("draw:fill", "none");
        break;
      }
    }
  }
}

void libcdr::CDRContentCollector::_lineProperties(librevenge::RVNGPropertyList &propList)
{
  if (m_currentLineStyle.lineType == (unsigned short)-1 && m_currentStyleId)
  {
    CDRStyle tmpStyle;
    m_ps.getRecursedStyle(tmpStyle, m_currentStyleId);
    m_currentLineStyle = tmpStyle.m_lineStyle;
  }

  if (m_currentLineStyle.lineType == (unsigned short)-1)
    /* No line style specified and also no line style from the style id,
       the shape has no outline then. */
    propList.insert("draw:stroke", "none");
  else
  {
    if (m_currentLineStyle.lineType & 0x1)
      propList.insert("draw:stroke", "none");
    else if (m_currentLineStyle.lineType & 0x6)
    {
      if (m_currentLineStyle.dashArray.size() && (m_currentLineStyle.lineType & 0x4))
        propList.insert("draw:stroke", "dash");
      else
        propList.insert("draw:stroke", "solid");
      double scale = 1.0;
      if (m_currentLineStyle.lineType & 0x20) // scale line with image
      {
        scale = m_currentTransforms.getScaleX();
        double scaleY = m_currentTransforms.getScaleY();
        if (scaleY > scale)
          scale = scaleY;
      }
      scale *= m_currentLineStyle.stretch;
      propList.insert("svg:stroke-width", m_currentLineStyle.lineWidth * scale);
      propList.insert("svg:stroke-color", m_ps.getRGBColorString(m_currentLineStyle.color));

      switch (m_currentLineStyle.capsType)
      {
      case 1:
        propList.insert("svg:stroke-linecap", "round");
        break;
      case 2:
        propList.insert("svg:stroke-linecap", "square");
        break;
      default:
        propList.insert("svg:stroke-linecap", "butt");
      }

      switch (m_currentLineStyle.joinType)
      {
      case 1:
        propList.insert("svg:stroke-linejoin", "round");
        break;
      case 2:
        propList.insert("svg:stroke-linejoin", "bevel");
        break;
      default:
        propList.insert("svg:stroke-linejoin", "miter");
      }

      if (m_currentLineStyle.dashArray.size())
      {
        int dots1 = 0;
        int dots2 = 0;
        unsigned dots1len = 0;
        unsigned dots2len = 0;
        unsigned gap = 0;

        if (m_currentLineStyle.dashArray.size() >= 2)
        {
          dots1len = m_currentLineStyle.dashArray[0];
          gap = m_currentLineStyle.dashArray[1];
        }

        unsigned long count = m_currentLineStyle.dashArray.size() / 2;
        unsigned i = 0;
        for (; i < count;)
        {
          if (dots1len == m_currentLineStyle.dashArray[2*i])
            dots1++;
          else
            break;
          gap = gap < m_currentLineStyle.dashArray[2*i+1] ?  m_currentLineStyle.dashArray[2*i+1] : gap;
          i++;
        }
        if (i < count)
        {
          dots2len = m_currentLineStyle.dashArray[2*i];
          gap = gap < m_currentLineStyle.dashArray[2*i+1] ? m_currentLineStyle.dashArray[2*i+1] : gap;
        }
        for (; i < count;)
        {
          if (dots2len == m_currentLineStyle.dashArray[2*i])
            dots2++;
          else
            break;
          gap = gap < m_currentLineStyle.dashArray[2*i+1] ? m_currentLineStyle.dashArray[2*i+1] : gap;
          i++;
        }
        if (!dots2)
        {
          dots2 = dots1;
          dots2len = dots1len;
        }
        propList.insert("draw:dots1", dots1);
        propList.insert("draw:dots1-length", 72.0*(m_currentLineStyle.lineWidth * scale)*dots1len, librevenge::RVNG_POINT);
        propList.insert("draw:dots2", dots2);
        propList.insert("draw:dots2-length", 72.0*(m_currentLineStyle.lineWidth * scale)*dots2len, librevenge::RVNG_POINT);
        propList.insert("draw:distance", 72.0*(m_currentLineStyle.lineWidth * scale)*gap, librevenge::RVNG_POINT);
      }
    }
    else
    {
      propList.insert("draw:stroke", "solid");
      propList.insert("svg:stroke-width", 0.0);
      propList.insert("svg:stroke-color", "#000000");
    }
  }

  // Deal with line markers (arrows, etc.)
  if (!m_currentLineStyle.startMarker.empty())
  {
    CDRPath startMarker(m_currentLineStyle.startMarker);
    startMarker.transform(m_currentTransforms);
    if (!m_groupTransforms.empty())
      startMarker.transform(m_groupTransforms.top());
    CDRTransform tmpTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, 0);
    startMarker.transform(tmpTrafo);
    librevenge::RVNGString path, viewBox;
    double width;
    startMarker.writeOut(path, viewBox, width);
    propList.insert("draw:marker-start-viewbox", viewBox);
    propList.insert("draw:marker-start-path", path);
    // propList.insert("draw:marker-start-width", width);
  }
  if (!m_currentLineStyle.endMarker.empty())
  {
    CDRPath endMarker(m_currentLineStyle.endMarker);
    endMarker.transform(m_currentTransforms);
    if (!m_groupTransforms.empty())
      endMarker.transform(m_groupTransforms.top());
    CDRTransform tmpTrafo = CDRTransform(-1.0, 0.0, 0.0, 0.0, -1.0, 0);
    endMarker.transform(tmpTrafo);
    librevenge::RVNGString path, viewBox;
    double width;
    endMarker.writeOut(path, viewBox, width);
    propList.insert("draw:marker-end-viewbox", viewBox);
    propList.insert("draw:marker-end-path", path);
    // propList.insert("draw:marker-end-width", width);
  }



}

void libcdr::CDRContentCollector::_generateBitmapFromPattern(librevenge::RVNGBinaryData &bitmap, const CDRPattern &pattern, const CDRColor &fgColor, const CDRColor &bgColor)
{
  unsigned height = pattern.height;
  unsigned width = pattern.width;
  auto tmpPixelSize = (unsigned)(height * width);
  if (tmpPixelSize < (unsigned)height) // overflow
    return;

  unsigned tmpDIBImageSize = tmpPixelSize * 4;
  if (tmpPixelSize > tmpDIBImageSize) // overflow !!!
    return;

  unsigned tmpDIBOffsetBits = 14 + 40;
  unsigned tmpDIBFileSize = tmpDIBOffsetBits + tmpDIBImageSize;
  if (tmpDIBImageSize > tmpDIBFileSize) // overflow !!!
    return;

  // Create DIB file header
  writeU16(bitmap, 0x4D42);  // Type
  writeU32(bitmap, (int)tmpDIBFileSize); // Size
  writeU16(bitmap, 0); // Reserved1
  writeU16(bitmap, 0); // Reserved2
  writeU32(bitmap, (int)tmpDIBOffsetBits); // OffsetBits

  // Create DIB Info header
  writeU32(bitmap, 40); // Size

  writeU32(bitmap, (int)width);  // Width
  writeU32(bitmap, (int)height); // Height

  writeU16(bitmap, 1); // Planes
  writeU16(bitmap, 32); // BitCount
  writeU32(bitmap, 0); // Compression
  writeU32(bitmap, (int)tmpDIBImageSize); // SizeImage
  writeU32(bitmap, 0); // XPelsPerMeter
  writeU32(bitmap, 0); // YPelsPerMeter
  writeU32(bitmap, 0); // ColorsUsed
  writeU32(bitmap, 0); // ColorsImportant

  // The Bitmaps in CDR are padded to 32bit border
  unsigned lineWidth = (width + 7) / 8;

  unsigned foreground = m_ps._getRGBColor(fgColor);
  unsigned background = m_ps._getRGBColor(bgColor);

  for (unsigned j = height; j > 0; --j)
  {
    unsigned i = 0;
    unsigned k = 0;
    while (i <lineWidth && k < width)
    {
      unsigned l = 0;
      unsigned char c = 0;
      const unsigned index = (j-1)*lineWidth+i;
      if (index < pattern.pattern.size())
        c = pattern.pattern[index];
      i++;
      while (k < width && l < 8)
      {
        if (c & 0x80)
          writeU32(bitmap, (int)background);
        else
          writeU32(bitmap, (int)foreground);
        c <<= 1;
        l++;
        k++;
      }
    }
  }
}

void libcdr::CDRContentCollector::collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2)
{
  auto iter = m_ps.m_bmps.find(imageId);
  if (iter != m_ps.m_bmps.end())
    m_currentImage = CDRImage(iter->second, x1, x2, y1, y2);
}

void libcdr::CDRContentCollector::collectPpdt(const std::vector<std::pair<double, double> > &points, const std::vector<unsigned> &knotVector)
{
  m_splineData = CDRSplineData(points, knotVector);
}

void libcdr::CDRContentCollector::collectFillOpacity(double opacity)
{
  m_fillOpacity = opacity;
}

void libcdr::CDRContentCollector::collectBBox(double x0, double y0, double x1, double y1)
{
  CDRBox bBox(x0, y0, x1, y1);
  if (m_currentVectLevel && CDR_ALMOST_ZERO(m_page.width) && CDR_ALMOST_ZERO(m_page.height))
  {
    m_page.width = bBox.getWidth();
    m_page.height = bBox.getHeight();
    m_page.offsetX = bBox.getMinX();
    m_page.offsetY = bBox.getMinY();
  }
  m_currentBBox = bBox;
}

void libcdr::CDRContentCollector::collectSpnd(unsigned spnd)
{
  if (m_currentVectLevel && !m_spnd)
    m_spnd = spnd;
  else if (!m_currentVectLevel)
    m_spnd = spnd;
}

void libcdr::CDRContentCollector::collectVectorPattern(unsigned id, const librevenge::RVNGBinaryData &data)
{
  librevenge::RVNGInputStream *input = data.getDataStream();
  if (!input)
    return;

  input->seek(0, librevenge::RVNG_SEEK_SET);
  if (!libcdr::CMXDocument::isSupported(input))
    return;
  input->seek(0, librevenge::RVNG_SEEK_SET);
  librevenge::RVNGStringVector svgOutput;
  librevenge::RVNGSVGDrawingGenerator generator(svgOutput, "");
  if (!libcdr::CMXDocument::parse(input, &generator))
    return;
  if (!svgOutput.empty())
  {
    const char *header =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    librevenge::RVNGBinaryData output((const unsigned char *)header, strlen(header));
    output.append((const unsigned char *)svgOutput[0].cstr(), strlen(svgOutput[0].cstr()));
    m_ps.m_vects[id] = output;
  }
#if DUMP_VECT
  librevenge::RVNGString filename;
  filename.sprintf("vect%.8x.cmx", id);
  FILE *f = fopen(filename.cstr(), "wb");
  if (f)
  {
    const unsigned char *tmpBuffer = data.getDataBuffer();
    for (unsigned long k = 0; k < data.size(); k++)
      fprintf(f, "%c",tmpBuffer[k]);
    fclose(f);
  }
  filename.sprintf("vect%.8x.svg", id);
  f = fopen(filename.cstr(), "wb");
  if (f)
  {
    const unsigned char *tmpBuffer = m_ps.m_vects[id].getDataBuffer();
    for (unsigned long k = 0; k < m_ps.m_vects[id].size(); k++)
      fprintf(f, "%c",tmpBuffer[k]);
    fclose(f);
  }
#endif
}

void libcdr::CDRContentCollector::collectArtisticText(double x, double y)
{
  m_currentTextBox = CDRBox(x, y, x, y);
  m_currentBBox.m_w *= 2.0;
  std::map<unsigned, std::vector<CDRTextLine> >::const_iterator iter = m_ps.m_texts.find(m_spnd);
  if (iter != m_ps.m_texts.end())
    m_currentText = &(iter->second);
}

void libcdr::CDRContentCollector::collectParagraphText(double x, double y, double width, double height)
{
  m_currentTextBox.m_x = x;
  m_currentTextBox.m_y = y;
  m_currentTextBox.m_w = width;
  m_currentTextBox.m_h = height;
  std::map<unsigned, std::vector<CDRTextLine> >::const_iterator iter = m_ps.m_texts.find(m_spnd);
  if (iter != m_ps.m_texts.end())
    m_currentText = &(iter->second);
}

void libcdr::CDRContentCollector::collectStyleId(unsigned styleId)
{
  m_currentStyleId = styleId;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
