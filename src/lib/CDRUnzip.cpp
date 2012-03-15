// CDRUnzip.cpp
//
// Authors:      Mark Adler et al. (see below)
//
// Modified by:  Lucian Wischik
//               lu@wischik.com
//
// Modified by:  Hans Dietrich
//               hdietrich@gmail.com
//
// Modified by:  Fridrich Strba
//               fridrich.strba@bluewin.ch
//
///////////////////////////////////////////////////////////////////////////////
//
// Lucian Wischik's comments:
// --------------------------
// THIS FILE is almost entirely based upon code by Info-ZIP.
// It has been modified by Lucian Wischik.
// The original code may be found at http://www.info-zip.org
// The original copyright text follows.
//
///////////////////////////////////////////////////////////////////////////////
//
// Original authors' comments:
// ---------------------------
// This is version 2002-Feb-16 of the Info-ZIP copyright and license. The
// definitive version of this document should be available at
// ftp://ftp.info-zip.org/pub/infozip/license.html indefinitely.
//
// Copyright (c) 1990-2002 Info-ZIP.  All rights reserved.
//
// For the purposes of this copyright and license, "Info-ZIP" is defined as
// the following set of individuals:
//
//   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
//   Jean-loup Gailly, Hunter Goatley, Ian Gorman, Chris Herborth, Dirk Haase,
//   Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz,
//   David Kirschbaum, Johnny Lee, Onno van der Linden, Igor Mandrichenko,
//   Steve P. Miller, Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs,
//   Kai Uwe Rommel, Steve Salisbury, Dave Smith, Christian Spieler,
//   Antoine Verheijen, Paul von Behren, Rich Wales, Mike White
//
// This software is provided "as is", without warranty of any kind, express
// or implied.  In no event shall Info-ZIP or its contributors be held liable
// for any direct, indirect, incidental, special or consequential damages
// arising out of the use of or inability to use this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. Redistributions of source code must retain the above copyright notice,
//       definition, disclaimer, and this list of conditions.
//
//    2. Redistributions in binary form (compiled executables) must reproduce
//       the above copyright notice, definition, disclaimer, and this list of
//       conditions in documentation and/or other materials provided with the
//       distribution. The sole exception to this condition is redistribution
//       of a standard UnZipSFX binary as part of a self-extracting archive;
//       that is permitted without inclusion of this license, as long as the
//       normal UnZipSFX banner has not been removed from the binary or disabled.
//
//    3. Altered versions--including, but not limited to, ports to new
//       operating systems, existing ports with new graphical interfaces, and
//       dynamic, shared, or static library versions--must be plainly marked
//       as such and must not be misrepresented as being the original source.
//       Such altered versions also must not be misrepresented as being
//       Info-ZIP releases--including, but not limited to, labeling of the
//       altered versions with the names "Info-ZIP" (or any variation thereof,
//       including, but not limited to, different capitalizations),
//       "Pocket UnZip", "WiZ" or "MacZip" without the explicit permission of
//       Info-ZIP.  Such altered versions are further prohibited from
//       misrepresentative use of the Zip-Bugs or Info-ZIP e-mail addresses or
//       of the Info-ZIP URL(s).
//
//    4. Info-ZIP retains the right to use the names "Info-ZIP", "Zip", "UnZip",
//       "UnZipSFX", "WiZ", "Pocket UnZip", "Pocket Zip", and "MacZip" for its
//       own source and binary releases.
//
///////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <libwpd-stream/libwpd-stream.h>
#include "CDRUnzip.h"

using namespace libcdr;

