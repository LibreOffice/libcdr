/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "libcdr_utils.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string.h>

#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/utf8.h>

#define CDR_NUM_ELEMENTS(array) sizeof(array)/sizeof(array[0])

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

namespace
{

static unsigned short getEncodingFromICUName(const char *name)
{
  // ANSI
  if (strcmp(name, "ISO-8859-1") == 0)
    return 0;
  if (strcmp(name, "windows-1252") == 0)
    return 0;
  // CENTRAL EUROPE
  if (strcmp(name, "ISO-8859-2") == 0)
    return 0xee;
  if (strcmp(name, "windows-1250") == 0)
    return 0xee;
  // RUSSIAN
  if (strcmp(name, "ISO-8859-5") == 0)
    return 0xcc;
  if (strcmp(name, "windows-1251") == 0)
    return 0xcc;
  if (strcmp(name, "KOI8-R") == 0)
    return 0xcc;
  // ARABIC
  if (strcmp(name, "ISO-8859-6") == 0)
    return 0xb2;
  if (strcmp(name, "windows-1256") == 0)
    return 0xb2;
  // TURKISH
  if (strcmp(name, "ISO-8859-9") == 0)
    return 0xa2;
  if (strcmp(name, "windows-1254") == 0)
    return 0xa2;
  // GREEK
  if (strcmp(name, "ISO-8859-7") == 0)
    return 0xa1;
  if (strcmp(name, "windows-1253") == 0)
    return 0xa1;
  // HEBREW
  if (strcmp(name, "ISO-8859-8") == 0)
    return 0xb1;
  if (strcmp(name, "windows-1255") == 0)
    return 0xb1;
  // JAPANESE
  if (strcmp(name, "Shift_JIS") == 0)
    return 0x80;
  if (strcmp(name, "ISO-2022-JP") == 0)
    return 0x80;
  if (strcmp(name, "EUC-JP") == 0)
    return 0x80;
  if (strcmp(name, "windows-932") == 0)
    return 0x80;
  // KOREAN
  if (strcmp(name, "ISO-2022-KR") == 0)
    return 0x81;
  if (strcmp(name, "EUC-KR") == 0)
    return 0x81;
  if (strcmp(name, "windows-949") == 0)
    return 0x81;
  // CHINESE SIMPLIFIED
  if (strcmp(name, "ISO-2022-CN") == 0)
    return 0x86;
  if (strcmp(name, "GB18030") == 0)
    return 0x86;
  if (strcmp(name, "windows-936") == 0)
    return 0x86;
  // CHINESE TRADITIONAL
  if (strcmp(name, "Big5") == 0)
    return 0x88;
  if (strcmp(name, "windows-950") == 0)
    return 0x88;

  return 0;
}

static unsigned short getEncoding(const unsigned char *buffer, unsigned long bufferLength)
{
  if (!buffer)
    return 0;
  UErrorCode status = U_ZERO_ERROR;
  UCharsetDetector *csd = nullptr;
  try
  {
    csd = ucsdet_open(&status);
    if (U_FAILURE(status) || !csd)
      return 0;
    ucsdet_enableInputFilter(csd, true);
    ucsdet_setText(csd, (const char *)buffer, (unsigned)bufferLength, &status);
    if (U_FAILURE(status))
      throw libcdr::EncodingException();
    const UCharsetMatch *csm = ucsdet_detect(csd, &status);
    if (U_FAILURE(status) || !csm)
      throw libcdr::EncodingException();
    const char *name = ucsdet_getName(csm, &status);
    if (U_FAILURE(status) || !name)
      throw libcdr::EncodingException();
    int32_t confidence = ucsdet_getConfidence(csm, &status);
    if (U_FAILURE(status))
      throw libcdr::EncodingException();
    CDR_DEBUG_MSG(("UCSDET: getEncoding name %s, confidence %i\n", name, confidence));
    unsigned short encoding = getEncodingFromICUName(name);
    ucsdet_close(csd);
    /* From ICU documentation
     * A confidence value of ten does have a general meaning - it is used
     * for charsets that can represent the input data, but for which there
     * is no other indication that suggests that the charset is the correct
     * one. Pure 7 bit ASCII data, for example, is compatible with a great
     * many charsets, most of which will appear as possible matches with
     * a confidence of 10.
     */
    if (confidence == 10)
      return 0;
    return encoding;
  }
  catch (const libcdr::EncodingException &)
  {
    ucsdet_close(csd);
    return 0;
  }
}

static void _appendUCS4(librevenge::RVNGString &text, UChar32 ucs4Character)
{
  // Convert carriage returns to new line characters
  // Writerperfect/LibreOffice will replace them by <text:line-break>
  if (ucs4Character == (UChar32) 0x0d)
    ucs4Character = (UChar32) '\n';

  unsigned char outbuf[U8_MAX_LENGTH+1];
  int i = 0;
  U8_APPEND_UNSAFE(&outbuf[0], i, ucs4Character);
  outbuf[i] = 0;

  text.append((char *)outbuf);
}

} // anonymous namespace

