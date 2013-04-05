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
 * Copyright (C) 2011 Fridrich Strba <fridrich.strba@bluewin.ch>
 * Copyright (C) 2011 Eilidh McAdam <tibbylickle@gmail.com>
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

#ifndef __LIBCDR_UTILS_H__
#define __LIBCDR_UTILS_H__

#include <stdio.h>
#include <string>
#include <math.h>
#include <vector>
#include <libwpd-stream/libwpd-stream.h>
#include <libwpd/libwpd.h>

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

uint8_t readU8(WPXInputStream *input, bool bigEndian=false);
uint16_t readU16(WPXInputStream *input, bool bigEndian=false);
uint32_t readU32(WPXInputStream *input, bool bigEndian=false);
uint64_t readU64(WPXInputStream *input, bool bigEndian=false);
int32_t readS32(WPXInputStream *input, bool bigEndian=false);
int16_t readS16(WPXInputStream *input, bool bigEndian=false);

double readDouble(WPXInputStream *input, bool bigEndian=false);

double readFixedPoint(WPXInputStream *input, bool bigEndian=false);

int cdr_round(double d);

void writeU8(WPXBinaryData &buffer, const int value);
void writeU16(WPXBinaryData &buffer, const int value);
void writeU32(WPXBinaryData &buffer, const int value);
void appendCharacters(WPXString &text, std::vector<unsigned char> characters, unsigned short charset);
void appendCharacters(WPXString &text, std::vector<unsigned char> characters);

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
