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

#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <librevenge-stream/librevenge-stream.h>
#include "CDRTypes.h"
#include "CommonParser.h"

namespace libcdr
{

class CDRCollector;

struct CMXParserState
{
  CMXParserState() : m_colorPalette(), m_dashArrays() {}
  std::map<unsigned, CDRColor> m_colorPalette;
  std::map<unsigned, std::vector<unsigned> > m_dashArrays;
};

class CMXParser : protected CommonParser
{
public:
  explicit CMXParser(CDRCollector *collector, CMXParserState &parserState);
  virtual ~CMXParser();
  bool parseRecords(librevenge::RVNGInputStream *input, long size = -1, unsigned level = 0);

private:
  CMXParser();
  CMXParser(const CMXParser &);
  CMXParser &operator=(const CMXParser &);
  bool parseRecord(librevenge::RVNGInputStream *input, unsigned level = 0);
  void readRecord(unsigned fourCC, unsigned &length, librevenge::RVNGInputStream *input);

  void readCMXHeader(librevenge::RVNGInputStream *input);
  void readDisp(librevenge::RVNGInputStream *input, unsigned length);
  void readCcmm(librevenge::RVNGInputStream *input, long &recordEnd);
  void readPage(librevenge::RVNGInputStream *input, unsigned length);
  void readRclr(librevenge::RVNGInputStream *input);
  void readRdot(librevenge::RVNGInputStream *input);

  // Command readers
  void readBeginPage(librevenge::RVNGInputStream *input);
  void readBeginLayer(librevenge::RVNGInputStream *input);
  void readBeginGroup(librevenge::RVNGInputStream *input);
  void readPolyCurve(librevenge::RVNGInputStream *input);
  void readEllipse(librevenge::RVNGInputStream *input);
  void readRectangle(librevenge::RVNGInputStream *input);
  void readJumpAbsolute(librevenge::RVNGInputStream *input);

  // Types readers
  CDRTransform readMatrix(librevenge::RVNGInputStream *input);
  CDRBox readBBox(librevenge::RVNGInputStream *input);
  librevenge::RVNGString readString(librevenge::RVNGInputStream *input);
  bool readFill(librevenge::RVNGInputStream *input);

  // Complex types readers
  bool readRenderingAttributes(librevenge::RVNGInputStream *input);

  // Helper Functions
  CDRColor getPaletteColor(unsigned id);
  CDRColor readColor(librevenge::RVNGInputStream *input, unsigned char colorModel);

  bool m_bigEndian;
  unsigned short m_unit;
  double m_scale;
  double m_xmin, m_xmax, m_ymin, m_ymax;
  unsigned m_indexSectionOffset;
  unsigned m_infoSectionOffset;
  unsigned m_thumbnailOffset;
  unsigned m_fillIndex;
  unsigned m_nextInstructionOffset;
  CMXParserState &m_parserState;
};

} // namespace libcdr

#endif // __CMXPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
