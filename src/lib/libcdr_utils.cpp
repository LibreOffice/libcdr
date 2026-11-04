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

#ifdef WITH_ICU
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/utf8.h>
#else
#include <cerrno>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>   // MultiByteToWideChar: Windows has no iconv
#include <cstdlib>
#else
#include <iconv.h>
#endif
#endif

#define CDR_NUM_ELEMENTS(array) sizeof(array)/sizeof(array[0])

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

namespace
{

#ifdef WITH_ICU

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

#else // WITH_ICU

// No charset given (or ANSI / DEFAULT, which old Hungarian files also use for Central European
// text): without ICU's charset detector, windows-1250 when the text has its o/u with double
// acute (0xd5 0xdb 0xf5 0xfb: Hungarian), else 1252. Not when it also has letters Hungarian never
// uses in 1250 but French / Portuguese do in 1252 (a grave or circumflex e, a grave a, u, i, o,
// an a with tilde: 1250's r with acute, c / e / n with caron...): then those bytes are 1252's
// O / U with tilde or circumflex (sur, aout, Portuguese o with tilde came out with o / u with
// double acute).
static unsigned short getEncoding(const unsigned char *buffer, unsigned long bufferLength)
{
  if (!buffer)
    return 0;
  bool doubleAcute = false;
  for (unsigned long i = 0; i < bufferLength; i++)
  {
    switch (buffer[i])
    {
    case 0xd5: case 0xdb: case 0xf5: case 0xfb:
      doubleAcute = true;
      break;
    case 0xc0: case 0xc3: case 0xc8: case 0xca: case 0xcc: case 0xd2: case 0xd9:
    case 0xe0: case 0xe3: case 0xe8: case 0xea: case 0xec: case 0xf2: case 0xf9:
      return 0;
    default:
      break;
    }
  }
  return doubleAcute ? 0xee : 0;
}

// Text from CorelDRAW for Mac (charset 0, Mac Central European code page): its Hungarian
// letters lie in 0x80..0x9f (á 0x87, é 0x8e, ö 0x9a...), where Windows code pages have only
// punctuation; text with Windows Hungarian letters stays Windows text
static bool isMacText(const unsigned char *buffer, unsigned long bufferLength)
{
  bool mac = false;
  for (unsigned long i = 0; i < bufferLength; i++)
  {
    switch (buffer[i])
    {
    case 0x83: case 0x85: case 0x86: case 0x87: case 0x8e: case 0x92: case 0x97: case 0x9a: case 0x9c: case 0x9f:
      mac = true;
      break;
    case 0xc1: case 0xc9: case 0xcd: case 0xd3: case 0xd5: case 0xd6: case 0xda: case 0xdb: case 0xdc:
    case 0xe1: case 0xe9: case 0xed: case 0xf3: case 0xf6: case 0xfa: case 0xfb: case 0xfc:
      return false;
    default:
      break;
    }
  }
  return mac;
}

static const uint32_t macCentralEurope[128] =   // 0x80..0xff
{
    0x00C4, 0x0100, 0x0101, 0x00C9, 0x0104, 0x00D6, 0x00DC, 0x00E1,
    0x0105, 0x010C, 0x00E4, 0x010D, 0x0106, 0x0107, 0x00E9, 0x0179,
    0x017A, 0x010E, 0x00ED, 0x010F, 0x0112, 0x0113, 0x0116, 0x00F3,
    0x0117, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x011A, 0x011B, 0x00FC,
    0x2020, 0x00B0, 0x0118, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
    0x00AE, 0x00A9, 0x2122, 0x0119, 0x00A8, 0x2260, 0x0123, 0x012E,
    0x012F, 0x012A, 0x2264, 0x2265, 0x012B, 0x0136, 0x2202, 0x2211,
    0x0142, 0x013B, 0x013C, 0x013D, 0x013E, 0x0139, 0x013A, 0x0145,
    0x0146, 0x0143, 0x00AC, 0x221A, 0x0144, 0x0147, 0x2206, 0x00AB,
    0x00BB, 0x2026, 0x00A0, 0x0148, 0x0150, 0x00D5, 0x0151, 0x014C,
    0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
    0x014D, 0x0154, 0x0155, 0x0158, 0x2039, 0x203A, 0x0159, 0x0156,
    0x0157, 0x0160, 0x201A, 0x201E, 0x0161, 0x015A, 0x015B, 0x00C1,
    0x0164, 0x0165, 0x00CD, 0x017D, 0x017E, 0x016A, 0x00D3, 0x00D4,
    0x016B, 0x016E, 0x00DA, 0x016F, 0x0170, 0x0171, 0x0172, 0x0173,
    0x00DD, 0x00FD, 0x0137, 0x017B, 0x0141, 0x017C, 0x0122, 0x02C7
};

static bool isUnicodeChar(uint32_t c)
{
  return c < 0x110000 && (c < 0xd800 || c > 0xdfff) && (c & 0xfffe) != 0xfffe && (c < 0xfdd0 || c > 0xfdef);
}

static void _appendUCS4(librevenge::RVNGString &text, uint32_t ucs4Character)
{
  // Convert carriage returns to new line characters
  // Writerperfect/LibreOffice will replace them by <text:line-break>
  if (ucs4Character == 0x0d)
    ucs4Character = '\n';

  unsigned char outbuf[5];
  int i = 0;
  if (ucs4Character < 0x80)
    outbuf[i++] = (unsigned char)ucs4Character;
  else if (ucs4Character < 0x800)
  {
    outbuf[i++] = (unsigned char)(0xc0 | (ucs4Character >> 6));
    outbuf[i++] = (unsigned char)(0x80 | (ucs4Character & 0x3f));
  }
  else if (ucs4Character < 0x10000)
  {
    outbuf[i++] = (unsigned char)(0xe0 | (ucs4Character >> 12));
    outbuf[i++] = (unsigned char)(0x80 | ((ucs4Character >> 6) & 0x3f));
    outbuf[i++] = (unsigned char)(0x80 | (ucs4Character & 0x3f));
  }
  else
  {
    outbuf[i++] = (unsigned char)(0xf0 | (ucs4Character >> 18));
    outbuf[i++] = (unsigned char)(0x80 | ((ucs4Character >> 12) & 0x3f));
    outbuf[i++] = (unsigned char)(0x80 | ((ucs4Character >> 6) & 0x3f));
    outbuf[i++] = (unsigned char)(0x80 | (ucs4Character & 0x3f));
  }
  outbuf[i] = 0;

  text.append((char *)outbuf);
}

// Converts bytes in a code page to UTF-8 with iconv (instead of ICU): characters it can't
// convert are skipped, as before.
static void _appendCodepage(librevenge::RVNGString &text, const std::vector<unsigned char> &characters, const char *codepage)
{
#ifdef _WIN32
  // Windows' own converter, the code page number from the "CPnnnn" name; characters it can't
  // convert (its replacement character) are skipped
  const unsigned cp = strncmp(codepage, "CP", 2) == 0 ? (unsigned)atoi(codepage + 2) : 0;
  if (!cp || characters.empty())
    return;
  const int n = MultiByteToWideChar(cp, 0, reinterpret_cast<const char *>(&characters[0]), (int)characters.size(), nullptr, 0);
  if (n <= 0)
    return;
  std::vector<wchar_t> wide((size_t)n);
  MultiByteToWideChar(cp, 0, reinterpret_cast<const char *>(&characters[0]), (int)characters.size(), &wide[0], n);
  for (size_t i = 0; i < wide.size(); ++i)
  {
    uint32_t c = (uint16_t)wide[i];
    if (c >= 0xd800 && c < 0xdc00 && i + 1 < wide.size() && (uint16_t)wide[i + 1] >= 0xdc00 && (uint16_t)wide[i + 1] < 0xe000)
    {
      c = SURROGATE_VALUE(c, (uint16_t)wide[i + 1]);
      ++i;
    }
    if (c != 0xfffd && isUnicodeChar(c))
      _appendUCS4(text, c);
  }
#else
  iconv_t cd = iconv_open("UTF-32LE", codepage);
  if (cd == (iconv_t)-1)
    return;
  auto *src = const_cast<char *>(reinterpret_cast<const char *>(&characters[0]));
  size_t srcLeft = characters.size();
  while (srcLeft)
  {
    unsigned char out[4 * 64];
    auto *dst = reinterpret_cast<char *>(out);
    size_t dstLeft = sizeof(out);
    size_t r = iconv(cd, &src, &srcLeft, &dst, &dstLeft);
    for (size_t k = 0; k + 4 <= sizeof(out) - dstLeft; k += 4)
    {
      uint32_t c = out[k] | (out[k + 1] << 8) | (out[k + 2] << 16) | ((uint32_t)out[k + 3] << 24);
      if (isUnicodeChar(c))
        _appendUCS4(text, c);
    }
    if (r == (size_t)-1 && errno != E2BIG)
    {
      if (errno == EINVAL)   // an incomplete sequence at the end
        break;
      src++;   // EILSEQ: skip the byte
      srcLeft--;
    }
  }
  iconv_close(cd);
#endif
}

#endif // WITH_ICU

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
  static const uint32_t symbolmap [] =
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
#ifdef WITH_ICU
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
#else
  // CorelDRAW for Mac text: its charsets are often wrong too (Hungarian text marked Baltic);
  // not tried for code pages with letters in 0x80..0x9f (Cyrillic, Arabic), none (Hebrew, Thai)
  // or two bytes a character (East Asian: Shift-JIS lead bytes are 0x81..0x9f, katakana came out
  // as Mac Central European letters)
  else if (charset != 0xcc && charset != 0xb2 && charset != 0xb1 && charset != 0xde &&
           charset != 0x80 && charset != 0x81 && charset != 0x86 && charset != 0x88 &&
           isMacText(&characters[0], characters.size()))
  {
    for (unsigned char c : characters)
      _appendUCS4(text, c < 0x80 ? c : macCentralEurope[c - 0x80]);
  }
  else
  {
    const char *codepage;
    switch (charset)
    {
    case 0x80: // SHIFTJIS
      codepage = "CP932";
      break;
    case 0x81: // HANGUL
      codepage = "CP949";
      break;
    case 0x86: // GB2312
      codepage = "CP936";
      break;
    case 0x88: // CHINESEBIG5
      codepage = "CP950";
      break;
    case 0xa1: // GREEEK
      codepage = "CP1253";
      break;
    case 0xa2: // TURKISH
      codepage = "CP1254";
      break;
    case 0xa3: // VIETNAMESE
      codepage = "CP1258";
      break;
    case 0xb1: // HEBREW
      codepage = "CP1255";
      break;
    case 0xb2: // ARABIC
      codepage = "CP1256";
      break;
    case 0xba: // BALTIC
      codepage = "CP1257";
      break;
    case 0xcc: // RUSSIAN
      codepage = "CP1251";
      break;
    case 0xde: // THAI
      codepage = "CP874";
      break;
    case 0xee: // CENTRAL EUROPE
      codepage = "CP1250";
      break;
    default:   // ANSI, DEFAULT and others: often Central European text (old Hungarian files)
      codepage = getEncoding(&characters[0], characters.size()) == 0xee ? "CP1250" : "CP1252";
      break;
    }
    _appendCodepage(text, characters, codepage);
  }
#endif
}

void libcdr::appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters)
{
  if (characters.empty())
    return;

#ifdef WITH_ICU
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
#else
  // UTF-16LE (decoded here; ICU did it before): unpaired surrogates are skipped
  for (size_t i = 0; i + 1 < characters.size(); i += 2)
  {
    uint32_t c = characters[i] | (characters[i + 1] << 8);
    if (c >= 0xd800 && c <= 0xdbff && i + 3 < characters.size())
    {
      uint32_t l = characters[i + 2] | (characters[i + 3] << 8);
      if (l >= 0xdc00 && l <= 0xdfff)
      {
        c = SURROGATE_VALUE(c, l);
        i += 2;
      }
    }
    if (isUnicodeChar(c))
      _appendUCS4(text, c);
  }
#endif
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
