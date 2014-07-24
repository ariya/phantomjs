/***************************************************************************/
/*                                                                         */
/*  cffparse.c                                                             */
/*                                                                         */
/*    CFF token stream parser (body)                                       */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2007, 2008, 2009 by             */
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
#include "cffparse.h"
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H

#include "cfferrs.h"
#include "cffpic.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffparse




  FT_LOCAL_DEF( void )
  cff_parser_init( CFF_Parser  parser,
                   FT_UInt     code,
                   void*       object,
                   FT_Library  library)
  {
    FT_MEM_ZERO( parser, sizeof ( *parser ) );

    parser->top         = parser->stack;
    parser->object_code = code;
    parser->object      = object;
    parser->library     = library;
  }


  /* read an integer */
  static FT_Long
  cff_parse_integer( FT_Byte*  start,
                     FT_Byte*  limit )
  {
    FT_Byte*  p   = start;
    FT_Int    v   = *p++;
    FT_Long   val = 0;


    if ( v == 28 )
    {
      if ( p + 2 > limit )
        goto Bad;

      val = (FT_Short)( ( (FT_Int)p[0] << 8 ) | p[1] );
      p  += 2;
    }
    else if ( v == 29 )
    {
      if ( p + 4 > limit )
        goto Bad;

      val = ( (FT_Long)p[0] << 24 ) |
            ( (FT_Long)p[1] << 16 ) |
            ( (FT_Long)p[2] <<  8 ) |
                       p[3];
      p += 4;
    }
    else if ( v < 247 )
    {
      val = v - 139;
    }
    else if ( v < 251 )
    {
      if ( p + 1 > limit )
        goto Bad;

      val = ( v - 247 ) * 256 + p[0] + 108;
      p++;
    }
    else
    {
      if ( p + 1 > limit )
        goto Bad;

      val = -( v - 251 ) * 256 - p[0] - 108;
      p++;
    }

  Exit:
    return val;

  Bad:
    val = 0;
    goto Exit;
  }


  static const FT_Long power_tens[] =
  {
    1L,
    10L,
    100L,
    1000L,
    10000L,
    100000L,
    1000000L,
    10000000L,
    100000000L,
    1000000000L
  };


  /* read a real */
  static FT_Fixed
  cff_parse_real( FT_Byte*  start,
                  FT_Byte*  limit,
                  FT_Long   power_ten,
                  FT_Long*  scaling )
  {
    FT_Byte*  p = start;
    FT_UInt   nib;
    FT_UInt   phase;

    FT_Long   result, number, rest, exponent;
    FT_Int    sign = 0, exponent_sign = 0;
    FT_Long   exponent_add, integer_length, fraction_length;


    if ( scaling )
      *scaling  = 0;

    result = 0;

    number   = 0;
    rest     = 0;
    exponent = 0;

    exponent_add    = 0;
    integer_length  = 0;
    fraction_length = 0;

    FT_UNUSED( rest );

    /* First of all, read the integer part. */
    phase = 4;

    for (;;)
    {
      /* If we entered this iteration with phase == 4, we need to */
      /* read a new byte.  This also skips past the initial 0x1E. */
      if ( phase )
      {
        p++;

        /* Make sure we don't read past the end. */
        if ( p >= limit )
          goto Exit;
      }

      /* Get the nibble. */
      nib   = ( p[0] >> phase ) & 0xF;
      phase = 4 - phase;

      if ( nib == 0xE )
        sign = 1;
      else if ( nib > 9 )
        break;
      else
      {
        /* Increase exponent if we can't add the digit. */
        if ( number >= 0xCCCCCCCL )
          exponent_add++;
        /* Skip leading zeros. */
        else if ( nib || number )
        {
          integer_length++;
          number = number * 10 + nib;
        }
      }
    }

    /* Read fraction part, if any. */
    if ( nib == 0xa )
      for (;;)
      {
        /* If we entered this iteration with phase == 4, we need */
        /* to read a new byte.                                   */
        if ( phase )
        {
          p++;

          /* Make sure we don't read past the end. */
          if ( p >= limit )
            goto Exit;
        }

        /* Get the nibble. */
        nib   = ( p[0] >> phase ) & 0xF;
        phase = 4 - phase;
        if ( nib >= 10 )
          break;

        /* Skip leading zeros if possible. */
        if ( !nib && !number )
          exponent_add--;
        /* Only add digit if we don't overflow. */
        else if ( number < 0xCCCCCCCL && fraction_length < 9 )
        {
          fraction_length++;
          number = number * 10 + nib;
        }
      }

    /* Read exponent, if any. */
    if ( nib == 12 )
    {
      exponent_sign = 1;
      nib           = 11;
    }

    if ( nib == 11 )
    {
      for (;;)
      {
        /* If we entered this iteration with phase == 4, */
        /* we need to read a new byte.                   */
        if ( phase )
        {
          p++;

          /* Make sure we don't read past the end. */
          if ( p >= limit )
            goto Exit;
        }

        /* Get the nibble. */
        nib   = ( p[0] >> phase ) & 0xF;
        phase = 4 - phase;
        if ( nib >= 10 )
          break;

        exponent = exponent * 10 + nib;

        /* Arbitrarily limit exponent. */
        if ( exponent > 1000 )
          goto Exit;
      }

      if ( exponent_sign )
        exponent = -exponent;
    }

    /* We don't check `power_ten' and `exponent_add'. */
    exponent += power_ten + exponent_add;

    if ( scaling )
    {
      /* Only use `fraction_length'. */
      fraction_length += integer_length;
      exponent        += integer_length;

      if ( fraction_length <= 5 )
      {
        if ( number > 0x7FFFL )
        {
          result   = FT_DivFix( number, 10 );
          *scaling = exponent - fraction_length + 1;
        }
        else
        {
          if ( exponent > 0 )
          {
            FT_Long  new_fraction_length, shift;


            /* Make `scaling' as small as possible. */
            new_fraction_length = FT_MIN( exponent, 5 );
            exponent           -= new_fraction_length;
            shift               = new_fraction_length - fraction_length;

            number *= power_tens[shift];
            if ( number > 0x7FFFL )
            {
              number   /= 10;
              exponent += 1;
            }
          }
          else
            exponent -= fraction_length;

          result   = number << 16;
          *scaling = exponent;
        }
      }
      else
      {
        if ( ( number / power_tens[fraction_length - 5] ) > 0x7FFFL )
        {
          result   = FT_DivFix( number, power_tens[fraction_length - 4] );
          *scaling = exponent - 4;
        }
        else
        {
          result   = FT_DivFix( number, power_tens[fraction_length - 5] );
          *scaling = exponent - 5;
        }
      }
    }
    else
    {
      integer_length  += exponent;
      fraction_length -= exponent;

      /* Check for overflow and underflow. */
      if ( FT_ABS( integer_length ) > 5 )
        goto Exit;

      /* Remove non-significant digits. */
      if ( integer_length < 0 ) {
        number          /= power_tens[-integer_length];
        fraction_length += integer_length;
      }

      /* Convert into 16.16 format. */
      if ( fraction_length > 0 )
      {
        if ( ( number / power_tens[fraction_length] ) > 0x7FFFL )
          goto Exit;

        result = FT_DivFix( number, power_tens[fraction_length] );
      }
      else
      {
        number *= power_tens[-fraction_length];

        if ( number > 0x7FFFL )
          goto Exit;

        result = number << 16;
      }
    }

    if ( sign )
      result = -result;

  Exit:
    return result;
  }


  /* read a number, either integer or real */
  static FT_Long
  cff_parse_num( FT_Byte**  d )
  {
    return **d == 30 ? ( cff_parse_real( d[0], d[1], 0, NULL ) >> 16 )
                     :   cff_parse_integer( d[0], d[1] );
  }


  /* read a floating point number, either integer or real */
  static FT_Fixed
  cff_parse_fixed( FT_Byte**  d )
  {
    return **d == 30 ? cff_parse_real( d[0], d[1], 0, NULL )
                     : cff_parse_integer( d[0], d[1] ) << 16;
  }


  /* read a floating point number, either integer or real, */
  /* but return `10^scaling' times the number read in      */
  static FT_Fixed
  cff_parse_fixed_scaled( FT_Byte**  d,
                          FT_Long    scaling )
  {
    return **d == 30 ? cff_parse_real( d[0], d[1], scaling, NULL )
                     : ( cff_parse_integer( d[0], d[1] ) *
                           power_tens[scaling] ) << 16;
  }


  /* read a floating point number, either integer or real,     */
  /* and return it as precise as possible -- `scaling' returns */
  /* the scaling factor (as a power of 10)                     */
  static FT_Fixed
  cff_parse_fixed_dynamic( FT_Byte**  d,
                           FT_Long*   scaling )
  {
    FT_ASSERT( scaling );

    if ( **d == 30 )
      return cff_parse_real( d[0], d[1], 0, scaling );
    else
    {
      FT_Long  number;
      FT_Int   integer_length;


      number = cff_parse_integer( d[0], d[1] );

      if ( number > 0x7FFFL )
      {
        for ( integer_length = 5; integer_length < 10; integer_length++ )
          if ( number < power_tens[integer_length] )
            break;

        if ( ( number / power_tens[integer_length - 5] ) > 0x7FFFL )
        {
          *scaling = integer_length - 4;
          return FT_DivFix( number, power_tens[integer_length - 4] );
        }
        else
        {
          *scaling = integer_length - 5;
          return FT_DivFix( number, power_tens[integer_length - 5] );
        }
      }
      else
      {
        *scaling = 0;
        return number << 16;
      }
    }
  }


  static FT_Error
  cff_parse_font_matrix( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict   = (CFF_FontRecDict)parser->object;
    FT_Matrix*       matrix = &dict->font_matrix;
    FT_Vector*       offset = &dict->font_offset;
    FT_ULong*        upm    = &dict->units_per_em;
    FT_Byte**        data   = parser->stack;
    FT_Error         error  = CFF_Err_Stack_Underflow;


    if ( parser->top >= parser->stack + 6 )
    {
      FT_Long  scaling;


      error = CFF_Err_Ok;

      /* We expect a well-formed font matrix, this is, the matrix elements */
      /* `xx' and `yy' are of approximately the same magnitude.  To avoid  */
      /* loss of precision, we use the magnitude of element `xx' to scale  */
      /* all other elements.  The scaling factor is then contained in the  */
      /* `units_per_em' value.                                             */

      matrix->xx = cff_parse_fixed_dynamic( data++, &scaling );

      scaling = -scaling;

      if ( scaling < 0 || scaling > 9 )
      {
        /* Return default matrix in case of unlikely values. */
        matrix->xx = 0x10000L;
        matrix->yx = 0;
        matrix->yx = 0;
        matrix->yy = 0x10000L;
        offset->x  = 0;
        offset->y  = 0;
        *upm       = 1;

        goto Exit;
      }

      matrix->yx = cff_parse_fixed_scaled( data++, scaling );
      matrix->xy = cff_parse_fixed_scaled( data++, scaling );
      matrix->yy = cff_parse_fixed_scaled( data++, scaling );
      offset->x  = cff_parse_fixed_scaled( data++, scaling );
      offset->y  = cff_parse_fixed_scaled( data,   scaling );

      *upm = power_tens[scaling];
    }

  Exit:
    return error;
  }


  static FT_Error
  cff_parse_font_bbox( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_BBox*         bbox = &dict->font_bbox;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 4 )
    {
      bbox->xMin = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->yMin = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->xMax = FT_RoundFix( cff_parse_fixed( data++ ) );
      bbox->yMax = FT_RoundFix( cff_parse_fixed( data   ) );
      error = CFF_Err_Ok;
    }

    return error;
  }


  static FT_Error
  cff_parse_private_dict( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 2 )
    {
      dict->private_size   = cff_parse_num( data++ );
      dict->private_offset = cff_parse_num( data   );
      error = CFF_Err_Ok;
    }

    return error;
  }


  static FT_Error
  cff_parse_cid_ros( CFF_Parser  parser )
  {
    CFF_FontRecDict  dict = (CFF_FontRecDict)parser->object;
    FT_Byte**        data = parser->stack;
    FT_Error         error;


    error = CFF_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 3 )
    {
      dict->cid_registry   = (FT_UInt)cff_parse_num ( data++ );
      dict->cid_ordering   = (FT_UInt)cff_parse_num ( data++ );
      if ( **data == 30 )
        FT_TRACE1(( "cff_parse_cid_ros: real supplement is rounded\n" ));
      dict->cid_supplement = cff_parse_num( data );
      if ( dict->cid_supplement < 0 )
        FT_TRACE1(( "cff_parse_cid_ros: negative supplement %d is found\n",
                   dict->cid_supplement ));
      error = CFF_Err_Ok;
    }

    return error;
  }


