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
#include "CDRInternalStream.h"
#include "libcdr_utils.h"


#define CHUNK 16384

libcdr::CDRInternalStream::CDRInternalStream(const std::vector<unsigned char> &buffer) :
  WPXInputStream(),
  m_offset(0),
  m_buffer()
{
  for (unsigned long i=0; i<buffer.size(); ++i)
    m_buffer.push_back(buffer[i]);
}

libcdr::CDRInternalStream::CDRInternalStream(WPXInputStream *input, unsigned long size, bool compressed) :
  WPXInputStream(),
  m_offset(0),
  m_buffer()
{
  unsigned long tmpNumBytesRead = 0;

  const unsigned char *tmpBuffer = 0;

  if (!compressed)
  {
    tmpBuffer = input->read(size, tmpNumBytesRead);

    if (size != tmpNumBytesRead)
      return;

    for (unsigned long i=0; i<size; i++)
      m_buffer.push_back(tmpBuffer[i]);
  }
  else
  {
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
      return;

    tmpBuffer = input->read(size, tmpNumBytesRead);

    if (size != tmpNumBytesRead)
      return;

    strm.avail_in = tmpNumBytesRead;
    strm.next_in = (Bytef *)tmpBuffer;

    do
    {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      switch (ret)
      {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        return;
      }

      have = CHUNK - strm.avail_out;

      for (unsigned long i=0; i<have; i++)
        m_buffer.push_back(out[i]);

    }
    while (strm.avail_out == 0);

  }
}

libcdr::CDRInternalStream::CDRInternalStream(const unsigned char *buffer, const unsigned long size) :
  WPXInputStream(),
  m_offset(0),
  m_buffer()
{
  for (unsigned long i = 0; i < size; ++i)
    m_buffer.push_back(buffer[i]);
}

const unsigned char *libcdr::CDRInternalStream::read(unsigned long numBytes, unsigned long &numBytesRead)
{
  numBytesRead = 0;

  if (numBytes == 0)
    return 0;

  int numBytesToRead;

  if ((m_offset+numBytes) < m_buffer.size())
    numBytesToRead = numBytes;
  else
    numBytesToRead = m_buffer.size() - m_offset;

  numBytesRead = numBytesToRead; // about as paranoid as we can be..

  if (numBytesToRead == 0)
    return 0;

  long oldOffset = m_offset;
  m_offset += numBytesToRead;

  return &m_buffer[oldOffset];
}

int libcdr::CDRInternalStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
  if (seekType == WPX_SEEK_CUR)
    m_offset += offset;
  else if (seekType == WPX_SEEK_SET)
    m_offset = offset;

  if (m_offset < 0)
  {
    m_offset = 0;
    return 1;
  }
  if ((long)m_offset > (long)m_buffer.size())
  {
    m_offset = m_buffer.size();
    return 1;
  }

  return 0;
}

long libcdr::CDRInternalStream::tell()
{
  return m_offset;
}

bool libcdr::CDRInternalStream::atEOS()
{
  if ((long)m_offset >= (long)m_buffer.size())
    return true;

  return false;
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