namespace
{

// These are the result codes:
#define ZR_OK         0x00000000     // nb. the pseudo-code zr-recent is never returned,
#define ZR_RECENT     0x00000001     // but can be passed to FormatZipMessage.
// The following come from general system stuff (e.g. files not openable)
#define ZR_GENMASK    0x0000FF00
#define ZR_NODUPH     0x00000100     // couldn't duplicate the handle
#define ZR_NOFILE     0x00000200     // couldn't create/open the file
#define ZR_NOALLOC    0x00000300     // failed to allocate some resource
#define ZR_WRITE      0x00000400     // a general error writing to the file
#define ZR_NOTFOUND   0x00000500     // couldn't find that file in the zip
#define ZR_MORE       0x00000600     // there's still more data to be unzipped
#define ZR_CORRUPT    0x00000700     // the zipfile is corrupt or not a zipfile
#define ZR_READ       0x00000800     // a general error reading the file
// The following come from mistakes on the part of the caller
#define ZR_CALLERMASK 0x00FF0000
#define ZR_ARGS       0x00010000     // general mistake with the arguments
#define ZR_NOTMMAP    0x00020000     // tried to ZipGetMemory, but that only works on mmap zipfiles, which yours wasn't
#define ZR_MEMSIZE    0x00030000     // the memory size is too small
#define ZR_FAILED     0x00040000     // the thing was already failed when you called this function
#define ZR_ENDED      0x00050000     // the zip creation has already been closed
#define ZR_MISSIZE    0x00060000     // the indicated input file size turned out mistaken
#define ZR_PARTIALUNZ 0x00070000     // the file had already been partially unzipped
#define ZR_ZMODE      0x00080000     // tried to mix creating/opening a zip 
// The following come from bugs within the zip library itself
#define ZR_BUGMASK    0xFF000000
#define ZR_NOTINITED  0x01000000     // initialisation didn't work
#define ZR_SEEK       0x02000000     // trying to seek in an unseekable file
#define ZR_NOCHANGE   0x04000000     // changed its mind on storage, but not allowed
#define ZR_FLATE      0x05000000     // an internal error in the de/inflation code

#define zmalloc(len) malloc(len)

#define zfree(p) free(p)

#define UNZ_OK                  (0)
#define UNZ_END_OF_LIST_OF_FILE (-100)
#define UNZ_ERRNO               (Z_ERRNO)
#define UNZ_EOF                 (0)
#define UNZ_PARAMERROR          (-102)
#define UNZ_BADZIPFILE          (-103)
#define UNZ_INTERNALERROR       (-104)
#define UNZ_CRCERROR            (-105)

#define UNZ_BUFSIZE (16384)
#define UNZ_MAXFILENAMEINZIP (256)
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)


// unz_global_info structure contain global data about the ZIPfile
typedef struct unz_global_info_s
{
  unsigned long number_entry;         // total number of entries in the central dir on this disk
  unsigned long size_comment;         // size of the global comment of the zipfile
} unz_global_info;

// unz_file_info contain information about a file in the zipfile
typedef struct unz_file_info_s
{
  unsigned long version;              // version made by                 2 bytes
  unsigned long version_needed;       // version needed to extract       2 bytes
  unsigned long flag;                 // general purpose bit flag        2 bytes
  unsigned long compression_method;   // compression method              2 bytes
  unsigned long dosDate;              // last mod file date in Dos fmt   4 bytes
  unsigned long crc;                  // crc-32                          4 bytes
  unsigned long compressed_size;      // compressed size                 4 bytes
  unsigned long uncompressed_size;    // uncompressed size               4 bytes
  unsigned long size_filename;        // filename length                 2 bytes
  unsigned long size_file_extra;      // extra field length              2 bytes
  unsigned long size_file_comment;    // file comment length             2 bytes
  unsigned long disk_num_start;       // disk number start               2 bytes
  unsigned long internal_fa;          // internal file attributes        2 bytes
  unsigned long external_fa;          // external file attributes        4 bytes
} unz_file_info;

const char unz_copyright[] = " ";//unzip 0.15 Copyright 1998 Gilles Vollant ";

// unz_file_info_interntal contain internal info about a file in zipfile
typedef struct unz_file_info_internal_s
{
  unsigned long offset_curfile;// relative offset of local header 4 bytes
} unz_file_info_internal;

typedef struct
{
  WPXInputStream *is;
  unsigned len; // if it's a memory block
} LUFILE;

// file_in_zip_read_info_s contain internal information about a file in zipfile,
//  when reading and decompress it
typedef struct
{
  char  *read_buffer;         // internal buffer for compressed data
  z_stream stream;            // zLib stream structure for inflate

  unsigned long pos_in_zipfile;       // position in byte on the zipfile, for fseek
  unsigned long stream_initialised;   // flag set if stream structure is initialised

  unsigned long offset_local_extrafield;// offset of the local extra field
  unsigned  size_local_extrafield;// size of the local extra field
  unsigned long pos_local_extrafield;   // position in the local extra field in read

  unsigned long crc32;                // crc32 of all data uncompressed
  unsigned long crc32_wait;           // crc32 we must obtain after decompress all
  unsigned long rest_read_compressed; // number of byte to be decompressed
  unsigned long rest_read_uncompressed;//number of byte to be obtained after decomp
  LUFILE *file;                 // io structore of the zipfile
  unsigned long compression_method;   // compression method (0==store)
  unsigned long byte_before_the_zipfile;// byte before the zipfile, (>0 for sfx)
} file_in_zip_read_info_s;

