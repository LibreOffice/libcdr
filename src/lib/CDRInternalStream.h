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
 * Copyright (C) 2011 Eilidh McAdam <tibbylickle@gmail.com>
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


#ifndef __CDRINTERNALSTREAM_H__
#define __CDRINTERNALSTREAM_H__

#include <vector>

#include <libwpd-stream/libwpd-stream.h>

namespace libcdr
{

class CDRInternalStream : public WPXInputStream
{
public:
  CDRInternalStream(WPXInputStream *input, unsigned long size, bool compressed=false);
  CDRInternalStream(const std::vector<unsigned char> &buffer);
  ~CDRInternalStream() {}

  bool isOLEStream()
  {
    return false;
  }
  WPXInputStream *getDocumentOLEStream(const char *)
  {
    return 0;
  }

  const unsigned char *read(unsigned long numBytes, unsigned long &numBytesRead);
  int seek(long offset, WPX_SEEK_TYPE seekType);
  long tell();
  bool atEOS();
  unsigned long getSize() const
  {
    return m_buffer.size();
  }

private:
  volatile long m_offset;
  std::vector<unsigned char> m_buffer;
  CDRInternalStream(const CDRInternalStream &);
  CDRInternalStream &operator=(const CDRInternalStream &);
};

} // namespace libcdr

#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