uint8_t libcdr::readU8(librevenge::RVNGInputStream *input, bool /* bigEndian */)
{
  if (!input || input->isEnd())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint8_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint8_t))
    return *(uint8_t const *)(p);
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

uint16_t libcdr::readU16(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (!input || input->isEnd())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint16_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint16_t))
  {
    if (bigEndian)
      return (uint16_t)(p[1]|((uint16_t)p[0]<<8));
    return (uint16_t)(p[0]|((uint16_t)p[1]<<8));
  }
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int16_t libcdr::readS16(librevenge::RVNGInputStream *input, bool bigEndian)
{
  return (int16_t)readU16(input, bigEndian);
}

uint32_t libcdr::readU32(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (!input || input->isEnd())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint32_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint32_t))
  {
    if (bigEndian)
      return (uint32_t)p[3]|((uint32_t)p[2]<<8)|((uint32_t)p[1]<<16)|((uint32_t)p[0]<<24);
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
  }
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

int32_t libcdr::readS32(librevenge::RVNGInputStream *input, bool bigEndian)
{
  return (int32_t)readU32(input, bigEndian);
}

uint64_t libcdr::readU64(librevenge::RVNGInputStream *input, bool bigEndian)
{
  if (!input || input->isEnd())
  {
    CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
    throw EndOfStreamException();
  }
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint64_t), numBytesRead);

  if (p && numBytesRead == sizeof(uint64_t))
  {
    if (bigEndian)
      return (uint64_t)p[7]|((uint64_t)p[6]<<8)|((uint64_t)p[5]<<16)|((uint64_t)p[4]<<24)|((uint64_t)p[3]<<32)|((uint64_t)p[2]<<40)|((uint64_t)p[1]<<48)|((uint64_t)p[0]<<56);
    return (uint64_t)p[0]|((uint64_t)p[1]<<8)|((uint64_t)p[2]<<16)|((uint64_t)p[3]<<24)|((uint64_t)p[4]<<32)|((uint64_t)p[5]<<40)|((uint64_t)p[6]<<48)|((uint64_t)p[7]<<56);
  }
  CDR_DEBUG_MSG(("Throwing EndOfStreamException\n"));
  throw EndOfStreamException();
}

double libcdr::readDouble(librevenge::RVNGInputStream *input, bool bigEndian)
{
  union
  {
    uint64_t u;
    double d;
  } tmpUnion;

  tmpUnion.u = readU64(input, bigEndian);

  return tmpUnion.d;
}

double libcdr::readFixedPoint(librevenge::RVNGInputStream *input, bool bigEndian)
{
  unsigned fixedPointNumber = readU32(input, bigEndian);
  auto fixedPointNumberIntegerPart = (short)((fixedPointNumber & 0xFFFF0000) >> 16);
  auto fixedPointNumberFractionalPart = (double)((double)(fixedPointNumber & 0x0000FFFF)/(double)0xFFFF);
  return ((double)fixedPointNumberIntegerPart + fixedPointNumberFractionalPart);
}