// unz_s contain internal information about the zipfile
typedef struct
{
  LUFILE *file;               // io structore of the zipfile
  unz_global_info gi;         // public global information
  unsigned long byte_before_the_zipfile;// byte before the zipfile, (>0 for sfx)
  unsigned long num_file;             // number of the current file in the zipfile
  unsigned long pos_in_central_dir;   // pos of the current file in the central dir
  unsigned long current_file_ok;      // flag about the usability of the current file
  unsigned long central_pos;          // position of the beginning of the central dir

  unsigned long size_central_dir;     // size of the central directory
  unsigned long offset_central_dir;   // offset of start of central directory with respect to the starting disk number

  unz_file_info cur_file_info; // public info about the current file in zip
  unz_file_info_internal cur_file_info_internal; // private info about it
  file_in_zip_read_info_s *pfile_in_zip_read; // structure about the current file if we are decompressing it
} unz_s, *unzFile;

static LUFILE *lufopen(WPXInputStream *is)
{
  LUFILE *lf = new LUFILE;
  while(!is->atEOS())
    is->seek(1, WPX_SEEK_CUR);
  lf->len=is->tell();
  is->seek(0, WPX_SEEK_SET);
  lf->is=is;
  return lf;
}

static int lufclose(LUFILE *stream)
{
  if (!stream)
    return EOF;
  delete stream;
  return 0;
}

static long int luftell(LUFILE *stream)
{
  return stream->is->tell();
}

static int lufseek(LUFILE *stream, long offset, int whence)
{
  if (whence==SEEK_SET)
    stream->is->seek(offset, WPX_SEEK_SET);
  else if (whence==SEEK_CUR)
    stream->is->seek(offset, WPX_SEEK_CUR);
  else if (whence==SEEK_END)
    stream->is->seek(stream->len+offset, WPX_SEEK_SET);
  return 0;
}

static size_t lufread(void *ptr,size_t size,size_t n,LUFILE *stream)
{
  unsigned long numBytes = (unsigned long)(size*n);
  if (numBytes+stream->is->tell() > stream->len)
    numBytes=stream->len-stream->is->tell();
  unsigned long numBytesRead;
  const unsigned char *tmpBuffer = stream->is->read(numBytes, numBytesRead);
  memcpy(ptr, tmpBuffer, numBytesRead);
  return numBytesRead/size;
}

static int unzlocal_getByte(LUFILE *fin,int *pi)
{
  unsigned char c;
  int err = (int)lufread(&c, 1, 1, fin);
  if (err==1)
  {
    *pi = (int)c;
    return UNZ_OK;
  }
  else
  {
    return UNZ_EOF;
  }
}

static int unzlocal_getShort (LUFILE *fin,unsigned long *pX)
{
  int i;
  int err = unzlocal_getByte(fin,&i);
  unsigned long x = (unsigned long)i;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((unsigned long)i)<<8;

  if (err==UNZ_OK)
    *pX = x;
  else
    *pX = 0;
  return err;
}

static int unzlocal_getLong (LUFILE *fin,unsigned long *pX)
{
  int i;
  int err = unzlocal_getByte(fin,&i);
  unsigned long x = (unsigned long)i;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((unsigned long)i)<<8;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((unsigned long)i)<<16;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((unsigned long)i)<<24;

  if (err==UNZ_OK)
    *pX = x;
  else
    *pX = 0;
  return err;
}

#define BUFREADCOMMENT (0x400)

static unsigned long unzlocal_SearchCentralDir(LUFILE *fin)
{
  if (lufseek(fin,0,SEEK_END) != 0)
    return 0;
  unsigned long uSizeFile = luftell(fin);

  unsigned long uMaxBack=0xffff; // maximum size of global comment
  if (uMaxBack>uSizeFile)
    uMaxBack = uSizeFile;

  unsigned char *buf = (unsigned char *)zmalloc(BUFREADCOMMENT+4);
  if (!buf)
    return 0;

  unsigned long uPosFound=0;
  unsigned long uBackRead = 4;
  while (uBackRead<uMaxBack)
  {
    unsigned long uReadSize = 0;
    unsigned long uReadPos = 0;
    if (uBackRead+BUFREADCOMMENT>uMaxBack)
      uBackRead = uMaxBack;
    else
      uBackRead+=BUFREADCOMMENT;
    uReadPos = uSizeFile-uBackRead ;
    uReadSize = ((BUFREADCOMMENT+4) < (uSizeFile-uReadPos)) ? (BUFREADCOMMENT+4) : (uSizeFile-uReadPos);
    if (lufseek(fin,uReadPos,SEEK_SET))
      break;
    if (lufread(buf,(unsigned)uReadSize,1,fin)!=1)
      break;
    for (int i=(int)uReadSize-3; (i--)>0;)
    {
      if (((*(buf+i))==0x50) && ((*(buf+i+1))==0x4b) &&	((*(buf+i+2))==0x05) && ((*(buf+i+3))==0x06))
      {
        uPosFound = uReadPos+i;
        break;
      }
    }
    if (uPosFound)
      break;
  }
  if (buf)
    zfree(buf);
  return uPosFound;
}

