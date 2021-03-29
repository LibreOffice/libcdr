/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CMXPARSER_H__
#define __CMXPARSER_H__

#include <vector>
#include <map>
#include <memory>

#include <librevenge-stream/librevenge-stream.h>

#include "CDRTransforms.h"
#include "CDRTypes.h"
#include "CommonParser.h"

#define CMX_MASTER_INDEX_TABLE 1
#define CMX_PAGE_INDEX_TABLE 2
#define CMX_MASTER_LAYER_TABLE 3
#define CMX_PROCEDURE_INDEX_TABLE 4
#define CMX_BITMAP_INDEX_TABLE 5
#define CMX_ARROW_INDEX_TABLE 6
#define CMX_FONT_INDEX_TABLE 7
#define CMX_EMBEDDED_FILE_INDEX_TABLE 8
#define CMX_THUMBNAIL_SECTION 10
#define CMX_OUTLINE_DESCRIPTION_SECTION 15
#define CMX_LINE_STYLE_DESCRIPTION_SECTION 16
#define CMX_ARROWHEADS_DESCRIPTION_SECTION 17
#define CMX_SCREEN_DESCRIPTION_SECTION 18
#define CMX_PEN_DESCRIPTION_SECTION 19
#define CMX_DOT_DASH_DESCRIPTION_SECTION 20
#define CMX_COLOR_DESCRIPTION_SECTION 21
#define CMX_COLOR_CORRECTION_SECTION 22
#define CMX_PREVIEW_BOX_SECTION 23

namespace libcdr
{

class CDRCollector;

struct CMXOutline
{
  CMXOutline()
    : m_lineStyle(0), m_screen(0), m_color(0),
      m_arrowHeads(0), m_pen(0), m_dashArray(0) {}
  unsigned short m_lineStyle;
  unsigned short m_screen;
  unsigned short m_color;
  unsigned short m_arrowHeads;
  unsigned short m_pen;
  unsigned short m_dashArray;
};

struct CMXPen
{
  CMXPen()
    : m_width(0.0), m_aspect(1.0), m_angle(0.0), m_matrix() {}
  double m_width;
  double m_aspect;
  double m_angle;
  CDRTransform m_matrix;
};

struct CMXLineStyle
{
  CMXLineStyle() : m_spec(0), m_capAndJoin(0) {}
  unsigned char m_spec;
  unsigned char m_capAndJoin;
};

struct CMXImageInfo
{
  CMXImageInfo()
    : m_type(0), m_compression(0), m_size(0), m_compressedSize(0) {}
  unsigned short m_type;
  unsigned short m_compression;
  unsigned m_size;
  unsigned m_compressedSize;
};

struct CMXParserState
{
  CMXParserState()
    : m_colorPalette(), m_dashArrays(), m_lineStyles(),
      m_pens(), m_outlines(), m_bitmapOffsets(), m_patternOffsets(),
      m_arrowOffsets(), m_embeddedOffsets(), m_embeddedOffsetTypes() {}
  std::map<unsigned, CDRColor> m_colorPalette;
  std::map<unsigned, std::vector<unsigned> > m_dashArrays;
  std::map<unsigned, CMXLineStyle> m_lineStyles;
  std::map<unsigned, CMXPen> m_pens;
  std::map<unsigned, CMXOutline> m_outlines;
  std::map<unsigned, unsigned> m_bitmapOffsets;
  std::map<unsigned, unsigned> m_patternOffsets;
  std::map<unsigned, unsigned> m_arrowOffsets;
  std::map<unsigned, unsigned> m_embeddedOffsets;
  std::map<unsigned, unsigned> m_embeddedOffsetTypes;
};

class CMXParser : protected CommonParser
{
public:
  explicit CMXParser(CDRCollector *collector, CMXParserState &parserState);
  ~CMXParser() override;
  bool parseRecords(librevenge::RVNGInputStream *input, long size = -1, unsigned level = 0);

private:
  CMXParser();
  CMXParser(const CMXParser &);
  CMXParser &operator=(const CMXParser &);
  bool parseRecord(librevenge::RVNGInputStream *input, unsigned level = 0);
  void readRecord(unsigned fourCC, unsigned long length, librevenge::RVNGInputStream *input);
  void parseImage(librevenge::RVNGInputStream *input);

  void readCMXHeader(librevenge::RVNGInputStream *input);
  void readDisp(librevenge::RVNGInputStream *input);
  void readPage(librevenge::RVNGInputStream *input);
  void readProc(librevenge::RVNGInputStream *input);
  void readRclr(librevenge::RVNGInputStream *input);
  void readRotl(librevenge::RVNGInputStream *input);
  void readRott(librevenge::RVNGInputStream *input);
  void readRdot(librevenge::RVNGInputStream *input);
  void readRpen(librevenge::RVNGInputStream *input);
  void readIxtl(librevenge::RVNGInputStream *input);
  void readIxef(librevenge::RVNGInputStream *input);
  void readIxmr(librevenge::RVNGInputStream *input);
  void readIxpg(librevenge::RVNGInputStream *input);
  void readIxpc(librevenge::RVNGInputStream *input);
  void readInfo(librevenge::RVNGInputStream *input);
  void readData(librevenge::RVNGInputStream *input);

  // Command readers
  void readCommands(librevenge::RVNGInputStream *input, unsigned length);
  void readBeginPage(librevenge::RVNGInputStream *input);
  void readBeginLayer(librevenge::RVNGInputStream *input);
  void readBeginGroup(librevenge::RVNGInputStream *input);
  void readPolyCurve(librevenge::RVNGInputStream *input);
  void readEllipse(librevenge::RVNGInputStream *input);
  void readRectangle(librevenge::RVNGInputStream *input);
  void readJumpAbsolute(librevenge::RVNGInputStream *input);
  void readDrawImage(librevenge::RVNGInputStream *input);
  void readBeginProcedure(librevenge::RVNGInputStream *input);

  // Types readers
  CDRTransform readMatrix(librevenge::RVNGInputStream *input);
  CDRBox readBBox(librevenge::RVNGInputStream *input);
  librevenge::RVNGString readString(librevenge::RVNGInputStream *input);
  bool readFill(librevenge::RVNGInputStream *input);
  bool readLens(librevenge::RVNGInputStream *input);

  // Complex types readers
  bool readRenderingAttributes(librevenge::RVNGInputStream *input);

  // Helper Functions
  CDRColor getPaletteColor(unsigned id);
  CDRColor readColor(librevenge::RVNGInputStream *input, unsigned char colorModel);
  CDRLineStyle getLineStyle(unsigned id);
  const unsigned *_getOffsetByType(unsigned short type, const std::map<unsigned short, unsigned> &offsets);

  bool m_bigEndian;
  unsigned short m_unit;
  double m_scale;
  double m_xmin, m_xmax, m_ymin, m_ymax;
  unsigned m_fillIndex;
  unsigned long m_nextInstructionOffset;
  CMXParserState &m_parserState;
  CMXImageInfo m_currentImageInfo;
  std::unique_ptr<CDRPattern> m_currentPattern;
  std::unique_ptr<CDRBitmap> m_currentBitmap;
};

} // namespace libcdr

#endif // __CMXPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
