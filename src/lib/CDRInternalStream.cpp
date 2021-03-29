/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CDRInternalStream.h"

#include <zlib.h>
#include <string.h>  // for memcpy


#define CHUNK 16384

libcdr::CDRInternalStream::CDRInternalStream(const std::vector<unsigned char> &buffer) :
  librevenge::RVNGInputStream(),
  m_offset(0),
  m_buffer(buffer)
{
}

libcdr::CDRInternalStream::CDRInternalStream(librevenge::RVNGInputStream *input, unsigned long size, bool compressed) :
  librevenge::RVNGInputStream(),
  m_offset(0),
  m_buffer()
{
  if (!size)
    return;

  if (!compressed)
  {
    unsigned long tmpNumBytesRead = 0;
    const unsigned char *tmpBuffer = input->read(size, tmpNumBytesRead);

    if (size != tmpNumBytesRead)
      return;

    m_buffer = std::vector<unsigned char>(size);
    memcpy(&m_buffer[0], tmpBuffer, size);
  }
  else
  {
    int ret;
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

    unsigned long tmpNumBytesRead = 0;
    const unsigned char *tmpBuffer = input->read(size, tmpNumBytesRead);

    if (size != tmpNumBytesRead)
    {
      (void)inflateEnd(&strm);
      return;
    }

    strm.avail_in = (uInt)tmpNumBytesRead;
    strm.next_in = const_cast<Bytef *>(tmpBuffer);

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
        m_buffer.clear();
        return;
      default:
        break;
      }

      unsigned have = CHUNK - strm.avail_out;

      for (unsigned long i=0; i<have; i++)
        m_buffer.push_back(out[i]);

    }
    while (strm.avail_out == 0);
    (void)inflateEnd(&strm);
  }
}

const unsigned char *libcdr::CDRInternalStream::read(unsigned long numBytes, unsigned long &numBytesRead)
{
  numBytesRead = 0;

  if (numBytes == 0)
    return nullptr;

  unsigned long numBytesToRead;

  if ((m_offset+numBytes) < m_buffer.size())
    numBytesToRead = numBytes;
  else
    numBytesToRead = m_buffer.size() - m_offset;

  numBytesRead = numBytesToRead; // about as paranoid as we can be..

  if (numBytesToRead == 0)
    return nullptr;

  long oldOffset = m_offset;
  m_offset += numBytesToRead;

  return &m_buffer[oldOffset];
}

int libcdr::CDRInternalStream::seek(long offset, librevenge::RVNG_SEEK_TYPE seekType)
{
  if (seekType == librevenge::RVNG_SEEK_CUR)
    m_offset += offset;
  else if (seekType == librevenge::RVNG_SEEK_SET)
    m_offset = offset;
  else if (seekType == librevenge::RVNG_SEEK_END)
    m_offset = long(static_cast<unsigned long>(m_buffer.size())) + offset;

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

bool libcdr::CDRInternalStream::isEnd()
{
  if ((long)m_offset >= (long)m_buffer.size())
    return true;

  return false;
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
