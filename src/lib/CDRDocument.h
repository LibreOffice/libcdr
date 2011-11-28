/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libcdr
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
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

#ifndef __CDRRAPHICS_H__
#define __CDRRAPHICS_H__

#include <libwpd/WPXString.h>

class WPXInputStream;

namespace libcdr
{
enum CDRFileFormat { CDR_AUTODETECT = 0, CDR_CDR1, CDR_CDR2 };

class CDRPaintInterface;

class CDRraphics
{
public:

	static bool isSupported(WPXInputStream *input);

	static bool parse(WPXInputStream *input, CDRPaintInterface *painter, CDRFileFormat fileFormat = CDR_AUTODETECT);
	static bool parse(const unsigned char *data, unsigned long size, CDRPaintInterface *painter, CDRFileFormat fileFormat = CDR_AUTODETECT);

	static bool generateSVG(WPXInputStream *input, WPXString &output, CDRFileFormat fileFormat = CDR_AUTODETECT);
	static bool generateSVG(const unsigned char *data, unsigned long size, WPXString &output, CDRFileFormat fileFormat = CDR_AUTODETECT);
};

} // namespace libcdr

#endif //  __CDRRAPHICS_H__
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
