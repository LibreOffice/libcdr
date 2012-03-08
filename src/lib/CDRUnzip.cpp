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


static unsigned zopenerror = ZR_OK;

typedef struct tm_unz_s
{
  unsigned tm_sec;            // seconds after the minute - [0,59]
  unsigned tm_min;            // minutes after the hour - [0,59]
  unsigned tm_hour;           // hours since midnight - [0,23]
  unsigned tm_mday;           // day of the month - [1,31]
  unsigned tm_mon;            // months since January - [0,11]
  unsigned tm_year;           // years - [1980..2044]
} tm_unz;

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
  tm_unz tmu_date;
} unz_file_info;

const char unz_copyright[] = " ";//unzip 0.15 Copyright 1998 Gilles Vollant ";

// unz_file_info_interntal contain internal info about a file in zipfile
typedef struct unz_file_info_internal_s
{
  uLong offset_curfile;// relative offset of local header 4 bytes
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

  uLong pos_in_zipfile;       // position in byte on the zipfile, for fseek
  uLong stream_initialised;   // flag set if stream structure is initialised

  uLong offset_local_extrafield;// offset of the local extra field
  uInt  size_local_extrafield;// size of the local extra field
  uLong pos_local_extrafield;   // position in the local extra field in read

  uLong crc32;                // crc32 of all data uncompressed
  uLong crc32_wait;           // crc32 we must obtain after decompress all
  uLong rest_read_compressed; // number of byte to be decompressed
  uLong rest_read_uncompressed;//number of byte to be obtained after decomp
  LUFILE *file;                 // io structore of the zipfile
  uLong compression_method;   // compression method (0==store)
  uLong byte_before_the_zipfile;// byte before the zipfile, (>0 for sfx)
} file_in_zip_read_info_s;

// unz_s contain internal information about the zipfile
typedef struct
{
  LUFILE *file;               // io structore of the zipfile
  unz_global_info gi;         // public global information
  uLong byte_before_the_zipfile;// byte before the zipfile, (>0 for sfx)
  uLong num_file;             // number of the current file in the zipfile
  uLong pos_in_central_dir;   // pos of the current file in the central dir
  uLong current_file_ok;      // flag about the usability of the current file
  uLong central_pos;          // position of the beginning of the central dir

  uLong size_central_dir;     // size of the central directory
  uLong offset_central_dir;   // offset of start of central directory with respect to the starting disk number

  unz_file_info cur_file_info; // public info about the current file in zip
  unz_file_info_internal cur_file_info_internal; // private info about it
  file_in_zip_read_info_s *pfile_in_zip_read; // structure about the current file if we are decompressing it
} unz_s, *unzFile;

class TUnzip
{
public:
  TUnzip() : uf(0), currentfile(-1), cze(), czei(-1) {}

  unzFile uf;
  int currentfile;
  ZipEntry cze;
  int czei;

  unsigned Open(WPXInputStream *is);
  unsigned Get(int index,ZipEntry *ze);
  unsigned Find(const char *name,int *index,ZipEntry *ze);
  unsigned Unzip(int index,void *dst,unsigned len);
  unsigned Close();
};

typedef struct
{
  unsigned flag;
  TUnzip *unz;
} TUnzipHandleData;

static LUFILE *lufopen(WPXInputStream *is,unsigned *err)
{
  LUFILE *lf = new LUFILE;
  while(!is->atEOS())
    is->seek(1, WPX_SEEK_CUR);
  lf->len=is->tell();
  is->seek(0, WPX_SEEK_SET);
  lf->is=is;
  *err=ZR_OK;
  return lf;
}

static int lufclose(LUFILE *stream)
{
  if (stream==NULL) return EOF;
  delete stream;
  return 0;
}

static int luferror(LUFILE * /* stream */)
{
  return 0;
}

static long int luftell(LUFILE *stream)
{
  return stream->is->tell();
}

static int lufseek(LUFILE *stream, long offset, int whence)
{
  if (whence==SEEK_SET) stream->is->seek(offset, WPX_SEEK_SET);
  else if (whence==SEEK_CUR) stream->is->seek(offset, WPX_SEEK_CUR);
  else if (whence==SEEK_END) stream->is->seek(stream->len+offset, WPX_SEEK_SET);
  return 0;
}

