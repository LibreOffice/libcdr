/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDRContentCollector.h"

#include <cstdint>
#include <string>
#include <utility>

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
// librevenge with movable property lists (LIBREVENGE_PROPERTYLIST_MOVE): a path's nodes are moved
// along, not copied; with any other one they are copied as before
#ifdef LIBREVENGE_PROPERTYLIST_MOVE
#define CDR_PATH_NODE(vec, k) (vec).at(k)
#else
#define CDR_PATH_NODE(vec, k) (vec)[k]
#endif

namespace
{

// An arrowhead's outline as SVG path data in its own units, as its arrw record gives it (libcdr's
// own libcdr:marker-start / -end: draw:marker-*-path is transformed for ODF's drawing of markers)
librevenge::RVNGString markerPath(const libcdr::CDRPath &marker)
{
  librevenge::RVNGPropertyListVector vec;
  marker.writeOut(vec);
  librevenge::RVNGString out;
  for (unsigned long i = 0; i < vec.count(); ++i)
  {
    const librevenge::RVNGPropertyList &e = vec[i];
    if (!e["librevenge:path-action"])
      continue;
    librevenge::RVNGString action = e["librevenge:path-action"]->getStr();
    librevenge::RVNGString part;
    if (action == "C" && e["svg:x1"] && e["svg:y1"] && e["svg:x2"] && e["svg:y2"] && e["svg:x"] && e["svg:y"])
      part.sprintf("C%g %g %g %g %g %g", e["svg:x1"]->getDouble(), e["svg:y1"]->getDouble(), e["svg:x2"]->getDouble(),
                   e["svg:y2"]->getDouble(), e["svg:x"]->getDouble(), e["svg:y"]->getDouble());
    else if ((action == "M" || action == "L") && e["svg:x"] && e["svg:y"])
      part.sprintf("%s%g %g", action.cstr(), e["svg:x"]->getDouble(), e["svg:y"]->getDouble());
    else if (action == "Z")
      part = "Z";
    out.append(part);
  }
  return out;
}

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

// The bounding box a CMX file's header gives, in inches: its drawing's, the page of the SVG made
// from it. False if it is not a little-endian CMX of 16- or 32-bit coordinates.
bool cmxBoundingBox(const librevenge::RVNGBinaryData &data, libcdr::CDRBox &box)
{
  const unsigned char *p = data.getDataBuffer();
  if (!p || data.size() < 0x84 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "CMX1", 4) || memcmp(p + 12, "cont", 4) || p[0x44] != '2')
    return false;
  double c[4];
  for (int k = 0; k < 4; ++k)
  {
    if (p[0x48] == '4')
    {
      const unsigned char *q = p + 0x74 + 4 * k;
      c[k] = (int32_t)((uint32_t)q[0] | (uint32_t)q[1] << 8 | (uint32_t)q[2] << 16 | (uint32_t)q[3] << 24) / 254000.0;
    }
    else if (p[0x48] == '2')
    {
      const unsigned char *q = p + 0x74 + 2 * k;
      c[k] = (int16_t)((uint16_t)q[0] | (uint16_t)q[1] << 8) / 1000.0;
    }
    else
      return false;
  }
  box = libcdr::CDRBox(c[0], c[1], c[2], c[3]);
  return box.getWidth() > 0 && box.getHeight() > 0;
}