unsigned long libcdr::getLength(librevenge::RVNGInputStream *const input)
{
  if (!input)
    throw EndOfStreamException();

  const long orig = input->tell();
  long end = 0;

  if (input->seek(0, librevenge::RVNG_SEEK_END) == 0)
  {
    end = input->tell();
  }
  else
  {
    // RVNG_SEEK_END does not work. Use the harder way.
    if (input->seek(0, librevenge::RVNG_SEEK_SET) != 0)
      throw EndOfStreamException();
    while (!input->isEnd())
    {
      readU8(input);
      ++end;
    }
  }
  assert(end >= 0);

  if (input->seek(orig, librevenge::RVNG_SEEK_SET) != 0)
    throw EndOfStreamException();

  return static_cast<unsigned long>(end);
}

unsigned long libcdr::getRemainingLength(librevenge::RVNGInputStream *const input)
{
  return getLength(input) - static_cast<unsigned long>(input->tell());
}

int libcdr::cdr_round(double d)
{
  return (d>0) ? int(d+0.5) : int(d-0.5);
}

void libcdr::writeU16(librevenge::RVNGBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
}

void libcdr::writeU32(librevenge::RVNGBinaryData &buffer, const int value)
{
  buffer.append((unsigned char)(value & 0xFF));
  buffer.append((unsigned char)((value >> 8) & 0xFF));
  buffer.append((unsigned char)((value >> 16) & 0xFF));
  buffer.append((unsigned char)((value >> 24) & 0xFF));
}

void libcdr::appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters, unsigned short charset)
{
  if (characters.empty())
    return;
  static const UChar32 symbolmap [] =
  {
    0x0020, 0x0021, 0x2200, 0x0023, 0x2203, 0x0025, 0x0026, 0x220D, // 0x20 ..
    0x0028, 0x0029, 0x2217, 0x002B, 0x002C, 0x2212, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x2245, 0x0391, 0x0392, 0x03A7, 0x0394, 0x0395, 0x03A6, 0x0393,
    0x0397, 0x0399, 0x03D1, 0x039A, 0x039B, 0x039C, 0x039D, 0x039F,
    0x03A0, 0x0398, 0x03A1, 0x03A3, 0x03A4, 0x03A5, 0x03C2, 0x03A9,
    0x039E, 0x03A8, 0x0396, 0x005B, 0x2234, 0x005D, 0x22A5, 0x005F,
    0xF8E5, 0x03B1, 0x03B2, 0x03C7, 0x03B4, 0x03B5, 0x03C6, 0x03B3,
    0x03B7, 0x03B9, 0x03D5, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BF,
    0x03C0, 0x03B8, 0x03C1, 0x03C3, 0x03C4, 0x03C5, 0x03D6, 0x03C9,
    0x03BE, 0x03C8, 0x03B6, 0x007B, 0x007C, 0x007D, 0x223C, 0x0020, // .. 0x7F
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
    0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009E, 0x009f,
    0x20AC, 0x03D2, 0x2032, 0x2264, 0x2044, 0x221E, 0x0192, 0x2663, // 0xA0 ..
    0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192, 0x2193,
    0x00B0, 0x00B1, 0x2033, 0x2265, 0x00D7, 0x221D, 0x2202, 0x2022,
    0x00F7, 0x2260, 0x2261, 0x2248, 0x2026, 0x23D0, 0x23AF, 0x21B5,
    0x2135, 0x2111, 0x211C, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229,
    0x222A, 0x2283, 0x2287, 0x2284, 0x2282, 0x2286, 0x2208, 0x2209,
    0x2220, 0x2207, 0x00AE, 0x00A9, 0x2122, 0x220F, 0x221A, 0x22C5,
    0x00AC, 0x2227, 0x2228, 0x21D4, 0x21D0, 0x21D1, 0x21D2, 0x21D3,
    0x25CA, 0x3008, 0x00AE, 0x00A9, 0x2122, 0x2211, 0x239B, 0x239C,
    0x239D, 0x23A1, 0x23A2, 0x23A3, 0x23A7, 0x23A8, 0x23A9, 0x23AA,
    0xF8FF, 0x3009, 0x222B, 0x2320, 0x23AE, 0x2321, 0x239E, 0x239F,
    0x23A0, 0x23A4, 0x23A5, 0x23A6, 0x23AB, 0x23AC, 0x23AD, 0x0020  // .. 0xFE
  };

  if (!charset && !characters.empty())
    charset = getEncoding(&characters[0], characters.size());

  if (charset == 0x02) // SYMBOL
  {
    uint32_t ucs4Character = 0;
    for (std::vector<unsigned char>::const_iterator iter = characters.begin();
         iter != characters.end(); ++iter)
    {
      if (*iter < 0x20)
        ucs4Character = 0x20;
      else
        ucs4Character = symbolmap[*iter - 0x20];
      _appendUCS4(text, ucs4Character);
    }
  }
  else
  {
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = nullptr;
    switch (charset)
    {
    case 0x80: // SHIFTJIS
      conv = ucnv_open("windows-932", &status);
      break;
    case 0x81: // HANGUL
      conv = ucnv_open("windows-949", &status);
      break;
    case 0x86: // GB2312
      conv = ucnv_open("windows-936", &status);
      break;
    case 0x88: // CHINESEBIG5
      conv = ucnv_open("windows-950", &status);
      break;
    case 0xa1: // GREEEK
      conv = ucnv_open("windows-1253", &status);
      break;
    case 0xa2: // TURKISH
      conv = ucnv_open("windows-1254", &status);
      break;
    case 0xa3: // VIETNAMESE
      conv = ucnv_open("windows-1258", &status);
      break;
    case 0xb1: // HEBREW
      conv = ucnv_open("windows-1255", &status);
      break;
    case 0xb2: // ARABIC
      conv = ucnv_open("windows-1256", &status);
      break;
    case 0xba: // BALTIC
      conv = ucnv_open("windows-1257", &status);
      break;
    case 0xcc: // RUSSIAN
      conv = ucnv_open("windows-1251", &status);
      break;
    case 0xde: // THAI
      conv = ucnv_open("windows-874", &status);
      break;
    case 0xee: // CENTRAL EUROPE
      conv = ucnv_open("windows-1250", &status);
      break;
    default:
      conv = ucnv_open("windows-1252", &status);
      break;
    }
    if (U_SUCCESS(status) && conv)
    {
      const auto *src = (const char *)&characters[0];
      const char *srcLimit = (const char *)src + characters.size();
      while (src < srcLimit)
      {
        UChar32 ucs4Character = ucnv_getNextUChar(conv, &src, srcLimit, &status);
        if (U_SUCCESS(status) && U_IS_UNICODE_CHAR(ucs4Character))
          _appendUCS4(text, ucs4Character);
      }
    }
    if (conv)
      ucnv_close(conv);
  }
}