static int unzlocal_GetCurrentFileInfoInternal (unzFile file, unz_file_info *pfile_info,
    unz_file_info_internal *pfile_info_internal, char *szFileName,
    unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize,
    char *szComment, unsigned long commentBufferSize)
{
  unz_file_info file_info;
  unz_file_info_internal file_info_internal;
  int err=UNZ_OK;
  unsigned long uMagic;
  long lSeek=0;

  if (!file)
    return UNZ_PARAMERROR;
  unz_s *s=(unz_s *)file;
  if (lufseek(s->file,s->pos_in_central_dir+s->byte_before_the_zipfile,SEEK_SET))
    err=UNZ_ERRNO;

  // we check the magic
  if (err==UNZ_OK)
  {
    if (unzlocal_getLong(s->file,&uMagic) != UNZ_OK)
      err=UNZ_ERRNO;
    else if (uMagic!=0x02014b50)
      err=UNZ_BADZIPFILE;
  }

  if (unzlocal_getShort(s->file,&file_info.version) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.version_needed) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.flag) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.compression_method) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&file_info.dosDate) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&file_info.crc) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&file_info.compressed_size) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&file_info.uncompressed_size) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.size_filename) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.size_file_extra) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.size_file_comment) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.disk_num_start) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&file_info.internal_fa) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&file_info.external_fa) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&file_info_internal.offset_curfile) != UNZ_OK)
    err=UNZ_ERRNO;

  lSeek+=file_info.size_filename;
  if ((err==UNZ_OK) && szFileName)
  {
    unsigned long uSizeRead ;
    if (file_info.size_filename<fileNameBufferSize)
    {
      *(szFileName+file_info.size_filename)='\0';
      uSizeRead = file_info.size_filename;
    }
    else
      uSizeRead = fileNameBufferSize;

    if ((file_info.size_filename>0) && (fileNameBufferSize>0))
    {
      if (lufread(szFileName,(unsigned)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    }
    lSeek -= uSizeRead;
  }

  if ((err==UNZ_OK) && extraField)
  {
    unsigned long uSizeRead = 0;
    if (file_info.size_file_extra<extraFieldBufferSize)
      uSizeRead = file_info.size_file_extra;
    else
      uSizeRead = extraFieldBufferSize;

    if (lSeek)
    {
      if (!lufseek(s->file,lSeek,SEEK_CUR))
        lSeek=0;
      else
        err=UNZ_ERRNO;
    }
    if ((file_info.size_file_extra>0) && (extraFieldBufferSize>0))
    {
      if (lufread(extraField,(unsigned)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    }
    lSeek += file_info.size_file_extra - uSizeRead;
  }
  else
    lSeek+=file_info.size_file_extra;

  if ((err==UNZ_OK) && szComment)
  {
    unsigned long uSizeRead = 0;
    if (file_info.size_file_comment<commentBufferSize)
    {
      *(szComment+file_info.size_file_comment)='\0';
      uSizeRead = file_info.size_file_comment;
    }
    else
      uSizeRead = commentBufferSize;

    if (lSeek)
    {
      if (!lufseek(s->file,lSeek,SEEK_CUR))
        {}
      else
        err=UNZ_ERRNO;
    }
    if ((file_info.size_file_comment>0) && (commentBufferSize>0))
    {
      if (lufread(szComment,(unsigned)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    }
  }
  else {}

  if ((err==UNZ_OK) && (pfile_info))
    *pfile_info=file_info;

  if ((err==UNZ_OK) && pfile_info_internal)
    *pfile_info_internal=file_info_internal;

  return err;
}

static int unzGoToFirstFile (unzFile file)
{
  if (!file)
    return UNZ_PARAMERROR;
  unz_s *s=(unz_s *)file;
  s->pos_in_central_dir=s->offset_central_dir;
  s->num_file=0;
  int err=unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
          &s->cur_file_info_internal,
          0,0,0,0,0,0);
  s->current_file_ok = (err == UNZ_OK);
  return err;
}

static int unzCloseCurrentFile (unzFile file)
{
  if (!file)
    return UNZ_PARAMERROR;
  unz_s *s=(unz_s *)file;
  file_in_zip_read_info_s *pfile_in_zip_read_info=s->pfile_in_zip_read;

  if (!pfile_in_zip_read_info)
    return UNZ_PARAMERROR;

  int err=UNZ_OK;
  if (!(pfile_in_zip_read_info->rest_read_uncompressed))
  {
    if (pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait)
      err=UNZ_CRCERROR;
  }

  if (pfile_in_zip_read_info->read_buffer)
  {
    void *buf = pfile_in_zip_read_info->read_buffer;
    zfree(buf);
    pfile_in_zip_read_info->read_buffer=0;
  }
  pfile_in_zip_read_info->read_buffer = 0;
  if (pfile_in_zip_read_info->stream_initialised)
    inflateEnd(&pfile_in_zip_read_info->stream);

  pfile_in_zip_read_info->stream_initialised = 0;
  if (pfile_in_zip_read_info)
    zfree(pfile_in_zip_read_info);  // unused pfile_in_zip_read_info=0;

  s->pfile_in_zip_read=0;

  return err;
}

static unzFile unzOpenInternal(LUFILE *fin)
{
  if (!fin)
    return 0;
  if (unz_copyright[0]!=' ')
  {
    lufclose(fin);
    return 0;
  }

  int err=UNZ_OK;
  unz_s us;
  unsigned long central_pos,uL;
  central_pos = unzlocal_SearchCentralDir(fin);
  if (!central_pos)
    err=UNZ_ERRNO;
  if (lufseek(fin,central_pos,SEEK_SET))
    err=UNZ_ERRNO;

  if (unzlocal_getLong(fin,&uL)!=UNZ_OK)
    err=UNZ_ERRNO;

  unsigned long number_disk;          // number of the current dist, used for spanning ZIP, unsupported, always 0
  if (unzlocal_getShort(fin,&number_disk)!=UNZ_OK)
    err=UNZ_ERRNO;

  unsigned long number_disk_with_CD;  // number the the disk with central dir, used for spaning ZIP, unsupported, always 0
  if (unzlocal_getShort(fin,&number_disk_with_CD)!=UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(fin,&us.gi.number_entry)!=UNZ_OK)
    err=UNZ_ERRNO;

  unsigned long number_entry_CD;      // total number of entries in the central dir (same than number_entry on nospan)
  if (unzlocal_getShort(fin,&number_entry_CD)!=UNZ_OK)
    err=UNZ_ERRNO;
  if ((number_entry_CD!=us.gi.number_entry) || (number_disk_with_CD) || (number_disk))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(fin,&us.size_central_dir)!=UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getLong(fin,&us.offset_central_dir)!=UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(fin,&us.gi.size_comment)!=UNZ_OK)
    err=UNZ_ERRNO;
  if ((central_pos<us.offset_central_dir+us.size_central_dir) && (err==UNZ_OK))
    err=UNZ_BADZIPFILE;

  if (err!=UNZ_OK)
  {
    lufclose(fin);
    return 0;
  }

  us.file=fin;
  us.byte_before_the_zipfile = central_pos - (us.offset_central_dir+us.size_central_dir);
  us.central_pos = central_pos;
  us.pfile_in_zip_read = 0;

  unz_s *s = (unz_s *)zmalloc(sizeof(unz_s));
  *s=us;
  unzGoToFirstFile((unzFile)s);
  return (unzFile)s;
}

static int unzClose (unzFile file)
{
  unz_s *s;
  if (!file)
    return UNZ_PARAMERROR;
  s=(unz_s *)file;

  if (!(s->pfile_in_zip_read))
    unzCloseCurrentFile(file);

  lufclose(s->file);
  if (s)
    zfree(s);
  return UNZ_OK;
}

static int unzGetCurrentFileInfo (unzFile file, unz_file_info *pfile_info,
                                  char *szFileName, unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize,
                                  char *szComment, unsigned long commentBufferSize)
{
  return unzlocal_GetCurrentFileInfoInternal(file,pfile_info,0,szFileName,fileNameBufferSize,
         extraField,extraFieldBufferSize, szComment,commentBufferSize);
}

static int unzGoToNextFile (unzFile file)
{
  if (!file)
    return UNZ_PARAMERROR;
  unz_s *s=(unz_s *)file;
  if (!s->current_file_ok)
    return UNZ_END_OF_LIST_OF_FILE;
  if (s->num_file+1==s->gi.number_entry)
    return UNZ_END_OF_LIST_OF_FILE;

  s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
                           s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment ;
  s->num_file++;
  int err = unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
            &s->cur_file_info_internal,
            0,0,0,0,0,0);
  s->current_file_ok = (err == UNZ_OK);
  return err;
}

static int unzLocateFile (unzFile file, const char *szFileName)
{
  if (!file)
    return UNZ_PARAMERROR;

  if (strlen(szFileName)>=UNZ_MAXFILENAMEINZIP)
    return UNZ_PARAMERROR;

  unz_s *s=(unz_s *)file;
  if (!s->current_file_ok)
    return UNZ_END_OF_LIST_OF_FILE;

  unsigned long num_fileSaved = s->num_file;
  unsigned long pos_in_central_dirSaved = s->pos_in_central_dir;

  int err = unzGoToFirstFile(file);

  while (err == UNZ_OK)
  {
    char szCurrentFileName[UNZ_MAXFILENAMEINZIP+1];
    unzGetCurrentFileInfo(file,0,
                          szCurrentFileName,sizeof(szCurrentFileName)-1,
                          0,0,0,0);
    if (!strcmp(szCurrentFileName,szFileName))
      return UNZ_OK;
    err = unzGoToNextFile(file);
  }

  s->num_file = num_fileSaved ;
  s->pos_in_central_dir = pos_in_central_dirSaved ;
  return err;
}

static int unzlocal_CheckCurrentFileCoherencyHeader (unz_s *s,unsigned *piSizeVar,
    unsigned long *poffset_local_extrafield, unsigned  *psize_local_extrafield)
{
  *piSizeVar = 0;
  *poffset_local_extrafield = 0;
  *psize_local_extrafield = 0;

  if (lufseek(s->file,s->cur_file_info_internal.offset_curfile + s->byte_before_the_zipfile,SEEK_SET))
    return UNZ_ERRNO;

  int err=UNZ_OK;
  unsigned long uMagic = 0;
  if (unzlocal_getLong(s->file,&uMagic) != UNZ_OK)
    err=UNZ_ERRNO;
  else if (uMagic!=0x04034b50)
    err=UNZ_BADZIPFILE;

  unsigned long uData = 0;
  if (unzlocal_getShort(s->file,&uData) != UNZ_OK)
    err=UNZ_ERRNO;
  unsigned long uFlags = 0;
  if (unzlocal_getShort(s->file,&uFlags) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&uData) != UNZ_OK)
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compression_method))
    err=UNZ_BADZIPFILE;

  if ((err==UNZ_OK) && (s->cur_file_info.compression_method) &&
      (s->cur_file_info.compression_method!=Z_DEFLATED))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // date/time
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // crc
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.crc) &&
           (!(uFlags & 8)))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // size compr
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compressed_size) &&
           (!(uFlags & 8)))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // size uncompr
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.uncompressed_size) &&
           (!(uFlags & 8)))
    err=UNZ_BADZIPFILE;

  unsigned long size_filename = 0;
  if (unzlocal_getShort(s->file,&size_filename) != UNZ_OK)
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (size_filename!=s->cur_file_info.size_filename))
    err=UNZ_BADZIPFILE;

  *piSizeVar += (unsigned)size_filename;

  unsigned long size_extra_field;
  if (unzlocal_getShort(s->file,&size_extra_field) != UNZ_OK)
    err=UNZ_ERRNO;
  *poffset_local_extrafield= s->cur_file_info_internal.offset_curfile +
                             SIZEZIPLOCALHEADER + size_filename;
  *psize_local_extrafield = (unsigned)size_extra_field;

  *piSizeVar += (unsigned)size_extra_field;

  return err;
}