static size_t lufread(void *ptr,size_t size,size_t n,LUFILE *stream)
{
  unsigned long numBytes = (unsigned long)(size*n);
  if (numBytes+stream->is->tell() > stream->len) numBytes=stream->len-stream->is->tell();
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
    if (luferror(fin)) return UNZ_ERRNO;
    else return UNZ_EOF;
  }
}

static int unzlocal_getShort (LUFILE *fin,uLong *pX)
{
  uLong x ;
  int i;
  int err;

  err = unzlocal_getByte(fin,&i);
  x = (uLong)i;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((uLong)i)<<8;

  if (err==UNZ_OK)
    *pX = x;
  else
    *pX = 0;
  return err;
}

static int unzlocal_getLong (LUFILE *fin,uLong *pX)
{
  uLong x ;
  int i;
  int err;

  err = unzlocal_getByte(fin,&i);
  x = (uLong)i;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((uLong)i)<<8;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((uLong)i)<<16;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((uLong)i)<<24;

  if (err==UNZ_OK)
    *pX = x;
  else
    *pX = 0;
  return err;
}

#define BUFREADCOMMENT (0x400)

static uLong unzlocal_SearchCentralDir(LUFILE *fin)
{
  if (lufseek(fin,0,SEEK_END) != 0) return 0;
  uLong uSizeFile = luftell(fin);

  uLong uMaxBack=0xffff; // maximum size of global comment
  if (uMaxBack>uSizeFile) uMaxBack = uSizeFile;

  unsigned char *buf = (unsigned char *)zmalloc(BUFREADCOMMENT+4);
  if (buf==NULL) return 0;
  uLong uPosFound=0;

  uLong uBackRead = 4;
  while (uBackRead<uMaxBack)
  {
    uLong uReadSize,uReadPos ;
    int i;
    if (uBackRead+BUFREADCOMMENT>uMaxBack) uBackRead = uMaxBack;
    else uBackRead+=BUFREADCOMMENT;
    uReadPos = uSizeFile-uBackRead ;
    uReadSize = ((BUFREADCOMMENT+4) < (uSizeFile-uReadPos)) ? (BUFREADCOMMENT+4) : (uSizeFile-uReadPos);
    if (lufseek(fin,uReadPos,SEEK_SET)!=0) break;
    if (lufread(buf,(uInt)uReadSize,1,fin)!=1) break;
    for (i=(int)uReadSize-3; (i--)>0;)
    {
      if (((*(buf+i))==0x50) && ((*(buf+i+1))==0x4b) &&	((*(buf+i+2))==0x05) && ((*(buf+i+3))==0x06))
      {
        uPosFound = uReadPos+i;
        break;
      }
    }
    if (uPosFound!=0) break;
  }
  if (buf) zfree(buf);
  return uPosFound;
}

static void unzlocal_DosDateToTmuDate (uLong ulDosDate, tm_unz *ptm)
{
  uLong uDate;
  uDate = (uLong)(ulDosDate>>16);
  ptm->tm_mday = (uInt)(uDate&0x1f) ;
  ptm->tm_mon =  (uInt)((((uDate)&0x1E0)/0x20)-1) ;
  ptm->tm_year = (uInt)(((uDate&0x0FE00)/0x0200)+1980) ;

  ptm->tm_hour = (uInt) ((ulDosDate &0xF800)/0x800);
  ptm->tm_min =  (uInt) ((ulDosDate&0x7E0)/0x20) ;
  ptm->tm_sec =  (uInt) (2*(ulDosDate&0x1f)) ;
}

