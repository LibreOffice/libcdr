/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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

#include "CDRraphics.h"
#include "CDRHeader.h"
#include "CDRXParser.h"
#include "CDR1Parser.h"
#include "CDR2Parser.h"
#include "libcdr_utils.h"
#include "CDRSVGGenerator.h"
#include "CDRInternalStream.h"
#include "CDRPaintInterface.h"
#include <sstream>

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\return A value that indicates whether the content from the input
stream is a WordPerfect Graphics that libcdr is able to parse
*/
bool libcdr::CDRraphics::isSupported(WPXInputStream *input)
{
	WPXInputStream *graphics = 0;
	bool isDocumentOLE = false;

	if (input->isOLEStream())
	{
		graphics = input->getDocumentOLEStream("PerfectOffice_MAIN");
		if (graphics)
			isDocumentOLE = true;
		else
			return false;
	}
	else
		graphics = input;

	graphics->seek(0, WPX_SEEK_SET);

	CDRHeader header;
	if(!header.load(graphics))
	{
		if (graphics && isDocumentOLE)
			delete graphics;
		return false;
	}

	bool retVal = header.isSupported();

	if (graphics && isDocumentOLE)
		delete graphics;
	return retVal;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
CDRPaintInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param painter A CDRPainterInterface implementation
\return A value that indicates whether the parsing was successful
*/
bool libcdr::CDRraphics::parse(::WPXInputStream *input, libcdr::CDRPaintInterface *painter, libcdr::CDRFileFormat fileFormat)
{
	CDRXParser *parser = 0;

	WPXInputStream *graphics = 0;
	bool isDocumentOLE = false;

	if (input->isOLEStream())
	{
		graphics = input->getDocumentOLEStream("PerfectOffice_MAIN");
		if (graphics)
			isDocumentOLE = true;
		else
			return false;
	}
	else
		graphics = input;

	graphics->seek(0, WPX_SEEK_SET);

	CDR_DEBUG_MSG(("Loading header...\n"));
	unsigned char tmpMajorVersion = 0x00;
	if (fileFormat == CDR_CDR1)
		tmpMajorVersion = 0x01;
	else if (fileFormat == CDR_CDR2)
		tmpMajorVersion = 0x02;
	CDRHeader header;
	if(!header.load(graphics))
	{
		if (graphics && isDocumentOLE)
			delete graphics;
		return false;
	}

	if(!header.isSupported() && (fileFormat == CDR_AUTODETECT))
	{
		CDR_DEBUG_MSG(("Unsupported file format!\n"));
		if (graphics && isDocumentOLE)
			delete graphics;
		return false;
	}
	else if (header.isSupported())
	{
		// seek to the start of document
		graphics->seek(header.startOfDocument(), WPX_SEEK_SET);
		tmpMajorVersion = (unsigned char)(header.majorVersion());
		if (tmpMajorVersion == 0x01)
		{
			unsigned long returnPosition = header.startOfDocument();
			/* Due to a bug in dumping mechanism, we produced
			 * invalid CDR files by prepending a CDR1 header
			 * to a valid WP file. Let us check this kind of files,
			 * so that we can load our own mess too. */
			if (header.load(graphics) && header.isSupported())
			{
				CDR_DEBUG_MSG(("An invalid graphics we produced :(\n"));
				graphics->seek(header.startOfDocument() + 16, WPX_SEEK_SET);
				tmpMajorVersion = (unsigned char)(header.majorVersion());
			}
			else
				graphics->seek(returnPosition, WPX_SEEK_SET);

		}
	}
	else
		// here we force parsing of headerless pictures
		graphics->seek(0, WPX_SEEK_SET);

	bool retval;
	switch (tmpMajorVersion)
	{
	case 0x01: // CDR1
		CDR_DEBUG_MSG(("Parsing CDR1\n"));
		parser = new CDR1Parser(graphics, painter);
		retval = parser->parse();
		break;
	case 0x02: // CDR2
		CDR_DEBUG_MSG(("Parsing CDR2\n"));
		parser = new CDR2Parser(graphics, painter);
		retval = parser->parse();
		break;
	default: // other :-)
		CDR_DEBUG_MSG(("Unknown format\n"));
		if (graphics && isDocumentOLE)
			delete graphics;
		return false;
	}

	if (parser)
		delete parser;
	if (graphics && isDocumentOLE)
		delete graphics;

	return retval;
}

bool libcdr::CDRraphics::parse(const unsigned char *data, unsigned long size, libcdr::CDRPaintInterface *painter, libcdr::CDRFileFormat fileFormat)
{
	CDRInternalInputStream tmpStream(data, size);
	return libcdr::CDRraphics::parse(&tmpStream, painter, fileFormat);
}
/**
Parses the input stream content and generates a valid Scalable Vector Graphics
Provided as a convenience function for applications that support SVG internally.
\param input The input stream
\param output The output string whose content is the resulting SVG
\return A value that indicates whether the SVG generation was successful.
*/
bool libcdr::CDRraphics::generateSVG(::WPXInputStream *input, WPXString &output, libcdr::CDRFileFormat fileFormat)
{
	std::ostringstream tmpOutputStream;
	libcdr::CDRSVGGenerator generator(tmpOutputStream);
	bool result = libcdr::CDRraphics::parse(input, &generator, fileFormat);
	if (result)
		output = WPXString(tmpOutputStream.str().c_str());
	else
		output = WPXString("");
	return result;
}

bool libcdr::CDRraphics::generateSVG(const unsigned char *data, unsigned long size, WPXString &output, libcdr::CDRFileFormat fileFormat)
{
	CDRInternalInputStream tmpStream(data, size);
	return libcdr::CDRraphics::generateSVG(&tmpStream, output, fileFormat);
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
