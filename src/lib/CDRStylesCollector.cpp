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
#include <stack>
#include "CDRStylesCollector.h"
#include "CDRInternalStream.h"
#include "libcdr_utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef DUMP_IMAGE
#define DUMP_IMAGE 0
#endif


libcdr::CDRStylesCollector::CDRStylesCollector(libcdr::CDRParserState &ps) :
  m_ps(ps), m_page(8.5, 11.0, -4.25, -5.5), m_charStyles()
{
}

libcdr::CDRStylesCollector::~CDRStylesCollector()
{
}

void libcdr::CDRStylesCollector::collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> &palette, const std::vector<unsigned char> &bitmap)
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

  // Cater for eventual padding
  unsigned lineWidth = bitmap.size() / height;

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
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor(colorModel, c)));
      }
    }
    else if (!palette.empty())
    {
      while (i < lineWidth && i < width)
      {
        unsigned char c = bitmap[j*lineWidth+i];
        i++;
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor(colorModel, palette[c])));
      }
    }
    else if (bpp == 24)
    {
      while (i < lineWidth && k < width)
      {
        unsigned c = ((unsigned)bitmap[j*lineWidth+i+2] << 16) | ((unsigned)bitmap[j*lineWidth+i+1] << 8) | ((unsigned)bitmap[j*lineWidth+i]);
        i += 3;
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor(colorModel, c)));
        k++;
      }
    }
    else if (bpp == 32)
    {
      while (i < lineWidth && k < width)
      {
        unsigned c = (bitmap[j*lineWidth+i+3] << 24) | (bitmap[j*lineWidth+i+2] << 16) | (bitmap[j*lineWidth+i+1] << 8) | (bitmap[j*lineWidth+i]);
        i += 4;
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor(colorModel, c)));
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

    m_ps.m_bmps[imageId] = image;
  }
}

void libcdr::CDRStylesCollector::collectBmp(unsigned imageId, const std::vector<unsigned char> &bitmap)
{
  WPXBinaryData image(&bitmap[0], bitmap.size());
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

  m_ps.m_bmps[imageId] = image;
}

void libcdr::CDRStylesCollector::collectPageSize(double width, double height, double offsetX, double offsetY)
{
  if (m_ps.m_pages.empty())
    m_page = CDRPage(width, height, offsetX, offsetY);
  else
    m_ps.m_pages.back() = CDRPage(width, height, offsetX, offsetY);
}

void libcdr::CDRStylesCollector::collectPage(unsigned /* level */)
{
  m_ps.m_pages.push_back(m_page);
}

void libcdr::CDRStylesCollector::collectBmpf(unsigned patternId, unsigned width, unsigned height, const std::vector<unsigned char> &pattern)
{
  m_ps.m_patterns[patternId] = CDRPattern(width, height, pattern);
}

void libcdr::CDRStylesCollector::collectColorProfile(const std::vector<unsigned char> &profile)
{
  if (!profile.empty())
    m_ps.setColorTransform(profile);
}

void libcdr::CDRStylesCollector::collectPaletteEntry(unsigned colorId, unsigned /* userId */, const libcdr::CDRColor &color)
{
  m_ps.m_documentPalette[colorId] = color;
}

void libcdr::CDRStylesCollector::collectText(unsigned textId, unsigned styleId, const std::vector<unsigned char> &data,
    const std::vector<unsigned char> &charDescriptions, const std::map<unsigned, CDRCharacterStyle> &styleOverrides)
{
  if (data.empty() || charDescriptions.empty())
    return;

  unsigned char tmpCharDescription = 0;
  unsigned i = 0;
  unsigned j = 0;
  std::vector<unsigned char> tmpTextData;
  CDRCharacterStyle defaultCharStyle, tmpCharStyle;
  getRecursedStyle(defaultCharStyle, styleId);

  CDRTextLine line;
  for (i=0, j=0; i<charDescriptions.size() && j<data.size(); ++i)
  {
    tmpCharStyle = defaultCharStyle;
    std::map<unsigned, CDRCharacterStyle>::const_iterator iter = styleOverrides.find(tmpCharDescription & 0xfe);
    if (iter != styleOverrides.end())
      tmpCharStyle.overrideCharacterStyle(iter->second);
    if (charDescriptions[i] != tmpCharDescription)
    {
      WPXString text;
      if (!tmpTextData.empty())
      {
        if (tmpCharDescription & 0x01)
          appendCharacters(text, tmpTextData);
        else
          appendCharacters(text, tmpTextData, tmpCharStyle.m_charSet);
      }
      line.append(CDRText(text, tmpCharStyle));
      tmpTextData.clear();
      tmpCharDescription = charDescriptions[i];

    }
    tmpTextData.push_back(data[j++]);
    if (tmpCharDescription & 0x01)
      tmpTextData.push_back(data[j++]);
  }
  if (!tmpTextData.empty())
  {
    WPXString text;
    if (tmpCharDescription & 0x01)
      appendCharacters(text, tmpTextData);
    else
      appendCharacters(text, tmpTextData, tmpCharStyle.m_charSet);
    line.append(CDRText(text, tmpCharStyle));
    CDR_DEBUG_MSG(("CDRStylesCollector::collectText - Text: %s\n", text.cstr()));
  }

  std::vector<CDRTextLine> &paragraphVector = m_ps.m_texts[textId];
  paragraphVector.push_back(line);
}

void libcdr::CDRStylesCollector::collectStld(unsigned id, const CDRCharacterStyle &charStyle)
{
  m_charStyles[id] = charStyle;
}

void libcdr::CDRStylesCollector::getRecursedStyle(CDRCharacterStyle &charStyle, unsigned styleId)
{
  std::map<unsigned, CDRCharacterStyle>::const_iterator iter = m_charStyles.find(styleId);
  if (iter == m_charStyles.end())
    return;

  std::stack<CDRCharacterStyle> styleStack;
  styleStack.push(iter->second);
  if (iter->second.m_parentId)
  {
    std::map<unsigned, CDRCharacterStyle>::const_iterator iter2 = m_charStyles.find(iter->second.m_parentId);
    while (iter2 != m_charStyles.end())
    {
      styleStack.push(iter2->second);
      if (iter2->second.m_parentId)
        iter2 = m_charStyles.find(iter2->second.m_parentId);
      else
        iter2 = m_charStyles.end();
    }
  }
  while (!styleStack.empty())
  {
    charStyle.overrideCharacterStyle(styleStack.top());
    styleStack.pop();
  }
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
