
/* png.c - location for general purpose libpng functions
 *
 * Last changed in libpng 1.5.4 [July 7, 2011]
 * Copyright (c) 1998-2011 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "pngpriv.h"

namespace PrivatePng {

/* Generate a compiler error if there is an old png.h in the search path. */
typedef png_libpng_version_1_5_4 Your_png_h_is_not_version_1_5_4;

/* Tells libpng that we have already handled the first "num_bytes" bytes
 * of the PNG file signature.  If the PNG data is embedded into another
 * stream we can set num_bytes = 8 so that libpng will not attempt to read
 * or write any of the magic bytes before it starts on the IHDR.
 */

#ifdef PNG_READ_SUPPORTED
void PNGAPI
png_set_sig_bytes(png_structp png_ptr, int num_bytes)
{
   png_debug(1, "in png_set_sig_bytes");

   if (png_ptr == NULL)
      return;

   if (num_bytes > 8)
      png_error(png_ptr, "Too many bytes for PNG signature");

   png_ptr->sig_bytes = (png_byte)(num_bytes < 0 ? 0 : num_bytes);
}

/* Checks whether the supplied bytes match the PNG signature.  We allow
 * checking less than the full 8-byte signature so that those apps that
 * already read the first few bytes of a file to determine the file type
 * can simply check the remaining bytes for extra assurance.  Returns
 * an integer less than, equal to, or greater than zero if sig is found,
 * respectively, to be less than, to match, or be greater than the correct
 * PNG signature (this is the same behaviour as strcmp, memcmp, etc).
 */
int PNGAPI
png_sig_cmp(png_const_bytep sig, png_size_t start, png_size_t num_to_check)
{
   png_byte png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

   if (num_to_check > 8)
      num_to_check = 8;

   else if (num_to_check < 1)
      return (-1);

   if (start > 7)
      return (-1);

   if (start + num_to_check > 8)
      num_to_check = 8 - start;

   return ((int)(png_memcmp(&sig[start], &png_signature[start], num_to_check)));
}

#endif /* PNG_READ_SUPPORTED */

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
/* Function to allocate memory for zlib */
PNG_FUNCTION(voidpf /* PRIVATE */,
png_zalloc,(voidpf png_ptr, uInt items, uInt size),PNG_ALLOCATED)
{
   png_voidp ptr;
   png_structp p=(png_structp)png_ptr;
   png_uint_32 save_flags=p->flags;
   png_alloc_size_t num_bytes;

   if (png_ptr == NULL)
      return (NULL);

   if (items > PNG_UINT_32_MAX/size)
   {
     png_warning (p, "Potential overflow in png_zalloc()");
     return (NULL);
   }
   num_bytes = (png_alloc_size_t)items * size;

   p->flags|=PNG_FLAG_MALLOC_NULL_MEM_OK;
   ptr = (png_voidp)png_malloc((png_structp)png_ptr, num_bytes);
   p->flags=save_flags;

   return ((voidpf)ptr);
}

/* Function to free memory for zlib */
void /* PRIVATE */
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_free((png_structp)png_ptr, (png_voidp)ptr);
}

/* Reset the CRC variable to 32 bits of 1's.  Care must be taken
 * in case CRC is > 32 bits to leave the top bits 0.
 */
void /* PRIVATE */
png_reset_crc(png_structp png_ptr)
{
   png_ptr->crc = crc32(0, Z_NULL, 0);
}

/* Calculate the CRC over a section of data.  We can only pass as
 * much data to this routine as the largest single buffer size.  We
 * also check that this data will actually be used before going to the
 * trouble of calculating it.
 */
void /* PRIVATE */
png_calculate_crc(png_structp png_ptr, png_const_bytep ptr, png_size_t length)
{
   int need_crc = 1;

   if (png_ptr->chunk_name[0] & 0x20)                     /* ancillary */
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }

   else                                                    /* critical */
   {
      if (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE)
         need_crc = 0;
   }

   if (need_crc)
      png_ptr->crc = crc32(png_ptr->crc, ptr, (uInt)length);
}

/* Check a user supplied version number, called from both read and write
 * functions that create a png_struct
 */
int
png_user_version_check(png_structp png_ptr, png_const_charp user_png_ver)
{
   if (user_png_ver)
   {
      int i = 0;

      do
      {
         if (user_png_ver[i] != png_libpng_ver[i])
            png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
      } while (png_libpng_ver[i++]);
   }

   else
      png_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;

   if (png_ptr->flags & PNG_FLAG_LIBRARY_MISMATCH)
   {
     /* Libpng 0.90 and later are binary incompatible with libpng 0.89, so
      * we must recompile any applications that use any older library version.
      * For versions after libpng 1.0, we will be compatible, so we need
      * only check the first digit.
      */
      if (user_png_ver == NULL || user_png_ver[0] != png_libpng_ver[0] ||
          (user_png_ver[0] == '1' && user_png_ver[2] != png_libpng_ver[2]) ||
          (user_png_ver[0] == '0' && user_png_ver[2] < '9'))
      {
#ifdef PNG_WARNINGS_SUPPORTED
         size_t pos = 0;
         char m[128];

         pos = png_safecat(m, sizeof m, pos, "Application built with libpng-");
         pos = png_safecat(m, sizeof m, pos, user_png_ver);
         pos = png_safecat(m, sizeof m, pos, " but running with ");
         pos = png_safecat(m, sizeof m, pos, png_libpng_ver);

         png_warning(png_ptr, m);
#endif

#ifdef PNG_ERROR_NUMBERS_SUPPORTED
         png_ptr->flags = 0;
#endif

         return 0;
      }
   }

   /* Success return. */
   return 1;
}

/* Allocate the memory for an info_struct for the application.  We don't
 * really need the png_ptr, but it could potentially be useful in the
 * future.  This should be used in favour of malloc(png_sizeof(png_info))
 * and png_info_init() so that applications that want to use a shared
 * libpng don't have to be recompiled if png_info changes size.
 */
PNG_FUNCTION(png_infop,PNGAPI
png_create_info_struct,(png_structp png_ptr),PNG_ALLOCATED)
{
   png_infop info_ptr;

   png_debug(1, "in png_create_info_struct");

   if (png_ptr == NULL)
      return (NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   info_ptr = (png_infop)png_create_struct_2(PNG_STRUCT_INFO,
      png_ptr->malloc_fn, png_ptr->mem_ptr);
#else
   info_ptr = (png_infop)png_create_struct(PNG_STRUCT_INFO);
#endif
   if (info_ptr != NULL)
      png_info_init_3(&info_ptr, png_sizeof(png_info));

   return (info_ptr);
}

/* This function frees the memory associated with a single info struct.
 * Normally, one would use either png_destroy_read_struct() or
 * png_destroy_write_struct() to free an info struct, but this may be
 * useful for some applications.
 */
void PNGAPI
png_destroy_info_struct(png_structp png_ptr, png_infopp info_ptr_ptr)
{
   png_infop info_ptr = NULL;

   png_debug(1, "in png_destroy_info_struct");

   if (png_ptr == NULL)
      return;

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      png_info_destroy(png_ptr, info_ptr);

#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)info_ptr, png_ptr->free_fn,
          png_ptr->mem_ptr);
#else
      png_destroy_struct((png_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }
}

/* Initialize the info structure.  This is now an internal function (0.89)
 * and applications using it are urged to use png_create_info_struct()
 * instead.
 */

void PNGAPI
png_info_init_3(png_infopp ptr_ptr, png_size_t png_info_struct_size)
{
   png_infop info_ptr = *ptr_ptr;

   png_debug(1, "in png_info_init_3");

   if (info_ptr == NULL)
      return;

   if (png_sizeof(png_info) > png_info_struct_size)
   {
      png_destroy_struct(info_ptr);
      info_ptr = (png_infop)png_create_struct(PNG_STRUCT_INFO);
      *ptr_ptr = info_ptr;
   }

   /* Set everything to 0 */
   png_memset(info_ptr, 0, png_sizeof(png_info));
}

void PNGAPI
png_data_freer(png_structp png_ptr, png_infop info_ptr,
   int freer, png_uint_32 mask)
{
   png_debug(1, "in png_data_freer");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (freer == PNG_DESTROY_WILL_FREE_DATA)
      info_ptr->free_me |= mask;

   else if (freer == PNG_USER_WILL_FREE_DATA)
      info_ptr->free_me &= ~mask;

   else
      png_warning(png_ptr,
         "Unknown freer parameter in png_data_freer");
}

void PNGAPI
png_free_data(png_structp png_ptr, png_infop info_ptr, png_uint_32 mask,
   int num)
{
   png_debug(1, "in png_free_data");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

#ifdef PNG_TEXT_SUPPORTED
   /* Free text item num or (if num == -1) all text items */
   if ((mask & PNG_FREE_TEXT) & info_ptr->free_me)
   {
      if (num != -1)
      {
         if (info_ptr->text && info_ptr->text[num].key)
         {
            png_free(png_ptr, info_ptr->text[num].key);
            info_ptr->text[num].key = NULL;
         }
      }

      else
      {
         int i;
         for (i = 0; i < info_ptr->num_text; i++)
             png_free_data(png_ptr, info_ptr, PNG_FREE_TEXT, i);
         png_free(png_ptr, info_ptr->text);
         info_ptr->text = NULL;
         info_ptr->num_text=0;
      }
   }
#endif

#ifdef PNG_tRNS_SUPPORTED
   /* Free any tRNS entry */
   if ((mask & PNG_FREE_TRNS) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->trans_alpha);
      info_ptr->trans_alpha = NULL;
      info_ptr->valid &= ~PNG_INFO_tRNS;
   }
#endif

#ifdef PNG_sCAL_SUPPORTED
   /* Free any sCAL entry */
   if ((mask & PNG_FREE_SCAL) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->scal_s_width);
      png_free(png_ptr, info_ptr->scal_s_height);
      info_ptr->scal_s_width = NULL;
      info_ptr->scal_s_height = NULL;
      info_ptr->valid &= ~PNG_INFO_sCAL;
   }
