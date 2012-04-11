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
#include <string.h>
#include "CDRCollector.h"

libcdr::CDRParserState::CDRParserState()
  : m_pageWidth(8.5), m_pageHeight(11.0), m_pageOffsetX(-4.25), m_pageOffsetY(-5.5),
    m_fillStyles(), m_lineStyles(), m_bmps(), m_patterns(),
    m_colorTransformCMYK2RGB(0), m_colorTransformLab2RGB(0), m_colorTransformRGB2RGB(0)
{
  cmsHPROFILE tmpRGBProfile = cmsCreate_sRGBProfile();
  m_colorTransformRGB2RGB = cmsCreateTransform(tmpRGBProfile, TYPE_RGB_8, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  cmsHPROFILE tmpCMYKProfile = cmsOpenProfileFromMem(SWOP_icc, sizeof(SWOP_icc)/sizeof(SWOP_icc[0]));
  m_colorTransformCMYK2RGB = cmsCreateTransform(tmpCMYKProfile, TYPE_CMYK_DBL, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  cmsHPROFILE tmpLabProfile = cmsCreateLab4Profile(0);
  m_colorTransformLab2RGB = cmsCreateTransform(tmpLabProfile, TYPE_Lab_DBL, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  cmsCloseProfile(tmpLabProfile);
  cmsCloseProfile(tmpCMYKProfile);
  cmsCloseProfile(tmpRGBProfile);
}

libcdr::CDRParserState::~CDRParserState()
{
  if (m_colorTransformCMYK2RGB)
    cmsDeleteTransform(m_colorTransformCMYK2RGB);
  if (m_colorTransformLab2RGB)
    cmsDeleteTransform(m_colorTransformLab2RGB);
  if (m_colorTransformRGB2RGB)
    cmsDeleteTransform(m_colorTransformRGB2RGB);
}

void libcdr::CDRParserState::setColorTransform(const std::vector<unsigned char> &profile)
{
  if (profile.empty())
    return;
  cmsHPROFILE tmpProfile = cmsOpenProfileFromMem(&profile[0], profile.size());
  cmsHPROFILE tmpRGBProfile = cmsCreate_sRGBProfile();
  cmsColorSpaceSignature signature = cmsGetColorSpace(tmpProfile);
  switch (signature)
  {
  case cmsSigCmykData:
  {
    if (m_colorTransformCMYK2RGB)
      cmsDeleteTransform(m_colorTransformCMYK2RGB);
    m_colorTransformCMYK2RGB = cmsCreateTransform(tmpProfile, TYPE_CMYK_DBL, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  }
  break;
  case cmsSigRgbData:
  {
    if (m_colorTransformRGB2RGB)
      cmsDeleteTransform(m_colorTransformRGB2RGB);
    m_colorTransformRGB2RGB = cmsCreateTransform(tmpProfile, TYPE_RGB_8, tmpRGBProfile, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
  }
  break;
  default:
    break;
  }
  cmsCloseProfile(tmpProfile);
  cmsCloseProfile(tmpRGBProfile);
}

void libcdr::CDRParserState::setColorTransform(WPXInputStream *input)
{
  if (!input)
    return;
  unsigned long numBytesRead = 0;
  const unsigned char *tmpProfile = input->read((unsigned long)-1, numBytesRead);
  if (!numBytesRead)
    return;
  std::vector<unsigned char> profile(numBytesRead);
  memcpy(&profile[0], tmpProfile, numBytesRead);
  setColorTransform(profile);
}

unsigned libcdr::CDRParserState::getBMPColor(const CDRColor &color)
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

unsigned libcdr::CDRParserState::_getRGBColor(const CDRColor &color)
{
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char col0 = color.m_colorValue & 0xff;
  unsigned char col1 = (color.m_colorValue & 0xff00) >> 8;
  unsigned char col2 = (color.m_colorValue & 0xff0000) >> 16;
  unsigned char col3 = (color.m_colorValue & 0xff000000) >> 24;
  if (color.m_colorModel == 0x01 || color.m_colorModel == 0x02) // CMYK100
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
  else if (color.m_colorModel == 0x03 || color.m_colorModel == 0x11) // CMYK255
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
    unsigned char input[3] = { col2, col1, col0 };
    unsigned char output[3] = { 0, 0, 0 };
    cmsDoTransform(m_colorTransformRGB2RGB, input, output, 1);
    red = output[0];
    green = output[1];
    blue = output[2];
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
    unsigned char HKS_red [] =
    {
      0xff, 0xe3, 0x00, 0x00, 0xff, 0x8f, 0x00, 0x00,
      0xff, 0x9b, 0x1d, 0x00, 0xe2, 0x1f, 0x33, 0x00,
      0x78, 0x89, 0x3a, 0x00, 0xca, 0x22, 0x6f, 0x00,
      0xb2, 0x34, 0x86, 0x00, 0xb0, 0x3b, 0x8e, 0x00,
      0x54, 0x3c, 0xcb, 0x00, 0x28, 0x53, 0xd2, 0x00,
      0x55, 0x96, 0xd3, 0x00, 0x00, 0xd2, 0xa0, 0x00,
      0x00, 0x98, 0x55, 0x00, 0x00, 0x6a, 0x7d, 0x00,
      0x2a, 0x6a, 0x40, 0x00, 0x46, 0xc6, 0x0d, 0x00,
      0xea, 0xa9, 0x00, 0x00, 0x92, 0x6d, 0x2b, 0x00,
      0x7a, 0x5e, 0x1f, 0x00, 0x66, 0x22, 0x8d, 0x00,
      0xad, 0x80, 0x59, 0x00, 0x83, 0x41
    };

    unsigned char HKS_green [] =
    {
      0xff, 0xe3, 0x00, 0x00, 0xff, 0x8f, 0x00, 0x00,
      0xff, 0x9b, 0x1d, 0x00, 0xe2, 0x1f, 0x33, 0x00,
      0x78, 0x89, 0x3a, 0x00, 0xca, 0x22, 0x6f, 0x00,
      0xb2, 0x34, 0x86, 0x00, 0xb0, 0x3b, 0x8e, 0x00,
      0x54, 0x3c, 0xcb, 0x00, 0x28, 0x53, 0xd2, 0x00,
      0x55, 0x96, 0xd3, 0x00, 0x00, 0xd2, 0xa0, 0x00,
      0x00, 0x98, 0x55, 0x00, 0x00, 0x6a, 0x7d, 0x00,
      0x2a, 0x6a, 0x40, 0x00, 0x46, 0xc6, 0x0d, 0x00,
      0xea, 0xa9, 0x00, 0x00, 0x92, 0x6d, 0x2b, 0x00,
      0x7a, 0x5e, 0x1f, 0x00, 0x66, 0x22, 0x8d, 0x00,
      0xad, 0x80, 0x59, 0x00, 0x83, 0x41
    };

    unsigned char HKS_blue [] =
    {
      0xff, 0xe3, 0x00, 0x00, 0xff, 0x8f, 0x00, 0x00,
      0xff, 0x9b, 0x1d, 0x00, 0xe2, 0x1f, 0x33, 0x00,
      0x78, 0x89, 0x3a, 0x00, 0xca, 0x22, 0x6f, 0x00,
      0xb2, 0x34, 0x86, 0x00, 0xb0, 0x3b, 0x8e, 0x00,
      0x54, 0x3c, 0xcb, 0x00, 0x28, 0x53, 0xd2, 0x00,
      0x55, 0x96, 0xd3, 0x00, 0x00, 0xd2, 0xa0, 0x00,
      0x00, 0x98, 0x55, 0x00, 0x00, 0x6a, 0x7d, 0x00,
      0x2a, 0x6a, 0x40, 0x00, 0x46, 0xc6, 0x0d, 0x00,
      0xea, 0xa9, 0x00, 0x00, 0x92, 0x6d, 0x2b, 0x00,
      0x7a, 0x5e, 0x1f, 0x00, 0x66, 0x22, 0x8d, 0x00,
      0xad, 0x80, 0x59, 0x00, 0x83, 0x41
    };

    unsigned short hks = (unsigned short)(color.m_colorValue & 0xffff)+85;
    unsigned hksIndex = hks % 86;
    hks /= 86;
    unsigned blackPercent = hks/10;
    switch (blackPercent)
    {
    case 2:
      blackPercent = 10;
      break;
    case 3:
      blackPercent = 30;
      break;
    case 4:
      blackPercent = 50;
      break;
    default:
      blackPercent = 0;
      break;
    }
    unsigned colorPercent = (hks % 10) ? (hks % 10) * 10 : 100;
    // try to avoid overflow due to rounding issues, so use first wider integer types and later, fill the unsigned char properly
    unsigned tmpRed = cdr_round((double)(1.0 - (double)blackPercent/100.0)*(255.0*(1.0 - (double)colorPercent/100.0) + HKS_red[hksIndex]*(double)colorPercent/100.0));
    unsigned tmpGreen = cdr_round((double)(1.0 - (double)blackPercent/100.0)*(255.0*(1.0 - (double)colorPercent/100.0) + HKS_green[hksIndex]*(double)colorPercent/100.0));
    unsigned tmpBlue = cdr_round((double)(1.0 - (double)blackPercent/100.0)*(255.0*(1.0 - (double)colorPercent/100.0) + HKS_blue[hksIndex]*(double)colorPercent/100.0));
    red = (tmpRed < 255 ? (unsigned char)tmpRed : 255);
    green = (tmpGreen < 255 ? (unsigned char)tmpGreen : 255);
    blue = (tmpBlue < 255 ? (unsigned char)tmpBlue : 255);

  }
  return (unsigned)((red << 16) | (green << 8) | blue);
}

WPXString libcdr::CDRParserState::getRGBColorString(const libcdr::CDRColor &color)
{
  WPXString tempString;
  tempString.sprintf("#%.6x", _getRGBColor(color));
  return tempString;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
