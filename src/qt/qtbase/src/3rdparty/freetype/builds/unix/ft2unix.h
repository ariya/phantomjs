/***************************************************************************/
/*                                                                         */
/*  ft2build.h                                                             */
/*                                                                         */
/*    Build macros of the FreeType 2 library.                              */
/*                                                                         */
/*  Copyright 1996-2001, 2003, 2006 by                                     */
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
  /* This is a Unix-specific version of <ft2build.h> that should be used   */
  /* exclusively *after* installation of the library.                      */
  /*                                                                       */
  /* It assumes that `/usr/local/include/freetype2' (or whatever is        */
  /* returned by the `freetype-config --cflags' or `pkg-config --cflags'   */
  /* command) is in your compilation include path.                         */
  /*                                                                       */
  /* We don't need to do anything special in this release.  However, for   */
  /* a future FreeType 2 release, the following installation changes will  */
  /* be performed:                                                         */
  /*                                                                       */
  /*   - The contents of `freetype-2.x/include/freetype' will be installed */
  /*     to `/usr/local/include/freetype2' instead of                      */
  /*     `/usr/local/include/freetype2/freetype'.                          */
  /*                                                                       */
  /*   - This file will #include <freetype2/config/ftheader.h>, instead    */
  /*     of <freetype/config/ftheader.h>.                                  */
  /*                                                                       */
  /*   - The contents of `ftheader.h' will be processed with `sed' to      */
  /*     replace all `<freetype/xxx>' with `<freetype2/xxx>'.              */
  /*                                                                       */
  /*   - Adding `/usr/local/include/freetype2' to your compilation include */
  /*     path will not be necessary anymore.                               */
  /*                                                                       */
  /* These changes will be transparent to client applications which use    */
  /* freetype-config (or pkg-config).  No modifications will be necessary  */
  /* to compile with the new scheme.                                       */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FT2_BUILD_UNIX_H__
#define __FT2_BUILD_UNIX_H__

  /* `<prefix>/include/freetype2' must be in your current inclusion path */
#include <freetype/config/ftheader.h>

#endif /* __FT2_BUILD_UNIX_H__ */


/* END */
