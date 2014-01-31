/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <string.h>

#include <libcdr/libcdr.h>
#include <libwpd-stream/libwpd-stream.h>
#include <libwpd/libwpd.h>

class TextPainter : public libwpg::WPGPaintInterface
{
public:
  TextPainter();

  void startGraphics(const ::WPXPropertyList &) {}
  void endGraphics() {}
  void startLayer(const ::WPXPropertyList &) {}
  void endLayer() {}
  void startEmbeddedGraphics(const ::WPXPropertyList &) {}
  void endEmbeddedGraphics() {}

  void setStyle(const ::WPXPropertyList &, const ::WPXPropertyListVector &) {}

  void drawRectangle(const ::WPXPropertyList &) {}
  void drawEllipse(const ::WPXPropertyList &) {}
  void drawPolyline(const ::WPXPropertyListVector &) {}
  void drawPolygon(const ::WPXPropertyListVector &) {}
  void drawPath(const ::WPXPropertyListVector &) {}
  void drawGraphicObject(const ::WPXPropertyList &, const ::WPXBinaryData &) {}
  void startTextObject(const ::WPXPropertyList &, const ::WPXPropertyListVector &) {}
  void endTextObject() {}
  void startTextLine(const ::WPXPropertyList &) {}
  void endTextLine();
  void startTextSpan(const ::WPXPropertyList &) {}
  void endTextSpan() {}
  void insertText(const ::WPXString &str);
};

TextPainter::TextPainter(): libwpg::WPGPaintInterface()
{
}

void TextPainter::insertText(const ::WPXString &str)
{
  printf("%s", str.cstr());
}

void TextPainter::endTextLine()
{
  printf("\n");
}

namespace
{

int printUsage()
{
  printf("Usage: cmx2text [OPTION] <Corel Binary Metafile>\n");
  printf("\n");
  printf("Options:\n");
  printf("--help                Shows this help message\n");
  return -1;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
  if (argc < 2)
    return printUsage();

  char *file = 0;

  for (int i = 1; i < argc; i++)
  {
    if (!file && strncmp(argv[i], "--", 2))
      file = argv[i];
    else
      return printUsage();
  }

  if (!file)
    return printUsage();

  WPXFileStream input(file);

  if (!libcdr::CDRDocument::isSupported(&input))
  {
    fprintf(stderr, "ERROR: Unsupported file format (unsupported version) or file is encrypted!\n");
    return 1;
  }

  TextPainter painter;
  if (!libcdr::CDRDocument::parse(&input, &painter))
  {
    fprintf(stderr, "ERROR: Parsing of document failed!\n");
    return 1;
  }

  return 0;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
