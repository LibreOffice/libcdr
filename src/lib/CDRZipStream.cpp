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
 * in which case the provisions of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
 */


#include <string.h>
#include <zlib.h>
#include "CDRZipStream.h"
#include "CDRInternalStream.h"
#include "libcdr_utils.h"


using namespace libcdr;

namespace
{

struct LocalFileHeader
{
  unsigned short min_version;
  unsigned short general_flag;
  unsigned short compression;
  unsigned short lastmod_time;
  unsigned short lastmod_date;
  unsigned crc32;
  unsigned compressed_size;
  unsigned uncompressed_size;
  unsigned short filename_size;
  unsigned short extra_field_size;
  WPXString filename;
  LocalFileHeader()
    : min_version(0), general_flag(0), compression(0), lastmod_time(0), lastmod_date(0),
      crc32(0), compressed_size(0), uncompressed_size(0), filename_size(0), extra_field_size(0),
      filename() {}
  ~LocalFileHeader() {}
};

struct CentralDirectoryEntry
{
  unsigned short creator_version;
  unsigned short min_version;
  unsigned short general_flag;
  unsigned short compression;
  unsigned short lastmod_time;
  unsigned short lastmod_date;
  unsigned crc32;
  unsigned compressed_size;
  unsigned uncompressed_size;
  unsigned short filename_size;
  unsigned short extra_field_size;
  unsigned short file_comment_size;
  unsigned short disk_num;
  unsigned short internal_attr;
  unsigned external_attr;
  unsigned offset;
  WPXString filename;
  CentralDirectoryEntry()
    : creator_version(0), min_version(0), general_flag(0), compression(0), lastmod_time(0),
      lastmod_date(0), crc32(0), compressed_size(0), uncompressed_size(0), filename_size(0),
      extra_field_size(0), file_comment_size(0), disk_num(0), internal_attr(0),
      external_attr(0), offset(0), filename() {}
  ~CentralDirectoryEntry() {}
};

struct CentralDirectoryEnd
{
  unsigned short disk_num;
  unsigned short cdir_disk;
  unsigned short disk_entries;
  unsigned short cdir_entries;
  unsigned cdir_size;
  unsigned cdir_offset;
  unsigned short comment_size;
  CentralDirectoryEnd()
    : disk_num(0), cdir_disk(0), disk_entries(0), cdir_entries(0),
      cdir_size(0), cdir_offset(0), comment_size(0) {}
  ~CentralDirectoryEnd() {}
};

#define CDIR_ENTRY_SIG 0x02014b50
#define LOC_FILE_HEADER_SIG 0x04034b50
#define CDIR_END_SIG 0x06054b50

static bool readCentralDirectoryEnd(WPXInputStream *input, CentralDirectoryEnd &end)
{
  try
  {
    unsigned signature = readU32(input);
    if (signature != CDIR_END_SIG)
      return false;

    end.disk_num = readU16(input);
    end.cdir_disk = readU16(input);
    end.disk_entries = readU16(input);
    end.cdir_entries = readU16(input);
    end.cdir_size = readU32(input);
    end.cdir_offset = readU32(input);
    end.comment_size = readU16(input);
	input->seek(end.comment_size, WPX_SEEK_CUR);
  }
  catch (...)
  {
    return false;
  }
  return true;
}

static bool readCentralDirectoryEntry(WPXInputStream *input, CentralDirectoryEntry &entry)
{
  try
  {
    unsigned signature = readU32(input);
    if (signature != CDIR_ENTRY_SIG)
      return false;

    entry.creator_version = readU16(input);
    entry.min_version = readU16(input);
    entry.general_flag = readU16(input);
    entry.compression = readU16(input);
    entry.lastmod_time = readU16(input);
    entry.lastmod_date = readU16(input);
    entry.crc32 = readU32(input);
    entry.compressed_size = readU32(input);
    entry.uncompressed_size = readU32(input);
    entry.filename_size = readU16(input);
    entry.extra_field_size = readU16(input);
    entry.file_comment_size = readU16(input);
    entry.disk_num = readU16(input);
    entry.internal_attr = readU16(input);
    entry.external_attr = readU32(input);
    entry.offset = readU32(input);
    unsigned short i = 0;
    entry.filename.clear();
    for (i=0; i < entry.filename_size; i++)
      entry.filename.append((char)readU8(input));
	input->seek(entry.extra_field_size, WPX_SEEK_CUR);
    input->seek(entry.file_comment_size, WPX_SEEK_CUR);
  }
  catch (...)
  {
    return false;
  }
  return true;
}

static bool readLocalFileHeader(WPXInputStream *input, LocalFileHeader &header)
{
  try
  {
    unsigned signature = readU32(input);
    if (signature != LOC_FILE_HEADER_SIG)
      return false;

    header.min_version = readU16(input);
    header.general_flag = readU16(input);
    header.compression = readU16(input);
    header.lastmod_time = readU16(input);
    header.lastmod_date = readU16(input);
    header.crc32 = readU32(input);
    header.compressed_size = readU32(input);
    header.uncompressed_size = readU32(input);
    header.filename_size = readU16(input);
    header.extra_field_size = readU16(input);
    unsigned short i = 0;
    header.filename.clear();
    for (i=0; i < header.filename_size; i++)
      header.filename.append((char)readU8(input));
	input->seek(header.extra_field_size, WPX_SEEK_CUR);
  }
  catch (...)
  {
    return false;
  }
  return true;
}

static bool areHeadersConsistent(const LocalFileHeader &header, const CentralDirectoryEntry &entry)
{
  if (header.min_version != entry.min_version)
    return false;
  if (header.general_flag != entry.general_flag)
    return false;
  if (header.compression != entry.compression)
    return false;
  if (!(header.general_flag & 0x08))
  {
    if (header.crc32 != entry.crc32)
      return false;
    if (header.compressed_size != entry.compressed_size)
      return false;
    if (header.uncompressed_size != entry.uncompressed_size)
      return false;
  }
  return true;
}

static bool findCentralDirectoryEnd(WPXInputStream *input, unsigned &offset)
{
  input->seek(offset, WPX_SEEK_SET);
  try
  {
    while (!input->atEOS())
    {
      unsigned signature = readU32(input);
      if (signature == CDIR_END_SIG)
      {
        input->seek(-4, WPX_SEEK_CUR);
		offset = input->tell();
        return true;
      }
      else
        input->seek(-3, WPX_SEEK_CUR);
    }
  }
  catch (...)
  {
    return false;
  }
  return false;
}

static bool isZipStream(WPXInputStream *input, unsigned &offset)
{
  if (!findCentralDirectoryEnd(input, offset))
    return false;
  CentralDirectoryEnd end;
  if (!readCentralDirectoryEnd(input, end))
    return false;
  input->seek(end.cdir_offset, WPX_SEEK_SET);
  CentralDirectoryEntry entry;
  if (!readCentralDirectoryEntry(input, entry))
    return false;
  input->seek(entry.offset, WPX_SEEK_SET);
  LocalFileHeader header;
  if (!readLocalFileHeader(input, header))
    return false;
  if (!areHeadersConsistent(header, entry))
    return false;
  return true;
}

static bool findDataStream(WPXInputStream *input, CentralDirectoryEntry &entry, const char *name, unsigned &offset)
{
  unsigned short name_size = strlen(name);
  if (!findCentralDirectoryEnd(input, offset))
    return false;
  CentralDirectoryEnd end;
  if (!readCentralDirectoryEnd(input, end))
    return false;
  input->seek(end.cdir_offset, WPX_SEEK_SET);
  while (!input->atEOS() && (unsigned)input->tell() < end.cdir_offset + end.cdir_size)
  {
    if (!readCentralDirectoryEntry(input, entry))
      return false;
    if (name_size == entry.filename_size && entry.filename == name)
      break;
  }
  if (name_size != entry.filename_size)
    return false;
  if (entry.filename != name)
    return false;
  input->seek(entry.offset, WPX_SEEK_SET);
  LocalFileHeader header;
  if (!readLocalFileHeader(input, header))
    return false;
  if (!areHeadersConsistent(header, entry))
    return false;
  return true;
}

WPXInputStream *getSubstream(WPXInputStream *input, const char *name, unsigned &offset)
{
  CentralDirectoryEntry entry;
  if (!findDataStream(input, entry, name, offset))
    return 0;
  if (!entry.compression)
    return new CDRInternalStream(input, entry.compressed_size);
  else
  {
    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,-MAX_WBITS);
    if (ret != Z_OK)
      return 0;

    unsigned long numBytesRead = 0;
    const unsigned char *compressedData = input->read(entry.compressed_size, numBytesRead);
    if (numBytesRead != entry.compressed_size)
      return 0;

    strm.avail_in = numBytesRead;
    strm.next_in = (Bytef *)compressedData;

    std::vector<unsigned char>data(entry.uncompressed_size);

    strm.avail_out = entry.uncompressed_size;
    strm.next_out = reinterpret_cast<Bytef *>(&data[0]);
    ret = inflate(&strm, Z_FINISH);
    switch (ret)
    {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      (void)inflateEnd(&strm);
      data.clear();
      return 0;
    }
    (void)inflateEnd(&strm);
    return new CDRInternalStream(data);
  }
}

} // anonymous namespace

libcdr::CDRZipStream::CDRZipStream(WPXInputStream *input) :
  WPXInputStream(),
  m_input(input),
  m_cdir_offset(0)
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
  return isZipStream(m_input, m_cdir_offset);
}

WPXInputStream *libcdr::CDRZipStream::getDocumentOLEStream(const char *name)
{
  return getSubstream(m_input, name, m_cdir_offset);
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
