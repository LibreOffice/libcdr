/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDRStylesCollector.h"

#include "CDRInternalStream.h"
#include "libcdr_utils.h"

#ifndef DUMP_IMAGE
#define DUMP_IMAGE 0
#endif


libcdr::CDRStylesCollector::CDRStylesCollector(libcdr::CDRParserState &ps) :
  m_ps(ps), m_page(8.5, 11.0, -4.25, -5.5)
{
}

libcdr::CDRStylesCollector::~CDRStylesCollector()
{
}

void libcdr::CDRStylesCollector::collectBmp(unsigned imageId, unsigned colorModel, unsigned width, unsigned height, unsigned bpp, const std::vector<unsigned> &palette, const std::vector<unsigned char> &bitmap)
{
  libcdr::CDRInternalStream stream(bitmap);
  librevenge::RVNGBinaryData image;

  if (height == 0)
    height = 1;

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
  unsigned long lineWidth = bitmap.size() / height;

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
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor((unsigned short)colorModel, c)));
      }
    }
    else if (!palette.empty())
    {
      while (i < lineWidth && i < width)
      {
        unsigned long c = bitmap[j*lineWidth+i];
        if (c >= palette.size())
          c = palette.size() - 1;
        i++;
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor((unsigned short)colorModel, palette[c])));
      }
    }
    else if (bpp == 24 && lineWidth >= 3)
    {
      while (i < lineWidth -2 && k < width)
      {
        unsigned c = ((unsigned)bitmap[j*lineWidth+i+2] << 16) | ((unsigned)bitmap[j*lineWidth+i+1] << 8) | ((unsigned)bitmap[j*lineWidth+i]);
        i += 3;
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor((unsigned short)colorModel, c)));
        k++;
      }
    }
    else if (bpp == 32 && lineWidth >= 4)
    {
      while (i < lineWidth - 3 && k < width)
      {
        unsigned c = (bitmap[j*lineWidth+i+3] << 24) | (bitmap[j*lineWidth+i+2] << 16) | (bitmap[j*lineWidth+i+1] << 8) | (bitmap[j*lineWidth+i]);
        i += 4;
        writeU32(image, m_ps.getBMPColor(libcdr::CDRColor((unsigned short)colorModel, c)));
        k++;
      }
    }
    else
      storeBMP = false;
  }

  if (storeBMP)
  {
#if DUMP_IMAGE
    librevenge::RVNGString filename;
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
  librevenge::RVNGBinaryData image(&bitmap[0], bitmap.size());
#if DUMP_IMAGE
  librevenge::RVNGString filename;
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
                                             const std::vector<unsigned char> &charDescriptions, const std::map<unsigned, CDRStyle> &styleOverrides)
{
  if (data.empty() && styleOverrides.empty())
    return;

  unsigned char tmpCharDescription = 0;
  unsigned i = 0;
  unsigned j = 0;
  std::vector<unsigned char> tmpTextData;
  CDRStyle defaultCharStyle, tmpCharStyle;
  m_ps.getRecursedStyle(defaultCharStyle, styleId);

  CDRTextLine line;
  for (i=0, j=0; i<charDescriptions.size() && j<data.size(); ++i)
  {
    tmpCharStyle = defaultCharStyle;
    auto iter = styleOverrides.find(tmpCharDescription & 0xfe);
    if (iter != styleOverrides.end())
      tmpCharStyle.overrideStyle(iter->second);
    if (charDescriptions[i] != tmpCharDescription)
    {
      librevenge::RVNGString text;
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
    if ((tmpCharDescription & 0x01) && (j < data.size()))
      tmpTextData.push_back(data[j++]);
  }
  librevenge::RVNGString text;
  if (!tmpTextData.empty())
  {
    if (tmpCharDescription & 0x01)
      appendCharacters(text, tmpTextData);
    else
      appendCharacters(text, tmpTextData, tmpCharStyle.m_charSet);
  }
  line.append(CDRText(text, tmpCharStyle));
  CDR_DEBUG_MSG(("CDRStylesCollector::collectText - Text: %s\n", text.cstr()));

  std::vector<CDRTextLine> &paragraphVector = m_ps.m_texts[textId];
  paragraphVector.push_back(line);
}

void libcdr::CDRStylesCollector::collectStld(unsigned id, const CDRStyle &style)
{
  m_ps.m_styles[id] = style;
}

void libcdr::CDRStylesCollector::collectFillStyle(unsigned id, const CDRFillStyle &fillStyle)
{
  m_ps.m_fillStyles[id] = fillStyle;
}

void libcdr::CDRStylesCollector::collectLineStyle(unsigned id, const CDRLineStyle &lineStyle)
{
  m_ps.m_lineStyles[id] = lineStyle;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
