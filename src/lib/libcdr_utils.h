/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LIBCDR_UTILS_H__
#define __LIBCDR_UTILS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <memory>
#include <vector>

#include <boost/cstdint.hpp>

#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CDR_EPSILON 1E-6
#define CDR_ALMOST_ZERO(m) (fabs(m) <= CDR_EPSILON)
#define CDR_ALMOST_EQUAL(m, n) CDR_ALMOST_ZERO(m-n)

#if defined(HAVE_FUNC_ATTRIBUTE_FORMAT)
#  define CDR_ATTRIBUTE_PRINTF(fmt, arg) __attribute__((__format__(__printf__, fmt, arg)))
#else
#  define CDR_ATTRIBUTE_PRINTF(fmt, arg)
#endif

#if defined(HAVE_CLANG_ATTRIBUTE_FALLTHROUGH)
#  define CDR_FALLTHROUGH [[clang::fallthrough]]
#elif defined(HAVE_GCC_ATTRIBUTE_FALLTHROUGH)
#  define CDR_FALLTHROUGH __attribute__((fallthrough))
#else
#  define CDR_FALLTHROUGH ((void) 0)
#endif

// do nothing with debug messages in a release compile
#ifdef DEBUG
namespace libcdr
{
void debugPrint(const char *format, ...) CDR_ATTRIBUTE_PRINTF(1, 2);
}
#define CDR_DEBUG_MSG(M) libcdr::debugPrint M
#define CDR_DEBUG(M) M
#else
#define CDR_DEBUG_MSG(M)
#define CDR_DEBUG(M)
#endif

namespace libcdr
{

struct CDRDummyDeleter
{
  void operator()(void *) const {}
};

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

uint8_t readU8(librevenge::RVNGInputStream *input, bool bigEndian=false);
uint16_t readU16(librevenge::RVNGInputStream *input, bool bigEndian=false);
uint32_t readU32(librevenge::RVNGInputStream *input, bool bigEndian=false);
uint64_t readU64(librevenge::RVNGInputStream *input, bool bigEndian=false);
int32_t readS32(librevenge::RVNGInputStream *input, bool bigEndian=false);
int16_t readS16(librevenge::RVNGInputStream *input, bool bigEndian=false);

double readDouble(librevenge::RVNGInputStream *input, bool bigEndian=false);

double readFixedPoint(librevenge::RVNGInputStream *input, bool bigEndian=false);

unsigned long getLength(librevenge::RVNGInputStream *input);
unsigned long getRemainingLength(librevenge::RVNGInputStream *input);

int cdr_round(double d);

void writeU16(librevenge::RVNGBinaryData &buffer, const int value);
void writeU32(librevenge::RVNGBinaryData &buffer, const int value);
void appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters, unsigned short charset);
void appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters);
void appendUTF8Characters(librevenge::RVNGString &text, std::vector<unsigned char> characters);

#ifdef DEBUG
const char *toFourCC(unsigned value, bool bigEndian=false);
#endif

class EndOfStreamException
{
};

class GenericException
{
};

class UnknownPrecisionException
{
};

class EncodingException
{
};

} // namespace libcdr

#endif // __LIBCDR_UTILS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
