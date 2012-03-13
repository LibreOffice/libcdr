/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef __CMXDOCUMENT_H__
#define __CMXDOCUMENT_H__

#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>
#include "CDRStringVector.h"

class WPXInputStream;

namespace libcdr
{
class CMXDocument
{
public:

  static bool isSupported(WPXInputStream *input);

  static bool parse(WPXInputStream *input, libwpg::WPGPaintInterface *painter);

  static bool generateSVG(WPXInputStream *input, CDRStringVector &output);
};

} // namespace libcdr

#endif //  __CMXDOCUMENT_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
