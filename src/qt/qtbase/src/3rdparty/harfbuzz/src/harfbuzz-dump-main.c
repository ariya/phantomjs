/*
 * Copyright (C) 2000  Red Hat, Inc.
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Owen Taylor
 */

#include <stdio.h>
#include <stdlib.h>

#include "harfbuzz.h"
#include "harfbuzz-dump.h"

#define N_ELEMENTS(arr) (sizeof(arr)/ sizeof((arr)[0]))

static int
croak (const char *situation, HB_Error error)
{
  fprintf (stderr, "%s: Error %d\n", situation, error);

  exit (1);
}

int 
main (int argc, char **argv)
{
  HB_Error error;
  FT_Library library;
  HB_Font font;
  HB_GSUB gsub;
  HB_GPOS gpos;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: harfbuzz-dump MYFONT.TTF\n");
      exit(1);
    }

  if ((error = FT_Init_FreeType (&library)))
    croak ("FT_Init_FreeType", error);

  if ((error = FT_New_Face (library, argv[1], 0, &font)))
    croak ("FT_New_Face", error);

  printf ("<?xml version=\"1.0\"?>\n");
  printf ("<OpenType>\n");

  if (!(error = HB_Load_GSUB_Table (font, &gsub, NULL)))
    {
      HB_Dump_GSUB_Table (gsub, stdout);
      
      if ((error = HB_Done_GSUB_Table (gsub)))
	croak ("HB_Done_GSUB_Table", error);
    }
  else if (error != HB_Err_Not_Covered)
    fprintf (stderr, "HB_Load_GSUB_Table: error 0x%x\n", error);

  if (!(error = HB_Load_GPOS_Table (font, &gpos, NULL)))
    {
      HB_Dump_GPOS_Table (gpos, stdout);
      
      if ((error = HB_Done_GPOS_Table (gpos)))
	croak ("HB_Done_GPOS_Table", error);
    }
  else if (error != HB_Err_Not_Covered)
    fprintf (stderr, "HB_Load_GPOS_Table: error 0x%x\n", error);

  printf ("</OpenType>\n");

  if ((error = FT_Done_Face (font)))
    croak ("FT_Done_Face", error);

  if ((error = FT_Done_FreeType (library)))
    croak ("FT_Done_FreeType", error);
  
  return 0;
}