static int unzOpenCurrentFile (unzFile file)
{

  if (!file)
    return UNZ_PARAMERROR;
  unz_s *s=(unz_s *)file;
  if (!s->current_file_ok)
    return UNZ_PARAMERROR;

  if (!s->pfile_in_zip_read)
    unzCloseCurrentFile(file);

  unsigned iSizeVar = 0;
  unsigned long offset_local_extrafield = 0;
  unsigned size_local_extrafield = 0;
  if (unzlocal_CheckCurrentFileCoherencyHeader(s,&iSizeVar,
      &offset_local_extrafield,&size_local_extrafield)!=UNZ_OK)
    return UNZ_BADZIPFILE;

  file_in_zip_read_info_s *pfile_in_zip_read_info = (file_in_zip_read_info_s *)zmalloc(sizeof(file_in_zip_read_info_s));
  if (!pfile_in_zip_read_info)
    return UNZ_INTERNALERROR;

  pfile_in_zip_read_info->read_buffer=(char *)zmalloc(UNZ_BUFSIZE);
  pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
  pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
  pfile_in_zip_read_info->pos_local_extrafield=0;

  if (!pfile_in_zip_read_info->read_buffer)
  {
    if (pfile_in_zip_read_info)
      zfree(pfile_in_zip_read_info);
    return UNZ_INTERNALERROR;
  }

  pfile_in_zip_read_info->stream_initialised=0;

  int Store = !(s->cur_file_info.compression_method);

  pfile_in_zip_read_info->crc32_wait=s->cur_file_info.crc;
  pfile_in_zip_read_info->crc32=0;
  pfile_in_zip_read_info->compression_method = s->cur_file_info.compression_method;
  pfile_in_zip_read_info->file=s->file;
  pfile_in_zip_read_info->byte_before_the_zipfile=s->byte_before_the_zipfile;

  pfile_in_zip_read_info->stream.total_out = 0;

  int err=Z_OK;
  if (!Store)
  {
    pfile_in_zip_read_info->stream.zalloc = (alloc_func)0;
    pfile_in_zip_read_info->stream.zfree = (free_func)0;
    pfile_in_zip_read_info->stream.opaque = (voidpf)0;

    err=inflateInit2(&pfile_in_zip_read_info->stream, 0);
    if (err == Z_OK)
      pfile_in_zip_read_info->stream_initialised=1;
  }
  pfile_in_zip_read_info->rest_read_compressed = s->cur_file_info.compressed_size ;
  pfile_in_zip_read_info->rest_read_uncompressed = s->cur_file_info.uncompressed_size ;

  pfile_in_zip_read_info->pos_in_zipfile = s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER + iSizeVar;

  pfile_in_zip_read_info->stream.avail_in = 0;

  s->pfile_in_zip_read = pfile_in_zip_read_info;
  return UNZ_OK;
}