static int unzlocal_GetCurrentFileInfoInternal (unzFile file, unz_file_info *pfile_info,
    unz_file_info_internal *pfile_info_internal, char *szFileName,
    uLong fileNameBufferSize, void *extraField, uLong extraFieldBufferSize,
    char *szComment, uLong commentBufferSize)
{
  unz_s *s;
  unz_file_info file_info;
  unz_file_info_internal file_info_internal;
  int err=UNZ_OK;
  uLong uMagic;
  long lSeek=0;

  if (file==NULL)
    return UNZ_PARAMERROR;
  s=(unz_s *)file;
  if (lufseek(s->file,s->pos_in_central_dir+s->byte_before_the_zipfile,SEEK_SET)!=0)
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

  unzlocal_DosDateToTmuDate(file_info.dosDate,&file_info.tmu_date);

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
  if ((err==UNZ_OK) && (szFileName!=NULL))
  {
    uLong uSizeRead ;
    if (file_info.size_filename<fileNameBufferSize)
    {
      *(szFileName+file_info.size_filename)='\0';
      uSizeRead = file_info.size_filename;
    }
    else
      uSizeRead = fileNameBufferSize;

    if ((file_info.size_filename>0) && (fileNameBufferSize>0))
      if (lufread(szFileName,(uInt)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    lSeek -= uSizeRead;
  }

  if ((err==UNZ_OK) && (extraField!=NULL))
  {
    uLong uSizeRead ;
    if (file_info.size_file_extra<extraFieldBufferSize)
      uSizeRead = file_info.size_file_extra;
    else
      uSizeRead = extraFieldBufferSize;

    if (lSeek!=0)
    {
      if (lufseek(s->file,lSeek,SEEK_CUR)==0)
        lSeek=0;
      else
        err=UNZ_ERRNO;
    }
    if ((file_info.size_file_extra>0) && (extraFieldBufferSize>0))
      if (lufread(extraField,(uInt)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    lSeek += file_info.size_file_extra - uSizeRead;
  }
  else
    lSeek+=file_info.size_file_extra;

  if ((err==UNZ_OK) && (szComment!=NULL))
  {
    uLong uSizeRead ;
    if (file_info.size_file_comment<commentBufferSize)
    {
      *(szComment+file_info.size_file_comment)='\0';
      uSizeRead = file_info.size_file_comment;
    }
    else
      uSizeRead = commentBufferSize;

    if (lSeek!=0)
    {
      if (lufseek(s->file,lSeek,SEEK_CUR)==0)
        {} // unused lSeek=0;
      else
        err=UNZ_ERRNO;
    }
    if ((file_info.size_file_comment>0) && (commentBufferSize>0))
      if (lufread(szComment,(uInt)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    //unused lSeek+=file_info.size_file_comment - uSizeRead;
  }
  else {} //unused lSeek+=file_info.size_file_comment;

  if ((err==UNZ_OK) && (pfile_info!=NULL))
    *pfile_info=file_info;

  if ((err==UNZ_OK) && (pfile_info_internal!=NULL))
    *pfile_info_internal=file_info_internal;

  return err;
}

static int unzGoToFirstFile (unzFile file)
{
  int err;
  unz_s *s;
  if (file==NULL) return UNZ_PARAMERROR;
  s=(unz_s *)file;
  s->pos_in_central_dir=s->offset_central_dir;
  s->num_file=0;
  err=unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
                                          &s->cur_file_info_internal,
                                          NULL,0,NULL,0,NULL,0);
  s->current_file_ok = (err == UNZ_OK);
  return err;
}

static int unzCloseCurrentFile (unzFile file)
{
  int err=UNZ_OK;

  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  if (file==NULL)
    return UNZ_PARAMERROR;
  s=(unz_s *)file;
  pfile_in_zip_read_info=s->pfile_in_zip_read;

  if (pfile_in_zip_read_info==NULL)
    return UNZ_PARAMERROR;

  if (pfile_in_zip_read_info->rest_read_uncompressed == 0)
  {
    if (pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait)
      err=UNZ_CRCERROR;
  }

  if (pfile_in_zip_read_info->read_buffer!=0)
  {
    void *buf = pfile_in_zip_read_info->read_buffer;
    zfree(buf);
    pfile_in_zip_read_info->read_buffer=0;
  }
  pfile_in_zip_read_info->read_buffer = NULL;
  if (pfile_in_zip_read_info->stream_initialised)
    inflateEnd(&pfile_in_zip_read_info->stream);

  pfile_in_zip_read_info->stream_initialised = 0;
  if (pfile_in_zip_read_info!=0) zfree(pfile_in_zip_read_info); // unused pfile_in_zip_read_info=0;

  s->pfile_in_zip_read=NULL;

  return err;
}

static unzFile unzOpenInternal(LUFILE *fin)
{
  zopenerror = ZR_OK;
  if (fin==NULL)
  {
    zopenerror = ZR_ARGS;
    return NULL;
  }
  if (unz_copyright[0]!=' ')
  {
    lufclose(fin);
    zopenerror = ZR_CORRUPT;
    return NULL;
  }

  int err=UNZ_OK;
  unz_s us;
  uLong central_pos,uL;
  central_pos = unzlocal_SearchCentralDir(fin);
  if (central_pos==0) err=UNZ_ERRNO;
  if (lufseek(fin,central_pos,SEEK_SET)!=0) err=UNZ_ERRNO;
  // the signature, already checked
  if (unzlocal_getLong(fin,&uL)!=UNZ_OK) err=UNZ_ERRNO;
  // number of this disk
  uLong number_disk;          // number of the current dist, used for spanning ZIP, unsupported, always 0
  if (unzlocal_getShort(fin,&number_disk)!=UNZ_OK) err=UNZ_ERRNO;
  // number of the disk with the start of the central directory
  uLong number_disk_with_CD;  // number the the disk with central dir, used for spaning ZIP, unsupported, always 0
  if (unzlocal_getShort(fin,&number_disk_with_CD)!=UNZ_OK) err=UNZ_ERRNO;
  // total number of entries in the central dir on this disk
  if (unzlocal_getShort(fin,&us.gi.number_entry)!=UNZ_OK) err=UNZ_ERRNO;
  // total number of entries in the central dir
  uLong number_entry_CD;      // total number of entries in the central dir (same than number_entry on nospan)
  if (unzlocal_getShort(fin,&number_entry_CD)!=UNZ_OK) err=UNZ_ERRNO;
  if ((number_entry_CD!=us.gi.number_entry) || (number_disk_with_CD!=0) || (number_disk!=0)) err=UNZ_BADZIPFILE;
  // size of the central directory
  if (unzlocal_getLong(fin,&us.size_central_dir)!=UNZ_OK) err=UNZ_ERRNO;
  // offset of start of central directory with respect to the starting disk number
  if (unzlocal_getLong(fin,&us.offset_central_dir)!=UNZ_OK) err=UNZ_ERRNO;
  // zipfile comment length
  if (unzlocal_getShort(fin,&us.gi.size_comment)!=UNZ_OK) err=UNZ_ERRNO;
  if ((central_pos<us.offset_central_dir+us.size_central_dir) && (err==UNZ_OK)) err=UNZ_BADZIPFILE;
  //if (err!=UNZ_OK) {lufclose(fin);return NULL;}
  if (err!=UNZ_OK)
  {
    lufclose(fin);
    zopenerror = err;
    return NULL;
  }

  us.file=fin;
  us.byte_before_the_zipfile = central_pos - (us.offset_central_dir+us.size_central_dir);
  us.central_pos = central_pos;
  us.pfile_in_zip_read = NULL;

  unz_s *s = (unz_s *)zmalloc(sizeof(unz_s));
  *s=us;
  unzGoToFirstFile((unzFile)s);
  return (unzFile)s;
}

static int unzClose (unzFile file)
{
  unz_s *s;
  if (file==NULL)
    return UNZ_PARAMERROR;
  s=(unz_s *)file;

  if (s->pfile_in_zip_read!=NULL)
    unzCloseCurrentFile(file);

  lufclose(s->file);
  if (s) zfree(s); // unused s=0;
  return UNZ_OK;
}

static int unzGetCurrentFileInfo (unzFile file, unz_file_info *pfile_info,
                                  char *szFileName, uLong fileNameBufferSize, void *extraField, uLong extraFieldBufferSize,
                                  char *szComment, uLong commentBufferSize)
{
  return unzlocal_GetCurrentFileInfoInternal(file,pfile_info,NULL,szFileName,fileNameBufferSize,
         extraField,extraFieldBufferSize, szComment,commentBufferSize);
}

static int unzGoToNextFile (unzFile file)
{
  unz_s *s;
  int err;

  if (file==NULL)
    return UNZ_PARAMERROR;
  s=(unz_s *)file;
  if (!s->current_file_ok)
    return UNZ_END_OF_LIST_OF_FILE;
  if (s->num_file+1==s->gi.number_entry)
    return UNZ_END_OF_LIST_OF_FILE;

  s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
                           s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment ;
  s->num_file++;
  err = unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
        &s->cur_file_info_internal,
        NULL,0,NULL,0,NULL,0);
  s->current_file_ok = (err == UNZ_OK);
  return err;
}

static int unzLocateFile (unzFile file, const char *szFileName)
{
  unz_s *s;
  int err;

  uLong num_fileSaved;
  uLong pos_in_central_dirSaved;

  if (file==NULL)
    return UNZ_PARAMERROR;

  if (strlen(szFileName)>=UNZ_MAXFILENAMEINZIP)
    return UNZ_PARAMERROR;

  s=(unz_s *)file;
  if (!s->current_file_ok)
    return UNZ_END_OF_LIST_OF_FILE;

  num_fileSaved = s->num_file;
  pos_in_central_dirSaved = s->pos_in_central_dir;

  err = unzGoToFirstFile(file);

  while (err == UNZ_OK)
  {
    char szCurrentFileName[UNZ_MAXFILENAMEINZIP+1];
    unzGetCurrentFileInfo(file,NULL,
                          szCurrentFileName,sizeof(szCurrentFileName)-1,
                          NULL,0,NULL,0);
    if (strcmp(szCurrentFileName,szFileName)==0)
      return UNZ_OK;
    err = unzGoToNextFile(file);
  }

  s->num_file = num_fileSaved ;
  s->pos_in_central_dir = pos_in_central_dirSaved ;
  return err;
}

static int unzlocal_CheckCurrentFileCoherencyHeader (unz_s *s,uInt *piSizeVar,
    uLong *poffset_local_extrafield, uInt  *psize_local_extrafield)
{
  uLong uMagic,uData,uFlags;
  uLong size_filename;
  uLong size_extra_field;
  int err=UNZ_OK;

  *piSizeVar = 0;
  *poffset_local_extrafield = 0;
  *psize_local_extrafield = 0;

  if (lufseek(s->file,s->cur_file_info_internal.offset_curfile + s->byte_before_the_zipfile,SEEK_SET)!=0)
    return UNZ_ERRNO;

  if (err==UNZ_OK)
  {
    if (unzlocal_getLong(s->file,&uMagic) != UNZ_OK)
      err=UNZ_ERRNO;
    else if (uMagic!=0x04034b50)
      err=UNZ_BADZIPFILE;
  }

  if (unzlocal_getShort(s->file,&uData) != UNZ_OK)
    err=UNZ_ERRNO;
  if (unzlocal_getShort(s->file,&uFlags) != UNZ_OK)
    err=UNZ_ERRNO;

  if (unzlocal_getShort(s->file,&uData) != UNZ_OK)
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compression_method))
    err=UNZ_BADZIPFILE;

  if ((err==UNZ_OK) && (s->cur_file_info.compression_method!=0) &&
      (s->cur_file_info.compression_method!=Z_DEFLATED))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // date/time
    err=UNZ_ERRNO;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // crc
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.crc) &&
           ((uFlags & 8)==0))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // size compr
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compressed_size) &&
           ((uFlags & 8)==0))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getLong(s->file,&uData) != UNZ_OK) // size uncompr
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (uData!=s->cur_file_info.uncompressed_size) &&
           ((uFlags & 8)==0))
    err=UNZ_BADZIPFILE;

  if (unzlocal_getShort(s->file,&size_filename) != UNZ_OK)
    err=UNZ_ERRNO;
  else if ((err==UNZ_OK) && (size_filename!=s->cur_file_info.size_filename))
    err=UNZ_BADZIPFILE;

  *piSizeVar += (uInt)size_filename;

  if (unzlocal_getShort(s->file,&size_extra_field) != UNZ_OK)
    err=UNZ_ERRNO;
  *poffset_local_extrafield= s->cur_file_info_internal.offset_curfile +
                             SIZEZIPLOCALHEADER + size_filename;
  *psize_local_extrafield = (uInt)size_extra_field;

  *piSizeVar += (uInt)size_extra_field;

  return err;
}

