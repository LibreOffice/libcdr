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
 * in which case the procdrns of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
 */


#include <zlib.h>
#include "CDRZipStream.h"
#include "CDRInternalStream.h"
#include "CDRUnzip.h"
#include "libcdr_utils.h"


#define CHUNK 16384

libcdr::CDRZipStream::CDRZipStream(WPXInputStream *input) :
  WPXInputStream(),
  m_input(input)
{
}

const unsigned char *libcdr::CDRZipStream::read(unsigned long numBytes, unsigned long &numBytesRead)
{
  return m_input->read(numBytes, numBytesRead);
}

int libcdr::CDRZipStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
  return m_input->seek(offset, seekType);
}

long libcdr::CDRZipStream::tell()
{
  return m_input->tell();
}

bool libcdr::CDRZipStream::atEOS()
{
  return m_input->atEOS();
}

bool libcdr::CDRZipStream::isOLEStream()
{
  std::vector<unsigned char> tmpBuffer;
  long position = m_input->tell();
  m_input->seek(0, WPX_SEEK_SET);
  while (!m_input->atEOS())
    tmpBuffer.push_back(readU8(m_input));
  void *result = OpenZip(&tmpBuffer[0], tmpBuffer.size());
  m_input->seek(position, WPX_SEEK_SET);
  if (!result)
    return false;
  CloseZip(result);
  return true;
}

WPXInputStream *libcdr::CDRZipStream::getDocumentOLEStream(const char *name)
{
  std::vector<unsigned char> tmpBuffer;
  long position = m_input->tell();
  m_input->seek(0, WPX_SEEK_SET);
  while (!m_input->atEOS())
    tmpBuffer.push_back(readU8(m_input));
  void *result = OpenZip(&tmpBuffer[0], tmpBuffer.size());
  m_input->seek(position, WPX_SEEK_SET);
  if (!result)
    return 0;
  int i;
  ZIPENTRY ze;
  if(FindZipItem(result, name, &i, &ze))
  {
    CloseZip(result);
    return 0;
  }
  std::vector<unsigned char> newBuffer(ze.unc_size);
  if (UnzipItem(result, i, &newBuffer[0], ze.unc_size))
  {
    CloseZip(result);
    return 0;
  }
  WPXInputStream *tmpStream = new libcdr::CDRInternalStream(newBuffer);
  if (!tmpStream)
  {
    CloseZip(result);
    return 0;
  }
  CloseZip(result);
  return tmpStream;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