#define CFF_FIELD_NUM( code, name ) \
          CFF_FIELD( code, name, cff_kind_num )
#define CFF_FIELD_FIXED( code, name ) \
          CFF_FIELD( code, name, cff_kind_fixed )
#define CFF_FIELD_FIXED_1000( code, name ) \
          CFF_FIELD( code, name, cff_kind_fixed_thousand )
#define CFF_FIELD_STRING( code, name ) \
          CFF_FIELD( code, name, cff_kind_string )
#define CFF_FIELD_BOOL( code, name ) \
          CFF_FIELD( code, name, cff_kind_bool )
#define CFF_FIELD_DELTA( code, name, max ) \
          CFF_FIELD( code, name, cff_kind_delta )

#define CFFCODE_TOPDICT  0x1000
#define CFFCODE_PRIVATE  0x2000

#ifndef FT_CONFIG_OPTION_PIC

#define CFF_FIELD_CALLBACK( code, name ) \
          {                              \
            cff_kind_callback,           \
            code | CFFCODE,              \
            0, 0,                        \
            cff_parse_ ## name,          \
            0, 0                         \
          },

#undef  CFF_FIELD
#define CFF_FIELD( code, name, kind ) \
          {                          \
            kind,                    \
            code | CFFCODE,          \
            FT_FIELD_OFFSET( name ), \
            FT_FIELD_SIZE( name ),   \
            0, 0, 0                  \
          },