#endif

#ifdef PNG_pCAL_SUPPORTED
   /* Free any pCAL entry */
   if ((mask & PNG_FREE_PCAL) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->pcal_purpose);
      png_free(png_ptr, info_ptr->pcal_units);
      info_ptr->pcal_purpose = NULL;
      info_ptr->pcal_units = NULL;
      if (info_ptr->pcal_params != NULL)
         {
            int i;
            for (i = 0; i < (int)info_ptr->pcal_nparams; i++)
            {
               png_free(png_ptr, info_ptr->pcal_params[i]);
               info_ptr->pcal_params[i] = NULL;
            }
            png_free(png_ptr, info_ptr->pcal_params);
            info_ptr->pcal_params = NULL;
         }
      info_ptr->valid &= ~PNG_INFO_pCAL;
   }
#endif

#ifdef PNG_iCCP_SUPPORTED
   /* Free any iCCP entry */
   if ((mask & PNG_FREE_ICCP) & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->iccp_name);
      png_free(png_ptr, info_ptr->iccp_profile);
      info_ptr->iccp_name = NULL;
      info_ptr->iccp_profile = NULL;
      info_ptr->valid &= ~PNG_INFO_iCCP;
   }
#endif

#ifdef PNG_sPLT_SUPPORTED
   /* Free a given sPLT entry, or (if num == -1) all sPLT entries */
   if ((mask & PNG_FREE_SPLT) & info_ptr->free_me)
   {
      if (num != -1)
      {
         if (info_ptr->splt_palettes)
         {
            png_free(png_ptr, info_ptr->splt_palettes[num].name);
            png_free(png_ptr, info_ptr->splt_palettes[num].entries);
            info_ptr->splt_palettes[num].name = NULL;
            info_ptr->splt_palettes[num].entries = NULL;
         }
      }

      else
      {
         if (info_ptr->splt_palettes_num)
         {
            int i;
            for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
               png_free_data(png_ptr, info_ptr, PNG_FREE_SPLT, i);

            png_free(png_ptr, info_ptr->splt_palettes);
            info_ptr->splt_palettes = NULL;
            info_ptr->splt_palettes_num = 0;
         }
         info_ptr->valid &= ~PNG_INFO_sPLT;
      }
   }
#endif

#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
   if (png_ptr->unknown_chunk.data)
   {
      png_free(png_ptr, png_ptr->unknown_chunk.data);
      png_ptr->unknown_chunk.data = NULL;
   }

   if ((mask & PNG_FREE_UNKN) & info_ptr->free_me)
   {
      if (num != -1)
      {
          if (info_ptr->unknown_chunks)
          {
             png_free(png_ptr, info_ptr->unknown_chunks[num].data);
             info_ptr->unknown_chunks[num].data = NULL;
          }
      }

      else
      {
         int i;

         if (info_ptr->unknown_chunks_num)
         {
            for (i = 0; i < info_ptr->unknown_chunks_num; i++)
               png_free_data(png_ptr, info_ptr, PNG_FREE_UNKN, i);

            png_free(png_ptr, info_ptr->unknown_chunks);
            info_ptr->unknown_chunks = NULL;
            info_ptr->unknown_chunks_num = 0;
         }
      }
   }
#endif

#ifdef PNG_hIST_SUPPORTED
   /* Free any hIST entry */
   if ((mask & PNG_FREE_HIST)  & info_ptr->free_me)
   {
      png_free(png_ptr, info_ptr->hist);
      info_ptr->hist = NULL;
      info_ptr->valid &= ~PNG_INFO_hIST;
   }
#endif

   /* Free any PLTE entry that was internally allocated */
   if ((mask & PNG_FREE_PLTE) & info_ptr->free_me)
   {
      png_zfree(png_ptr, info_ptr->palette);
      info_ptr->palette = NULL;
      info_ptr->valid &= ~PNG_INFO_PLTE;
      info_ptr->num_palette = 0;
   }

#ifdef PNG_INFO_IMAGE_SUPPORTED
   /* Free any image bits attached to the info structure */
   if ((mask & PNG_FREE_ROWS) & info_ptr->free_me)
   {
      if (info_ptr->row_pointers)
      {
         int row;
         for (row = 0; row < (int)info_ptr->height; row++)
         {
            png_free(png_ptr, info_ptr->row_pointers[row]);
            info_ptr->row_pointers[row] = NULL;
         }
         png_free(png_ptr, info_ptr->row_pointers);
         info_ptr->row_pointers = NULL;
      }
      info_ptr->valid &= ~PNG_INFO_IDAT;
   }
#endif

   if (num != -1)
      mask &= ~PNG_FREE_MUL;

   info_ptr->free_me &= ~mask;
}

/* This is an internal routine to free any memory that the info struct is
 * pointing to before re-using it or freeing the struct itself.  Recall
 * that png_free() checks for NULL pointers for us.
 */
void /* PRIVATE */
png_info_destroy(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_info_destroy");

   png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
   if (png_ptr->num_chunk_list)
   {
      png_free(png_ptr, png_ptr->chunk_list);
      png_ptr->chunk_list = NULL;
      png_ptr->num_chunk_list = 0;
   }
#endif

   png_info_init_3(&info_ptr, png_sizeof(png_info));
}
#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */

/* This function returns a pointer to the io_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before png_write_destroy() or png_read_destroy() are called.
 */
png_voidp PNGAPI
png_get_io_ptr(png_structp png_ptr)
{
   if (png_ptr == NULL)
      return (NULL);

   return (png_ptr->io_ptr);
}

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#  ifdef PNG_STDIO_SUPPORTED
/* Initialize the default input/output functions for the PNG file.  If you
 * use your own read or write routines, you can call either png_set_read_fn()
 * or png_set_write_fn() instead of png_init_io().  If you have defined
 * PNG_NO_STDIO, you must use a function of your own because "FILE *" isn't
 * necessarily available.
 */
void PNGAPI
png_init_io(png_structp png_ptr, png_FILE_p fp)
{
   png_debug(1, "in png_init_io");

   if (png_ptr == NULL)
      return;

   png_ptr->io_ptr = (png_voidp)fp;
}
#  endif

#  ifdef PNG_TIME_RFC1123_SUPPORTED
/* Convert the supplied time into an RFC 1123 string suitable for use in
 * a "Creation Time" or other text-based time string.
 */
png_const_charp PNGAPI
png_convert_to_rfc1123(png_structp png_ptr, png_const_timep ptime)
{
   static PNG_CONST char short_months[12][4] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (png_ptr == NULL)
      return (NULL);

   {
      size_t pos = 0;
      char number_buf[5]; /* enough for a four digit year */

#     define APPEND_STRING(string)\
         pos = png_safecat(png_ptr->time_buffer, sizeof png_ptr->time_buffer,\
            pos, (string))
#     define APPEND_NUMBER(format, value)\
         APPEND_STRING(PNG_FORMAT_NUMBER(number_buf, format, (value)))
#     define APPEND(ch)\
         if (pos < (sizeof png_ptr->time_buffer)-1)\
            png_ptr->time_buffer[pos++] = (ch)

      APPEND_NUMBER(PNG_NUMBER_FORMAT_u, (unsigned)ptime->day % 32);
      APPEND(' ');
      APPEND_STRING(short_months[(ptime->month - 1) % 12]);
      APPEND(' ');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_u, ptime->year);
      APPEND(' ');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->hour % 24);
      APPEND(':');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->minute % 60);
      APPEND(':');
      APPEND_NUMBER(PNG_NUMBER_FORMAT_02u, (unsigned)ptime->second % 61);
      APPEND_STRING(" +0000"); /* This reliably terminates the buffer */

#     undef APPEND
#     undef APPEND_NUMBER
#     undef APPEND_STRING
   }

   return png_ptr->time_buffer;
}
#  endif /* PNG_TIME_RFC1123_SUPPORTED */

#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */

png_const_charp PNGAPI
png_get_copyright(png_const_structp png_ptr)
{
   PNG_UNUSED(png_ptr)  /* Silence compiler warning about unused png_ptr */
#ifdef PNG_STRING_COPYRIGHT
   return PNG_STRING_COPYRIGHT
#else
#  ifdef __STDC__
   return PNG_STRING_NEWLINE \
     "libpng version 1.5.4 - July 7, 2011" PNG_STRING_NEWLINE \
     "Copyright (c) 1998-2011 Glenn Randers-Pehrson" PNG_STRING_NEWLINE \
     "Copyright (c) 1996-1997 Andreas Dilger" PNG_STRING_NEWLINE \
     "Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc." \
     PNG_STRING_NEWLINE;
#  else
      return "libpng version 1.5.4 - July 7, 2011\
      Copyright (c) 1998-2011 Glenn Randers-Pehrson\
      Copyright (c) 1996-1997 Andreas Dilger\
      Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.";
#  endif
#endif
}

/* The following return the library version as a short string in the
 * format 1.0.0 through 99.99.99zz.  To get the version of *.h files
 * used with your application, print out PNG_LIBPNG_VER_STRING, which
 * is defined in png.h.
 * Note: now there is no difference between png_get_libpng_ver() and
 * png_get_header_ver().  Due to the version_nn_nn_nn typedef guard,
 * it is guaranteed that png.c uses the correct version of png.h.
 */
