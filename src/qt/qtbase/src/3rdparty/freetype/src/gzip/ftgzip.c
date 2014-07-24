/***************************************************************************/
/*                                                                         */
/*  ftgzip.c                                                               */
/*                                                                         */
/*    FreeType support for .gz compressed files.                           */
/*                                                                         */
/*  This optional component relies on zlib.  It should mainly be used to   */
/*  parse compressed PCF fonts, as found with many X11 server              */
/*  distributions.                                                         */
/*                                                                         */
/*  Copyright 2002, 2003, 2004, 2005, 2006, 2009 by                        */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H
#include FT_GZIP_H
#include FT_CONFIG_STANDARD_LIBRARY_H


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#define FT_ERR_PREFIX  Gzip_Err_
#define FT_ERR_BASE    FT_Mod_Err_Gzip

#include FT_ERRORS_H


#ifdef FT_CONFIG_OPTION_USE_ZLIB

#ifdef FT_CONFIG_OPTION_PIC
#error "gzip code does not support PIC yet"
#endif 

#ifdef FT_CONFIG_OPTION_SYSTEM_ZLIB

#include <zlib.h>

#else /* !FT_CONFIG_OPTION_SYSTEM_ZLIB */

 /* In this case, we include our own modified sources of the ZLib    */
 /* within the "ftgzip" component.  The modifications were necessary */
 /* to #include all files without conflicts, as well as preventing   */
 /* the definition of "extern" functions that may cause linking      */
 /* conflicts when a program is linked with both FreeType and the    */
 /* original ZLib.                                                   */

#define NO_DUMMY_DECL
#ifndef USE_ZLIB_ZCALLOC
#define MY_ZCALLOC /* prevent all zcalloc() & zfree() in zutils.c */
#endif

#include "zlib.h"

#undef  SLOW
#define SLOW  1  /* we can't use asm-optimized sources here! */

  /* Urgh.  `inflate_mask' must not be declared twice -- C++ doesn't like
     this.  We temporarily disable it and load all necessary header files. */
#define NO_INFLATE_MASK
#include "zutil.h"
#include "inftrees.h"
#include "infblock.h"
#include "infcodes.h"
#include "infutil.h"
#undef  NO_INFLATE_MASK

  /* infutil.c must be included before infcodes.c */
#include "zutil.c"
#include "inftrees.c"
#include "infutil.c"
#include "infcodes.c"
#include "infblock.c"
#include "inflate.c"
#include "adler32.c"

#endif /* !FT_CONFIG_OPTION_SYSTEM_ZLIB */


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****            Z L I B   M E M O R Y   M A N A G E M E N T          *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

  /* it is better to use FreeType memory routines instead of raw
     'malloc/free' */

  static voidpf
  ft_gzip_alloc( FT_Memory  memory,
                 uInt       items,
                 uInt       size )
  {
    FT_ULong    sz = (FT_ULong)size * items;
    FT_Error    error;
    FT_Pointer  p;


    (void)FT_ALLOC( p, sz );
    return p;
  }


  static void
  ft_gzip_free( FT_Memory  memory,
                voidpf     address )
  {
    FT_MEM_FREE( address );
  }


#if !defined( FT_CONFIG_OPTION_SYSTEM_ZLIB ) && !defined( USE_ZLIB_ZCALLOC )

  local voidpf
  zcalloc ( voidpf    opaque,
            unsigned  items,
            unsigned  size )
  {
    return ft_gzip_alloc( (FT_Memory)opaque, items, size );
  }

  local void
  zcfree( voidpf  opaque,
          voidpf  ptr )
  {
    ft_gzip_free( (FT_Memory)opaque, ptr );
  }

#endif /* !SYSTEM_ZLIB && !USE_ZLIB_ZCALLOC */


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****               Z L I B   F I L E   D E S C R I P T O R           *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