void libcdr::appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters)
{
  if (characters.empty())
    return;

  UErrorCode status = U_ZERO_ERROR;
  UConverter *conv = ucnv_open("UTF-16LE", &status);

  if (U_SUCCESS(status) && conv)
  {
    const auto *src = (const char *)&characters[0];
    const char *srcLimit = (const char *)src + characters.size();
    while (src < srcLimit)
    {
      UChar32 ucs4Character = ucnv_getNextUChar(conv, &src, srcLimit, &status);
      if (U_SUCCESS(status) && U_IS_UNICODE_CHAR(ucs4Character))
        _appendUCS4(text, ucs4Character);
    }
  }
  if (conv)
    ucnv_close(conv);
}

void libcdr::appendUTF8Characters(librevenge::RVNGString &text, std::vector<unsigned char> characters)
{
  if (characters.empty())
    return;

  for (std::vector<unsigned char>::const_iterator iter = characters.begin(); iter != characters.end(); ++iter)
    text.append((char)*iter);
}

#ifdef DEBUG

void libcdr::debugPrint(const char *const format, ...)
{
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}

const char *libcdr::toFourCC(unsigned value, bool bigEndian)
{
  static char sValue[5] = { 0, 0, 0, 0, 0 };
  if (bigEndian)
  {
    sValue[3] = (char)(value & 0xff);
    sValue[2] = (char)((value & 0xff00) >> 8);
    sValue[1] = (char)((value & 0xff0000) >> 16);
    sValue[0] = (char)((value & 0xff000000) >> 24);
  }
  else
  {
    sValue[0] = (char)(value & 0xff);
    sValue[1] = (char)((value & 0xff00) >> 8);
    sValue[2] = (char)((value & 0xff0000) >> 16);
    sValue[3] = (char)((value & 0xff000000) >> 24);
  }
  return sValue;
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