png_const_charp PNGAPI
png_get_libpng_ver(png_const_structp png_ptr)
{
   /* Version of *.c files used when building libpng */
   return png_get_header_ver(png_ptr);
}

png_const_charp PNGAPI
png_get_header_ver(png_const_structp png_ptr)
{
   /* Version of *.h files used when building libpng */
   PNG_UNUSED(png_ptr)  /* Silence compiler warning about unused png_ptr */
   return PNG_LIBPNG_VER_STRING;
}

png_const_charp PNGAPI
png_get_header_version(png_const_structp png_ptr)
{
   /* Returns longer string containing both version and date */
   PNG_UNUSED(png_ptr)  /* Silence compiler warning about unused png_ptr */
#ifdef __STDC__
   return PNG_HEADER_VERSION_STRING
#  ifndef PNG_READ_SUPPORTED
   "     (NO READ SUPPORT)"
#  endif
   PNG_STRING_NEWLINE;
#else
   return PNG_HEADER_VERSION_STRING;
#endif
}

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#  ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
int PNGAPI
png_handle_as_unknown(png_structp png_ptr, png_const_bytep chunk_name)
{
   /* Check chunk_name and return "keep" value if it's on the list, else 0 */
   int i;
   png_bytep p;
   if (png_ptr == NULL || chunk_name == NULL || png_ptr->num_chunk_list<=0)
      return 0;

   p = png_ptr->chunk_list + png_ptr->num_chunk_list*5 - 5;
   for (i = png_ptr->num_chunk_list; i; i--, p -= 5)
      if (!png_memcmp(chunk_name, p, 4))
        return ((int)*(p + 4));
   return 0;
}
#  endif
#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */

#ifdef PNG_READ_SUPPORTED
/* This function, added to libpng-1.0.6g, is untested. */
int PNGAPI
png_reset_zstream(png_structp png_ptr)
{
   if (png_ptr == NULL)
      return Z_STREAM_ERROR;

   return (inflateReset(&png_ptr->zstream));
}
#endif /* PNG_READ_SUPPORTED */

/* This function was added to libpng-1.0.7 */
png_uint_32 PNGAPI
png_access_version_number(void)
{
   /* Version of *.c files used when building libpng */
   return((png_uint_32)PNG_LIBPNG_VER);
}



#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
#  ifdef PNG_SIZE_T
/* Added at libpng version 1.2.6 */
   PNG_EXTERN png_size_t PNGAPI png_convert_size PNGARG((size_t size));
png_size_t PNGAPI
png_convert_size(size_t size)
{
   if (size > (png_size_t)-1)
      PNG_ABORT();  /* We haven't got access to png_ptr, so no png_error() */

   return ((png_size_t)size);
}
#  endif /* PNG_SIZE_T */

/* Added at libpng version 1.2.34 and 1.4.0 (moved from pngset.c) */
#  ifdef PNG_CHECK_cHRM_SUPPORTED

int /* PRIVATE */
png_check_cHRM_fixed(png_structp png_ptr,
   png_fixed_point white_x, png_fixed_point white_y, png_fixed_point red_x,
   png_fixed_point red_y, png_fixed_point green_x, png_fixed_point green_y,
   png_fixed_point blue_x, png_fixed_point blue_y)
{
   int ret = 1;
   unsigned long xy_hi,xy_lo,yx_hi,yx_lo;

   png_debug(1, "in function png_check_cHRM_fixed");

   if (png_ptr == NULL)
      return 0;

   /* (x,y,z) values are first limited to 0..100000 (PNG_FP_1), the white
    * y must also be greater than 0.  To test for the upper limit calculate
    * (PNG_FP_1-y) - x must be <= to this for z to be >= 0 (and the expression
    * cannot overflow.)  At this point we know x and y are >= 0 and (x+y) is
    * <= PNG_FP_1.  The previous test on PNG_MAX_UINT_31 is removed because it
    * pointless (and it produces compiler warnings!)
    */
   if (white_x < 0 || white_y <= 0 ||
         red_x < 0 ||   red_y <  0 ||
       green_x < 0 || green_y <  0 ||
        blue_x < 0 ||  blue_y <  0)
   {
      png_warning(png_ptr,
        "Ignoring attempt to set negative chromaticity value");
      ret = 0;
   }
   /* And (x+y) must be <= PNG_FP_1 (so z is >= 0) */
   if (white_x > PNG_FP_1 - white_y)
   {
      png_warning(png_ptr, "Invalid cHRM white point");
      ret = 0;
   }

   if (red_x > PNG_FP_1 - red_y)
   {
      png_warning(png_ptr, "Invalid cHRM red point");
      ret = 0;
   }

   if (green_x > PNG_FP_1 - green_y)
   {
      png_warning(png_ptr, "Invalid cHRM green point");
      ret = 0;
   }

   if (blue_x > PNG_FP_1 - blue_y)
   {
      png_warning(png_ptr, "Invalid cHRM blue point");
      ret = 0;
   }

   png_64bit_product(green_x - red_x, blue_y - red_y, &xy_hi, &xy_lo);
   png_64bit_product(green_y - red_y, blue_x - red_x, &yx_hi, &yx_lo);

   if (xy_hi == yx_hi && xy_lo == yx_lo)
   {
      png_warning(png_ptr,
         "Ignoring attempt to set cHRM RGB triangle with zero area");
      ret = 0;
   }

   return ret;
}
#  endif /* PNG_CHECK_cHRM_SUPPORTED */

void /* PRIVATE */
png_check_IHDR(png_structp png_ptr,
   png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_type, int compression_type,
   int filter_type)
{
   int error = 0;

   /* Check for width and height valid values */
   if (width == 0)
   {
      png_warning(png_ptr, "Image width is zero in IHDR");
      error = 1;
   }

   if (height == 0)
   {
      png_warning(png_ptr, "Image height is zero in IHDR");
      error = 1;
   }

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (width > png_ptr->user_width_max)

#  else
   if (width > PNG_USER_WIDTH_MAX)
#  endif
   {
      png_warning(png_ptr, "Image width exceeds user limit in IHDR");
      error = 1;
   }

#  ifdef PNG_SET_USER_LIMITS_SUPPORTED
   if (height > png_ptr->user_height_max)
#  else
   if (height > PNG_USER_HEIGHT_MAX)
#  endif
   {
      png_warning(png_ptr, "Image height exceeds user limit in IHDR");
      error = 1;
   }

   if (width > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image width in IHDR");
      error = 1;
   }

   if (height > PNG_UINT_31_MAX)
   {
      png_warning(png_ptr, "Invalid image height in IHDR");
      error = 1;
   }

   if (width > (PNG_UINT_32_MAX
                 >> 3)      /* 8-byte RGBA pixels */
                 - 48       /* bigrowbuf hack */
                 - 1        /* filter byte */
                 - 7*8      /* rounding of width to multiple of 8 pixels */
                 - 8)       /* extra max_pixel_depth pad */
      png_warning(png_ptr, "Width is too large for libpng to process pixels");

   /* Check other values */
   if (bit_depth != 1 && bit_depth != 2 && bit_depth != 4 &&
       bit_depth != 8 && bit_depth != 16)
   {
      png_warning(png_ptr, "Invalid bit depth in IHDR");
      error = 1;
   }

   if (color_type < 0 || color_type == 1 ||
       color_type == 5 || color_type > 6)
   {
      png_warning(png_ptr, "Invalid color type in IHDR");
      error = 1;
   }

   if (((color_type == PNG_COLOR_TYPE_PALETTE) && bit_depth > 8) ||
       ((color_type == PNG_COLOR_TYPE_RGB ||
         color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
         color_type == PNG_COLOR_TYPE_RGB_ALPHA) && bit_depth < 8))
   {
      png_warning(png_ptr, "Invalid color type/bit depth combination in IHDR");
      error = 1;
   }

   if (interlace_type >= PNG_INTERLACE_LAST)
   {
      png_warning(png_ptr, "Unknown interlace method in IHDR");
      error = 1;
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown compression method in IHDR");
      error = 1;
   }

#  ifdef PNG_MNG_FEATURES_SUPPORTED
   /* Accept filter_method 64 (intrapixel differencing) only if
    * 1. Libpng was compiled with PNG_MNG_FEATURES_SUPPORTED and
    * 2. Libpng did not read a PNG signature (this filter_method is only
    *    used in PNG datastreams that are embedded in MNG datastreams) and
    * 3. The application called png_permit_mng_features with a mask that
    *    included PNG_FLAG_MNG_FILTER_64 and
    * 4. The filter_method is 64 and
    * 5. The color_type is RGB or RGBA
    */
   if ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) &&
       png_ptr->mng_features_permitted)
      png_warning(png_ptr, "MNG features are not allowed in a PNG datastream");

   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      if (!((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
          (filter_type == PNG_INTRAPIXEL_DIFFERENCING) &&
          ((png_ptr->mode & PNG_HAVE_PNG_SIGNATURE) == 0) &&
          (color_type == PNG_COLOR_TYPE_RGB ||
          color_type == PNG_COLOR_TYPE_RGB_ALPHA)))
      {
         png_warning(png_ptr, "Unknown filter method in IHDR");
         error = 1;
      }

      if (png_ptr->mode & PNG_HAVE_PNG_SIGNATURE)
      {
         png_warning(png_ptr, "Invalid filter method in IHDR");
         error = 1;
      }
   }

#  else
   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      png_warning(png_ptr, "Unknown filter method in IHDR");
      error = 1;
   }