#define FT_GZIP_BUFFER_SIZE  4096

  typedef struct  FT_GZipFileRec_
  {
    FT_Stream  source;         /* parent/source stream        */
    FT_Stream  stream;         /* embedding stream            */
    FT_Memory  memory;         /* memory allocator            */
    z_stream   zstream;        /* zlib input stream           */

    FT_ULong   start;          /* starting position, after .gz header */
    FT_Byte    input[FT_GZIP_BUFFER_SIZE];   /* input read buffer  */

    FT_Byte    buffer[FT_GZIP_BUFFER_SIZE];  /* output buffer      */
    FT_ULong   pos;                          /* position in output */
    FT_Byte*   cursor;
    FT_Byte*   limit;

  } FT_GZipFileRec, *FT_GZipFile;


  /* gzip flag byte */
#define FT_GZIP_ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define FT_GZIP_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define FT_GZIP_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define FT_GZIP_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define FT_GZIP_COMMENT      0x10 /* bit 4 set: file comment present */
#define FT_GZIP_RESERVED     0xE0 /* bits 5..7: reserved */


  /* check and skip .gz header - we don't support `transparent' compression */
  static FT_Error
  ft_gzip_check_header( FT_Stream  stream )
  {
    FT_Error  error;
    FT_Byte   head[4];


    if ( FT_STREAM_SEEK( 0 )       ||
         FT_STREAM_READ( head, 4 ) )
      goto Exit;

    /* head[0] && head[1] are the magic numbers;    */
    /* head[2] is the method, and head[3] the flags */
    if ( head[0] != 0x1f              ||
         head[1] != 0x8b              ||
         head[2] != Z_DEFLATED        ||
        (head[3] & FT_GZIP_RESERVED)  )
    {
      error = Gzip_Err_Invalid_File_Format;
      goto Exit;
    }

    /* skip time, xflags and os code */
    (void)FT_STREAM_SKIP( 6 );

    /* skip the extra field */
    if ( head[3] & FT_GZIP_EXTRA_FIELD )
    {
      FT_UInt  len;


      if ( FT_READ_USHORT_LE( len ) ||
           FT_STREAM_SKIP( len )    )
        goto Exit;
    }

    /* skip original file name */
    if ( head[3] & FT_GZIP_ORIG_NAME )
      for (;;)
      {
        FT_UInt  c;


        if ( FT_READ_BYTE( c ) )
          goto Exit;

        if ( c == 0 )
          break;
      }

    /* skip .gz comment */
    if ( head[3] & FT_GZIP_COMMENT )
      for (;;)
      {
        FT_UInt  c;


        if ( FT_READ_BYTE( c ) )
          goto Exit;

        if ( c == 0 )
          break;
      }

    /* skip CRC */
    if ( head[3] & FT_GZIP_HEAD_CRC )
      if ( FT_STREAM_SKIP( 2 ) )
        goto Exit;

  Exit:
    return error;
  }


  static FT_Error
  ft_gzip_file_init( FT_GZipFile  zip,
                     FT_Stream    stream,
                     FT_Stream    source )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Error   error   = Gzip_Err_Ok;


    zip->stream = stream;
    zip->source = source;
    zip->memory = stream->memory;

    zip->limit  = zip->buffer + FT_GZIP_BUFFER_SIZE;
    zip->cursor = zip->limit;
    zip->pos    = 0;

    /* check and skip .gz header */
    {
      stream = source;

      error = ft_gzip_check_header( stream );
      if ( error )
        goto Exit;

      zip->start = FT_STREAM_POS();
    }

    /* initialize zlib -- there is no zlib header in the compressed stream */
    zstream->zalloc = (alloc_func)ft_gzip_alloc;
    zstream->zfree  = (free_func) ft_gzip_free;
    zstream->opaque = stream->memory;

    zstream->avail_in = 0;
    zstream->next_in  = zip->buffer;

    if ( inflateInit2( zstream, -MAX_WBITS ) != Z_OK ||
         zstream->next_in == NULL                     )
      error = Gzip_Err_Invalid_File_Format;

  Exit:
    return error;
  }


  static void
  ft_gzip_file_done( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;


    inflateEnd( zstream );

    /* clear the rest */
    zstream->zalloc    = NULL;
    zstream->zfree     = NULL;
    zstream->opaque    = NULL;
    zstream->next_in   = NULL;
    zstream->next_out  = NULL;
    zstream->avail_in  = 0;
    zstream->avail_out = 0;

    zip->memory = NULL;
    zip->source = NULL;
    zip->stream = NULL;
  }


  static FT_Error
  ft_gzip_file_reset( FT_GZipFile  zip )
  {
    FT_Stream  stream = zip->source;
    FT_Error   error;


    if ( !FT_STREAM_SEEK( zip->start ) )
    {
      z_stream*  zstream = &zip->zstream;


      inflateReset( zstream );

      zstream->avail_in  = 0;
      zstream->next_in   = zip->input;
      zstream->avail_out = 0;
      zstream->next_out  = zip->buffer;

      zip->limit  = zip->buffer + FT_GZIP_BUFFER_SIZE;
      zip->cursor = zip->limit;
      zip->pos    = 0;
    }

    return error;
  }


  static FT_Error
  ft_gzip_file_fill_input( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Stream  stream  = zip->source;
    FT_ULong   size;


    if ( stream->read )
    {
      size = stream->read( stream, stream->pos, zip->input,
                           FT_GZIP_BUFFER_SIZE );
      if ( size == 0 )
        return Gzip_Err_Invalid_Stream_Operation;
    }
    else
    {
      size = stream->size - stream->pos;
      if ( size > FT_GZIP_BUFFER_SIZE )
        size = FT_GZIP_BUFFER_SIZE;

      if ( size == 0 )
        return Gzip_Err_Invalid_Stream_Operation;

      FT_MEM_COPY( zip->input, stream->base + stream->pos, size );
    }
    stream->pos += size;

    zstream->next_in  = zip->input;
    zstream->avail_in = size;

    return Gzip_Err_Ok;
  }


  static FT_Error
  ft_gzip_file_fill_output( FT_GZipFile  zip )
  {
    z_stream*  zstream = &zip->zstream;
    FT_Error   error   = 0;


    zip->cursor        = zip->buffer;
    zstream->next_out  = zip->cursor;
    zstream->avail_out = FT_GZIP_BUFFER_SIZE;

    while ( zstream->avail_out > 0 )
    {
      int  err;


      if ( zstream->avail_in == 0 )
      {
        error = ft_gzip_file_fill_input( zip );
        if ( error )
          break;
      }

      err = inflate( zstream, Z_NO_FLUSH );

      if ( err == Z_STREAM_END )
      {
        zip->limit = zstream->next_out;
        if ( zip->limit == zip->cursor )
          error = Gzip_Err_Invalid_Stream_Operation;
        break;
      }
      else if ( err != Z_OK )
      {
        error = Gzip_Err_Invalid_Stream_Operation;
        break;
      }
    }

    return error;
  }


  /* fill output buffer; `count' must be <= FT_GZIP_BUFFER_SIZE */
  static FT_Error
  ft_gzip_file_skip_output( FT_GZipFile  zip,
                            FT_ULong     count )
  {
    FT_Error  error = Gzip_Err_Ok;
    FT_ULong  delta;


    for (;;)
    {
      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_gzip_file_fill_output( zip );
      if ( error )
        break;
    }

    return error;
  }


  static FT_ULong
  ft_gzip_file_io( FT_GZipFile  zip,
                   FT_ULong     pos,
                   FT_Byte*     buffer,
                   FT_ULong     count )
  {
    FT_ULong  result = 0;
    FT_Error  error;


    /* Reset inflate stream if we're seeking backwards.        */
    /* Yes, that is not too efficient, but it saves memory :-) */
    if ( pos < zip->pos )
    {
      error = ft_gzip_file_reset( zip );
      if ( error )
        goto Exit;
    }

    /* skip unwanted bytes */
    if ( pos > zip->pos )
    {
      error = ft_gzip_file_skip_output( zip, (FT_ULong)( pos - zip->pos ) );
      if ( error )
        goto Exit;
    }

    if ( count == 0 )
      goto Exit;

    /* now read the data */
    for (;;)
    {
      FT_ULong  delta;


      delta = (FT_ULong)( zip->limit - zip->cursor );
      if ( delta >= count )
        delta = count;

      FT_MEM_COPY( buffer, zip->cursor, delta );
      buffer      += delta;
      result      += delta;
      zip->cursor += delta;
      zip->pos    += delta;

      count -= delta;
      if ( count == 0 )
        break;

      error = ft_gzip_file_fill_output( zip );
      if ( error )
        break;
    }

  Exit:
    return result;
  }


