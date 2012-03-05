// CDRUnzip.cpp  Version 1.3
//
// Authors:      Mark Adler et al. (see below)
//
// Modified by:  Lucian Wischik
//               lu@wischik.com
//
// Version 1.0   - Turned C files into just a single CPP file
//               - Made them compile cleanly as C++ files
//               - Gave them simpler APIs
//               - Added the ability to zip/unzip directly in memory without
//                 any intermediate files
//
// Modified by:  Hans Dietrich
//               hdietrich@gmail.com
//
// Version 1.3:  - Corrected size bug introduced by 1.2
//
// Version 1.2:  - Many bug fixes.  See CodeProject article for list.
//
// Version 1.1:  - Added Unicode support to CreateZip() and ZipAdd()
//               - Changed file names to avoid conflicts with Lucian's files
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
#include <zutil.h>
#include "CDRUnzip.h"

using namespace libcdr;

#define zmalloc(len) malloc(len)

#define zfree(p) free(p)

static unsigned zopenerror = 0;

typedef struct tm_unz_s
{
  unsigned int tm_sec;            // seconds after the minute - [0,59]
  unsigned int tm_min;            // minutes after the hour - [0,59]
  unsigned int tm_hour;           // hours since midnight - [0,23]
  unsigned int tm_mday;           // day of the month - [1,31]
  unsigned int tm_mon;            // months since January - [0,11]
  unsigned int tm_year;           // years - [1980..2044]
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

#define UNZ_OK                  (0)
#define UNZ_END_OF_LIST_OF_FILE (-100)
#define UNZ_ERRNO               (Z_ERRNO)
#define UNZ_EOF                 (0)
#define UNZ_PARAMERROR          (-102)
#define UNZ_BADZIPFILE          (-103)
#define UNZ_INTERNALERROR       (-104)
#define UNZ_CRCERROR            (-105)

// case sensitivity when searching for filenames
#define CASE_SENSITIVE 1

// unzip.c -- IO on .zip files using zlib
// Version 0.15 beta, Mar 19th, 1998,
// Read unzip.h for more info

#define UNZ_BUFSIZE (16384)
#define UNZ_MAXFILENAMEINZIP (256)
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)

const char unz_copyright[] = " ";//unzip 0.15 Copyright 1998 Gilles Vollant ";

// unz_file_info_interntal contain internal info about a file in zipfile
typedef struct unz_file_info_internal_s
{
  unsigned long offset_curfile;// relative offset of local header 4 bytes
} unz_file_info_internal;

typedef struct
{
  void *buf;
  unsigned int len,pos; // if it's a memory block
} LUFILE;

LUFILE *lufopen(void *z,unsigned int len,unsigned *err)
{
  *err=0;
  LUFILE *lf = new LUFILE;
  lf->buf=z;
  lf->len=len;
  lf->pos=0;
  *err=0;
  return lf;
}

int lufclose(LUFILE *stream)
{
  if (stream==NULL) return EOF;
  delete stream;
  return 0;
}

int luferror(LUFILE *stream)
{
  return 0;
}

long int luftell(LUFILE *stream)
{
  return stream->pos;
}

int lufseek(LUFILE *stream, long offset, int whence)
{
  if (whence==SEEK_SET) stream->pos=offset;
  else if (whence==SEEK_CUR) stream->pos+=offset;
  else if (whence==SEEK_END) stream->pos=stream->len+offset;
  return 0;

}

size_t lufread(void *ptr,size_t size,size_t n,LUFILE *stream)
{
  unsigned int toread = (unsigned int)(size*n);
  if (stream->pos+toread > stream->len) toread = stream->len-stream->pos;
  memcpy(ptr, (char *)stream->buf + stream->pos, toread);
  unsigned red = toread;
  stream->pos += red;
  return red/size;
}