#  endif

   if (error == 1)
      png_error(png_ptr, "Invalid IHDR data");
}

#if defined(PNG_sCAL_SUPPORTED) || defined(PNG_pCAL_SUPPORTED)
/* ASCII to fp functions */
/* Check an ASCII formated floating point value, see the more detailed
 * comments in pngpriv.h
 */
/* The following is used internally to preserve the sticky flags */
#define png_fp_add(state, flags) ((state) |= (flags))
#define png_fp_set(state, value) ((state) = (value) | ((state) & PNG_FP_STICKY))

int /* PRIVATE */
png_check_fp_number(png_const_charp string, png_size_t size, int *statep,
   png_size_tp whereami)
{
   int state = *statep;
   png_size_t i = *whereami;

   while (i < size)
   {
      int type;
      /* First find the type of the next character */
      switch (string[i])
      {
      case 43:  type = PNG_FP_SAW_SIGN;                   break;
      case 45:  type = PNG_FP_SAW_SIGN + PNG_FP_NEGATIVE; break;
      case 46:  type = PNG_FP_SAW_DOT;                    break;
      case 48:  type = PNG_FP_SAW_DIGIT;                  break;
      case 49: case 50: case 51: case 52:
      case 53: case 54: case 55: case 56:
      case 57:  type = PNG_FP_SAW_DIGIT + PNG_FP_NONZERO; break;
      case 69:
      case 101: type = PNG_FP_SAW_E;                      break;
      default:  goto PNG_FP_End;
      }

      /* Now deal with this type according to the current
       * state, the type is arranged to not overlap the
       * bits of the PNG_FP_STATE.
       */
      switch ((state & PNG_FP_STATE) + (type & PNG_FP_SAW_ANY))
      {
      case PNG_FP_INTEGER + PNG_FP_SAW_SIGN:
         if (state & PNG_FP_SAW_ANY)
            goto PNG_FP_End; /* not a part of the number */

         png_fp_add(state, type);
         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DOT:
         /* Ok as trailer, ok as lead of fraction. */
         if (state & PNG_FP_SAW_DOT) /* two dots */
            goto PNG_FP_End;

         else if (state & PNG_FP_SAW_DIGIT) /* trailing dot? */
            png_fp_add(state, type);

         else
            png_fp_set(state, PNG_FP_FRACTION | type);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_DIGIT:
         if (state & PNG_FP_SAW_DOT) /* delayed fraction */
            png_fp_set(state, PNG_FP_FRACTION | PNG_FP_SAW_DOT);

         png_fp_add(state, type | PNG_FP_WAS_VALID);

         break;

      case PNG_FP_INTEGER + PNG_FP_SAW_E:
         if ((state & PNG_FP_SAW_DIGIT) == 0)
            goto PNG_FP_End;

         png_fp_set(state, PNG_FP_EXPONENT);

         break;

   /* case PNG_FP_FRACTION + PNG_FP_SAW_SIGN:
         goto PNG_FP_End; ** no sign in fraction */

   /* case PNG_FP_FRACTION + PNG_FP_SAW_DOT:
         goto PNG_FP_End; ** Because SAW_DOT is always set */

      case PNG_FP_FRACTION + PNG_FP_SAW_DIGIT:
         png_fp_add(state, type | PNG_FP_WAS_VALID);
         break;

      case PNG_FP_FRACTION + PNG_FP_SAW_E:
         /* This is correct because the trailing '.' on an
          * integer is handled above - so we can only get here
          * with the sequence ".E" (with no preceding digits).
          */
         if ((state & PNG_FP_SAW_DIGIT) == 0)
            goto PNG_FP_End;

         png_fp_set(state, PNG_FP_EXPONENT);

         break;

      case PNG_FP_EXPONENT + PNG_FP_SAW_SIGN:
         if (state & PNG_FP_SAW_ANY)
            goto PNG_FP_End; /* not a part of the number */

         png_fp_add(state, PNG_FP_SAW_SIGN);

         break;

   /* case PNG_FP_EXPONENT + PNG_FP_SAW_DOT:
         goto PNG_FP_End; */

      case PNG_FP_EXPONENT + PNG_FP_SAW_DIGIT:
         png_fp_add(state, PNG_FP_SAW_DIGIT | PNG_FP_WAS_VALID);

         break;

   /* case PNG_FP_EXPONEXT + PNG_FP_SAW_E:
         goto PNG_FP_End; */

      default: goto PNG_FP_End; /* I.e. break 2 */
      }

      /* The character seems ok, continue. */
      ++i;
   }

PNG_FP_End:
   /* Here at the end, update the state and return the correct
    * return code.
    */
   *statep = state;
   *whereami = i;

   return (state & PNG_FP_SAW_DIGIT) != 0;
}


/* The same but for a complete string. */
int
png_check_fp_string(png_const_charp string, png_size_t size)
{
   int        state=0;
   png_size_t char_index=0;

   if (png_check_fp_number(string, size, &state, &char_index) &&
      (char_index == size || string[char_index] == 0))
      return state /* must be non-zero - see above */;

   return 0; /* i.e. fail */
}
#endif /* pCAL or sCAL */

#ifdef PNG_READ_sCAL_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
/* Utility used below - a simple accurate power of ten from an integral
 * exponent.
 */
static double
png_pow10(int power)
{
   int recip = 0;
   double d = 1;

   /* Handle negative exponent with a reciprocal at the end because
    * 10 is exact whereas .1 is inexact in base 2
    */
   if (power < 0)
   {
      if (power < DBL_MIN_10_EXP) return 0;
      recip = 1, power = -power;
   }

   if (power > 0)
   {
      /* Decompose power bitwise. */
      double mult = 10;
      do
      {
         if (power & 1) d *= mult;
         mult *= mult;
         power >>= 1;
      }
      while (power > 0);

      if (recip) d = 1/d;
   }
   /* else power is 0 and d is 1 */

   return d;
}

/* Function to format a floating point value in ASCII with a given
 * precision.
 */