static int unzOpenCurrentFile (unzFile file)
{
  int err;
  int Store;
  uInt iSizeVar;
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  uLong offset_local_extrafield;  // offset of the local extra field
  uInt  size_local_extrafield;    // size of the local extra field

  if (file==NULL)
    return UNZ_PARAMERROR;
  s=(unz_s *)file;
  if (!s->current_file_ok)
    return UNZ_PARAMERROR;

  if (s->pfile_in_zip_read != NULL)
    unzCloseCurrentFile(file);

  if (unzlocal_CheckCurrentFileCoherencyHeader(s,&iSizeVar,
      &offset_local_extrafield,&size_local_extrafield)!=UNZ_OK)
    return UNZ_BADZIPFILE;

  pfile_in_zip_read_info = (file_in_zip_read_info_s *)zmalloc(sizeof(file_in_zip_read_info_s));
  if (pfile_in_zip_read_info==NULL)
    return UNZ_INTERNALERROR;

  pfile_in_zip_read_info->read_buffer=(char *)zmalloc(UNZ_BUFSIZE);
  pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
  pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
  pfile_in_zip_read_info->pos_local_extrafield=0;

  if (pfile_in_zip_read_info->read_buffer==NULL)
  {
    if (pfile_in_zip_read_info!=0) zfree(pfile_in_zip_read_info); //unused pfile_in_zip_read_info=0;
    return UNZ_INTERNALERROR;
  }

  pfile_in_zip_read_info->stream_initialised=0;

  if ((s->cur_file_info.compression_method!=0) && (s->cur_file_info.compression_method!=Z_DEFLATED))
  {
    // unused err=UNZ_BADZIPFILE;
  }
  Store = s->cur_file_info.compression_method==0;

  pfile_in_zip_read_info->crc32_wait=s->cur_file_info.crc;
  pfile_in_zip_read_info->crc32=0;
  pfile_in_zip_read_info->compression_method =
    s->cur_file_info.compression_method;
  pfile_in_zip_read_info->file=s->file;
  pfile_in_zip_read_info->byte_before_the_zipfile=s->byte_before_the_zipfile;

  pfile_in_zip_read_info->stream.total_out = 0;

  if (!Store)
  {
    pfile_in_zip_read_info->stream.zalloc = (alloc_func)0;
    pfile_in_zip_read_info->stream.zfree = (free_func)0;
    pfile_in_zip_read_info->stream.opaque = (voidpf)0;

    err=inflateInit2(&pfile_in_zip_read_info->stream, 0);
    if (err == Z_OK)
      pfile_in_zip_read_info->stream_initialised=1;
    // windowBits is passed < 0 to tell that there is no zlib header.
    // Note that in this case inflate *requires* an extra "dummy" byte
    // after the compressed stream in order to complete decompression and
    // return Z_STREAM_END.
    // In unzip, i don't wait absolutely Z_STREAM_END because I known the
    // size of both compressed and uncompressed data
  }
  pfile_in_zip_read_info->rest_read_compressed =
    s->cur_file_info.compressed_size ;
  pfile_in_zip_read_info->rest_read_uncompressed =
    s->cur_file_info.uncompressed_size ;

  pfile_in_zip_read_info->pos_in_zipfile =
    s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER +
    iSizeVar;

  pfile_in_zip_read_info->stream.avail_in = (uInt)0;

  s->pfile_in_zip_read = pfile_in_zip_read_info;
  return UNZ_OK;
}