// A full colour pattern's SVG (its drawing, on the page `page`) cut to its tile: the root's viewBox
// (points, y down from the page's top) and size set to the tile's
librevenge::RVNGBinaryData tileSVG(const librevenge::RVNGBinaryData &svg, const libcdr::CDRBox &page, const libcdr::CDRBox &tile)
{
  std::string s((const char *)svg.getDataBuffer(), svg.size());
  const size_t start = s.find("<svg ");
  const size_t end = start == std::string::npos ? std::string::npos : s.find('>', start);
  if (end == std::string::npos)
    return svg;
  std::string tag = s.substr(start, end - start);
  for (const char *name : {" width=\"", " height=\"", " viewBox=\""})
  {
    const size_t i = tag.find(name);
    const size_t j = i == std::string::npos ? i : tag.find('"', i + strlen(name));
    if (j != std::string::npos)
      tag.erase(i, j + 1 - i);
  }
  librevenge::RVNGString attributes;
  attributes.sprintf(" width=\"%.4fin\" height=\"%.4fin\" viewBox=\"%.4f %.4f %.4f %.4f\"", tile.getWidth(), tile.getHeight(),
                     (tile.getMinX() - page.getMinX()) * 72.0, (page.getMinY() + page.getHeight() - tile.getMinY() - tile.getHeight()) * 72.0,
                     tile.getWidth() * 72.0, tile.getHeight() * 72.0);
  tag += attributes.cstr();
  s.replace(start, end - start, tag);
  return librevenge::RVNGBinaryData((const unsigned char *)s.data(), s.size());
}

}
}

libcdr::CDRContentCollector::CDRContentCollector(libcdr::CDRParserState &ps, librevenge::RVNGDrawingInterface *painter,
                                                 bool reverseOrder)
  : m_painter(painter), m_isDocumentStarted(false), m_isPageProperties(false), m_isPageStarted(false),
    m_ignorePage(false), m_page(ps.m_pages[0]), m_pageIndex(0), m_currentFillStyle(), m_currentLineStyle(),
    m_spnd(0), m_currentObjectLevel(0), m_currentGroupLevel(0), m_currentVectLevel(0), m_currentPageLevel(0),
    m_currentStyleId(0), m_currentImage(), m_currentImageMono(false), m_currentText(nullptr), m_currentTextObjectStyle(false), m_currentBBox(), m_currentTextBox(), m_currentPath(),
    m_currentTransforms(), m_fillTransforms(), m_polygon(), m_isInPolygon(false), m_isInSpline(false),
    m_outputElementsStack(nullptr), m_contentOutputElementsStack(), m_fillOutputElementsStack(),
    m_outputElementsQueue(nullptr), m_contentOutputElementsQueue(), m_fillOutputElementsQueue(),
    m_groupLevels(), m_groupTransforms(), m_writtenStartMarker(), m_writtenEndMarker(), m_splineData(), m_fillOpacity(1.0), m_reverseOrder(reverseOrder),
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

// The lengths of the current object's x and y axes on the output page: an artistic text
// stretched or squeezed in CorelDRAW keeps its font size and gets this scaling.
void libcdr::CDRContentCollector::_textScale(double &sx, double &sy)
{
  double ox = 0.0, oy = 0.0, ux = 1.0, uy = 0.0, vx = 0.0, vy = 1.0;
  m_currentTransforms.applyToPoint(ox, oy);
  m_currentTransforms.applyToPoint(ux, uy);
  m_currentTransforms.applyToPoint(vx, vy);
  if (!m_groupTransforms.empty())
  {
    m_groupTransforms.top().applyToPoint(ox, oy);
    m_groupTransforms.top().applyToPoint(ux, uy);
    m_groupTransforms.top().applyToPoint(vx, vy);
  }
  sx = hypot(ux - ox, uy - oy);
  sy = hypot(vx - ox, vy - oy);
  if (CDR_ALMOST_ZERO(sx) || CDR_ALMOST_ZERO(sy))
    sx = sy = 1.0;
}

// The rotation of the current object's x axis on the output page (y down), in degrees.
double libcdr::CDRContentCollector::_textRotation()
{
  double ox = 0.0, oy = 0.0, ux = 1.0, uy = 0.0;
  m_currentTransforms.applyToPoint(ox, oy);
  m_currentTransforms.applyToPoint(ux, uy);
  if (!m_groupTransforms.empty())
  {
    m_groupTransforms.top().applyToPoint(ox, oy);
    m_groupTransforms.top().applyToPoint(ux, uy);
  }
  if (CDR_ALMOST_ZERO(ux - ox) && CDR_ALMOST_ZERO(uy - oy))
    return 0.0;
  return atan2(-(uy - oy), ux - ox) * 180.0 / M_PI;   // page y is flipped below
}