void /* PRIVATE */
png_ascii_from_fp(png_structp png_ptr, png_charp ascii, png_size_t size,
    double fp, unsigned int precision)
{
   /* We use standard functions from math.h, but not printf because
    * that would require stdio.  The caller must supply a buffer of
    * sufficient size or we will png_error.  The tests on size and
    * the space in ascii[] consumed are indicated below.
    */
   if (precision < 1)
      precision = DBL_DIG;

   /* Enforce the limit of the implementation precision too. */
   if (precision > DBL_DIG+1)
      precision = DBL_DIG+1;

   /* Basic sanity checks */
   if (size >= precision+5) /* See the requirements below. */
   {
      if (fp < 0)
      {
         fp = -fp;
         *ascii++ = 45; /* '-'  PLUS 1 TOTAL 1 */
         --size;
      }

      if (fp >= DBL_MIN && fp <= DBL_MAX)
      {
         int exp_b10;       /* A base 10 exponent */
         double base;   /* 10^exp_b10 */

         /* First extract a base 10 exponent of the number,
          * the calculation below rounds down when converting
          * from base 2 to base 10 (multiply by log10(2) -
          * 0.3010, but 77/256 is 0.3008, so exp_b10 needs to
          * be increased.  Note that the arithmetic shift
          * performs a floor() unlike C arithmetic - using a
          * C multiply would break the following for negative
          * exponents.
          */
         (void)frexp(fp, &exp_b10); /* exponent to base 2 */

         exp_b10 = (exp_b10 * 77) >> 8; /* <= exponent to base 10 */

         /* Avoid underflow here. */
         base = png_pow10(exp_b10); /* May underflow */

         while (base < DBL_MIN || base < fp)
         {
            /* And this may overflow. */
            double test = png_pow10(exp_b10+1);

            if (test <= DBL_MAX)
               ++exp_b10, base = test;

            else
               break;
         }

         /* Normalize fp and correct exp_b10, after this fp is in the
          * range [.1,1) and exp_b10 is both the exponent and the digit
          * *before* which the decimal point should be inserted
          * (starting with 0 for the first digit).  Note that this
          * works even if 10^exp_b10 is out of range because of the
          * test on DBL_MAX above.
          */
         fp /= base;
         while (fp >= 1) fp /= 10, ++exp_b10;

         /* Because of the code above fp may, at this point, be
          * less than .1, this is ok because the code below can
          * handle the leading zeros this generates, so no attempt
          * is made to correct that here.
          */

         {
            int czero, clead, cdigits;
            char exponent[10];

            /* Allow up to two leading zeros - this will not lengthen
             * the number compared to using E-n.
             */
            if (exp_b10 < 0 && exp_b10 > -3) /* PLUS 3 TOTAL 4 */
            {
               czero = -exp_b10; /* PLUS 2 digits: TOTAL 3 */
               exp_b10 = 0;      /* Dot added below before first output. */
            }
            else
               czero = 0;    /* No zeros to add */

            /* Generate the digit list, stripping trailing zeros and
             * inserting a '.' before a digit if the exponent is 0.
             */
            clead = czero; /* Count of leading zeros */
            cdigits = 0;   /* Count of digits in list. */

            do
            {
               double d;

               fp *= 10;
               /* Use modf here, not floor and subtract, so that
                * the separation is done in one step.  At the end
                * of the loop don't break the number into parts so
                * that the final digit is rounded.
                */
               if (cdigits+czero-clead+1 < (int)precision)
                  fp = modf(fp, &d);

               else
               {
                  d = floor(fp + .5);

                  if (d > 9)
                  {
                     /* Rounding up to 10, handle that here. */
                     if (czero > 0)
                     {
                        --czero, d = 1;
                        if (cdigits == 0) --clead;
                     }
                     else
                     {
                        while (cdigits > 0 && d > 9)
                        {
                           int ch = *--ascii;

                           if (exp_b10 != (-1))
                              ++exp_b10;

                           else if (ch == 46)
                           {
                              ch = *--ascii, ++size;
                              /* Advance exp_b10 to '1', so that the
                               * decimal point happens after the
                               * previous digit.
                               */
                              exp_b10 = 1;
                           }

                           --cdigits;
                           d = ch - 47;  /* I.e. 1+(ch-48) */
                        }

                        /* Did we reach the beginning? If so adjust the
                         * exponent but take into account the leading
                         * decimal point.
                         */
                        if (d > 9)  /* cdigits == 0 */
                        {
                           if (exp_b10 == (-1))
                           {
                              /* Leading decimal point (plus zeros?), if
                               * we lose the decimal point here it must
                               * be reentered below.
                               */
                              int ch = *--ascii;

                              if (ch == 46)
                                 ++size, exp_b10 = 1;

                              /* Else lost a leading zero, so 'exp_b10' is
                               * still ok at (-1)
                               */
                           }
                           else
                              ++exp_b10;

                           /* In all cases we output a '1' */
                           d = 1;
                        }
                     }
                  }
                  fp = 0; /* Guarantees termination below. */
               }

               if (d == 0)
               {
                  ++czero;
                  if (cdigits == 0) ++clead;
               }
               else
               {
                  /* Included embedded zeros in the digit count. */
                  cdigits += czero - clead;
                  clead = 0;

                  while (czero > 0)
                  {
                     /* exp_b10 == (-1) means we just output the decimal
                      * place - after the DP don't adjust 'exp_b10' any
                      * more!
                      */
                     if (exp_b10 != (-1))
                     {
                        if (exp_b10 == 0) *ascii++ = 46, --size;
                        /* PLUS 1: TOTAL 4 */
                        --exp_b10;
                     }
                     *ascii++ = 48, --czero;
                  }

                  if (exp_b10 != (-1))
                  {
                     if (exp_b10 == 0) *ascii++ = 46, --size; /* counted
                                                                 above */
                     --exp_b10;
                  }
                  *ascii++ = (char)(48 + (int)d), ++cdigits;
               }
            }
            while (cdigits+czero-clead < (int)precision && fp > DBL_MIN);

            /* The total output count (max) is now 4+precision */

            /* Check for an exponent, if we don't need one we are
             * done and just need to terminate the string.  At
             * this point exp_b10==(-1) is effectively if flag - it got
             * to '-1' because of the decrement after outputing
             * the decimal point above (the exponent required is
             * *not* -1!)
             */
            if (exp_b10 >= (-1) && exp_b10 <= 2)
            {
               /* The following only happens if we didn't output the
                * leading zeros above for negative exponent, so this
                * doest add to the digit requirement.  Note that the
                * two zeros here can only be output if the two leading
                * zeros were *not* output, so this doesn't increase
                * the output count.
                */
               while (--exp_b10 >= 0) *ascii++ = 48;

               *ascii = 0;

               /* Total buffer requirement (including the '\0') is
                * 5+precision - see check at the start.
                */
               return;
            }

            /* Here if an exponent is required, adjust size for
             * the digits we output but did not count.  The total
             * digit output here so far is at most 1+precision - no
             * decimal point and no leading or trailing zeros have
             * been output.
             */
            size -= cdigits;

            *ascii++ = 69, --size;    /* 'E': PLUS 1 TOTAL 2+precision */
            if (exp_b10 < 0)
            {
               *ascii++ = 45, --size; /* '-': PLUS 1 TOTAL 3+precision */
               exp_b10 = -exp_b10;
            }

            cdigits = 0;

            while (exp_b10 > 0)
            {
               exponent[cdigits++] = (char)(48 + exp_b10 % 10);
               exp_b10 /= 10;
            }

            /* Need another size check here for the exponent digits, so
             * this need not be considered above.
             */
            if ((int)size > cdigits)
            {
               while (cdigits > 0) *ascii++ = exponent[--cdigits];

               *ascii = 0;

               return;
            }
         }
      }
      else if (!(fp >= DBL_MIN))
      {
         *ascii++ = 48; /* '0' */
         *ascii = 0;
         return;
      }
      else
      {
         *ascii++ = 105; /* 'i' */
         *ascii++ = 110; /* 'n' */
         *ascii++ = 102; /* 'f' */
         *ascii = 0;
         return;
      }
   }

   /* Here on buffer too small. */
   png_error(png_ptr, "ASCII conversion buffer too small");
}

#  endif /* FLOATING_POINT */

#  ifdef PNG_FIXED_POINT_SUPPORTED
/* Function to format a fixed point value in ASCII.
 */
void /* PRIVATE */
png_ascii_from_fixed(png_structp png_ptr, png_charp ascii, png_size_t size,
    png_fixed_point fp)
{
   /* Require space for 10 decimal digits, a decimal point, a minus sign and a
    * trailing \0, 13 characters:
    */
   if (size > 12)
   {
      png_uint_32 num;

      /* Avoid overflow here on the minimum integer. */
      if (fp < 0)
         *ascii++ = 45, --size, num = -fp;
      else
         num = fp;

      if (num <= 0x80000000U) /* else overflowed */
      {
         unsigned int ndigits = 0, first = 16 /* flag value */;
         char digits[10];

         while (num)
         {
            /* Split the low digit off num: */
            unsigned int tmp = num/10;
            num -= tmp*10;
            digits[ndigits++] = (char)(48 + num);
            /* Record the first non-zero digit, note that this is a number
             * starting at 1, it's not actually the array index.
             */
            if (first == 16 && num > 0)
               first = ndigits;
            num = tmp;
         }

         if (ndigits > 0)
         {
            while (ndigits > 5) *ascii++ = digits[--ndigits];
            /* The remaining digits are fractional digits, ndigits is '5' or
             * smaller at this point.  It is certainly not zero.  Check for a
             * non-zero fractional digit:
             */
            if (first <= 5)
            {
               unsigned int i;
               *ascii++ = 46; /* decimal point */
               /* ndigits may be <5 for small numbers, output leading zeros
                * then ndigits digits to first:
                */
               i = 5;
               while (ndigits < i) *ascii++ = 48, --i;
               while (ndigits >= first) *ascii++ = digits[--ndigits];
               /* Don't output the trailing zeros! */
            }
         }
         else
            *ascii++ = 48;

         /* And null terminate the string: */
         *ascii = 0;
         return;
      }
   }

   /* Here on buffer too small. */
   png_error(png_ptr, "ASCII conversion buffer too small");
}
#   endif /* FIXED_POINT */
#endif /* READ_SCAL */

#if defined(PNG_FLOATING_POINT_SUPPORTED) && \
   !defined(PNG_FIXED_POINT_MACRO_SUPPORTED)
png_fixed_point
png_fixed(png_structp png_ptr, double fp, png_const_charp text)
{
   double r = floor(100000 * fp + .5);

   if (r > 2147483647. || r < -2147483648.)
      png_fixed_error(png_ptr, text);

   return (png_fixed_point)r;
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || \
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG__READ_pHYs_SUPPORTED)
/* muldiv functions */
/* This API takes signed arguments and rounds the result to the nearest
 * integer (or, for a fixed point number - the standard argument - to
 * the nearest .00001).  Overflow and divide by zero are signalled in
 * the result, a boolean - true on success, false on overflow.
 */
int
png_muldiv(png_fixed_point_p res, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   /* Return a * times / divisor, rounded. */
   if (divisor != 0)
   {
      if (a == 0 || times == 0)
      {
         *res = 0;
         return 1;
      }
      else
      {
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = a;
         r *= times;
         r /= divisor;
         r = floor(r+.5);

         /* A png_fixed_point is a 32-bit integer. */
         if (r <= 2147483647. && r >= -2147483648.)
         {
            *res = (png_fixed_point)r;
            return 1;
         }
#else
         int negative = 0;
         png_uint_32 A, T, D;
         png_uint_32 s16, s32, s00;

         if (a < 0)
            negative = 1, A = -a;
         else
            A = a;

         if (times < 0)
            negative = !negative, T = -times;
         else
            T = times;

         if (divisor < 0)
            negative = !negative, D = -divisor;
         else
            D = divisor;

         /* Following can't overflow because the arguments only
          * have 31 bits each, however the result may be 32 bits.
          */
         s16 = (A >> 16) * (T & 0xffff) +
                           (A & 0xffff) * (T >> 16);
         /* Can't overflow because the a*times bit is only 30
          * bits at most.
          */
         s32 = (A >> 16) * (T >> 16) + (s16 >> 16);
         s00 = (A & 0xffff) * (T & 0xffff);

         s16 = (s16 & 0xffff) << 16;
         s00 += s16;

         if (s00 < s16)
            ++s32; /* carry */

         if (s32 < D) /* else overflow */
         {
            /* s32.s00 is now the 64-bit product, do a standard
             * division, we know that s32 < D, so the maximum
             * required shift is 31.
             */
            int bitshift = 32;
            png_fixed_point result = 0; /* NOTE: signed */

            while (--bitshift >= 0)
            {
               png_uint_32 d32, d00;

               if (bitshift > 0)
                  d32 = D >> (32-bitshift), d00 = D << bitshift;

               else
                  d32 = 0, d00 = D;

               if (s32 > d32)
               {
                  if (s00 < d00) --s32; /* carry */
                  s32 -= d32, s00 -= d00, result += 1<<bitshift;
               }

               else
                  if (s32 == d32 && s00 >= d00)
                     s32 = 0, s00 -= d00, result += 1<<bitshift;
            }

            /* Handle the rounding. */
            if (s00 >= (D >> 1))
               ++result;

            if (negative)
               result = -result;

            /* Check for overflow. */
            if ((negative && result <= 0) || (!negative && result >= 0))
            {
               *res = result;
               return 1;
            }
         }
#endif
      }
   }

   return 0;
}
#endif /* READ_GAMMA || INCH_CONVERSIONS */

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_INCH_CONVERSIONS_SUPPORTED)
/* The following is for when the caller doesn't much care about the
 * result.
 */