/***************************************************************************/
/***************************************************************************/
/*****                                                                 *****/
/*****               G Z   E M B E D D I N G   S T R E A M             *****/
/*****                                                                 *****/
/***************************************************************************/
/***************************************************************************/

  static void
  ft_gzip_stream_close( FT_Stream  stream )
  {
    FT_GZipFile  zip    = (FT_GZipFile)stream->descriptor.pointer;
    FT_Memory    memory = stream->memory;


    if ( zip )
    {
      /* finalize gzip file descriptor */
      ft_gzip_file_done( zip );

      FT_FREE( zip );

      stream->descriptor.pointer = NULL;
    }
  }


  static FT_ULong
  ft_gzip_stream_io( FT_Stream  stream,
                     FT_ULong   pos,
                     FT_Byte*   buffer,
                     FT_ULong   count )
  {
    FT_GZipFile  zip = (FT_GZipFile)stream->descriptor.pointer;


    return ft_gzip_file_io( zip, pos, buffer, count );
  }


  static FT_ULong
  ft_gzip_get_uncompressed_size( FT_Stream  stream )
  {
    FT_Error  error;
    FT_ULong  old_pos;
    FT_ULong  result = 0;


    old_pos = stream->pos;
    if ( !FT_Stream_Seek( stream, stream->size - 4 ) )
    {
      result = (FT_ULong)FT_Stream_ReadLong( stream, &error );
      if ( error )
        result = 0;

      (void)FT_Stream_Seek( stream, old_pos );
    }

    return result;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenGzip( FT_Stream  stream,
                      FT_Stream  source )
  {
    FT_Error     error;
    FT_Memory    memory = source->memory;
    FT_GZipFile  zip;


    /*
     *  check the header right now; this prevents allocating un-necessary
     *  objects when we don't need them
     */
    error = ft_gzip_check_header( source );
    if ( error )
      goto Exit;

    FT_ZERO( stream );
    stream->memory = memory;

    if ( !FT_QNEW( zip ) )
    {
      error = ft_gzip_file_init( zip, stream, source );
      if ( error )
      {
        FT_FREE( zip );
        goto Exit;
      }

      stream->descriptor.pointer = zip;
    }

    /*
     *  We use the following trick to try to dramatically improve the
     *  performance while dealing with small files.  If the original stream
     *  size is less than a certain threshold, we try to load the whole font
     *  file into memory.  This saves us from using the 32KB buffer needed
     *  to inflate the file, plus the two 4KB intermediate input/output
     *  buffers used in the `FT_GZipFile' structure.
     */
    {
      FT_ULong  zip_size = ft_gzip_get_uncompressed_size( source );


      if ( zip_size != 0 && zip_size < 40 * 1024 )
      {
        FT_Byte*  zip_buff;


        if ( !FT_ALLOC( zip_buff, zip_size ) )
        {
          FT_ULong  count;


          count = ft_gzip_file_io( zip, 0, zip_buff, zip_size );
          if ( count == zip_size )
          {
            ft_gzip_file_done( zip );
            FT_FREE( zip );

            stream->descriptor.pointer = NULL;

            stream->size  = zip_size;
            stream->pos   = 0;
            stream->base  = zip_buff;
            stream->read  = NULL;
            stream->close = ft_gzip_stream_close;

            goto Exit;
          }

          ft_gzip_file_io( zip, 0, NULL, 0 );
          FT_FREE( zip_buff );
        }
        error = 0;
      }
    }

    stream->size  = 0x7FFFFFFFL;  /* don't know the real size! */
    stream->pos   = 0;
    stream->base  = 0;
    stream->read  = ft_gzip_stream_io;
    stream->close = ft_gzip_stream_close;

  Exit:
    return error;
  }

#else  /* !FT_CONFIG_OPTION_USE_ZLIB */

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_OpenGzip( FT_Stream  stream,
                      FT_Stream  source )
  {
    FT_UNUSED( stream );
    FT_UNUSED( source );

    return Gzip_Err_Unimplemented_Feature;
  }

#endif /* !FT_CONFIG_OPTION_USE_ZLIB */


/* END */