static int unzReadCurrentFile  (unzFile file, voidp buf, unsigned len)
{
  int err=UNZ_OK;
  uInt iRead = 0;

  unz_s *s = (unz_s *)file;
  if (s==NULL) return UNZ_PARAMERROR;

  file_in_zip_read_info_s *pfile_in_zip_read_info = s->pfile_in_zip_read;
  if (pfile_in_zip_read_info==NULL) return UNZ_PARAMERROR;
  if ((pfile_in_zip_read_info->read_buffer == NULL)) return UNZ_END_OF_LIST_OF_FILE;
  if (len==0) return 0;

  pfile_in_zip_read_info->stream.next_out = (Byte *)buf;
  pfile_in_zip_read_info->stream.avail_out = (uInt)len;

  if (len>pfile_in_zip_read_info->rest_read_uncompressed)
  {
    pfile_in_zip_read_info->stream.avail_out = (uInt)pfile_in_zip_read_info->rest_read_uncompressed;
  }

  while (pfile_in_zip_read_info->stream.avail_out>0)
  {
    if ((pfile_in_zip_read_info->stream.avail_in==0) && (pfile_in_zip_read_info->rest_read_compressed>0))
    {
      uInt uReadThis = UNZ_BUFSIZE;
      if (pfile_in_zip_read_info->rest_read_compressed<uReadThis) uReadThis = (uInt)pfile_in_zip_read_info->rest_read_compressed;
      if (uReadThis == 0) return UNZ_EOF;
      if (lufseek(pfile_in_zip_read_info->file, pfile_in_zip_read_info->pos_in_zipfile + pfile_in_zip_read_info->byte_before_the_zipfile,SEEK_SET)!=0) return UNZ_ERRNO;
      if (lufread(pfile_in_zip_read_info->read_buffer,uReadThis,1,pfile_in_zip_read_info->file)!=1) return UNZ_ERRNO;
      pfile_in_zip_read_info->pos_in_zipfile += uReadThis;
      pfile_in_zip_read_info->rest_read_compressed-=uReadThis;
      pfile_in_zip_read_info->stream.next_in = (Byte *)pfile_in_zip_read_info->read_buffer;
      pfile_in_zip_read_info->stream.avail_in = (uInt)uReadThis;
    }

    if (pfile_in_zip_read_info->compression_method==0)
    {
      uInt uDoCopy,i ;
      if (pfile_in_zip_read_info->stream.avail_out < pfile_in_zip_read_info->stream.avail_in)
      {
        uDoCopy = pfile_in_zip_read_info->stream.avail_out ;
      }
      else
      {
        uDoCopy = pfile_in_zip_read_info->stream.avail_in ;
      }
      for (i=0; i<uDoCopy; i++)
      {
        *(pfile_in_zip_read_info->stream.next_out+i) = *(pfile_in_zip_read_info->stream.next_in+i);
      }
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
      uLong uTotalOutBefore,uTotalOutAfter;
      const Byte *bufBefore;
      uLong uOutThis;
      int flush=Z_SYNC_FLUSH;
      uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
      bufBefore = pfile_in_zip_read_info->stream.next_out;
      err=inflate(&pfile_in_zip_read_info->stream,flush);
      uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
      uOutThis = uTotalOutAfter-uTotalOutBefore;
      pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,bufBefore,(uInt)(uOutThis));
      pfile_in_zip_read_info->rest_read_uncompressed -= uOutThis;
      iRead += (uInt)(uTotalOutAfter - uTotalOutBefore);
      if (err==Z_STREAM_END) return (iRead==0) ? UNZ_EOF : iRead;			//+++1.3
      //if (err==Z_STREAM_END) return (iRead==len) ? UNZ_EOF : iRead;

      if (err != Z_OK) break;
    }
  }

  if (err==Z_OK) return iRead;

  return iRead;
}