static int unzReadCurrentFile (unzFile file, void *buf, unsigned len)
{
  unz_s *s = (unz_s *)file;
  if (!s)
    return UNZ_PARAMERROR;

  file_in_zip_read_info_s *pfile_in_zip_read_info = s->pfile_in_zip_read;
  if (!pfile_in_zip_read_info)
    return UNZ_PARAMERROR;
  if (!pfile_in_zip_read_info->read_buffer)
    return UNZ_END_OF_LIST_OF_FILE;
  if (!len)
    return 0;

  pfile_in_zip_read_info->stream.next_out = (Byte *)buf;
  pfile_in_zip_read_info->stream.avail_out = (unsigned)len;

  if (len>pfile_in_zip_read_info->rest_read_uncompressed)
    pfile_in_zip_read_info->stream.avail_out = (unsigned)pfile_in_zip_read_info->rest_read_uncompressed;

  int err=UNZ_OK;
  unsigned iRead = 0;
  while (pfile_in_zip_read_info->stream.avail_out>0)
  {
    if (!(pfile_in_zip_read_info->stream.avail_in) && (pfile_in_zip_read_info->rest_read_compressed>0))
    {
      unsigned uReadThis = UNZ_BUFSIZE;
      if (pfile_in_zip_read_info->rest_read_compressed<uReadThis)
        uReadThis = (unsigned)pfile_in_zip_read_info->rest_read_compressed;
      if (uReadThis == 0)
        return UNZ_EOF;
      if (lufseek(pfile_in_zip_read_info->file, pfile_in_zip_read_info->pos_in_zipfile + pfile_in_zip_read_info->byte_before_the_zipfile,SEEK_SET))
        return UNZ_ERRNO;
      if (lufread(pfile_in_zip_read_info->read_buffer,uReadThis,1,pfile_in_zip_read_info->file)!=1)
        return UNZ_ERRNO;
      pfile_in_zip_read_info->pos_in_zipfile += uReadThis;
      pfile_in_zip_read_info->rest_read_compressed-=uReadThis;
      pfile_in_zip_read_info->stream.next_in = (Byte *)pfile_in_zip_read_info->read_buffer;
      pfile_in_zip_read_info->stream.avail_in = (unsigned)uReadThis;
    }

    if (!(pfile_in_zip_read_info->compression_method))
    {
      unsigned uDoCopy = 0;
      if (pfile_in_zip_read_info->stream.avail_out < pfile_in_zip_read_info->stream.avail_in)
        uDoCopy = pfile_in_zip_read_info->stream.avail_out ;
      else
        uDoCopy = pfile_in_zip_read_info->stream.avail_in ;
      for (unsigned i=0; i<uDoCopy; i++)
        *(pfile_in_zip_read_info->stream.next_out+i) = *(pfile_in_zip_read_info->stream.next_in+i);
      pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,pfile_in_zip_read_info->stream.next_out,uDoCopy);
      pfile_in_zip_read_info->rest_read_uncompressed-=uDoCopy;
      pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
      pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
      pfile_in_zip_read_info->stream.next_out += uDoCopy;
      pfile_in_zip_read_info->stream.next_in += uDoCopy;
      pfile_in_zip_read_info->stream.total_out += uDoCopy;
      iRead += uDoCopy;
    }
    else
    {
      unsigned long uTotalOutBefore,uTotalOutAfter;
      const Byte *bufBefore;
      unsigned long uOutThis;
      int flush=Z_SYNC_FLUSH;
      uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
      bufBefore = pfile_in_zip_read_info->stream.next_out;
      err=inflate(&pfile_in_zip_read_info->stream,flush);
      uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
      uOutThis = uTotalOutAfter-uTotalOutBefore;
      pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,bufBefore,(unsigned)(uOutThis));
      pfile_in_zip_read_info->rest_read_uncompressed -= uOutThis;
      iRead += (unsigned)(uTotalOutAfter - uTotalOutBefore);
      if (err==Z_STREAM_END)
        return (!iRead) ? UNZ_EOF : iRead; //+++1.3

      if (err != Z_OK)
        break;
    }
  }
  return iRead;
}

} // anonymous namespace