png_fixed_point
png_muldiv_warn(png_structp png_ptr, png_fixed_point a, png_int_32 times,
    png_int_32 divisor)
{
   png_fixed_point result;

   if (png_muldiv(&result, a, times, divisor))
      return result;

   png_warning(png_ptr, "fixed point overflow ignored");
   return 0;
}
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED /* more fixed point functions for gammma */
/* Calculate a reciprocal, return 0 on div-by-zero or overflow. */
png_fixed_point
png_reciprocal(png_fixed_point a)
{
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = floor(1E10/a+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, 100000, 100000, a))
      return res;
#endif

   return 0; /* error/overflow */
}

/* A local convenience routine. */
static png_fixed_point
png_product2(png_fixed_point a, png_fixed_point b)
{
   /* The required result is 1/a * 1/b; the following preserves accuracy. */
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = a * 1E-5;
   r *= b;
   r = floor(r+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   png_fixed_point res;

   if (png_muldiv(&res, a, b, 100000))
      return res;
#endif

   return 0; /* overflow */
}

/* The inverse of the above. */
png_fixed_point
png_reciprocal2(png_fixed_point a, png_fixed_point b)
{
   /* The required result is 1/a * 1/b; the following preserves accuracy. */
#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
   double r = 1E15/a;
   r /= b;
   r = floor(r+.5);

   if (r <= 2147483647. && r >= -2147483648.)
      return (png_fixed_point)r;
#else
   /* This may overflow because the range of png_fixed_point isn't symmetric,
    * but this API is only used for the product of file and screen gamma so it
    * doesn't matter that the smallest number it can produce is 1/21474, not
    * 1/100000
    */
   png_fixed_point res = png_product2(a, b);

   if (res != 0)
      return png_reciprocal(res);
#endif

   return 0; /* overflow */
}
#endif /* READ_GAMMA */

#ifdef PNG_CHECK_cHRM_SUPPORTED
/* Added at libpng version 1.2.34 (Dec 8, 2008) and 1.4.0 (Jan 2,
 * 2010: moved from pngset.c) */
/*
 *    Multiply two 32-bit numbers, V1 and V2, using 32-bit
 *    arithmetic, to produce a 64-bit result in the HI/LO words.
 *
 *                  A B
 *                x C D
 *               ------
 *              AD || BD
 *        AC || CB || 0
 *
 *    where A and B are the high and low 16-bit words of V1,
 *    C and D are the 16-bit words of V2, AD is the product of
 *    A and D, and X || Y is (X << 16) + Y.
*/

void /* PRIVATE */
png_64bit_product (long v1, long v2, unsigned long *hi_product,
    unsigned long *lo_product)
{
   int a, b, c, d;
   long lo, hi, x, y;

   a = (v1 >> 16) & 0xffff;
   b = v1 & 0xffff;
   c = (v2 >> 16) & 0xffff;
   d = v2 & 0xffff;

   lo = b * d;                   /* BD */
   x = a * d + c * b;            /* AD + CB */
   y = ((lo >> 16) & 0xffff) + x;

   lo = (lo & 0xffff) | ((y & 0xffff) << 16);
   hi = (y >> 16) & 0xffff;

   hi += a * c;                  /* AC */

   *hi_product = (unsigned long)hi;
   *lo_product = (unsigned long)lo;
}
#endif /* CHECK_cHRM */

#ifdef PNG_READ_GAMMA_SUPPORTED /* gamma table code */
#ifndef PNG_FLOATING_ARITHMETIC_SUPPORTED
/* Fixed point gamma.
 *
 * To calculate gamma this code implements fast log() and exp() calls using only
 * fixed point arithmetic.  This code has sufficient precision for either 8-bit
 * or 16-bit sample values.
 *
 * The tables used here were calculated using simple 'bc' programs, but C double
 * precision floating point arithmetic would work fine.  The programs are given
 * at the head of each table.
 *
 * 8-bit log table
 *   This is a table of -log(value/255)/log(2) for 'value' in the range 128 to
 *   255, so it's the base 2 logarithm of a normalized 8-bit floating point
 *   mantissa.  The numbers are 32-bit fractions.
 */
static png_uint_32
png_8bit_l2[128] =
{
#  if PNG_DO_BC
      for (i=128;i<256;++i) { .5 - l(i/255)/l(2)*65536*65536; }
#  endif
   4270715492U, 4222494797U, 4174646467U, 4127164793U, 4080044201U, 4033279239U,
   3986864580U, 3940795015U, 3895065449U, 3849670902U, 3804606499U, 3759867474U,
   3715449162U, 3671346997U, 3627556511U, 3584073329U, 3540893168U, 3498011834U,
   3455425220U, 3413129301U, 3371120137U, 3329393864U, 3287946700U, 3246774933U,
   3205874930U, 3165243125U, 3124876025U, 3084770202U, 3044922296U, 3005329011U,
   2965987113U, 2926893432U, 2888044853U, 2849438323U, 2811070844U, 2772939474U,
   2735041326U, 2697373562U, 2659933400U, 2622718104U, 2585724991U, 2548951424U,
   2512394810U, 2476052606U, 2439922311U, 2404001468U, 2368287663U, 2332778523U,
   2297471715U, 2262364947U, 2227455964U, 2192742551U, 2158222529U, 2123893754U,
   2089754119U, 2055801552U, 2022034013U, 1988449497U, 1955046031U, 1921821672U,
   1888774511U, 1855902668U, 1823204291U, 1790677560U, 1758320682U, 1726131893U,
   1694109454U, 1662251657U, 1630556815U, 1599023271U, 1567649391U, 1536433567U,
   1505374214U, 1474469770U, 1443718700U, 1413119487U, 1382670639U, 1352370686U,
   1322218179U, 1292211689U, 1262349810U, 1232631153U, 1203054352U, 1173618059U,
   1144320946U, 1115161701U, 1086139034U, 1057251672U, 1028498358U, 999877854U,
   971388940U, 943030410U, 914801076U, 886699767U, 858725327U, 830876614U,
   803152505U, 775551890U, 748073672U, 720716771U, 693480120U, 666362667U,
   639363374U, 612481215U, 585715177U, 559064263U, 532527486U, 506103872U,
   479792461U, 453592303U, 427502463U, 401522014U, 375650043U, 349885648U,
   324227938U, 298676034U, 273229066U, 247886176U, 222646516U, 197509248U,
   172473545U, 147538590U, 122703574U, 97967701U, 73330182U, 48790236U,
   24347096U, 0U
#if 0
   /* The following are the values for 16-bit tables - these work fine for the
    * 8-bit conversions but produce very slightly larger errors in the 16-bit
    * log (about 1.2 as opposed to 0.7 absolute error in the final value).  To
    * use these all the shifts below must be adjusted appropriately.
    */
   65166, 64430, 63700, 62976, 62257, 61543, 60835, 60132, 59434, 58741, 58054,
   57371, 56693, 56020, 55352, 54689, 54030, 53375, 52726, 52080, 51439, 50803,
   50170, 49542, 48918, 48298, 47682, 47070, 46462, 45858, 45257, 44661, 44068,
   43479, 42894, 42312, 41733, 41159, 40587, 40020, 39455, 38894, 38336, 37782,
   37230, 36682, 36137, 35595, 35057, 34521, 33988, 33459, 32932, 32408, 31887,
   31369, 30854, 30341, 29832, 29325, 28820, 28319, 27820, 27324, 26830, 26339,
   25850, 25364, 24880, 24399, 23920, 23444, 22970, 22499, 22029, 21562, 21098,
   20636, 20175, 19718, 19262, 18808, 18357, 17908, 17461, 17016, 16573, 16132,
   15694, 15257, 14822, 14390, 13959, 13530, 13103, 12678, 12255, 11834, 11415,
   10997, 10582, 10168, 9756, 9346, 8937, 8531, 8126, 7723, 7321, 6921, 6523,
   6127, 5732, 5339, 4947, 4557, 4169, 3782, 3397, 3014, 2632, 2251, 1872, 1495,
   1119, 744, 372
#endif
};

PNG_STATIC png_int_32
png_log8bit(unsigned int x)
{
   unsigned int lg2 = 0;
   /* Each time 'x' is multiplied by 2, 1 must be subtracted off the final log,
    * because the log is actually negate that means adding 1.  The final
    * returned value thus has the range 0 (for 255 input) to 7.994 (for 1
    * input), return 7.99998 for the overflow (log 0) case - so the result is
    * always at most 19 bits.
    */
   if ((x &= 0xff) == 0)
      return 0xffffffff;

   if ((x & 0xf0) == 0)
      lg2  = 4, x <<= 4;

   if ((x & 0xc0) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x80) == 0)
      lg2 += 1, x <<= 1;

   /* result is at most 19 bits, so this cast is safe: */
   return (png_int_32)((lg2 << 16) + ((png_8bit_l2[x-128]+32768)>>16));
}