unsigned TUnzip::Open(WPXInputStream *is)
{
  if (uf!=0 || currentfile!=-1)
    return ZR_NOTINITED;
  unsigned e;
  LUFILE *f = lufopen(is,&e);
  if (f==NULL)
    return e;
  uf = unzOpenInternal(f);
  return zopenerror;
}

unsigned TUnzip::Get(int index,ZipEntry *ze)
{
  if (index<-1 || index>=(int)uf->gi.number_entry)
    return ZR_ARGS;
  if (currentfile!=-1)
    unzCloseCurrentFile(uf);
  currentfile=-1;
  if (index==czei && index!=-1)
  {
    memcpy(ze,&cze,sizeof(ZipEntry));
    return ZR_OK;
  }
  if (index==-1)
  {
    ze->index = uf->gi.number_entry;
    ze->comp_size=0;
    ze->unc_size=0;
    return ZR_OK;
  }
  if (index<(int)uf->num_file) unzGoToFirstFile(uf);
  while ((int)uf->num_file<index) unzGoToNextFile(uf);
  unz_file_info ufi;
  char fn[1024];
  unzGetCurrentFileInfo(uf,&ufi,fn,1024,NULL,0,NULL,0);

  unsigned extralen,iSizeVar;
  unsigned long offset;
  int res = unzlocal_CheckCurrentFileCoherencyHeader(uf,&iSizeVar,&offset,&extralen);
  if (res!=UNZ_OK) return ZR_CORRUPT;
  if (lufseek(uf->file,offset,SEEK_SET)!=0) return ZR_READ;
  char *extra = new char[extralen];
  if (lufread(extra,1,(uInt)extralen,uf->file)!=extralen)
  {
    delete[] extra;
    return ZR_READ;
  }
  //
  ze->index=uf->num_file;
  ze->comp_size = ufi.compressed_size;
  ze->unc_size = ufi.uncompressed_size;
  //
  if (extra!=0) delete[] extra;
  memcpy(&cze,ze,sizeof(ZipEntry));
  czei=index;
  return ZR_OK;
}

