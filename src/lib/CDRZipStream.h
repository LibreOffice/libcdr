/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
