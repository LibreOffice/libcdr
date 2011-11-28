/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* libcdr
 * Copyright (C) 2004 Marc Oude Kotte (marc@solcon.nl)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02111-1301 USA
 *
 * For further information visit http://libcdr.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef __LIBCDR_UTILS_H__
#define __LIBCDR_UTILS_H__

#include <stdio.h>

//#define DEBUG // FIXME !

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

#endif // __LIBCDR_UTILS_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