unsigned TUnzip::Find(const char *name, int *index, ZipEntry *ze)
{
  int res = unzLocateFile(uf,name);
  if (res!=UNZ_OK)
  {
    if (index!=0)
      *index=-1;
    return ZR_NOTFOUND;
  }
  if (currentfile!=-1)
    unzCloseCurrentFile(uf);
  currentfile=-1;
  int i = (int)uf->num_file;
  if (index!=NULL)
    *index=i;
  if (ze!=NULL)
  {
    unsigned zres = Get(i,ze);
    if (zres!=ZR_OK)
      return zres;
  }
  return ZR_OK;
}

unsigned TUnzip::Unzip(int index,void *dst,unsigned len)
{
  if (index!=currentfile)
  {
    if (currentfile!=-1)
      unzCloseCurrentFile(uf);
    currentfile=-1;
    if (index>=(int)uf->gi.number_entry)
      return ZR_ARGS;
    if (index<(int)uf->num_file)
      unzGoToFirstFile(uf);
    while ((int)uf->num_file<index)
      unzGoToNextFile(uf);
    unzOpenCurrentFile(uf);
    currentfile=index;
  }
  int res = unzReadCurrentFile(uf,dst,len);
  if (res>0)
    return ZR_MORE;
  unzCloseCurrentFile(uf);
  currentfile=-1;
  if (res==0)
    return ZR_OK;
  else
    return ZR_FLATE;
}

