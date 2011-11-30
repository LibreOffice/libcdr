/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* libcdr
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2007 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <sstream>
#include <string>
#include "CDRDocument.h"
#include "CDRParser.h"
#include "CDRSVGGenerator.h"
#include "libcdr_utils.h"

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\return A value that indicates whether the content from the input
stream is a Corel Draw Document that libcdr is able to parse
*/
bool libcdr::CDRDocument::isSupported(WPXInputStream * /* input */)
{
  return true;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
CDRPaintInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param painter A CDRPainterInterface implementation
\return A value that indicates whether the parsing was successful
*/
bool libcdr::CDRDocument::parse(::WPXInputStream *input, libwpg::WPGPaintInterface *painter)
{
  CDRParser parser(input, painter);
  return parser.parseRecords(input);
}

/**
Parses the input stream content and generates a valid Scalable Vector Graphics
Provided as a convenience function for applications that support SVG internally.
\param input The input stream
\param output The output string whose content is the resulting SVG
\return A value that indicates whether the SVG generation was successful.
*/
bool libcdr::CDRDocument::generateSVG(::WPXInputStream *input, WPXString &output)
{
  std::ostringstream tmpOutputStream;
  libcdr::CDRSVGGenerator generator(tmpOutputStream);
  bool result = libcdr::CDRDocument::parse(input, &generator);
  if (result)
    output = WPXString(tmpOutputStream.str().c_str());
  else
    output = WPXString("");
  return result;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
