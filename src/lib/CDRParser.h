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

#ifndef __CDRPARSER_H__
#define __CDRPARSER_H__

#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <libwpd-stream/libwpd-stream.h>
#include "CDRTypes.h"
#include "CommonParser.h"

namespace libcdr
{

class CDRCollector;

class CDRParser : protected CommonParser
{
public:
  explicit CDRParser(const std::vector<WPXInputStream *> &externalStreams, CDRCollector *collector);
  virtual ~CDRParser();
  bool parseRecords(WPXInputStream *input, unsigned *blockLengths = 0, unsigned level = 0);
  bool parseWaldo(WPXInputStream *input);

private:
  CDRParser();
  CDRParser(const CDRParser &);
  CDRParser &operator=(const CDRParser &);
  bool parseWaldoStructure(WPXInputStream *input, std::stack<WaldoRecordType1> &waldoStack,
                           const std::map<unsigned, WaldoRecordType1> &records1,
                           std::map<unsigned, WaldoRecordInfo> &records2);
  bool gatherWaldoInformation(WPXInputStream *input, std::vector<WaldoRecordInfo> &records, std::map<unsigned, WaldoRecordInfo> &records2,
                              std::map<unsigned, WaldoRecordInfo> &records3, std::map<unsigned, WaldoRecordInfo> &records4,
                              std::map<unsigned, WaldoRecordInfo> &records6, std::map<unsigned, WaldoRecordInfo> &records7,
                              std::map<unsigned, WaldoRecordInfo> &records8, std::map<unsigned, WaldoRecordInfo> recordsOther);
  void readWaldoRecord(WPXInputStream *input, const WaldoRecordInfo &info);
  bool parseRecord(WPXInputStream *input, unsigned *blockLengths = 0, unsigned level = 0);
  void readRecord(unsigned fourCC, unsigned length, WPXInputStream *input);
  double readRectCoord(WPXInputStream *input);
  CDRColor readColor(WPXInputStream *input);

  void readRectangle(WPXInputStream *input);
  void readEllipse(WPXInputStream *input);
  void readLineAndCurve(WPXInputStream *input);
  void readBitmap(WPXInputStream *input);
  void readPageSize(WPXInputStream *input);
  void readWaldoBmp(WPXInputStream *input, unsigned length, unsigned id);
  void readWaldoBmpf(WPXInputStream *input, unsigned id);
  void readWaldoTrfd(WPXInputStream *input);
  void readWaldoOutl(WPXInputStream *input);
  void readWaldoFill(WPXInputStream *input);
  void readWaldoLoda(WPXInputStream *input, unsigned length);
  void readOpacity(WPXInputStream *input, unsigned length);
  void readTrfd(WPXInputStream *input, unsigned length);
  void readFild(WPXInputStream *input, unsigned length);
  void readOutl(WPXInputStream *input, unsigned length);
  void readLoda(WPXInputStream *input, unsigned length);
  void readFlags(WPXInputStream *input, unsigned length);
  void readMcfg(WPXInputStream *input, unsigned length);
  void readPath(WPXInputStream *input);
  void readPolygonCoords(WPXInputStream *input);
  void readPolygonTransform(WPXInputStream *input);
  void readBmp(WPXInputStream *input, unsigned length);
  void readBmpf(WPXInputStream *input, unsigned length);
  void readPpdt(WPXInputStream *input, unsigned length);
  void readFtil(WPXInputStream *input, unsigned length);
  void readDisp(WPXInputStream *input, unsigned length);
  void readVersion(WPXInputStream *input, unsigned length);
  void readIccd(WPXInputStream *input, unsigned length);
  void readBBox(WPXInputStream *input, unsigned length);
  void readSpnd(WPXInputStream *input, unsigned length);
  void readVpat(WPXInputStream *input, unsigned length);
  void readUidr(WPXInputStream *input, unsigned length);
  void readFont(WPXInputStream *input, unsigned length);
  void readStlt(WPXInputStream *input, unsigned length);
  void readStyd(WPXInputStream *input);
  void readTxsm(WPXInputStream *input, unsigned length);
  void readTxsm16(WPXInputStream *input);
  void readTxsm6(WPXInputStream *input);
  void readTxsm5(WPXInputStream *input);
  void readArtisticText(WPXInputStream *input);
  void readParagraphText(WPXInputStream *input);

  bool _redirectX6Chunk(WPXInputStream **input, unsigned &length);

  std::vector<WPXInputStream *> m_externalStreams;

  std::map<unsigned, CDRFont> m_fonts;
  std::map<unsigned, CDRFillStyle> m_fillStyles;
  std::map<unsigned, CDRLineStyle> m_lineStyles;

  unsigned m_version;

};

} // namespace libcdr

#endif // __CDRPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