void libcdr::CDRContentCollector::_flushCurrentPath()
{
  CDR_DEBUG_MSG(("CDRContentCollector::_flushCurrentPath\n"));
  CDROutputElementList outputElement;
  librevenge::RVNGPropertyListVector imageClip;   // the outline of a bitmap (see below)
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
    if (m_currentImage.getImage().size() && m_currentImageMono)
    {
      // a 1-bit bitmap's fill and outline are its colours (see _monoImage), not drawn
      style.insert("draw:fill", "none");
      style.insert("draw:stroke", "none");
    }
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

    for (unsigned long k = 0; k < path.count(); ++k)   // (the nodes are moved into tmpPath)
    {
      if (!CDR_PATH_NODE(path, k)["librevenge:path-action"])
        continue;
      if (CDR_PATH_NODE(path, k)["svg:x"] && CDR_PATH_NODE(path, k)["svg:y"])
      {
        bool ignoreM = false;
        x = CDR_PATH_NODE(path, k)["svg:x"]->getDouble();
        y = CDR_PATH_NODE(path, k)["svg:y"]->getDouble();
        if (firstPoint)
        {
          initialX = x;
          initialY = y;
          firstPoint = false;
          wasMove = true;
        }
        else if (CDR_PATH_NODE(path, k)["librevenge:path-action"]->getStr() == "M")
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
          tmpPath.push_back(std::move(CDR_PATH_NODE(path, k)));
          previousX = x;
          previousY = y;
        }

      }
      else if (CDR_PATH_NODE(path, k)["librevenge:path-action"]->getStr() == "Z")
      {
        if (!tmpPath.empty() && tmpPath.back()["librevenge:path-action"] && tmpPath.back()["librevenge:path-action"]->getStr() != "Z")
          tmpPath.push_back(std::move(CDR_PATH_NODE(path, k)));
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
    // CorelDRAW fills only closed paths: an open one (a polyline) keeps just its outline
    bool hasClosed = false;
    for (const auto &node : tmpPath)
      if (node["librevenge:path-action"] && node["librevenge:path-action"]->getStr() == "Z")
        hasClosed = true;
    if (!hasClosed)
      style.insert("draw:fill", "none");
    outputElement.addStyle(style);
    if (!tmpPath.empty())
    {
      // (moved along, not copied: big plans have millions of path nodes)
      librevenge::RVNGPropertyListVector outputPath;
      for (auto &node : tmpPath)
        outputPath.append(std::move(node));
      if (m_currentImage.getImage().size() && hasClosed)
        imageClip = outputPath;
      librevenge::RVNGPropertyList propList;
      propList.insert("svg:d", std::move(outputPath));
      outputElement.addPath(std::move(propList));
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

    // (libcdr's bitmaps are BMPs; embedded objects' images, their own format)
    const librevenge::RVNGBinaryData &imageData = m_currentImage.getImage();
    const unsigned char *head = imageData.getDataBuffer();
    const char *mimeType = "image/bmp";
    if (imageData.size() > 4 && !memcmp(head, "8BPS", 4))
      mimeType = "image/vnd.adobe.photoshop";
    else if (imageData.size() > 4 && !memcmp(head, "\x89PNG", 4))
      mimeType = "image/png";
    else if (imageData.size() > 3 && !memcmp(head, "\xff\xd8\xff", 3))
      mimeType = "image/jpeg";
    else if (imageData.size() > 4 && !memcmp(head, "GIF8", 4))
      mimeType = "image/gif";
    propList.insert("librevenge:mime-type", mimeType);
    const librevenge::RVNGBinaryData image = m_currentImageMono ? _monoImage(imageData) : imageData;
    propList.insert("office:binary-data", image);
    // the bitmap's own outline (a bitmap cropped with the Shape tool, or a pattern tile of an
    // imported PDF): it shows only inside it (libcdr's own: librevenge has no clip for images)
    if (imageClip.count())
      propList.insert("libcdr:clip-path", imageClip);
    if (image.size())   // (empty: a 1-bit bitmap with nothing to show)
      outputElement.addGraphicObject(propList);
  }
  if (m_currentText && !m_currentText->empty())
  {
    double x1 = m_currentTextBox.m_x;
    double y1 = m_currentTextBox.m_y;
    double x2 = m_currentTextBox.m_x + m_currentTextBox.m_w;
    double y2 = m_currentTextBox.m_y - m_currentTextBox.m_h;
    double rotate = 0.0;   // the text frame's rotation (degrees, as for images)
    double scaleX = 1.0, scaleY = 1.0;   // artistic text stretched by its transform
    bool mirrored = false;   // artistic text mirrored by its transform (its characters are drawn
                             // unmirrored, at their mirrored places: their shifts' y turns over)
    if (!CDR_ALMOST_ZERO(m_currentTextBox.m_h) && !CDR_ALMOST_ZERO(m_currentTextBox.m_w))
    {
      // three corners of the frame (top left, top right, bottom left): its centre, size and
      // rotation; only transforming two opposite corners lost the rotation
      double c[3][2] = {{x1, y1}, {x2, y1}, {x1, y2}};
      for (auto &p : c)
      {
        m_currentTransforms.applyToPoint(p[0], p[1]);
        if (!m_groupTransforms.empty())
          m_groupTransforms.top().applyToPoint(p[0], p[1]);
        CDRTransform pageTrafo(1.0, 0.0, -m_page.offsetX, 0.0, 1.0, -m_page.offsetY);
        pageTrafo.applyToPoint(p[0], p[1]);
        pageTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, m_page.height);
        pageTrafo.applyToPoint(p[0], p[1]);
      }
      double width = sqrt((c[1][0] - c[0][0]) * (c[1][0] - c[0][0]) + (c[1][1] - c[0][1]) * (c[1][1] - c[0][1]));
      double height = sqrt((c[2][0] - c[0][0]) * (c[2][0] - c[0][0]) + (c[2][1] - c[0][1]) * (c[2][1] - c[0][1]));
      double cx = (c[1][0] + c[2][0]) / 2.0, cy = (c[1][1] + c[2][1]) / 2.0;
      rotate = atan2(c[1][1] - c[0][1], c[1][0] - c[0][0]) * 180.0 / M_PI;
      // back into page coordinates before the page transforms below (y up)
      x1 = cx - width / 2.0 + m_page.offsetX;
      x2 = cx + width / 2.0 + m_page.offsetX;
      y1 = m_page.height - (cy - height / 2.0) + m_page.offsetY;
      y2 = m_page.height - (cy + height / 2.0) + m_page.offsetY;
    }
    else if (!m_currentTransforms.empty())
    {
      // artistic text (no frame): anchored at its transformed origin (the first line's baseline
      // at the alignment point), turned around it; the bounding box only approximated that
      rotate = _textRotation();
      _textScale(scaleX, scaleY);
      double o[2] = {0.0, 0.0}, u[2] = {1.0, 0.0}, v[2] = {0.0, 1.0};
      for (double *p : {o, u, v})
      {
        m_currentTransforms.applyToPoint(p[0], p[1]);
        if (!m_groupTransforms.empty())
          m_groupTransforms.top().applyToPoint(p[0], p[1]);
      }
      mirrored = (u[0] - o[0]) * (v[1] - o[1]) - (u[1] - o[1]) * (v[0] - o[0]) < 0;
      x1 = 0.0;
      y1 = 0.0;
      m_currentTransforms.applyToPoint(x1, y1);
      if (!m_groupTransforms.empty())
        m_groupTransforms.top().applyToPoint(x1, y1);
      x2 = x1;
      y2 = y1;
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
    if (!CDR_ALMOST_ZERO(rotate))
      textFrameProps.insert("librevenge:rotate", rotate, librevenge::RVNG_GENERIC);
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
      if (currentLine[0].m_style.m_lineSpacing > 0)
        paraProps.insert("fo:line-height", currentLine[0].m_style.m_lineSpacing, librevenge::RVNG_PERCENT);
      else if (currentLine[0].m_style.m_lineSpacing < 0)
        paraProps.insert("fo:line-height", -currentLine[0].m_style.m_lineSpacing, librevenge::RVNG_POINT);
      // (as the line height: a share of the character height, or points)
      if (currentLine[0].m_style.m_hasParagraphSpacing)
      {
        double before = currentLine[0].m_style.m_spaceBefore, after = currentLine[0].m_style.m_spaceAfter;
        paraProps.insert("fo:margin-top", fabs(before), before < 0 ? librevenge::RVNG_POINT : librevenge::RVNG_PERCENT);
        paraProps.insert("fo:margin-bottom", fabs(after), after < 0 ? librevenge::RVNG_POINT : librevenge::RVNG_PERCENT);
      }
      outputElement.addOpenParagraph(paraProps);
      // an empty paragraph: a span without text all the same, so that its line gets the height of
      // its font (else a default one)
      bool emptyParagraph = true;
      for (const auto &j : currentLine)
        if (!j.m_text.empty())
          emptyParagraph = false;
      for (const auto &j : currentLine)
      {
        if (!j.m_text.empty() || (emptyParagraph && &j == &currentLine[0]))
        {
          librevenge::RVNGPropertyList spanProps;
          // (to 1/100 pt: rounded to half points, the words a PDF import placed one by one
          // moved apart)
          double fontSize = (double)cdr_round(7200.0*j.m_style.m_fontSize*scaleY) / 100.0;
          if (!CDR_ALMOST_EQUAL(scaleY, 1.0) || !CDR_ALMOST_EQUAL(scaleX, 1.0))   // stretched artistic text
            spanProps.insert("style:text-scale", scaleX / scaleY, librevenge::RVNG_PERCENT);
          if (!CDR_ALMOST_ZERO(j.m_style.m_shiftX))   // (libcdr's own: no ODF property for this)
            spanProps.insert("libcdr:shift-x", j.m_style.m_shiftX, librevenge::RVNG_GENERIC);
          if (!CDR_ALMOST_ZERO(j.m_style.m_shiftY))
            spanProps.insert("libcdr:shift-y", mirrored ? -j.m_style.m_shiftY : j.m_style.m_shiftY, librevenge::RVNG_GENERIC);
          if (!CDR_ALMOST_ZERO(j.m_style.m_rotation))
            spanProps.insert("libcdr:rotate", j.m_style.m_rotation, librevenge::RVNG_GENERIC);
          spanProps.insert("fo:font-size", fontSize, librevenge::RVNG_POINT);
          if (j.m_style.m_fontName.len())
            spanProps.insert("style:font-name", j.m_style.m_fontName);
          if (j.m_style.m_fontWeight == 700)
            spanProps.insert("fo:font-weight", "bold");
          if (j.m_style.m_fontItalic == 2)
            spanProps.insert("fo:font-style", "italic");
          // (a CorelDRAW 4 text: its object's fill and outline, when it has them: a text with an
          // outline and no fill came out black)
          const CDRFillStyle &fill = m_currentTextObjectStyle && m_currentFillStyle.fillType != (unsigned short)-1 ? m_currentFillStyle : j.m_style.m_fillStyle;
          if (fill.fillType == 0)   // (no fill: libcdr's own, as for shapes)
            spanProps.insert("draw:fill", "none");
          else if (fill.fillType != (unsigned short)-1)
            spanProps.insert("fo:color", m_ps.getRGBColorString(fill.color1));
          // the character and word spacing: shares of a space's width (libcdr's own: ODF's
          // fo:letter-spacing is a length, and the font's space is not known here)
          if (!CDR_ALMOST_ZERO(j.m_style.m_charSpacing))
            spanProps.insert("libcdr:letter-spacing", j.m_style.m_charSpacing, librevenge::RVNG_GENERIC);
          if (!CDR_ALMOST_EQUAL(j.m_style.m_wordSpacing, 1.0))
            spanProps.insert("libcdr:word-spacing", j.m_style.m_wordSpacing, librevenge::RVNG_GENERIC);
          // the characters' outline (as a shape's; no ODF property for this): it was left out, so
          // white texts with a thin black outline melted into a light background
          const CDRLineStyle &outline = m_currentTextObjectStyle && m_currentLineStyle.lineType != (unsigned short)-1 ? m_currentLineStyle : j.m_style.m_lineStyle;
          if (outline.lineType != (unsigned short)-1 && !(outline.lineType & 0x1) && (outline.lineType & 0x6))
          {
            double scale = outline.lineType & 0x20 ? (scaleX > scaleY ? scaleX : scaleY) : 1.0;   // scale with image
            spanProps.insert("draw:stroke", "solid");
            spanProps.insert("svg:stroke-width", outline.lineWidth * outline.stretch * scale);
            spanProps.insert("svg:stroke-color", m_ps.getRGBColorString(outline.color));
          }
          outputElement.addOpenSpan(spanProps);
          if (!j.m_text.empty())
            outputElement.addInsertText(j.m_text);
          outputElement.addCloseSpan();
        }
      }
      outputElement.addCloseParagraph();
    }
    outputElement.addEndTextObject();
  }
  m_currentImage = libcdr::CDRImage();
  m_currentImageMono = false;
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
  m_currentTextObjectStyle = false;
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

// A 1-bit bitmap as CorelDRAW colours it: its black pixels in the object's outline colour (black
// without an outline), its white ones in the fill colour (transparent without a fill). bmp: the
// 32-bit BMP of CDRStylesCollector::collectBmp, its pixels 0x000000 or 0xffffff. Empty when all
// its pixels are transparent (all white without a fill): a BMP whose alpha is 0 everywhere is
// read as an opaque one (a black rectangle).
librevenge::RVNGBinaryData libcdr::CDRContentCollector::_monoImage(const librevenge::RVNGBinaryData &bmp)
{
  librevenge::RVNGPropertyList style;   // (resolves the object's fill and line styles)
  _fillProperties(style);
  _lineProperties(style);
  bool outline = m_currentLineStyle.lineType != (unsigned short)-1 && !(m_currentLineStyle.lineType & 0x1);
  bool fill = m_currentFillStyle.fillType == 1;
  unsigned fg = 0xff000000 | (outline ? m_ps._getRGBColor(m_currentLineStyle.color) : 0);
  unsigned bg = fill ? 0xff000000 | m_ps._getRGBColor(m_currentFillStyle.color1) : 0;
  const unsigned long offset = 54;
  if (bmp.size() < offset)
    return bmp;
  std::vector<unsigned char> data(bmp.getDataBuffer(), bmp.getDataBuffer() + bmp.size());
  bool shown = false;
  for (unsigned long i = offset; i + 4 <= data.size(); i += 4)
  {
    unsigned v = (data[i] & 1) ? bg : fg;
    shown = shown || v;
    data[i] = (unsigned char)v;
    data[i + 1] = (unsigned char)(v >> 8);
    data[i + 2] = (unsigned char)(v >> 16);
    data[i + 3] = (unsigned char)(v >> 24);
  }
  if (!shown)
    return librevenge::RVNGBinaryData();
  return librevenge::RVNGBinaryData(data.data(), data.size());
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
  // CorelDRAW fills every object even-odd (a ring of two circles drawn the same way round, with a
  // gradient or a pattern too)
  propList.insert("svg:fill-rule", "evenodd");
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
        break;
      case 2: // Gradient
        if (m_currentFillStyle.gradient.m_stops.empty())
          propList.insert("draw:fill", "none");
        else if (m_currentFillStyle.gradient.m_stops.size() == 1)
        {
          propList.insert("draw:fill", "solid");
          propList.insert("draw:fill-color", m_ps.getRGBColorString(m_currentFillStyle.gradient.m_stops[0].m_color));
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
            propList.insert("svg:cx", (double)(0.5 + m_currentFillStyle.gradient.m_centerXOffset/100.0), librevenge::RVNG_PERCENT);
            propList.insert("svg:cy", (double)(0.5 - m_currentFillStyle.gradient.m_centerYOffset/100.0), librevenge::RVNG_PERCENT);   // (a share of the box, up positive)
            break;
          case 4: // square
            propList.insert("draw:border", (2.0 * (double)(m_currentFillStyle.gradient.m_edgeOffset)/100.0), librevenge::RVNG_PERCENT);
            propList.insert("draw:style", "square");
            propList.insert("svg:cx", (double)(0.5 + m_currentFillStyle.gradient.m_centerXOffset/100.0), librevenge::RVNG_PERCENT);
            propList.insert("svg:cy", (double)(0.5 - m_currentFillStyle.gradient.m_centerYOffset/100.0), librevenge::RVNG_PERCENT);   // (a share of the box, up positive)
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
        // the fountain fill as CorelDRAW draws it (libcdr's own properties: ODF has no conical
        // gradient, and its radial and square ones are sized otherwise): the type, the angle
        // (degrees, counter-clockwise), the centre's offset and the edge pad (shares) and the stops
        // from the start colour to the end colour
        if (m_currentFillStyle.gradient.m_stops.size() > 1)
        {
          static const char *const types[] = { "linear", "linear", "radial", "conical", "square" };
          const CDRGradient &gradient = m_currentFillStyle.gradient;
          propList.insert("libcdr:gradient-type", types[gradient.m_type < 5 ? gradient.m_type : 0]);
          propList.insert("libcdr:gradient-angle", gradient.m_angle * 180 / M_PI);
          propList.insert("libcdr:gradient-center-x", gradient.m_centerXOffset / 100.0);
          propList.insert("libcdr:gradient-center-y", gradient.m_centerYOffset / 100.0);
          propList.insert("libcdr:gradient-edge", gradient.m_edgeOffset / 100.0);
          librevenge::RVNGPropertyListVector stops;
          for (auto &gradStop : gradient.m_stops)
          {
            librevenge::RVNGPropertyList stopElement;
            stopElement.insert("svg:offset", gradStop.m_offset, librevenge::RVNG_PERCENT);
            stopElement.insert("svg:stop-color", m_ps.getRGBColorString(gradStop.m_color));
            stops.append(stopElement);
            // a two colour fill's midpoint (versions 7+; 0 before): the colours half mixed there,
            // evenly on both sides of it (as CorelDRAW 9's PDF exports show)
            if (gradient.m_stops.size() == 2 && &gradStop == &gradient.m_stops[0] && gradient.m_midPoint > 0.0
                && gradient.m_midPoint < 1.0 && gradient.m_midPoint != 0.5)
            {
              const unsigned a = m_ps._getRGBColor(gradient.m_stops[0].m_color);
              const unsigned b = m_ps._getRGBColor(gradient.m_stops[1].m_color);
              const double at = gradient.m_stops[0].m_offset + gradient.m_midPoint * (gradient.m_stops[1].m_offset - gradient.m_stops[0].m_offset);
              librevenge::RVNGString mixed;
              mixed.sprintf("#%.2x%.2x%.2x", (((a >> 16) & 0xff) + ((b >> 16) & 0xff) + 1) / 2,
                            (((a >> 8) & 0xff) + ((b >> 8) & 0xff) + 1) / 2, ((a & 0xff) + (b & 0xff) + 1) / 2);
              librevenge::RVNGPropertyList midElement;
              midElement.insert("svg:offset", at, librevenge::RVNG_PERCENT);
              midElement.insert("svg:stop-color", mixed);
              stops.append(midElement);
            }
          }
          propList.insert("libcdr:gradient-stops", stops);
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
          const CDRBox &tile = m_currentFillStyle.imageFill.tile;
          auto iterBox = m_ps.m_vectBoxes.find(m_currentFillStyle.imageFill.id);
          if (tile.getWidth() > 0 && tile.getHeight() > 0 && iterBox != m_ps.m_vectBoxes.end())
            propList.insert("draw:fill-image", tileSVG(iterVect->second, iterBox->second, tile));
          else
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
      // arrowheads, in their own units: 1 (1000 in a CorelDRAW 5 arrw record) is twice the line's
      // width, but no less than 0.028 inch (as CorelDRAW 9's PDF exports show); the tip at the
      // line's end, the arrowhead's y axis along the line (libcdr's own properties)
      const bool hasStartMarker = m_currentLineStyle.startMarker && !m_currentLineStyle.startMarker->empty();
      const bool hasEndMarker = m_currentLineStyle.endMarker && !m_currentLineStyle.endMarker->empty();
      if (hasStartMarker || hasEndMarker)
      {
        double width = m_currentLineStyle.lineWidth * scale;
        propList.insert("libcdr:marker-unit", 2.0 * (width > 0.014 ? width : 0.014));
        if (hasStartMarker)
          propList.insert("libcdr:marker-start", markerPath(*m_currentLineStyle.startMarker));
        if (hasEndMarker)
          propList.insert("libcdr:marker-end", markerPath(*m_currentLineStyle.endMarker));
      }
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
  if (m_currentLineStyle.startMarker && !m_currentLineStyle.startMarker->empty())
  {
    _writeMarker(m_writtenStartMarker, m_currentLineStyle.startMarker, CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, 0));
    propList.insert("draw:marker-start-viewbox", m_writtenStartMarker.viewBox);
    propList.insert("draw:marker-start-path", m_writtenStartMarker.path);
  }
  if (m_currentLineStyle.endMarker && !m_currentLineStyle.endMarker->empty())
  {
    _writeMarker(m_writtenEndMarker, m_currentLineStyle.endMarker, CDRTransform(-1.0, 0.0, 0.0, 0.0, -1.0, 0));
    propList.insert("draw:marker-end-viewbox", m_writtenEndMarker.viewBox);
    propList.insert("draw:marker-end-path", m_writtenEndMarker.path);
  }



}

void libcdr::CDRContentCollector::_writeMarker(WrittenMarker &written, const std::shared_ptr<const CDRPath> &marker, const CDRTransform &mirror)
{
  bool const hasGroupTransforms = !m_groupTransforms.empty();
  if (written.marker == marker && written.transforms == m_currentTransforms &&
      written.hasGroupTransforms == hasGroupTransforms &&
      (!hasGroupTransforms || written.groupTransforms == m_groupTransforms.top()))
    return;

  CDRPath path(*marker);
  path.transform(m_currentTransforms);
  if (hasGroupTransforms)
    path.transform(m_groupTransforms.top());
  path.transform(mirror);
  written.path.clear();
  written.viewBox.clear();
  double width;
  path.writeOut(written.path, written.viewBox, width);

  written.marker = marker;
  written.transforms = m_currentTransforms;
  written.hasGroupTransforms = hasGroupTransforms;
  if (hasGroupTransforms)
    written.groupTransforms = m_groupTransforms.top();
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
  {
    m_currentImage = CDRImage(iter->second, x1, x2, y1, y2);
    m_currentImageMono = m_ps.m_monoBmps.count(imageId) != 0;
  }
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
    CDRBox box;
    if (cmxBoundingBox(data, box))
      m_ps.m_vectBoxes[id] = box;
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

void libcdr::CDRContentCollector::collectArtisticText(double x, double y, bool objectStyle)
{
  m_currentTextObjectStyle = objectStyle;
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
