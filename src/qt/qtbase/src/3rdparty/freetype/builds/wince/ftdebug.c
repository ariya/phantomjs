/***************************************************************************/
/*                                                                         */
/*  ftdebug.c                                                              */
/*                                                                         */
/*    Debugging and logging component for WinCE (body).                    */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2005, 2008, 2009 by                         */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This component contains various macros and functions used to ease the */
  /* debugging of the FreeType engine.  Its main purpose is in assertion   */
  /* checking, tracing, and error detection.                               */
  /*                                                                       */
  /* There are now three debugging modes:                                  */
  /*                                                                       */
  /* - trace mode                                                          */
  /*                                                                       */
  /*   Error and trace messages are sent to the log file (which can be the */
  /*   standard error output).                                             */
  /*                                                                       */
  /* - error mode                                                          */
  /*                                                                       */
  /*   Only error messages are generated.                                  */
  /*                                                                       */
  /* - release mode:                                                       */
  /*                                                                       */
  /*   No error message is sent or generated.  The code is free from any   */
  /*   debugging parts.                                                    */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H


#ifdef FT_DEBUG_LEVEL_ERROR


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>


  void
  OutputDebugStringEx( const char*  str )
  {
    static WCHAR  buf[8192];


    int sz = MultiByteToWideChar( CP_ACP, 0, str, -1, buf,
                                  sizeof ( buf ) / sizeof ( *buf ) );
    if ( !sz )
      lstrcpyW( buf, L"OutputDebugStringEx: MultiByteToWideChar failed" );

    OutputDebugStringW( buf );
  }


  FT_BASE_DEF( void )
  FT_Message( const char*  fmt, ... )
  {
    static char  buf[8192];
    va_list      ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    /* send the string to the debugger as well */
    vsprintf( buf, fmt, ap );
    OutputDebugStringEx( buf );
    va_end( ap );
  }


  FT_BASE_DEF( void )
  FT_Panic( const char*  fmt, ... )
  {
    static char  buf[8192];
    va_list      ap;


    va_start( ap, fmt );
    vsprintf( buf, fmt, ap );
    OutputDebugStringEx( buf );
    va_end( ap );

    exit( EXIT_FAILURE );
  }


#ifdef FT_DEBUG_LEVEL_TRACE


  /* array of trace levels, initialized to 0 */
  int  ft_trace_levels[trace_count];

  /* define array of trace toggle names */
#define FT_TRACE_DEF( x )  #x ,

  static const char*  ft_trace_toggles[trace_count + 1] =
  {
#include FT_INTERNAL_TRACE_H
    NULL
  };

#undef FT_TRACE_DEF


  /*************************************************************************/
  /*                                                                       */
  /* Initialize the tracing sub-system.  This is done by retrieving the    */
  /* value of the "FT2_DEBUG" environment variable.  It must be a list of  */
  /* toggles, separated by spaces, `;' or `,'.  Example:                   */
  /*                                                                       */
  /*    "any:3 memory:6 stream:5"                                          */
  /*                                                                       */
  /* This will request that all levels be set to 3, except the trace level */
  /* for the memory and stream components which are set to 6 and 5,        */
  /* respectively.                                                         */
  /*                                                                       */
  /* See the file <freetype/internal/fttrace.h> for details of the         */
  /* available toggle names.                                               */
  /*                                                                       */
  /* The level must be between 0 and 6; 0 means quiet (except for serious  */
  /* runtime errors), and 6 means _very_ verbose.                          */
  /*                                                                       */
  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
    /* Windows Mobile doesn't have environment API:           */
    /* GetEnvironmentStrings, GetEnvironmentVariable, getenv. */
    /*                                                        */
    /* FIXME!!! How to set debug mode?                        */

    /* const char*  ft2_debug = getenv( "FT2_DEBUG" ); */

    const char*  ft2_debug = 0;


    if ( ft2_debug )
    {
      const char*  p = ft2_debug;
      const char*  q;


      for ( ; *p; p++ )
      {
        /* skip leading whitespace and separators */
        if ( *p == ' ' || *p == '\t' || *p == ',' || *p == ';' || *p == '=' )
          continue;

        /* read toggle name, followed by ':' */
        q = p;
        while ( *p && *p != ':' )
          p++;

        if ( *p == ':' && p > q )
        {
          int  n, i, len = p - q;
          int  level = -1, found = -1;


          for ( n = 0; n < trace_count; n++ )
          {
            const char*  toggle = ft_trace_toggles[n];


            for ( i = 0; i < len; i++ )
            {
              if ( toggle[i] != q[i] )
                break;
            }

            if ( i == len && toggle[i] == 0 )
            {
              found = n;
              break;
            }
          }

          /* read level */
          p++;
          if ( *p )
          {
            level = *p++ - '0';
            if ( level < 0 || level > 7 )
              level = -1;
          }

          if ( found >= 0 && level >= 0 )
          {
            if ( found == trace_any )
            {
              /* special case for "any" */
              for ( n = 0; n < trace_count; n++ )
                ft_trace_levels[n] = level;
            }
            else
              ft_trace_levels[found] = level;
          }
        }
      }
    }
  }


#else  /* !FT_DEBUG_LEVEL_TRACE */


  FT_BASE_DEF( void )
  ft_debug_init( void )
  {
    /* nothing */
  }


#endif /* !FT_DEBUG_LEVEL_TRACE */

#endif /* FT_DEBUG_LEVEL_ERROR */


/* END */
