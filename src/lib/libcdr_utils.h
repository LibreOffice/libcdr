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

#include <stdio.h>
#include <string>
#include <math.h>
#include <vector>
#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CDR_EPSILON 1E-6
#define CDR_ALMOST_ZERO(m) (fabs(m) <= CDR_EPSILON)

#ifdef _MSC_VER

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned uint32_t;
typedef int int32_t;
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;

#else

#ifdef HAVE_CONFIG_H

#include <config.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#else

// assume that the headers are there inside LibreOffice build when no HAVE_CONFIG_H is defined
#include <stdint.h>
#include <inttypes.h>

#endif

#endif

// debug message includes source file and line number
//#define VERBOSE_DEBUG 1

// do nothing with debug messages in a release compile
#ifdef DEBUG
#ifdef VERBOSE_DEBUG
#define CDR_DEBUG_MSG(M) printf("%15s:%5d: ", __FILE__, __LINE__); printf M
#define CDR_DEBUG(M) M
#else
#define CDR_DEBUG_MSG(M) printf M
#define CDR_DEBUG(M) M
#endif
#else
#define CDR_DEBUG_MSG(M)
#define CDR_DEBUG(M)
#endif

namespace libcdr
{

uint8_t readU8(librevenge::RVNGInputStream *input, bool bigEndian=false);
uint16_t readU16(librevenge::RVNGInputStream *input, bool bigEndian=false);
uint32_t readU32(librevenge::RVNGInputStream *input, bool bigEndian=false);
uint64_t readU64(librevenge::RVNGInputStream *input, bool bigEndian=false);
int32_t readS32(librevenge::RVNGInputStream *input, bool bigEndian=false);
int16_t readS16(librevenge::RVNGInputStream *input, bool bigEndian=false);

double readDouble(librevenge::RVNGInputStream *input, bool bigEndian=false);

double readFixedPoint(librevenge::RVNGInputStream *input, bool bigEndian=false);

int cdr_round(double d);

void writeU16(librevenge::RVNGBinaryData &buffer, const int value);
void writeU32(librevenge::RVNGBinaryData &buffer, const int value);
void appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters, unsigned short charset);
void appendCharacters(librevenge::RVNGString &text, std::vector<unsigned char> characters);

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