/* The above gives exact (to 16 binary places) log2 values for 8-bit images,
 * for 16-bit images we use the most significant 8 bits of the 16-bit value to
 * get an approximation then multiply the approximation by a correction factor
 * determined by the remaining up to 8 bits.  This requires an additional step
 * in the 16-bit case.
 *
 * We want log2(value/65535), we have log2(v'/255), where:
 *
 *    value = v' * 256 + v''
 *          = v' * f
 *
 * So f is value/v', which is equal to (256+v''/v') since v' is in the range 128
 * to 255 and v'' is in the range 0 to 255 f will be in the range 256 to less
 * than 258.  The final factor also needs to correct for the fact that our 8-bit
 * value is scaled by 255, whereas the 16-bit values must be scaled by 65535.
 *
 * This gives a final formula using a calculated value 'x' which is value/v' and
 * scaling by 65536 to match the above table:
 *
 *   log2(x/257) * 65536
 *
 * Since these numbers are so close to '1' we can use simple linear
 * interpolation between the two end values 256/257 (result -368.61) and 258/257
 * (result 367.179).  The values used below are scaled by a further 64 to give
 * 16-bit precision in the interpolation:
 *
 * Start (256): -23591
 * Zero  (257):      0
 * End   (258):  23499
 */
PNG_STATIC png_int_32
png_log16bit(png_uint_32 x)
{
   unsigned int lg2 = 0;

   /* As above, but now the input has 16 bits. */
   if ((x &= 0xffff) == 0)
      return 0xffffffff;

   if ((x & 0xff00) == 0)
      lg2  = 8, x <<= 8;

   if ((x & 0xf000) == 0)
      lg2 += 4, x <<= 4;

   if ((x & 0xc000) == 0)
      lg2 += 2, x <<= 2;

   if ((x & 0x8000) == 0)
      lg2 += 1, x <<= 1;

   /* Calculate the base logarithm from the top 8 bits as a 28-bit fractional
    * value.
    */
   lg2 <<= 28;
   lg2 += (png_8bit_l2[(x>>8)-128]+8) >> 4;

   /* Now we need to interpolate the factor, this requires a division by the top
    * 8 bits.  Do this with maximum precision.
    */
   x = ((x << 16) + (x >> 9)) / (x >> 8);

   /* Since we divided by the top 8 bits of 'x' there will be a '1' at 1<<24,
    * the value at 1<<16 (ignoring this) will be 0 or 1; this gives us exactly
    * 16 bits to interpolate to get the low bits of the result.  Round the
    * answer.  Note that the end point values are scaled by 64 to retain overall
    * precision and that 'lg2' is current scaled by an extra 12 bits, so adjust
    * the overall scaling by 6-12.  Round at every step.
    */
   x -= 1U << 24;

   if (x <= 65536U) /* <= '257' */
      lg2 += ((23591U * (65536U-x)) + (1U << (16+6-12-1))) >> (16+6-12);

   else
      lg2 -= ((23499U * (x-65536U)) + (1U << (16+6-12-1))) >> (16+6-12);

   /* Safe, because the result can't have more than 20 bits: */
   return (png_int_32)((lg2 + 2048) >> 12);
}

/* The 'exp()' case must invert the above, taking a 20-bit fixed point
 * logarithmic value and returning a 16 or 8-bit number as appropriate.  In
 * each case only the low 16 bits are relevant - the fraction - since the
 * integer bits (the top 4) simply determine a shift.
 *
 * The worst case is the 16-bit distinction between 65535 and 65534, this
 * requires perhaps spurious accuracty in the decoding of the logarithm to
 * distinguish log2(65535/65534.5) - 10^-5 or 17 bits.  There is little chance
 * of getting this accuracy in practice.
 *
 * To deal with this the following exp() function works out the exponent of the
 * frational part of the logarithm by using an accurate 32-bit value from the
 * top four fractional bits then multiplying in the remaining bits.
 */
static png_uint_32
png_32bit_exp[16] =
{
#  if PNG_DO_BC
      for (i=0;i<16;++i) { .5 + e(-i/16*l(2))*2^32; }
#  endif
   /* NOTE: the first entry is deliberately set to the maximum 32-bit value. */
   4294967295U, 4112874773U, 3938502376U, 3771522796U, 3611622603U, 3458501653U,
   3311872529U, 3171459999U, 3037000500U, 2908241642U, 2784941738U, 2666869345U,
   2553802834U, 2445529972U, 2341847524U, 2242560872U
};

/* Adjustment table; provided to explain the numbers in the code below. */
#if PNG_DO_BC
for (i=11;i>=0;--i){ print i, " ", (1 - e(-(2^i)/65536*l(2))) * 2^(32-i), "\n"}
   11 44937.64284865548751208448
   10 45180.98734845585101160448
    9 45303.31936980687359311872
    8 45364.65110595323018870784
    7 45395.35850361789624614912
    6 45410.72259715102037508096
    5 45418.40724413220722311168
    4 45422.25021786898173001728
    3 45424.17186732298419044352
    2 45425.13273269940811464704
    1 45425.61317555035558641664
    0 45425.85339951654943850496
#endif

PNG_STATIC png_uint_32
png_exp(png_fixed_point x)
{
   if (x > 0 && x <= 0xfffff) /* Else overflow or zero (underflow) */
   {
      /* Obtain a 4-bit approximation */
      png_uint_32 e = png_32bit_exp[(x >> 12) & 0xf];

      /* Incorporate the low 12 bits - these decrease the returned value by
       * multiplying by a number less than 1 if the bit is set.  The multiplier
       * is determined by the above table and the shift. Notice that the values
       * converge on 45426 and this is used to allow linear interpolation of the
       * low bits.
       */
      if (x & 0x800)
         e -= (((e >> 16) * 44938U) +  16U) >> 5;

      if (x & 0x400)
         e -= (((e >> 16) * 45181U) +  32U) >> 6;

      if (x & 0x200)
         e -= (((e >> 16) * 45303U) +  64U) >> 7;

      if (x & 0x100)
         e -= (((e >> 16) * 45365U) + 128U) >> 8;

      if (x & 0x080)
         e -= (((e >> 16) * 45395U) + 256U) >> 9;

      if (x & 0x040)
         e -= (((e >> 16) * 45410U) + 512U) >> 10;

      /* And handle the low 6 bits in a single block. */
      e -= (((e >> 16) * 355U * (x & 0x3fU)) + 256U) >> 9;

      /* Handle the upper bits of x. */
      e >>= x >> 16;
      return e;
   }

   /* Check for overflow */
   if (x <= 0)
      return png_32bit_exp[0];

   /* Else underflow */
   return 0;
}

PNG_STATIC png_byte
png_exp8bit(png_fixed_point lg2)
{
   /* Get a 32-bit value: */
   png_uint_32 x = png_exp(lg2);

   /* Convert the 32-bit value to 0..255 by multiplying by 256-1, note that the
    * second, rounding, step can't overflow because of the first, subtraction,
    * step.
    */
   x -= x >> 8;
   return (png_byte)((x + 0x7fffffU) >> 24);
}

PNG_STATIC png_uint_16
png_exp16bit(png_fixed_point lg2)
{
   /* Get a 32-bit value: */
   png_uint_32 x = png_exp(lg2);

   /* Convert the 32-bit value to 0..65535 by multiplying by 65536-1: */
   x -= x >> 16;
   return (png_uint_16)((x + 32767U) >> 16);
}
#endif /* FLOATING_ARITHMETIC */

png_byte
png_gamma_8bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 255)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = floor(255*pow(value/255.,gamma_val*.00001)+.5);
         return (png_byte)r;
#     else
         png_int_32 lg2 = png_log8bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1))
            return png_exp8bit(res);

         /* Overflow. */
         value = 0;
#     endif
   }

   return (png_byte)value;
}

png_uint_16
png_gamma_16bit_correct(unsigned int value, png_fixed_point gamma_val)
{
   if (value > 0 && value < 65535)
   {
#     ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
         double r = floor(65535*pow(value/65535.,gamma_val*.00001)+.5);
         return (png_uint_16)r;
#     else
         png_int_32 lg2 = png_log16bit(value);
         png_fixed_point res;

         if (png_muldiv(&res, gamma_val, lg2, PNG_FP_1))
            return png_exp16bit(res);

         /* Overflow. */
         value = 0;
#     endif
   }

   return (png_uint_16)value;
}

/* This does the right thing based on the bit_depth field of the
 * png_struct, interpreting values as 8-bit or 16-bit.  While the result
 * is nominally a 16-bit value if bit depth is 8 then the result is
 * 8-bit (as are the arguments.)
 */
png_uint_16 /* PRIVATE */
png_gamma_correct(png_structp png_ptr, unsigned int value,
    png_fixed_point gamma_val)
{
   if (png_ptr->bit_depth == 8)
      return png_gamma_8bit_correct(value, gamma_val);

   else
      return png_gamma_16bit_correct(value, gamma_val);
}

/* This is the shared test on whether a gamma value is 'significant' - whether
 * it is worth doing gamma correction.
 */
int /* PRIVATE */
png_gamma_significant(png_fixed_point gamma_val)
{
   return gamma_val < PNG_FP_1 - PNG_GAMMA_THRESHOLD_FIXED ||
       gamma_val > PNG_FP_1 + PNG_GAMMA_THRESHOLD_FIXED;
}

/* Internal function to build a single 16-bit table - the table consists of
 * 'num' 256 entry subtables, where 'num' is determined by 'shift' - the amount
 * to shift the input values right (or 16-number_of_signifiant_bits).
 *
 * The caller is responsible for ensuring that the table gets cleaned up on
 * png_error (i.e. if one of the mallocs below fails) - i.e. the *table argument
 * should be somewhere that will be cleaned.
 */
