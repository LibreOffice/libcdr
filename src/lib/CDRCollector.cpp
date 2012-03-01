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

#include <math.h>
#include "CDRCollector.h"
#include "CDRInternalStream.h"
#include "libcdr_utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef DUMP_IMAGE
#define DUMP_IMAGE 0
#endif

namespace
{
int cdr_round(double d)
{
  return (d>0) ? int(d+0.5) : int(d-0.5);
}

void writeU16(WPXBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
}

void writeU32(WPXBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
  buffer.append((unsigned char)((value >> 16) & 0xFF));
  buffer.append((unsigned char)((value >> 24) & 0xFF));
}

void writeU8(WPXBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
}

#include "CDRColorProfiles.h"

}

libcdr::CDRCollector::CDRCollector(libwpg::WPGPaintInterface *painter) :
  m_painter(painter),
  m_isPageProperties(false),
  m_isPageStarted(false),
  m_pageOffsetX(-4.25), m_pageOffsetY(-5.5),
  m_pageWidth(8.5), m_pageHeight(11.0),
  m_currentFildId(0.0), m_currentOutlId(0),
  m_currentObjectLevel(0), m_currentPageLevel(0),
  m_currentImage(),
  m_currentPath(), m_currentTransform(),
  m_fillStyles(), m_lineStyles(), m_polygon(0),
  m_bmps(), m_isInPolygon(false), m_outputElements(),
  m_colorTransformCMYK2RGB(0), m_colorTransformLab2RGB(0)
{
  cmsHPROFILE tmpCMYKProfile = cmsOpenProfileFromMem(SWOP_icc, sizeof(SWOP_icc)/sizeof(SWOP_icc[0]));
  cmsHPROFILE tmpRGBProfile = cmsCreate_sRGBProfile();
  m_colorTransformCMYK2RGB = cmsCreateTransform(tmpCMYKProfile, TYPE_CMYK_DBL, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  cmsHPROFILE tmpLabProfile = cmsCreateLab4Profile(0);
  m_colorTransformLab2RGB = cmsCreateTransform(tmpLabProfile, TYPE_Lab_DBL, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(tmpLabProfile);
  cmsCloseProfile(tmpCMYKProfile);
  cmsCloseProfile(tmpRGBProfile);
}

libcdr::CDRCollector::~CDRCollector()
{
  if (m_isPageStarted)
    _endPage();
  if (m_colorTransformCMYK2RGB)
    cmsDeleteTransform(m_colorTransformCMYK2RGB);
  if (m_colorTransformLab2RGB)
    cmsDeleteTransform(m_colorTransformLab2RGB);
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
  if (!m_isPageStarted)
    return;
  while (!m_outputElements.empty())
  {
    m_outputElements.top().draw(m_painter);
    m_outputElements.pop();
  }
  if (m_painter)
    m_painter->endGraphics();
  m_isPageStarted = false;
}

void libcdr::CDRCollector::collectPage(unsigned level)
{
  m_isPageProperties = true;
  m_currentPageLevel = level;
}

void libcdr::CDRCollector::collectObject(unsigned level)
{
  m_currentObjectLevel = level;
  m_currentFildId = 0;
  m_currentOutlId = 0;
}

void libcdr::CDRCollector::collectFlags(unsigned flags)
{
  if (!m_isPageProperties || (flags & 0x00ff0000))
  {
    m_isPageProperties = false;
  }
  else
  {
    m_isPageProperties = false;
    if (!m_isPageStarted)
      _startPage(m_pageWidth, m_pageHeight);
  }
}

void libcdr::CDRCollector::collectOtherList()
{
  m_isPageProperties = false;
}

void libcdr::CDRCollector::collectPageSize(double width, double height)
{
  m_pageWidth = width;
  m_pageHeight = height;
  m_pageOffsetX = -width / 2.0;
  m_pageOffsetY = -height / 2.0;
}

void libcdr::CDRCollector::collectCubicBezier(double x1, double y1, double x2, double y2, double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectCubicBezier(%f, %f, %f, %f, %f, %f)\n", x1, y1, x2, y2, x, y));
  m_currentPath.appendCubicBezierTo(x1, y1, x2, y2, x, y);
}

void libcdr::CDRCollector::collectQuadraticBezier(double x1, double y1, double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectQuadraticBezier(%f, %f, %f, %f)\n", x1, y1, x, y));
  m_currentPath.appendQuadraticBezierTo(x1, y1, x, y);
}

void libcdr::CDRCollector::collectMoveTo(double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectMoveTo(%f, %f)\n", x, y));
  m_currentPath.appendMoveTo(x,y);
}

void libcdr::CDRCollector::collectLineTo(double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectLineTo(%f, %f)\n", x, y));
  m_currentPath.appendLineTo(x, y);
}

void libcdr::CDRCollector::collectArcTo(double rx, double ry, bool largeArc, bool sweep, double x, double y)
{
  CDR_DEBUG_MSG(("CDRCollector::collectArcTo(%f, %f)\n", x, y));
  m_currentPath.appendArcTo(rx, ry, 0.0, largeArc, sweep, x, y);
}

void libcdr::CDRCollector::collectClosePath()
{
  CDR_DEBUG_MSG(("CDRCollector::collectClosePath\n"));
  m_currentPath.appendClosePath();
}

void libcdr::CDRCollector::_flushCurrentPath()
{
  CDR_DEBUG_MSG(("CDRCollector::collectFlushPath\n"));
  CDROutputElementList outputElement;
  if (!m_currentPath.empty())
  {
    if (m_polygon && m_isInPolygon)
      m_polygon->create(m_currentPath);
    if (m_polygon)
    {
      delete m_polygon;
      m_polygon = 0;
    }
    m_isInPolygon = false;
    bool firstPoint = true;
    double initialX = 0.0;
    double initialY = 0.0;
    double previousX = 0.0;
    double previousY = 0.0;
    double x = 0.0;
    double y = 0.0;
    WPXPropertyList style;
    WPXPropertyListVector gradient;
    _fillProperties(style, gradient);
    _lineProperties(style);
    outputElement.addStyle(style, gradient);
    m_currentPath.transform(m_currentTransform);
    CDRTransform tmpTrafo(1.0, 0.0, -m_pageOffsetX, 0.0, 1.0, -m_pageOffsetY);
    m_currentPath.transform(tmpTrafo);
    tmpTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, m_pageHeight);
    m_currentPath.transform(tmpTrafo);
    WPXPropertyListVector tmpPath;
    m_currentPath.writeOut(tmpPath);
    WPXPropertyListVector path;
    WPXPropertyListVector::Iter i(tmpPath);
    bool ignoreM = false;
    for (i.rewind(); i.next();)
    {
      if (!i()["libwpg:path-action"])
        continue;
      if (i()["svg:x"] && i()["svg:y"])
      {
        x = i()["svg:x"]->getDouble();
        y = i()["svg:y"]->getDouble();
        if (firstPoint)
        {
          initialX = x;
          initialY = y;
          firstPoint = false;
        }
        else if (i()["libwpg:path-action"]->getStr() == "M")
        {
          if (CDR_ALMOST_ZERO(previousX - x) && CDR_ALMOST_ZERO(previousY - y))
            ignoreM = true;
          else
          {
            if ((CDR_ALMOST_ZERO(initialX - previousX) && CDR_ALMOST_ZERO(initialY - previousY)) || (style["draw:fill"] && style["draw:fill"]->getStr() != "none"))
            {
              WPXPropertyList node;
              node.insert("libwpg:path-action", "Z");
              path.append(node);
            }
            initialX = x;
            initialY = y;
          }
        }
      }
      previousX = x;
      previousY = y;
      if (!ignoreM)
        path.append(i());
      ignoreM = false;
    }
    if ((CDR_ALMOST_ZERO(initialX - previousX) && CDR_ALMOST_ZERO(initialY - previousY)) || (style["draw:fill"] && style["draw:fill"]->getStr() != "none"))
    {
      WPXPropertyList node;
      node.insert("libwpg:path-action", "Z");
      path.append(node);
    }

    outputElement.addPath(path);
    m_currentPath.clear();
  }
  if (m_currentImage.getImage().size())
  {
    double cx = m_currentImage.getMiddleX();
    double cy = m_currentImage.getMiddleY();
    m_currentTransform.applyToPoint(cx, cy);
    CDRTransform tmpTrafo(1.0, 0.0, -m_pageOffsetX, 0.0, 1.0, -m_pageOffsetY);
    tmpTrafo.applyToPoint(cx, cy);
    tmpTrafo = CDRTransform(1.0, 0.0, 0.0, 0.0, -1.0, m_pageHeight);
    tmpTrafo.applyToPoint(cx, cy);
    double scaleX = (m_currentTransform.m_v0 < 0.0 ? -1.0 : 1.0)*sqrt(m_currentTransform.m_v0 * m_currentTransform.m_v0 + m_currentTransform.m_v1 * m_currentTransform.m_v1);
    double scaleY = (m_currentTransform.m_v3 < 0.0 ? -1.0 : 1.0)*sqrt(m_currentTransform.m_v3 * m_currentTransform.m_v3 + m_currentTransform.m_v4 * m_currentTransform.m_v4);
    bool flipX(scaleX < 0);
    bool flipY(scaleY < 0);
    double width = fabs(scaleX)*m_currentImage.getWidth();
    double height = fabs(scaleY)*m_currentImage.getHeight();
    double rotate = -atan2(m_currentTransform.m_v3, m_currentTransform.m_v4);

    WPXPropertyList propList;

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

    if (rotate != 0.0)
      propList.insert("libwpg:rotate", rotate * 180 / M_PI, WPX_GENERIC);

    propList.insert("libwpg:mime-type", "image/bmp");

    outputElement.addGraphicObject(propList, m_currentImage.getImage());
  }
  m_currentImage = libcdr::CDRImage();
  if (!outputElement.empty())
    m_outputElements.push(outputElement);

}

void libcdr::CDRCollector::collectTransform(double v0, double v1, double x0, double v3, double v4, double y0)
{
  m_currentTransform = CDRTransform(v0, v1, x0, v3, v4, y0);
}

void libcdr::CDRCollector::collectLevel(unsigned level)
{
  if (level <= m_currentObjectLevel)
  {
    _flushCurrentPath();
    m_currentObjectLevel = 0;
  }
  if (level <= m_currentPageLevel)
  {
    _endPage();
    m_currentPageLevel = 0;
  }
}

void libcdr::CDRCollector::collectFildId(unsigned id)
{
  m_currentFildId = id;
}

void libcdr::CDRCollector::collectOutlId(unsigned id)
{
  m_currentOutlId = id;
}

void libcdr::CDRCollector::collectFild(unsigned id, unsigned short fillType, const libcdr::CDRColor &color1, const libcdr::CDRColor &color2, const libcdr::CDRGradient &gradient, unsigned patternId)
{
  m_fillStyles[id] = CDRFillStyle(fillType, color1, color2, gradient, patternId);
}

void libcdr::CDRCollector::collectOutl(unsigned id, unsigned short lineType, unsigned short capsType, unsigned short joinType,
                                       double lineWidth, const CDRColor &color, const std::vector<unsigned short> &dashArray,
                                       unsigned startMarkerId, unsigned endMarkerId)
{
  m_lineStyles[id] = CDRLineStyle(lineType, capsType, joinType, lineWidth, color, dashArray, startMarkerId, endMarkerId);
}

void libcdr::CDRCollector::collectRotate(double /* angle */)
{
}

void libcdr::CDRCollector::collectPolygon()
{
  m_isInPolygon = true;
}

void libcdr::CDRCollector::collectPolygonTransform(unsigned numAngles, unsigned nextPoint, double rx, double ry, double cx, double cy)
{
  if (m_polygon)
    delete m_polygon;
  m_polygon = new CDRPolygon(numAngles, nextPoint, rx, ry, cx, cy);
}

unsigned libcdr::CDRCollector::_getBMPColor(const CDRColor &color)
{
  switch (color.m_colorModel)
  {
  case 0:
    return _getRGBColor(libcdr::CDRColor(0, color.m_colorValue));
  case 1:
    return _getRGBColor(libcdr::CDRColor(5, color.m_colorValue));
  case 2:
    return _getRGBColor(libcdr::CDRColor(4, color.m_colorValue));
  case 3:
    return _getRGBColor(libcdr::CDRColor(3, color.m_colorValue));
  case 4:
    return _getRGBColor(libcdr::CDRColor(6, color.m_colorValue));
  case 5:
    return _getRGBColor(libcdr::CDRColor(9, color.m_colorValue));
  case 6:
    return _getRGBColor(libcdr::CDRColor(8, color.m_colorValue));
  case 7:
    return _getRGBColor(libcdr::CDRColor(7, color.m_colorValue));
  case 8:
    return color.m_colorValue;
  case 9:
    return color.m_colorValue;
  case 10:
    return _getRGBColor(libcdr::CDRColor(5, color.m_colorValue));
  case 11:
    return _getRGBColor(libcdr::CDRColor(18, color.m_colorValue));
  default:
    return color.m_colorValue;
  }
}

unsigned libcdr::CDRCollector::_getRGBColor(const CDRColor &color)
{
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char col0 = color.m_colorValue & 0xff;
  unsigned char col1 = (color.m_colorValue & 0xff00) >> 8;
  unsigned char col2 = (color.m_colorValue & 0xff0000) >> 16;
  unsigned char col3 = (color.m_colorValue & 0xff000000) >> 24;
  if (color.m_colorModel == 0x02) // CMYK100
  {
    double cmyk[4] =
    {
      (double)col0,
      (double)col1,
      (double)col2,
      (double)col3
    };
    unsigned char rgb[3] = { 0, 0, 0 };
    cmsDoTransform(m_colorTransformCMYK2RGB, cmyk, rgb, 1);
    red = rgb[0];
    green = rgb[1];
    blue = rgb[2];
  }
  else if (color.m_colorModel == 0x03 || color.m_colorModel == 0x01 || color.m_colorModel == 0x11) // CMYK255
  {
    double cmyk[4] =
    {
      (double)col0*100.0/255.0,
      (double)col1*100.0/255.0,
      (double)col2*100.0/255.0,
      (double)col3*100.0/255.0
    };
    unsigned char rgb[3] = { 0, 0, 0 };
    cmsDoTransform(m_colorTransformCMYK2RGB, cmyk, rgb, 1);
    red = rgb[0];
    green = rgb[1];
    blue = rgb[2];
  }
  else if (color.m_colorModel == 0x04) // CMY
  {
    red = 255 - col0;
    green = 255 - col1;
    blue = 255 - col2;
  }
  else if (color.m_colorModel == 0x05) // RGB
  {
    red = col2;
    green = col1;
    blue = col0;
  }
  else if (color.m_colorModel == 0x06) // HSB
  {
    unsigned short hue = (col1<<8) | col0;
    double saturation = (double)col2/255.0;
    double brightness = (double)col3/255.0;

    while (hue > 360)
      hue -= 360;

    double satRed, satGreen, satBlue;

    if (hue < 120)
    {
      satRed = (double)(120 - hue) / 60.0;
      satGreen = (double)hue / 60.0;
      satBlue = 0;
    }
    else if (hue < 240)
    {
      satRed = 0;
      satGreen = (double)(240 - hue) / 60.0;
      satBlue = (double)(hue - 120) / 60.0;
    }
    else
    {
      satRed = (double)(hue - 240) / 60.0;
      satGreen = 0.0;
      satBlue = (double)(360 - hue) / 60.0;
    }
    red = (unsigned char)cdr_round(255*(1 - saturation + saturation * (satRed > 1 ? 1 : satRed)) * brightness);
    green = (unsigned char)cdr_round(255*(1 - saturation + saturation * (satGreen > 1 ? 1 : satGreen)) * brightness);
    blue = (unsigned char)cdr_round(255*(1 - saturation + saturation * (satBlue > 1 ? 1 : satBlue)) * brightness);
  }
  else if (color.m_colorModel == 0x07) // HLS
  {
    unsigned short hue = (col1<<8) | col0;
    double lightness = (double)col2/255.0;
    double saturation = (double)col3/255.0;

    while (hue > 360)
      hue -= 360;

    double satRed, satGreen, satBlue;

    if (hue < 120)
    {
      satRed =  (double)(120 - hue) / 60.0;
      satGreen = (double)hue/60.0;
      satBlue = 0.0;
    }
    else if (hue < 240)
    {
      satRed = 0;
      satGreen = (double)(240 - hue) / 60.0;
      satBlue = (double)(hue - 120) / 60.0;
    }
    else
    {
      satRed = (double)(hue - 240) / 60.0;
      satGreen = 0;
      satBlue = (double)(360 - hue) / 60.0;
    }

    double tmpRed = 2*saturation*(satRed > 1 ? 1 : satRed) + 1 - saturation;
    double tmpGreen = 2*saturation*(satGreen > 1 ? 1 : satGreen) + 1 - saturation;
    double tmpBlue = 2*saturation*(satBlue > 1 ? 1 : satBlue) + 1 - saturation;

    if (lightness < 0.5)
    {
      red = (unsigned char)cdr_round(255.0*lightness*tmpRed);
      green = (unsigned char)cdr_round(255.0*lightness*tmpGreen);
      blue = (unsigned char)cdr_round(255.0*lightness*tmpBlue);
    }
    else
    {
      red = (unsigned char)cdr_round(255*((1 - lightness) * tmpRed + 2 * lightness - 1));
      green = (unsigned char)cdr_round(255*((1 - lightness) * tmpGreen + 2 * lightness - 1));
      blue = (unsigned char)cdr_round(255*((1 - lightness) * tmpBlue + 2 * lightness - 1));
    }
  }
  else if (color.m_colorModel == 0x09) // Grayscale
  {
    red = col0;
    green = col0;
    blue = col0;
  }
  else if (color.m_colorModel == 0x0c) // Lab
  {
    cmsCIELab Lab;
    Lab.L = (double)col0*100.0/255.0;
    Lab.a = (double)(signed char)col1;
    Lab.b = (double)(signed char)col2;
    unsigned char rgb[3] = { 0, 0, 0 };
    cmsDoTransform(m_colorTransformLab2RGB, &Lab, rgb, 1);
    red = rgb[0];
    green = rgb[1];
    blue = rgb[2];
  }
  else if (color.m_colorModel == 0x12) // Lab
  {
    cmsCIELab Lab;
    Lab.L = (double)col0*100.0/255.0;
    Lab.a = (double)((signed char)(col1 - 0x80));
    Lab.b = (double)((signed char)(col2 - 0x80));
    unsigned char rgb[3] = { 0, 0, 0 };
    cmsDoTransform(m_colorTransformLab2RGB, &Lab, rgb, 1);
    red = rgb[0];
    green = rgb[1];
    blue = rgb[2];
  }
  else if (color.m_colorModel == 0x19) // HKS
  {
    const unsigned char C_channel[] =
    {
      3,     4,   0,   0,   0,   0,   0,   0,   0,   0,
      0,     0,   1,   7,  12,  14,  18,   0,   0,   1,
      9,     8,  14,  12,  16,  26,  14,  13,  29,  43,
      37,   35,  36,  38,  37,  38,  38,  39,  39,  39,
      35,   36,  39,  39,  39,  32,  38,  39,  35,  39,

      100, 100, 100, 100,  60, 100,  80,  70,  10,  60,
      85,   65,  60,  10,  20,  20,  10,  10,   0,   0,
      10,    0,   0,  60,  10,   0,   0,   0,   0,   0,
      10,    0,  10,  20,   0,   0
    };
    const unsigned char M_channel[] =
    {
      19,   13,  12,  35,  44,  69,  79,  87,  94,  68,
      99,   99,  99, 100,  99, 100,  98,  78,  96, 100,
      100, 100, 100,  99, 100,  98, 100, 100,  97,  97,
      93,   96,  97,  74,  66,  31,  75,  85,  83,  65,
      77,   34,  20,   9,  32,   0,   3,  13,   0,   3,

      0,     0,  50,   0,   0,   0,   5,   0,   0,   0,
      0,     0,   0,   0,  20,   0,  50,  55,  55,  50,
      30,   60,  60,  95,  80,  85,  75,  80,   0,  10,
      0,    25,   0,   0,   0,   0
    };
    const unsigned char Y_channel[] =
    {
      78,   99, 100, 100, 100, 100, 100,  97,  97,  61,
      97,   93,  91,  81,  77,  65,  57,  24,  83,  68,
      49,   36,  16,   0,  51,   0,   0,  29,   0,   0,
      2,     0,   7,  40,   0,  23,  37,   0,   0,   0,
      0,    20,  11,  31,  26,  20,  52,  71,  66,  87,

      80,   70,  80,  90,  65,  50, 100, 100,  70,  90,
      100, 100, 100,  20, 100, 100, 100, 100, 100, 100,
      80,   90,  60, 100, 100, 100,  70,  70,   0,  30,
      10,   30,   0,  10,  20,  20
    };
    const unsigned char K_channel[] =
    {
      0,    0,   0,   0,   0,   0,   1,   2,   2,   0,
      4,    3,   3,   7,  18,  12,  39,   0,   1,   0,
      0,    0,   0,   0,   4,   0,   0,   0,   0,   2,
      3,    2,   9,  48,   0,   0,  39,   2,   2,   0,
      0,    0,   0,   0,   0,   0,   0,   2,   0,   2,

      10,  30,   0,  20,  50,  20,   0,  75,  60,  40,
      10,   0,   0,  90,   0,   0,   0,   0,  30,  55,
      60,  80,  70,   0,   0,  30,  50,  60, 100,  35,
      40,  90,  70,  80,  50,  60
    };
    unsigned short hks = (unsigned short)(color.m_colorValue & 0xffff)+85;
    unsigned hksIndex = hks % 86;
    hks /= 86;
    unsigned K_percent = hks/10;
    switch (K_percent)
    {
    case 2:
      K_percent = 10;
      break;
    case 3:
      K_percent = 30;
      break;
    case 4:
      K_percent = 50;
      break;
    default:
      K_percent = 0;
      break;
    }
    unsigned O_percent = (hks % 10) ? (hks % 10) * 10 : 100;
    col0 = (double)O_percent*C_channel[hksIndex];
    col1 = (double)O_percent*M_channel[hksIndex];
    col2 = (double)O_percent*Y_channel[hksIndex];
    col3 = (100.0 - (double)K_percent)*K_channel[hksIndex]+(double)K_percent*100.0;
    col3 = (double)O_percent*col3/100.0;
    double cmyk[4] =
    {
      (double)col0,
      (double)col1,
      (double)col2,
      (double)col3
    };
    unsigned char rgb[3] = { 0, 0, 0 };
    cmsDoTransform(m_colorTransformCMYK2RGB, cmyk, rgb, 1);
    red = rgb[0];
    green = rgb[1];
    blue = rgb[2];

  }
  return (unsigned)((red << 16) | (green << 8) | blue);
}

WPXString libcdr::CDRCollector::_getRGBColorString(const libcdr::CDRColor &color)
{
  WPXString tempString;
  tempString.sprintf("#%.6x", _getRGBColor(color));
  return tempString;
}

void libcdr::CDRCollector::_fillProperties(WPXPropertyList &propList, WPXPropertyListVector &vec)
{
  if (m_currentFildId == 0)
    propList.insert("draw:fill", "none");
  else
  {
    std::map<unsigned, CDRFillStyle>::iterator iter = m_fillStyles.find(m_currentFildId);
    if (iter == m_fillStyles.end())
      propList.insert("draw:fill", "none");
    else
    {
      switch (iter->second.fillType)
      {
      case 1: // Solid
        propList.insert("draw:fill", "solid");
        propList.insert("draw:fill-color", _getRGBColorString(iter->second.color1));
        propList.insert("svg:fill-rule", "evenodd");
        break;
      case 2: // Gradient
        if (iter->second.gradient.m_stops.empty())
          propList.insert("draw:fill", "none");
        else if (iter->second.gradient.m_stops.size() == 1)
        {
          propList.insert("draw:fill", "solid");
          propList.insert("draw:fill-color", _getRGBColorString(iter->second.gradient.m_stops[0].m_color));
          propList.insert("svg:fill-rule", "evenodd");
        }
        else if (iter->second.gradient.m_stops.size() == 2)
        {
          double angle = iter->second.gradient.m_angle;
          while (angle < 0.0)
            angle += 360.0;
          while (angle > 360.0)
            angle -= 360.0;
          propList.insert("draw:fill", "gradient");
          propList.insert("draw:start-color", _getRGBColorString(iter->second.gradient.m_stops[0].m_color));
          propList.insert("draw:end-color", _getRGBColorString(iter->second.gradient.m_stops[1].m_color));
          propList.insert("draw:angle", (int)angle);
          switch (iter->second.gradient.m_type)
          {
          case 1: // linear
          case 3: // conical
            propList.insert("draw:style", "linear");
            angle += 90.0;
            while (angle < 0.0)
              angle += 360.0;
            while (angle > 360.0)
              angle -= 360.0;
            propList.insert("draw:angle", (int)angle);
            break;
          case 2: // radial
            propList.insert("draw:style", "radial");
            propList.insert("svg:cx", (double)(0.5 + iter->second.gradient.m_centerXOffset/200.0), WPX_PERCENT);
            propList.insert("svg:cy", (double)(0.5 + iter->second.gradient.m_centerXOffset/200.0), WPX_PERCENT);
            break;
          case 4: // square
            propList.insert("draw:style", "square");
            propList.insert("svg:cx", (double)(0.5 + iter->second.gradient.m_centerXOffset/200.0), WPX_PERCENT);
            propList.insert("svg:cy", (double)(0.5 + iter->second.gradient.m_centerXOffset/200.0), WPX_PERCENT);
            break;
          default:
            propList.insert("draw:fill", "none");
            break;
          }
        }
        else // output svg gradient as a hail mary pass towards ODG that does not really support it
        {
        }
        break;
      default:
        propList.insert("draw:fill", "none");
        break;
      }
    }
  }
}

void libcdr::CDRCollector::_lineProperties(WPXPropertyList &propList)
{
  if (m_currentOutlId == 0)
  {
    propList.insert("draw:stroke", "solid");
    propList.insert("svg:stroke-width", 0.0);
    propList.insert("svg:stroke-color", "#000000");
  }
  else
  {
    std::map<unsigned, CDRLineStyle>::iterator iter = m_lineStyles.find(m_currentOutlId);
    if (iter == m_lineStyles.end())
    {
      propList.insert("draw:stroke", "solid");
      propList.insert("svg:stroke-width", 0.0);
      propList.insert("svg:stroke-color", "#000000");
    }
    else if (iter->second.lineType & 0x1)
      propList.insert("draw:stroke", "none");
    else if (iter->second.lineType & 0x6)
    {
      if (iter->second.dashArray.size() && (iter->second.lineType & 0x4))
        propList.insert("draw:stroke", "dash");
      else
        propList.insert("draw:stroke", "solid");
      propList.insert("svg:stroke-width", iter->second.lineWidth);
      propList.insert("svg:stroke-color", _getRGBColorString(iter->second.color));

      switch (iter->second.capsType)
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

      switch (iter->second.joinType)
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

      if (iter->second.dashArray.size())
      {
        int dots1 = 0;
        int dots2 = 0;
        unsigned dots1len = 0;
        unsigned dots2len = 0;
        unsigned gap = 0;

        if (iter->second.dashArray.size() >= 2)
        {
          dots1len = iter->second.dashArray[0];
          gap = iter->second.dashArray[1];
        }

        unsigned count = iter->second.dashArray.size() / 2;
        unsigned i = 0;
        for (; i < count;)
        {
          if (dots1len == iter->second.dashArray[2*i])
            dots1++;
          else
            break;
          gap = gap < iter->second.dashArray[2*i+1] ?  iter->second.dashArray[2*i+1] : gap;
          i++;
        }
        if (i < count)
        {
          dots2len = iter->second.dashArray[2*i];
          gap = gap < iter->second.dashArray[2*i+1] ? iter->second.dashArray[2*i+1] : gap;
        }
        for (; i < count;)
        {
          if (dots2len == iter->second.dashArray[2*i])
            dots2++;
          else
            break;
          gap = gap < iter->second.dashArray[2*i+1] ? iter->second.dashArray[2*i+1] : gap;
          i++;
        }
        if (!dots2)
        {
          dots2 = dots1;
          dots2len = dots1len;
        }
        propList.insert("draw:dots1", dots1);
        propList.insert("draw:dots1-length", 72.0*(iter->second.lineWidth)*dots1len, WPX_POINT);
        propList.insert("draw:dots2", dots2);
        propList.insert("draw:dots2-length", 72.0*(iter->second.lineWidth)*dots2len, WPX_POINT);
        propList.insert("draw:distance", 72.0*(iter->second.lineWidth)*gap, WPX_POINT);
      }
    }
    else
    {
      propList.insert("draw:stroke", "solid");
      propList.insert("svg:stroke-width", 0.0);
      propList.insert("svg:stroke-color", "#000000");
    }

  }
}

void libcdr::CDRCollector::collectBitmap(unsigned imageId, double x1, double x2, double y1, double y2)
{
  std::map<unsigned, WPXBinaryData>::iterator iter = m_bmps.find(imageId);
  if (iter != m_bmps.end())
    m_currentImage = CDRImage(iter->second, x1, x2, y1, y2);
}

void libcdr::CDRCollector::collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> palette, const std::vector<unsigned char> bitmap)
{
  libcdr::CDRInternalStream stream(bitmap);
  WPXBinaryData image;

  unsigned tmpPixelSize = (unsigned)(height * width);
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
  writeU16(image, 0x4D42);  // Type
  writeU32(image, tmpDIBFileSize); // Size
  writeU16(image, 0); // Reserved1
  writeU16(image, 0); // Reserved2
  writeU32(image, tmpDIBOffsetBits); // OffsetBits

  // Create DIB Info header
  writeU32(image, 40); // Size

  writeU32(image, width);  // Width
  writeU32(image, height); // Height

  writeU16(image, 1); // Planes
  writeU16(image, 32); // BitCount
  writeU32(image, 0); // Compression
  writeU32(image, tmpDIBImageSize); // SizeImage
  writeU32(image, 0); // XPelsPerMeter
  writeU32(image, 0); // YPelsPerMeter
  writeU32(image, 0); // ColorsUsed
  writeU32(image, 0); // ColorsImportant

  // The Bitmaps in CDR are padded to 32bit border
  unsigned lineWidth = ((width * bpp + 32 - bpp) / 32) * 4;

  bool storeBMP = true;

  for (unsigned j = 0; j < height; ++j)
  {
    unsigned i = 0;
    unsigned k = 0;
    if (colorModel == 6)
    {
      while (i <lineWidth && k < width)
      {
        unsigned l = 0;
        unsigned char c = bitmap[j*lineWidth+i];
        i++;
        while (k < width && l < 8)
        {
          if (c & 0x80)
            writeU32(image, 0xffffff);
          else
            writeU32(image, 0);
          c <<= 1;
          l++;
          k++;
        }
      }
    }
    else if (colorModel == 5)
    {
      while (i <lineWidth && i < width)
      {
        unsigned char c = bitmap[j*lineWidth+i];
        i++;
        writeU32(image, _getBMPColor(libcdr::CDRColor(colorModel, c)));
      }
    }
    else if (!palette.empty())
    {
      while (i < lineWidth && i < width)
      {
        unsigned char c = bitmap[j*lineWidth+i];
        i++;
        writeU32(image, _getBMPColor(libcdr::CDRColor(colorModel, palette[c])));
      }
    }
    else if (bpp == 24)
    {
      while (i < lineWidth && k < width)
      {
        unsigned c = ((unsigned)bitmap[j*lineWidth+i+2] << 16) | ((unsigned)bitmap[j*lineWidth+i+1] << 8) | ((unsigned)bitmap[j*lineWidth+i]);
        i += 3;
        writeU32(image, _getBMPColor(libcdr::CDRColor(colorModel, c)));
        k++;
      }
    }
    else if (bpp == 32)
    {
      while (i < lineWidth && k < width)
      {
        unsigned c = (bitmap[j*lineWidth+i+3] << 24) | (bitmap[j*lineWidth+i+2] << 16) | (bitmap[j*lineWidth+i+1] << 8) | (bitmap[j*lineWidth+i]);
        i += 4;
        writeU32(image, _getBMPColor(libcdr::CDRColor(colorModel, c)));
        k++;
      }
    }
    else
      storeBMP = false;
  }

  if (storeBMP)
  {
#if DUMP_IMAGE
    WPXString filename;
    filename.sprintf("bitmap%.8x.bmp", imageId);
    FILE *f = fopen(filename.cstr(), "wb");
    if (f)
    {
      const unsigned char *tmpBuffer = image.getDataBuffer();
      for (unsigned long k = 0; k < image.size(); k++)
        fprintf(f, "%c",tmpBuffer[k]);
      fclose(f);
    }
#endif

    m_bmps[imageId] = image;
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