// file_in_zip_read_info_s contain internal information about a file in zipfile,
//  when reading and decompress it
typedef struct
{
  char  *read_buffer;         // internal buffer for compressed data
  z_stream stream;            // zLib stream structure for inflate

  unsigned long pos_in_zipfile;       // position in byte on the zipfile, for fseek
  unsigned long stream_initialised;   // flag set if stream structure is initialised

  unsigned long offset_local_extrafield;// offset of the local extra field
  unsigned short  size_local_extrafield;// size of the local extra field
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

int unzStringFileNameCompare (const char *fileName1,const char *fileName2,int iCaseSensitivity);
//   Compare two filename (fileName1,fileName2).

int unzGetLocalExtrafield (unzFile file, void *buf, unsigned len);
//  Read extra field from the current file (opened by unzOpenCurrentFile)
//  This is the local-header version of the extra field (sometimes, there is
//    more info in the local-header version than in the central-header)
//
//  if buf==NULL, it return the size of the local extra field
//
//  if buf!=NULL, len is the size of the buffer, the extra header is copied in
//	buf.
//  the return value is the number of bytes copied in buf, or (if <0)
//	the error code

// ===========================================================================
//   Read a byte from a gz_stream; update next_in and avail_in. Return EOF
// for end of file.
// IN assertion: the stream s has been sucessfully opened for reading.

int unzlocal_getByte(LUFILE *fin,int *pi)
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

// ===========================================================================
// Reads a long in LSB order from the given gz_stream. Sets
int unzlocal_getShort (LUFILE *fin,unsigned long *pX)
{
  unsigned long x ;
  int i;
  int err;

  err = unzlocal_getByte(fin,&i);
  x = (unsigned long)i;

  if (err==UNZ_OK)
    err = unzlocal_getByte(fin,&i);
  x += ((unsigned long)i)<<8;

  if (err==UNZ_OK)
    *pX = x;
  else
    *pX = 0;
  return err;
}

int unzlocal_getLong (LUFILE *fin,unsigned long *pX)
{
  unsigned long x ;
  int i;
  int err;

  err = unzlocal_getByte(fin,&i);
  x = (unsigned long)i;

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

// My own strcmpi / strcasecmp
int strcmpcasenosensitive_internal (const char *fileName1,const char *fileName2)
{
  for (;;)
  {
    char c1=*(fileName1++);
    char c2=*(fileName2++);
    if ((c1>='a') && (c1<='z'))
      c1 -= (char)0x20;
    if ((c2>='a') && (c2<='z'))
      c2 -= (char)0x20;
    if (c1=='\0')
      return ((c2=='\0') ? 0 : -1);
    if (c2=='\0')
      return 1;
    if (c1<c2)
      return -1;
    if (c1>c2)
      return 1;
  }
}

//
// Compare two filename (fileName1,fileName2).
// If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
// If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi or strcasecmp)
//
int unzStringFileNameCompare (const char *fileName1,const char *fileName2,int iCaseSensitivity)
{
  if (iCaseSensitivity==1) return strcmp(fileName1,fileName2);
  else return strcmpcasenosensitive_internal(fileName1,fileName2);
}

#define BUFREADCOMMENT (0x400)

//  Locate the Central directory of a zipfile (at the end, just before
// the global comment)
unsigned long unzlocal_SearchCentralDir(LUFILE *fin)
{
  if (lufseek(fin,0,SEEK_END) != 0) return 0;
  unsigned long uSizeFile = luftell(fin);

  unsigned long uMaxBack=0xffff; // maximum size of global comment
  if (uMaxBack>uSizeFile) uMaxBack = uSizeFile;

  unsigned char *buf = (unsigned char *)zmalloc(BUFREADCOMMENT+4);
  if (buf==NULL) return 0;
  unsigned long uPosFound=0;

  unsigned long uBackRead = 4;
  while (uBackRead<uMaxBack)
  {
    unsigned long uReadSize,uReadPos ;
    int i;
    if (uBackRead+BUFREADCOMMENT>uMaxBack) uBackRead = uMaxBack;
    else uBackRead+=BUFREADCOMMENT;
    uReadPos = uSizeFile-uBackRead ;
    uReadSize = ((BUFREADCOMMENT+4) < (uSizeFile-uReadPos)) ? (BUFREADCOMMENT+4) : (uSizeFile-uReadPos);
    if (lufseek(fin,uReadPos,SEEK_SET)!=0) break;
    if (lufread(buf,(unsigned short)uReadSize,1,fin)!=1) break;
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

int unzGoToFirstFile (unzFile file);
int unzCloseCurrentFile (unzFile file);

// Open a Zip file.
// If the zipfile cannot be opened (file don't exist or in not valid), return NULL.
// Otherwise, the return value is a unzFile Handle, usable with other unzip functions
unzFile unzOpenInternal(LUFILE *fin)
{
  zopenerror = 0; //+++1.2
  if (fin==NULL)
  {
    zopenerror = 1;  //+++1.2
    return NULL;
  }
  if (unz_copyright[0]!=' ')
  {
    lufclose(fin);  //+++1.2
    zopenerror = 1;
    return NULL;
  }

  int err=UNZ_OK;
  unz_s us;
  unsigned long central_pos,uL;
  central_pos = unzlocal_SearchCentralDir(fin);
  if (central_pos==0) err=UNZ_ERRNO;
  if (lufseek(fin,central_pos,SEEK_SET)!=0) err=UNZ_ERRNO;
  // the signature, already checked
  if (unzlocal_getLong(fin,&uL)!=UNZ_OK) err=UNZ_ERRNO;
  // number of this disk
  unsigned long number_disk;          // number of the current dist, used for spanning ZIP, unsupported, always 0
  if (unzlocal_getShort(fin,&number_disk)!=UNZ_OK) err=UNZ_ERRNO;
  // number of the disk with the start of the central directory
  unsigned long number_disk_with_CD;  // number the the disk with central dir, used for spaning ZIP, unsupported, always 0
  if (unzlocal_getShort(fin,&number_disk_with_CD)!=UNZ_OK) err=UNZ_ERRNO;
  // total number of entries in the central dir on this disk
  if (unzlocal_getShort(fin,&us.gi.number_entry)!=UNZ_OK) err=UNZ_ERRNO;
  // total number of entries in the central dir
  unsigned long number_entry_CD;      // total number of entries in the central dir (same than number_entry on nospan)
  if (unzlocal_getShort(fin,&number_entry_CD)!=UNZ_OK) err=UNZ_ERRNO;
  if ((number_entry_CD!=us.gi.number_entry) || (number_disk_with_CD!=0) || (number_disk!=0)) err=UNZ_BADZIPFILE;
  // size of the central directory
  if (unzlocal_getLong(fin,&us.size_central_dir)!=UNZ_OK) err=UNZ_ERRNO;
  // offset of start of central directory with respect to the starting disk number
  if (unzlocal_getLong(fin,&us.offset_central_dir)!=UNZ_OK) err=UNZ_ERRNO;
  // zipfile comment length
  if (unzlocal_getShort(fin,&us.gi.size_comment)!=UNZ_OK) err=UNZ_ERRNO;
  if ((central_pos<us.offset_central_dir+us.size_central_dir) && (err==UNZ_OK)) err=UNZ_BADZIPFILE;
  if (err!=UNZ_OK)
  {
    lufclose(fin);  //+++1.2
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

//  Close a ZipFile opened with unzipOpen.
//  If there is files inside the .Zip opened with unzipOpenCurrentFile (see later),
//    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
//  return UNZ_OK if there is no problem.
int unzClose (unzFile file)
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

//   Translate date/time from Dos format to tm_unz (readable more easilty)
void unzlocal_DosDateToTmuDate (unsigned long ulDosDate, tm_unz *ptm)
{
  unsigned long uDate;
  uDate = (unsigned long)(ulDosDate>>16);
  ptm->tm_mday = (unsigned short)(uDate&0x1f) ;
  ptm->tm_mon =  (unsigned short)((((uDate)&0x1E0)/0x20)-1) ;
  ptm->tm_year = (unsigned short)(((uDate&0x0FE00)/0x0200)+1980) ;

  ptm->tm_hour = (unsigned short) ((ulDosDate &0xF800)/0x800);
  ptm->tm_min =  (unsigned short) ((ulDosDate&0x7E0)/0x20) ;
  ptm->tm_sec =  (unsigned short) (2*(ulDosDate&0x1f)) ;
}

//  Get Info about the current file in the zipfile, with internal only info
int unzlocal_GetCurrentFileInfoInternal (unzFile file,
    unz_file_info *pfile_info,
    unz_file_info_internal
    *pfile_info_internal,
    char *szFileName,
    unsigned long fileNameBufferSize,
    void *extraField,
    unsigned long extraFieldBufferSize,
    char *szComment,
    unsigned long commentBufferSize);

int unzlocal_GetCurrentFileInfoInternal (unzFile file, unz_file_info *pfile_info,
    unz_file_info_internal *pfile_info_internal, char *szFileName,
    unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize,
    char *szComment, unsigned long commentBufferSize)
{
  unz_s *s;
  unz_file_info file_info;
  unz_file_info_internal file_info_internal;
  int err=UNZ_OK;
  unsigned long uMagic;
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
    unsigned long uSizeRead ;
    if (file_info.size_filename<fileNameBufferSize)
    {
      *(szFileName+file_info.size_filename)='\0';
      uSizeRead = file_info.size_filename;
    }
    else
      uSizeRead = fileNameBufferSize;

    if ((file_info.size_filename>0) && (fileNameBufferSize>0))
      if (lufread(szFileName,(unsigned short)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    lSeek -= uSizeRead;
  }

  if ((err==UNZ_OK) && (extraField!=NULL))
  {
    unsigned long uSizeRead ;
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
      if (lufread(extraField,(unsigned short)uSizeRead,1,s->file)!=1)
        err=UNZ_ERRNO;
    lSeek += file_info.size_file_extra - uSizeRead;
  }
  else
    lSeek+=file_info.size_file_extra;

  if ((err==UNZ_OK) && (szComment!=NULL))
  {
    unsigned long uSizeRead ;
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
      if (lufread(szComment,(unsigned short)uSizeRead,1,s->file)!=1)
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

//  Write info about the ZipFile in the *pglobal_info structure.
//  No preparation of the structure is needed
//  return UNZ_OK if there is no problem.
int unzGetCurrentFileInfo (unzFile file, unz_file_info *pfile_info,
                           char *szFileName, unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize,
                           char *szComment, unsigned long commentBufferSize)
{
  return unzlocal_GetCurrentFileInfoInternal(file,pfile_info,NULL,szFileName,fileNameBufferSize,
         extraField,extraFieldBufferSize, szComment,commentBufferSize);
}

//  Set the current file of the zipfile to the first file.
//  return UNZ_OK if there is no problem
int unzGoToFirstFile (unzFile file)
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

//  Set the current file of the zipfile to the next file.
//  return UNZ_OK if there is no problem
//  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
int unzGoToNextFile (unzFile file)
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

//  Try locate the file szFileName in the zipfile.
//  For the iCaseSensitivity signification, see unzStringFileNameCompare
//  return value :
//  UNZ_OK if the file is found. It becomes the current file.
//  UNZ_END_OF_LIST_OF_FILE if the file is not found
int unzLocateFile (unzFile file, const char *szFileName, int iCaseSensitivity)
{
  unz_s *s;
  int err;

  unsigned long num_fileSaved;
  unsigned long pos_in_central_dirSaved;

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
    if (unzStringFileNameCompare(szCurrentFileName,szFileName,iCaseSensitivity)==0)
      return UNZ_OK;
    err = unzGoToNextFile(file);
  }

  s->num_file = num_fileSaved ;
  s->pos_in_central_dir = pos_in_central_dirSaved ;
  return err;
}

//  Read the local header of the current zipfile
//  Check the coherency of the local header and info in the end of central
//        directory about this file
//  store in *piSizeVar the size of extra info in local header
//        (filename and size of extra field data)
int unzlocal_CheckCurrentFileCoherencyHeader (unz_s *s,unsigned short *piSizeVar,
    unsigned long *poffset_local_extrafield, unsigned short  *psize_local_extrafield)
{
  unsigned long uMagic,uData,uFlags;
  unsigned long size_filename;
  unsigned long size_extra_field;
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
//	else if ((err==UNZ_OK) && (uData!=s->cur_file_info.wVersion))
//		err=UNZ_BADZIPFILE;
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

  *piSizeVar += (unsigned short)size_filename;

  if (unzlocal_getShort(s->file,&size_extra_field) != UNZ_OK)
    err=UNZ_ERRNO;
  *poffset_local_extrafield= s->cur_file_info_internal.offset_curfile +
                             SIZEZIPLOCALHEADER + size_filename;
  *psize_local_extrafield = (unsigned short)size_extra_field;

  *piSizeVar += (unsigned short)size_extra_field;

  return err;
}

//  Open for reading data the current file in the zipfile.
//  If there is no error and the file is opened, the return value is UNZ_OK.
int unzOpenCurrentFile (unzFile file)
{
  int err;
  int Store;
  unsigned short iSizeVar;
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  unsigned long offset_local_extrafield;  // offset of the local extra field
  unsigned short  size_local_extrafield;    // size of the local extra field

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
    pfile_in_zip_read_info->stream.opaque = (void *)0;

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

  pfile_in_zip_read_info->stream.avail_in = (unsigned short)0;

  s->pfile_in_zip_read = pfile_in_zip_read_info;
  return UNZ_OK;
}

//  Read bytes from the current file.
//  buf contain buffer where data must be copied
//  len the size of buf.
//  return the number of byte copied if somes bytes are copied
//  return 0 if the end of file was reached
//  return <0 with error code if there is an error
//    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
int unzReadCurrentFile  (unzFile file, void *buf, unsigned len)
{
  int err=UNZ_OK;
  unsigned short iRead = 0;

  unz_s *s = (unz_s *)file;
  if (s==NULL) return UNZ_PARAMERROR;

  file_in_zip_read_info_s *pfile_in_zip_read_info = s->pfile_in_zip_read;
  if (pfile_in_zip_read_info==NULL) return UNZ_PARAMERROR;
  if ((pfile_in_zip_read_info->read_buffer == NULL)) return UNZ_END_OF_LIST_OF_FILE;
  if (len==0) return 0;

  pfile_in_zip_read_info->stream.next_out = (unsigned char *)buf;
  pfile_in_zip_read_info->stream.avail_out = (unsigned short)len;

  if (len>pfile_in_zip_read_info->rest_read_uncompressed)
  {
    pfile_in_zip_read_info->stream.avail_out = (unsigned short)pfile_in_zip_read_info->rest_read_uncompressed;
  }

  while (pfile_in_zip_read_info->stream.avail_out>0)
  {
    if ((pfile_in_zip_read_info->stream.avail_in==0) && (pfile_in_zip_read_info->rest_read_compressed>0))
    {
      unsigned short uReadThis = UNZ_BUFSIZE;
      if (pfile_in_zip_read_info->rest_read_compressed<uReadThis) uReadThis = (unsigned short)pfile_in_zip_read_info->rest_read_compressed;
      if (uReadThis == 0) return UNZ_EOF;
      if (lufseek(pfile_in_zip_read_info->file, pfile_in_zip_read_info->pos_in_zipfile + pfile_in_zip_read_info->byte_before_the_zipfile,SEEK_SET)!=0) return UNZ_ERRNO;
      if (lufread(pfile_in_zip_read_info->read_buffer,uReadThis,1,pfile_in_zip_read_info->file)!=1) return UNZ_ERRNO;
      pfile_in_zip_read_info->pos_in_zipfile += uReadThis;
      pfile_in_zip_read_info->rest_read_compressed-=uReadThis;
      pfile_in_zip_read_info->stream.next_in = (unsigned char *)pfile_in_zip_read_info->read_buffer;
      pfile_in_zip_read_info->stream.avail_in = (unsigned short)uReadThis;
    }

    if (pfile_in_zip_read_info->compression_method==0)
    {
      unsigned short uDoCopy,i ;
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
      unsigned long uTotalOutBefore,uTotalOutAfter;
      const unsigned char *bufBefore;
      unsigned long uOutThis;
      int flush=Z_SYNC_FLUSH;
      uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
      bufBefore = pfile_in_zip_read_info->stream.next_out;
      err=inflate(&pfile_in_zip_read_info->stream,flush);
      uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
      uOutThis = uTotalOutAfter-uTotalOutBefore;
      pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,bufBefore,(unsigned short)(uOutThis));
      pfile_in_zip_read_info->rest_read_uncompressed -= uOutThis;
      iRead += (unsigned short)(uTotalOutAfter - uTotalOutBefore);
      if (err==Z_STREAM_END) return (iRead==0) ? UNZ_EOF : iRead;			//+++1.3
      //if (err==Z_STREAM_END) return (iRead==len) ? UNZ_EOF : iRead;		//+++1.2

      if (err != Z_OK) break;
    }
  }

  if (err==Z_OK) return iRead;

  return iRead;
}

//  Close the file in zip opened with unzipOpenCurrentFile
//  Return UNZ_CRCERROR if all the file was read but the CRC is not good
int unzCloseCurrentFile (unzFile file)
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

int unzOpenCurrentFile (unzFile file);
int unzReadCurrentFile (unzFile file, void *buf, unsigned len);
int unzCloseCurrentFile (unzFile file);

struct TUnzip
{
  TUnzip() : uf(0), currentfile(-1), cze(), czei(-1) {}

  unzFile uf;
  int currentfile;
  ZIPENTRY cze;
  int czei;

  unsigned Open(void *z,unsigned int len);
  unsigned Get(int index,ZIPENTRY *ze);
  unsigned Find(const char *name,int *index,ZIPENTRY *ze);
  unsigned Unzip(int index,void *dst,unsigned int len);
  unsigned Close();
};

unsigned TUnzip::Open(void *z,unsigned int len)
{
  if (uf!=0 || currentfile!=-1)
    return 1;
  unsigned e;
  LUFILE *f = lufopen(z,len,&e);
  if (f==NULL)
    return e;
  uf = unzOpenInternal(f);
  //return 0;
  return zopenerror;	//+++1.2
}

unsigned TUnzip::Get(int index,ZIPENTRY *ze)
{
  if (index<-1 || index>=(int)uf->gi.number_entry)
    return 1;
  if (currentfile!=-1)
    unzCloseCurrentFile(uf);
  currentfile=-1;
  if (index==czei && index!=-1)
  {
    memcpy(ze,&cze,sizeof(ZIPENTRY));
    return 0;
  }
  if (index==-1)
  {
    ze->index = uf->gi.number_entry;
    ze->comp_size=0;
    ze->unc_size=0;
    return 0;
  }
  if (index<(int)uf->num_file) unzGoToFirstFile(uf);
  while ((int)uf->num_file<index) unzGoToNextFile(uf);
  unz_file_info ufi;
  char fn[1024];
  unzGetCurrentFileInfo(uf,&ufi,fn,1024,NULL,0,NULL,0);

  // now get the extra header. We do this ourselves, instead of
  // calling unzOpenCurrentFile &c., to avoid allocating more than necessary.
  unsigned short extralen,iSizeVar;
  unsigned long offset;
  int res = unzlocal_CheckCurrentFileCoherencyHeader(uf,&iSizeVar,&offset,&extralen);
  if (res!=UNZ_OK) return 1;
  if (lufseek(uf->file,offset,SEEK_SET)!=0) return 1;
  char *extra = new char[extralen];
  if (lufread(extra,1,(unsigned short)extralen,uf->file)!=extralen)
  {
    delete[] extra;
    return 1;
  }
  //
  ze->index=uf->num_file;
  // zip has an 'attribute' 32bit value. Its lower half is windows stuff
  // its upper half is standard unix attr.
  ze->comp_size = ufi.compressed_size;
  ze->unc_size = ufi.uncompressed_size;
  //
  if (extra!=0) delete[] extra;
  memcpy(&cze,ze,sizeof(ZIPENTRY));
  czei=index;
  return 0;
}

unsigned TUnzip::Find(const char *name, int *index, ZIPENTRY *ze)
{
  int res = unzLocateFile(uf,name,CASE_SENSITIVE);
  if (res!=UNZ_OK)
  {
    if (index!=0)
      *index=-1;
    if (ze!=NULL)
      ze->index=-1;
    return 1;
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
    if (zres!=0)
      return zres;
  }
  return 0;
}

unsigned TUnzip::Unzip(int index,void *dst,unsigned int len)
{
  if (index!=currentfile)
  {
    if (currentfile!=-1)
      unzCloseCurrentFile(uf);
    currentfile=-1;
    if (index>=(int)uf->gi.number_entry)
      return 1;
    if (index<(int)uf->num_file)
      unzGoToFirstFile(uf);
    while ((int)uf->num_file<index)
      unzGoToNextFile(uf);
    unzOpenCurrentFile(uf);
    currentfile=index;
  }
  int res = unzReadCurrentFile(uf,dst,len);
  if (res>0)
    return 1;
  unzCloseCurrentFile(uf);
  currentfile=-1;
  if (res==0)
    return 0;
  else
    return 1;

}

unsigned TUnzip::Close()
{
  if (currentfile!=-1) unzCloseCurrentFile(uf);
  currentfile=-1;
  if (uf!=0) unzClose(uf);
  uf=0;
  return 0;
}

typedef struct
{
  unsigned flag;
  TUnzip *unz;
} TUnzipHandleData;

void *libcdr::OpenZip(void *z,unsigned int len)
{
  TUnzip *unz = new TUnzip();
  if (!unz->Open(z,len))
  {
    delete unz;
    return 0;
  }
  TUnzipHandleData *han = new TUnzipHandleData;
  han->flag=1;
  han->unz=unz;
  return (void *)han;
}

unsigned libcdr::FindZipItem(void *hz, const char *name, int *index, ZIPENTRY *ze)
{
  if (!hz)
    return 1;
  TUnzipHandleData *han = (TUnzipHandleData *)hz;
  if (han->flag!=1)
    return 1;
  TUnzip *unz = han->unz;
  return unz->Find(name,index,ze);
}

unsigned libcdr::UnzipItem(void *hz, int index, void *dst, unsigned int len)
{
  if (!hz)
    return 1;
  TUnzipHandleData *han = (TUnzipHandleData *)hz;
  if (han->flag!=1)
    return 1;
  TUnzip *unz = han->unz;
  return unz->Unzip(index,dst,len);
}

unsigned libcdr::CloseZip(void *hz)
{
  if (!hz)
    return 1;
  TUnzipHandleData *han = (TUnzipHandleData *)hz;
  if (han->flag!=1)
    return 1;
  TUnzip *unz = han->unz;
  unsigned ret = unz->Close();
  delete unz;
  delete han;
  return ret;
}
