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
 * Copyright (C) 2012 Fridrich Strba <fridrich.strba@bluewin.ch>
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


#ifndef __CDRZIPSTREAM_H__
#define __CDRZIPSTREAM_H__

#include <vector>

#include <libwpd-stream/libwpd-stream.h>

namespace libcdr
{
struct CDRZipStreamImpl;

class CDRZipStream : public WPXInputStream
{
public:
  CDRZipStream(WPXInputStream *input);
  ~CDRZipStream();

  bool isOLEStream();
  WPXInputStream *getDocumentOLEStream(const char *);

  const unsigned char *read(unsigned long numBytes, unsigned long &numBytesRead);
  int seek(long offset, WPX_SEEK_TYPE seekType);
  long tell();
  bool atEOS();

private:
  CDRZipStream(const CDRZipStream &);
  CDRZipStream &operator=(const CDRZipStream &);
  CDRZipStreamImpl *m_pImpl;
};

} // namespace libcdr

#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