unsigned TUnzip::Close()
{
  if (currentfile!=-1) unzCloseCurrentFile(uf);
  currentfile=-1;
  if (uf!=0) unzClose(uf);
  uf=0;
  return ZR_OK;
}

} // anonymous namespace

void *libcdr::CDRUnzip::OpenZip(WPXInputStream *is)
{
  TUnzip *unz = new TUnzip();
  if (unz->Open(is)!=ZR_OK)
  {
    delete unz;
    return 0;
  }
  TUnzipHandleData *han = new TUnzipHandleData;
  han->flag=1;
  han->unz=unz;
  return (void *)han;
}

unsigned libcdr::CDRUnzip::FindZipItem(void *hz, const char *name, int *index, ZipEntry *ze)
{
  if (hz==0)
    return ZR_ARGS;
  TUnzipHandleData *han = (TUnzipHandleData *)hz;
  if (han->flag!=1)
    return ZR_ZMODE;
  TUnzip *unz = han->unz;
  return unz->Find(name,index,ze);
}

unsigned libcdr::CDRUnzip::UnzipItem(void *hz, int index, void *dst, unsigned len)
{
  if (hz==0)
    return ZR_ARGS;
  TUnzipHandleData *han = (TUnzipHandleData *)hz;
  if (han->flag!=1)
    return ZR_ZMODE;
  TUnzip *unz = han->unz;
  return unz->Unzip(index,dst,len);
}

unsigned libcdr::CDRUnzip::CloseZip(void *hz)
{
  if (hz==0)
    return ZR_ARGS;
  TUnzipHandleData *han = (TUnzipHandleData *)hz;
  if (han->flag!=1)
    return ZR_ZMODE;
  TUnzip *unz = han->unz;
  unsigned ret = unz->Close();
  delete unz;
  delete han;
  return ret;
}