static void
png_build_16bit_table(png_structp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   /* Various values derived from 'shift': */
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   PNG_CONST unsigned int max_by_2 = 1U << (15U-shift);
   unsigned int i;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * png_sizeof(png_uint_16p));

   for (i = 0; i < num; i++)
   {
      png_uint_16p sub_table = table[i] =
          (png_uint_16p)png_malloc(png_ptr, 256 * png_sizeof(png_uint_16));

      /* The 'threshold' test is repeated here because it can arise for one of
       * the 16-bit tables even if the others don't hit it.
       */
      if (png_gamma_significant(gamma_val))
      {
         /* The old code would overflow at the end and this would cause the
          * 'pow' function to return a result >1, resulting in an
          * arithmetic error.  This code follows the spec exactly; ig is
          * the recovered input sample, it always has 8-16 bits.
          *
          * We want input * 65535/max, rounded, the arithmetic fits in 32
          * bits (unsigned) so long as max <= 32767.
          */
         unsigned int j;
         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;
#           ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
               /* Inline the 'max' scaling operation: */
               double d = floor(65535*pow(ig/(double)max, gamma_val*.00001)+.5);
               sub_table[j] = (png_uint_16)d;
#           else
               if (shift)
                  ig = (ig * 65535U + max_by_2)/max;

               sub_table[j] = png_gamma_16bit_correct(ig, gamma_val);
#           endif
         }
      }
      else
      {
         /* We must still build a table, but do it the fast way. */
         unsigned int j;

         for (j = 0; j < 256; j++)
         {
            png_uint_32 ig = (j << (8-shift)) + i;

            if (shift)
               ig = (ig * 65535U + max_by_2)/max;

            sub_table[j] = (png_uint_16)ig;
         }
      }
   }
}

/* NOTE: this function expects the *inverse* of the overall gamma transformation
 * required.
 */
static void
png_build_16to8_table(png_structp png_ptr, png_uint_16pp *ptable,
   PNG_CONST unsigned int shift, PNG_CONST png_fixed_point gamma_val)
{
   PNG_CONST unsigned int num = 1U << (8U - shift);
   PNG_CONST unsigned int max = (1U << (16U - shift))-1U;
   unsigned int i;
   png_uint_32 last;

   png_uint_16pp table = *ptable =
       (png_uint_16pp)png_calloc(png_ptr, num * png_sizeof(png_uint_16p));

   /* 'num' is the number of tables and also the number of low bits of low
    * bits of the input 16-bit value used to select a table.  Each table is
    * itself index by the high 8 bits of the value.
    */
   for (i = 0; i < num; i++)
      table[i] = (png_uint_16p)png_malloc(png_ptr,
          256 * png_sizeof(png_uint_16));

   /* 'gamma_val' is set to the reciprocal of the value calculated above, so
    * pow(out,g) is an *input* value.  'last' is the last input value set.
    *
    * In the loop 'i' is used to find output values.  Since the output is
    * 8-bit there are only 256 possible values.  The tables are set up to
    * select the closest possible output value for each input by finding
    * the input value at the boundary between each pair of output values
    * and filling the table up to that boundary with the lower output
    * value.
    *
    * The boundary values are 0.5,1.5..253.5,254.5.  Since these are 9-bit
    * values the code below uses a 16-bit value in i; the values start at
    * 128.5 (for 0.5) and step by 257, for a total of 254 values (the last
    * entries are filled with 255).  Start i at 128 and fill all 'last'
    * table entries <= 'max'
    */
   last = 0;
   for (i = 0; i < 255; ++i) /* 8-bit output value */
   {
      /* Find the corresponding maximum input value */
      png_uint_16 out = (png_uint_16)(i * 257U); /* 16-bit output value */

      /* Find the boundary value in 16 bits: */
      png_uint_32 bound = png_gamma_16bit_correct(out+128U, gamma_val);

      /* Adjust (round) to (16-shift) bits: */
      bound = (bound * max + 32768U)/65535U + 1U;

      while (last < bound)
      {
         table[last & (0xffU >> shift)][last >> (8U - shift)] = out;
         last++;
      }
   }

   /* And fill in the final entries. */
   while (last < (num << 8))
   {
      table[last & (0xff >> shift)][last >> (8U - shift)] = 65535U;
      last++;
   }
}

/* Build a single 8-bit table: same as the 16-bit case but much simpler (and
 * typically much faster).  Note that libpng currently does no sBIT processing
 * (apparently contrary to the spec) so a 256 entry table is always generated.
 */
static void
png_build_8bit_table(png_structp png_ptr, png_bytepp ptable,
   PNG_CONST png_fixed_point gamma_val)
{
   unsigned int i;
   png_bytep table = *ptable = (png_bytep)png_malloc(png_ptr, 256);

   if (png_gamma_significant(gamma_val)) for (i=0; i<256; i++)
      table[i] = png_gamma_8bit_correct(i, gamma_val);

   else for (i=0; i<256; ++i)
      table[i] = (png_byte)i;
}

/* We build the 8- or 16-bit gamma tables here.  Note that for 16-bit
 * tables, we don't make a full table if we are reducing to 8-bit in
 * the future.  Note also how the gamma_16 tables are segmented so that
 * we don't need to allocate > 64K chunks for a full 16-bit table.
 */
void /* PRIVATE */
png_build_gamma_table(png_structp png_ptr, int bit_depth)
{
  png_debug(1, "in png_build_gamma_table");

  if (bit_depth <= 8)
  {
     png_build_8bit_table(png_ptr, &png_ptr->gamma_table,
         png_ptr->screen_gamma > 0 ?  png_reciprocal2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if (png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY))
     {
        png_build_8bit_table(png_ptr, &png_ptr->gamma_to_1,
            png_reciprocal(png_ptr->gamma));

        png_build_8bit_table(png_ptr, &png_ptr->gamma_from_1,
            png_ptr->screen_gamma > 0 ?  png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->gamma/* Probably doing rgb_to_gray */);
     }
#endif /* READ_BACKGROUND || READ_ALPHA_MODE || RGB_TO_GRAY */
  }
  else
  {
     png_byte shift, sig_bit;

     if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
     {
        sig_bit = png_ptr->sig_bit.red;

        if (png_ptr->sig_bit.green > sig_bit)
           sig_bit = png_ptr->sig_bit.green;

        if (png_ptr->sig_bit.blue > sig_bit)
           sig_bit = png_ptr->sig_bit.blue;
     }
     else
        sig_bit = png_ptr->sig_bit.gray;

     /* 16-bit gamma code uses this equation:
      *
      *   ov = table[(iv & 0xff) >> gamma_shift][iv >> 8]
      *
      * Where 'iv' is the input color value and 'ov' is the output value -
      * pow(iv, gamma).
      *
      * Thus the gamma table consists of up to 256 256 entry tables.  The table
      * is selected by the (8-gamma_shift) most significant of the low 8 bits of
      * the color value then indexed by the upper 8 bits:
      *
      *   table[low bits][high 8 bits]
      *
      * So the table 'n' corresponds to all those 'iv' of:
      *
      *   <all high 8-bit values><n << gamma_shift>..<(n+1 << gamma_shift)-1>
      *
      */
     if (sig_bit > 0 && sig_bit < 16U)
        shift = (png_byte)(16U - sig_bit); /* shift == insignificant bits */

     else
        shift = 0; /* keep all 16 bits */

     if (png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8))
     {
        /* PNG_MAX_GAMMA_8 is the number of bits to keep - effectively
         * the significant bits in the *input* when the output will
         * eventually be 8 bits.  By default it is 11.
         */
        if (shift < (16U - PNG_MAX_GAMMA_8))
           shift = (16U - PNG_MAX_GAMMA_8);
     }

     if (shift > 8U)
        shift = 8U; /* Guarantees at least one table! */

     png_ptr->gamma_shift = shift;

#ifdef PNG_16BIT_SUPPORTED
     /* NOTE: prior to 1.5.4 this test used to include PNG_BACKGROUND (now
      * PNG_COMPOSE).  This effectively smashed the background calculation for
      * 16-bit output because the 8-bit table assumes the result will be reduced
      * to 8 bits.
      */
     if (png_ptr->transformations & (PNG_16_TO_8 | PNG_SCALE_16_TO_8))
#endif
         png_build_16to8_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_product2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);

#ifdef PNG_16BIT_SUPPORTED
     else
         png_build_16bit_table(png_ptr, &png_ptr->gamma_16_table, shift,
         png_ptr->screen_gamma > 0 ? png_reciprocal2(png_ptr->gamma,
         png_ptr->screen_gamma) : PNG_FP_1);
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
   defined(PNG_READ_ALPHA_MODE_SUPPORTED) || \
   defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
     if (png_ptr->transformations & (PNG_COMPOSE | PNG_RGB_TO_GRAY))
     {
        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_to_1, shift,
            png_reciprocal(png_ptr->gamma));

        /* Notice that the '16 from 1' table should be full precision, however
         * the lookup on this table still uses gamma_shift, so it can't be.
         * TODO: fix this.
         */
        png_build_16bit_table(png_ptr, &png_ptr->gamma_16_from_1, shift,
            png_ptr->screen_gamma > 0 ? png_reciprocal(png_ptr->screen_gamma) :
            png_ptr->gamma/* Probably doing rgb_to_gray */);
     }
#endif /* READ_BACKGROUND || READ_ALPHA_MODE || RGB_TO_GRAY */
  }
}
#endif /* READ_GAMMA */
#endif /* defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED) */
} // namespace PrivatePng
