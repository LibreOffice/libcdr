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
#include <libwpd-stream/libwpd-stream.h>
#include "CDRTypes.h"
#include "CommonParser.h"

namespace libcdr
{

class CDRCollector;

class CMXParser : protected CommonParser
{
public:
  explicit CMXParser(CDRCollector *collector);
  virtual ~CMXParser();
  bool parseRecords(WPXInputStream *input, long size = -1, unsigned level = 0);

private:
  CMXParser();
  CMXParser(const CMXParser &);
  CMXParser &operator=(const CMXParser &);
  bool parseRecord(WPXInputStream *input, unsigned level = 0);
  void readRecord(unsigned fourCC, unsigned &length, WPXInputStream *input);

  void readCMXHeader(WPXInputStream *input);
  void readDisp(WPXInputStream *input, unsigned length);
  void readCcmm(WPXInputStream *input, long &recordEnd);
  void readPage(WPXInputStream *input, unsigned length);

  // Command readers
  void readBeginPage(WPXInputStream *input);
  void readBeginLayer(WPXInputStream *input);
  void readBeginGroup(WPXInputStream *input);
  void readPolyCurve(WPXInputStream *input);
  void readEllipse(WPXInputStream *input);
  void readRectangle(WPXInputStream *input);
  void readJumpAbsolute(WPXInputStream *input);

  // Types readers
  CDRTransform readMatrix(WPXInputStream *input);
  CDRBox readBBox(WPXInputStream *input);
  void readFill(WPXInputStream *input);

  // Complex types readers
  void readRenderingAttributes(WPXInputStream *input);

  bool m_bigEndian;
  unsigned short m_unit;
  double m_scale;
  double m_xmin, m_xmax, m_ymin, m_ymax;
  unsigned m_indexSectionOffset;
  unsigned m_infoSectionOffset;
  unsigned m_thumbnailOffset;
  unsigned m_fillIndex;
  unsigned m_nextInstructionOffset;
};

} // namespace libcdr

#endif // __CMXPARSER_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