#undef  CFF_FIELD_DELTA
#define CFF_FIELD_DELTA( code, name, max ) \
        {                                  \
          cff_kind_delta,                  \
          code | CFFCODE,                  \
          FT_FIELD_OFFSET( name ),         \
          FT_FIELD_SIZE_DELTA( name ),     \
          0,                               \
          max,                             \
          FT_FIELD_OFFSET( num_ ## name )  \
        },

  static const CFF_Field_Handler  cff_field_handlers[] =
  {

#include "cfftoken.h"

    { 0, 0, 0, 0, 0, 0, 0 }
  };


#else /* FT_CONFIG_OPTION_PIC */

  void FT_Destroy_Class_cff_field_handlers(FT_Library library, CFF_Field_Handler* clazz)
  {
    FT_Memory memory = library->memory;
    if ( clazz )
      FT_FREE( clazz );
  }

  FT_Error FT_Create_Class_cff_field_handlers(FT_Library library, CFF_Field_Handler** output_class)
  {
    CFF_Field_Handler*  clazz;
    FT_Error          error;
    FT_Memory memory = library->memory;
    int i=0;

#undef CFF_FIELD
#undef CFF_FIELD_DELTA
#undef CFF_FIELD_CALLBACK
#define CFF_FIELD_CALLBACK( code, name ) i++;
#define CFF_FIELD( code, name, kind ) i++;
#define CFF_FIELD_DELTA( code, name, max ) i++;

#include "cfftoken.h"
    i++;/*{ 0, 0, 0, 0, 0, 0, 0 }*/

    if ( FT_ALLOC( clazz, sizeof(CFF_Field_Handler)*i ) )
      return error;

    i=0;
#undef CFF_FIELD
#undef CFF_FIELD_DELTA
#undef CFF_FIELD_CALLBACK

#define CFF_FIELD_CALLBACK( code_, name_ )                                   \
    clazz[i].kind = cff_kind_callback;                                       \
    clazz[i].code = code_ | CFFCODE;                                         \
    clazz[i].offset = 0;                                                     \
    clazz[i].size = 0;                                                       \
    clazz[i].reader = cff_parse_ ## name_;                                   \
    clazz[i].array_max = 0;                                                  \
    clazz[i].count_offset = 0;                                               \
    i++;

#undef  CFF_FIELD
#define CFF_FIELD( code_, name_, kind_ )                                     \
    clazz[i].kind = kind_;                                                   \
    clazz[i].code = code_ | CFFCODE;                                         \
    clazz[i].offset = FT_FIELD_OFFSET( name_ );                              \
    clazz[i].size = FT_FIELD_SIZE( name_ );                                  \
    clazz[i].reader = 0;                                                     \
    clazz[i].array_max = 0;                                                  \
    clazz[i].count_offset = 0;                                               \
    i++;                                                                     \

#undef  CFF_FIELD_DELTA
#define CFF_FIELD_DELTA( code_, name_, max_ )                                \
    clazz[i].kind = cff_kind_delta;                                          \
    clazz[i].code = code_ | CFFCODE;                                         \
    clazz[i].offset = FT_FIELD_OFFSET( name_ );                              \
    clazz[i].size = FT_FIELD_SIZE_DELTA( name_ );                            \
    clazz[i].reader = 0;                                                     \
    clazz[i].array_max = max_;                                               \
    clazz[i].count_offset = FT_FIELD_OFFSET( num_ ## name_ );                \
    i++;

#include "cfftoken.h"

    clazz[i].kind = 0;
    clazz[i].code = 0;
    clazz[i].offset = 0;
    clazz[i].size = 0;
    clazz[i].reader = 0;
    clazz[i].array_max = 0;
    clazz[i].count_offset = 0;

    *output_class = clazz;
    return FT_Err_Ok;
  }


#endif /* FT_CONFIG_OPTION_PIC */


  FT_LOCAL_DEF( FT_Error )
  cff_parser_run( CFF_Parser  parser,
                  FT_Byte*    start,
                  FT_Byte*    limit )
  {
    FT_Byte*    p       = start;
    FT_Error    error   = CFF_Err_Ok;
    FT_Library  library = parser->library;
    FT_UNUSED(library);


    parser->top    = parser->stack;
    parser->start  = start;
    parser->limit  = limit;
    parser->cursor = start;

    while ( p < limit )
    {
      FT_UInt  v = *p;


      if ( v >= 27 && v != 31 )
      {
        /* it's a number; we will push its position on the stack */
        if ( parser->top - parser->stack >= CFF_MAX_STACK_DEPTH )
          goto Stack_Overflow;

        *parser->top ++ = p;

        /* now, skip it */
        if ( v == 30 )
        {
          /* skip real number */
          p++;
          for (;;)
          {
            /* An unterminated floating point number at the */
            /* end of a dictionary is invalid but harmless. */
            if ( p >= limit )
              goto Exit;
            v = p[0] >> 4;
            if ( v == 15 )
              break;
            v = p[0] & 0xF;
            if ( v == 15 )
              break;
            p++;
          }
        }
        else if ( v == 28 )
          p += 2;
        else if ( v == 29 )
          p += 4;
        else if ( v > 246 )
          p += 1;
      }
      else
      {
        /* This is not a number, hence it's an operator.  Compute its code */
        /* and look for it in our current list.                            */

        FT_UInt                   code;
        FT_UInt                   num_args = (FT_UInt)
                                             ( parser->top - parser->stack );
        const CFF_Field_Handler*  field;


        *parser->top = p;
        code = v;
        if ( v == 12 )
        {
          /* two byte operator */
          p++;
          if ( p >= limit )
            goto Syntax_Error;

          code = 0x100 | p[0];
        }
        code = code | parser->object_code;

        for ( field = FT_CFF_FIELD_HANDLERS_GET; field->kind; field++ )
        {
          if ( field->code == (FT_Int)code )
          {
            /* we found our field's handler; read it */
            FT_Long   val;
            FT_Byte*  q = (FT_Byte*)parser->object + field->offset;


            /* check that we have enough arguments -- except for */
            /* delta encoded arrays, which can be empty          */
            if ( field->kind != cff_kind_delta && num_args < 1 )
              goto Stack_Underflow;

            switch ( field->kind )
            {
            case cff_kind_bool:
            case cff_kind_string:
            case cff_kind_num:
              val = cff_parse_num( parser->stack );
              goto Store_Number;

            case cff_kind_fixed:
              val = cff_parse_fixed( parser->stack );
              goto Store_Number;

            case cff_kind_fixed_thousand:
              val = cff_parse_fixed_scaled( parser->stack, 3 );

            Store_Number:
              switch ( field->size )
              {
              case (8 / FT_CHAR_BIT):
                *(FT_Byte*)q = (FT_Byte)val;
                break;

              case (16 / FT_CHAR_BIT):
                *(FT_Short*)q = (FT_Short)val;
                break;

              case (32 / FT_CHAR_BIT):
                *(FT_Int32*)q = (FT_Int)val;
                break;

              default:  /* for 64-bit systems */
                *(FT_Long*)q = val;
              }
              break;

            case cff_kind_delta:
              {
                FT_Byte*   qcount = (FT_Byte*)parser->object +
                                      field->count_offset;

                FT_Byte**  data = parser->stack;


                if ( num_args > field->array_max )
                  num_args = field->array_max;

                /* store count */
                *qcount = (FT_Byte)num_args;

                val = 0;
                while ( num_args > 0 )
                {
                  val += cff_parse_num( data++ );
                  switch ( field->size )
                  {
                  case (8 / FT_CHAR_BIT):
                    *(FT_Byte*)q = (FT_Byte)val;
                    break;

                  case (16 / FT_CHAR_BIT):
                    *(FT_Short*)q = (FT_Short)val;
                    break;

                  case (32 / FT_CHAR_BIT):
                    *(FT_Int32*)q = (FT_Int)val;
                    break;

                  default:  /* for 64-bit systems */
                    *(FT_Long*)q = val;
                  }

                  q += field->size;
                  num_args--;
                }
              }
              break;

            default:  /* callback */
              error = field->reader( parser );
              if ( error )
                goto Exit;
            }
            goto Found;
          }
        }

        /* this is an unknown operator, or it is unsupported; */
        /* we will ignore it for now.                         */

      Found:
        /* clear stack */
        parser->top = parser->stack;
      }
      p++;
    }

  Exit:
    return error;

  Stack_Overflow:
    error = CFF_Err_Invalid_Argument;
    goto Exit;

  Stack_Underflow:
    error = CFF_Err_Invalid_Argument;
    goto Exit;

  Syntax_Error:
    error = CFF_Err_Invalid_Argument;
    goto Exit;
  }


/* END */