void *libcdr::CDRUnzip::OpenZip(WPXInputStream *is)
{
  LUFILE *f = lufopen(is);
  if (!f)
    return 0;
  unzFile uf = unzOpenInternal(f);
  return (void *)uf;
}

unsigned libcdr::CDRUnzip::FindZipItem(void *hz, const char *name, int &index, unsigned long &size)
{
  if (!hz || !name)
    return ZR_ARGS;
  unzFile uf = (unzFile)hz;
  if (UNZ_OK!=unzLocateFile(uf,name))
  {
    index=-1;
    return ZR_NOTFOUND;
  }
  index = (int)uf->num_file;
  if (index<-1 || index>=(int)uf->gi.number_entry)
    return ZR_ARGS;
  if (index==-1)
  {
    index = uf->gi.number_entry;
    size=0;
    return ZR_OK;
  }
  if (index<(int)uf->num_file)
    unzGoToFirstFile(uf);
  while ((int)uf->num_file<index)
    unzGoToNextFile(uf);
  unz_file_info ufi;
  char fn[1024];
  unzGetCurrentFileInfo(uf,&ufi,fn,1024,0,0,0,0);

  unsigned extralen,iSizeVar;
  unsigned long offset;
  int res = unzlocal_CheckCurrentFileCoherencyHeader(uf,&iSizeVar,&offset,&extralen);
  if (res!=UNZ_OK)
    return ZR_CORRUPT;
  if (lufseek(uf->file,offset,SEEK_SET))
    return ZR_READ;
  char *extra = new char[extralen];
  if (lufread(extra,1,(unsigned)extralen,uf->file)!=extralen)
  {
    delete[] extra;
    return ZR_READ;
  }

  size = ufi.uncompressed_size;

  if (extra)
    delete[] extra;

  return ZR_OK;
}

unsigned libcdr::CDRUnzip::UnzipItem(void *hz, void *dst, int index, unsigned long size)
{
  if (!hz)
    return ZR_ARGS;
  unzFile uf = (unzFile)hz;
  if (index>=(int)uf->gi.number_entry)
    return ZR_ARGS;
  if (index<(int)uf->num_file)
    unzGoToFirstFile(uf);
  while ((int)uf->num_file<index)
    unzGoToNextFile(uf);
  unzOpenCurrentFile(uf);
  int res = unzReadCurrentFile(uf,dst,size);
  if (res>0)
    return ZR_MORE;
  unzCloseCurrentFile(uf);
  if (!res)
    return ZR_OK;
  else
    return ZR_FLATE;
}

unsigned libcdr::CDRUnzip::CloseZip(void *hz)
{
  if (!hz)
    return ZR_ARGS;
  unzFile uf = (unzFile)hz;
  unzClose(uf);
  uf=0;
  return ZR_OK;
}
