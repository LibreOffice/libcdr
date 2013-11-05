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
#include <librevenge-stream/librevenge-stream.h>
#include "CDRTypes.h"
#include "CommonParser.h"

namespace libcdr
{

class CDRCollector;

class CDRParser : protected CommonParser
{
public:
  explicit CDRParser(const std::vector<librevenge::RVNGInputStream *> &externalStreams, CDRCollector *collector);
  virtual ~CDRParser();
  bool parseRecords(librevenge::RVNGInputStream *input, unsigned *blockLengths = 0, unsigned level = 0);
  bool parseWaldo(librevenge::RVNGInputStream *input);

private:
  CDRParser();
  CDRParser(const CDRParser &);
  CDRParser &operator=(const CDRParser &);
  bool parseWaldoStructure(librevenge::RVNGInputStream *input, std::stack<WaldoRecordType1> &waldoStack,
                           const std::map<unsigned, WaldoRecordType1> &records1,
                           std::map<unsigned, WaldoRecordInfo> &records2);
  bool gatherWaldoInformation(librevenge::RVNGInputStream *input, std::vector<WaldoRecordInfo> &records, std::map<unsigned, WaldoRecordInfo> &records2,
                              std::map<unsigned, WaldoRecordInfo> &records3, std::map<unsigned, WaldoRecordInfo> &records4,
                              std::map<unsigned, WaldoRecordInfo> &records6, std::map<unsigned, WaldoRecordInfo> &records7,
                              std::map<unsigned, WaldoRecordInfo> &records8, std::map<unsigned, WaldoRecordInfo> recordsOther);
  void readWaldoRecord(librevenge::RVNGInputStream *input, const WaldoRecordInfo &info);
  bool parseRecord(librevenge::RVNGInputStream *input, unsigned *blockLengths = 0, unsigned level = 0);
  void readRecord(unsigned fourCC, unsigned length, librevenge::RVNGInputStream *input);
  double readRectCoord(librevenge::RVNGInputStream *input);
  CDRColor readColor(librevenge::RVNGInputStream *input);

  void readRectangle(librevenge::RVNGInputStream *input);
  void readEllipse(librevenge::RVNGInputStream *input);
  void readLineAndCurve(librevenge::RVNGInputStream *input);
  void readBitmap(librevenge::RVNGInputStream *input);
  void readPageSize(librevenge::RVNGInputStream *input);
  void readWaldoBmp(librevenge::RVNGInputStream *input, unsigned length, unsigned id);
  void readWaldoBmpf(librevenge::RVNGInputStream *input, unsigned id);
  void readWaldoTrfd(librevenge::RVNGInputStream *input);
  void readWaldoOutl(librevenge::RVNGInputStream *input);
  void readWaldoFill(librevenge::RVNGInputStream *input);
  void readWaldoLoda(librevenge::RVNGInputStream *input, unsigned length);
  void readOpacity(librevenge::RVNGInputStream *input, unsigned length);
  void readTrfd(librevenge::RVNGInputStream *input, unsigned length);
  void readFild(librevenge::RVNGInputStream *input, unsigned length);
  void readOutl(librevenge::RVNGInputStream *input, unsigned length);
  void readLoda(librevenge::RVNGInputStream *input, unsigned length);
  void readFlags(librevenge::RVNGInputStream *input, unsigned length);
  void readMcfg(librevenge::RVNGInputStream *input, unsigned length);
  void readPath(librevenge::RVNGInputStream *input);
  void readArrw(librevenge::RVNGInputStream *input, unsigned length);
  void readPolygonCoords(librevenge::RVNGInputStream *input);
  void readPolygonTransform(librevenge::RVNGInputStream *input);
  void readBmp(librevenge::RVNGInputStream *input, unsigned length);
  void readBmpf(librevenge::RVNGInputStream *input, unsigned length);
  void readPpdt(librevenge::RVNGInputStream *input, unsigned length);
  void readFtil(librevenge::RVNGInputStream *input, unsigned length);
  void readDisp(librevenge::RVNGInputStream *input, unsigned length);
  void readVersion(librevenge::RVNGInputStream *input, unsigned length);
  void readIccd(librevenge::RVNGInputStream *input, unsigned length);
  void readBBox(librevenge::RVNGInputStream *input, unsigned length);
  void readSpnd(librevenge::RVNGInputStream *input, unsigned length);
  void readVpat(librevenge::RVNGInputStream *input, unsigned length);
  void readUidr(librevenge::RVNGInputStream *input, unsigned length);
  void readFont(librevenge::RVNGInputStream *input, unsigned length);
  void readStlt(librevenge::RVNGInputStream *input, unsigned length);
  void readStyd(librevenge::RVNGInputStream *input);
  void readTxsm(librevenge::RVNGInputStream *input, unsigned length);
  void readTxsm16(librevenge::RVNGInputStream *input);
  void readTxsm6(librevenge::RVNGInputStream *input);
  void readTxsm5(librevenge::RVNGInputStream *input);
  void readUdta(librevenge::RVNGInputStream *input);
  void readArtisticText(librevenge::RVNGInputStream *input);
  void readParagraphText(librevenge::RVNGInputStream *input);

  bool _redirectX6Chunk(librevenge::RVNGInputStream **input, unsigned &length);

  std::vector<librevenge::RVNGInputStream *> m_externalStreams;

  std::map<unsigned, CDRFont> m_fonts;
  std::map<unsigned, CDRFillStyle> m_fillStyles;
  std::map<unsigned, CDRLineStyle> m_lineStyles;
  std::map<unsigned, CDRPath> m_arrows;

  unsigned m_version;

};

} // namespace libcdr

#endif // __CDRPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
