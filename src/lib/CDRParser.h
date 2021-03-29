/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRPARSER_H__
#define __CDRPARSER_H__

#include <memory>
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
  explicit CDRParser(const std::vector<std::unique_ptr<librevenge::RVNGInputStream>> &externalStreams, CDRCollector *collector);
  ~CDRParser() override;
  bool parseRecords(librevenge::RVNGInputStream *input, const std::vector<unsigned> &blockLengths = std::vector<unsigned>(), unsigned level = 0);
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
  bool parseRecord(librevenge::RVNGInputStream *input, const std::vector<unsigned> &blockLengths = std::vector<unsigned>(), unsigned level = 0);
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
  void _readX6StyleString(librevenge::RVNGInputStream *input, unsigned long length, CDRStyle &style);
  void _skipX3Optional(librevenge::RVNGInputStream *input);
  void _resolveColorPalette(CDRColor &color);

  const std::vector<std::unique_ptr<librevenge::RVNGInputStream>> &m_externalStreams;

  std::map<unsigned, CDRFont> m_fonts;
  std::map<unsigned, CDRFillStyle> m_fillStyles;
  std::map<unsigned, CDRLineStyle> m_lineStyles;
  std::map<unsigned, CDRPath> m_arrows;

  unsigned m_version;
  unsigned m_waldoOutlId;
  unsigned m_waldoFillId;

};

} // namespace libcdr

#endif // __CDRPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
