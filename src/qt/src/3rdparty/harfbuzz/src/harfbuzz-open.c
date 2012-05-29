/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2006  Behdad Esfahbod
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
 */

#include "harfbuzz-impl.h"
#include "harfbuzz-open-private.h"


/***************************
 * Script related functions
 ***************************/


/* LangSys */

static HB_Error  Load_LangSys( HB_LangSys*  ls,
			       HB_Stream     stream )
{
  HB_Error   error;
  HB_UShort  n, count;
  HB_UShort* fi;


  if ( ACCESS_Frame( 6L ) )
    return error;

  ls->LookupOrderOffset    = GET_UShort();    /* should be 0 */
  ls->ReqFeatureIndex      = GET_UShort();
  count = ls->FeatureCount = GET_UShort();

  FORGET_Frame();

  ls->FeatureIndex = NULL;

  if ( ALLOC_ARRAY( ls->FeatureIndex, count, HB_UShort ) )
    return error;

  if ( ACCESS_Frame( count * 2L ) )
  {
    FREE( ls->FeatureIndex );
    return error;
  }

  fi = ls->FeatureIndex;

  for ( n = 0; n < count; n++ )
    fi[n] = GET_UShort();

  FORGET_Frame();

  return HB_Err_Ok;
}


static void  Free_LangSys( HB_LangSys*  ls )
{
  FREE( ls->FeatureIndex );
}


/* Script */

static HB_Error  Load_Script( HB_ScriptTable*  s,
			      HB_Stream    stream )
{
  HB_Error   error;
  HB_UShort  n, m, count;
  HB_UInt   cur_offset, new_offset, base_offset;

  HB_LangSysRecord*  lsr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  if ( new_offset != base_offset )        /* not a NULL offset */
  {
    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_LangSys( &s->DefaultLangSys,
				 stream ) ) != HB_Err_Ok )
      return error;
    (void)FILE_Seek( cur_offset );
  }
  else
  {
    /* we create a DefaultLangSys table with no entries */

    s->DefaultLangSys.LookupOrderOffset = 0;
    s->DefaultLangSys.ReqFeatureIndex   = 0xFFFF;
    s->DefaultLangSys.FeatureCount      = 0;
    s->DefaultLangSys.FeatureIndex      = NULL;
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = s->LangSysCount = GET_UShort();

  /* safety check; otherwise the official handling of TrueType Open
     fonts won't work */

  if ( s->LangSysCount == 0 && s->DefaultLangSys.FeatureCount == 0 )
  {
    error = HB_Err_Not_Covered;
    goto Fail2;
  }

  FORGET_Frame();

  s->LangSysRecord = NULL;

  if ( ALLOC_ARRAY( s->LangSysRecord, count, HB_LangSysRecord ) )
    goto Fail2;

  lsr = s->LangSysRecord;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 6L ) )
      goto Fail1;

    lsr[n].LangSysTag = GET_ULong();
    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_LangSys( &lsr[n].LangSys, stream ) ) != HB_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return HB_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_LangSys( &lsr[m].LangSys );

  FREE( s->LangSysRecord );

Fail2:
  Free_LangSys( &s->DefaultLangSys );
  return error;
}


static void  Free_Script( HB_ScriptTable*  s )
{
  HB_UShort           n, count;

  HB_LangSysRecord*  lsr;


  Free_LangSys( &s->DefaultLangSys );

  if ( s->LangSysRecord )
  {
    count = s->LangSysCount;
    lsr   = s->LangSysRecord;

    for ( n = 0; n < count; n++ )
      Free_LangSys( &lsr[n].LangSys );

    FREE( lsr );
  }
}


/* ScriptList */

HB_INTERNAL HB_Error
_HB_OPEN_Load_ScriptList( HB_ScriptList* sl,
			   HB_Stream        stream )
{
  HB_Error   error;

  HB_UShort          n, script_count;
  HB_UInt           cur_offset, new_offset, base_offset;

  HB_ScriptRecord*  sr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  script_count = GET_UShort();

  FORGET_Frame();

  sl->ScriptRecord = NULL;

  if ( ALLOC_ARRAY( sl->ScriptRecord, script_count, HB_ScriptRecord ) )
    return error;

  sr = sl->ScriptRecord;

  sl->ScriptCount= 0;
  for ( n = 0; n < script_count; n++ )
  {
    if ( ACCESS_Frame( 6L ) )
      goto Fail;

    sr[sl->ScriptCount].ScriptTag = GET_ULong();
    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();

    if ( FILE_Seek( new_offset ) )
      goto Fail;

    error = Load_Script( &sr[sl->ScriptCount].Script, stream );
    if ( error == HB_Err_Ok )
      sl->ScriptCount += 1;
    else if ( error != HB_Err_Not_Covered )
      goto Fail;

    (void)FILE_Seek( cur_offset );
  }

  /* Empty tables are harmless and generated by fontforge.
   * See http://bugzilla.gnome.org/show_bug.cgi?id=347073
   */
#if 0
  if ( sl->ScriptCount == 0 )
  {
    error = ERR(HB_Err_Invalid_SubTable);
    goto Fail;
  }
#endif
  
  return HB_Err_Ok;

Fail:
  for ( n = 0; n < sl->ScriptCount; n++ )
    Free_Script( &sr[n].Script );

  FREE( sl->ScriptRecord );
  return error;
}


HB_INTERNAL void
_HB_OPEN_Free_ScriptList( HB_ScriptList* sl )
{
  HB_UShort          n, count;

  HB_ScriptRecord*  sr;


  if ( sl->ScriptRecord )
  {
    count = sl->ScriptCount;
    sr    = sl->ScriptRecord;

    for ( n = 0; n < count; n++ )
      Free_Script( &sr[n].Script );

    FREE( sr );
  }
}



/*********************************
 * Feature List related functions
 *********************************/


/* Feature */

static HB_Error  Load_Feature( HB_Feature*  f,
			       HB_Stream     stream )
{
  HB_Error   error;

  HB_UShort   n, count;

  HB_UShort*  lli;


  if ( ACCESS_Frame( 4L ) )
    return error;

  f->FeatureParams           = GET_UShort();    /* should be 0 */
  count = f->LookupListCount = GET_UShort();

  FORGET_Frame();

  f->LookupListIndex = NULL;

  if ( ALLOC_ARRAY( f->LookupListIndex, count, HB_UShort ) )
    return error;

  lli = f->LookupListIndex;

  if ( ACCESS_Frame( count * 2L ) )
  {
    FREE( f->LookupListIndex );
    return error;
  }

  for ( n = 0; n < count; n++ )
    lli[n] = GET_UShort();

  FORGET_Frame();

  return HB_Err_Ok;
}


static void  Free_Feature( HB_Feature*  f )
{
  FREE( f->LookupListIndex );
}


/* FeatureList */

HB_INTERNAL HB_Error
_HB_OPEN_Load_FeatureList( HB_FeatureList* fl,
			    HB_Stream         stream )
{
  HB_Error   error;

  HB_UShort           n, m, count;
  HB_UInt            cur_offset, new_offset, base_offset;

  HB_FeatureRecord*  fr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = fl->FeatureCount = GET_UShort();

  FORGET_Frame();

  fl->FeatureRecord = NULL;

  if ( ALLOC_ARRAY( fl->FeatureRecord, count, HB_FeatureRecord ) )
    return error;
  if ( ALLOC_ARRAY( fl->ApplyOrder, count, HB_UShort ) )
    goto Fail2;
  
  fl->ApplyCount = 0;

  fr = fl->FeatureRecord;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 6L ) )
      goto Fail1;

    fr[n].FeatureTag = GET_ULong();
    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_Feature( &fr[n].Feature, stream ) ) != HB_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return HB_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_Feature( &fr[m].Feature );

  FREE( fl->ApplyOrder );

Fail2:
  FREE( fl->FeatureRecord );

  return error;
}


HB_INTERNAL void
_HB_OPEN_Free_FeatureList( HB_FeatureList*  fl )
{
  HB_UShort           n, count;

  HB_FeatureRecord*  fr;


  if ( fl->FeatureRecord )
  {
    count = fl->FeatureCount;
    fr    = fl->FeatureRecord;

    for ( n = 0; n < count; n++ )
      Free_Feature( &fr[n].Feature );

    FREE( fr );
  }
  
  FREE( fl->ApplyOrder );
}



/********************************
 * Lookup List related functions
 ********************************/

/* the subroutines of the following two functions are defined in
   ftxgsub.c and ftxgpos.c respectively                          */


/* SubTable */

static HB_Error  Load_SubTable( HB_SubTable*  st,
				HB_Stream     stream,
				HB_Type       table_type,
				HB_UShort     lookup_type )
{
  if ( table_type == HB_Type_GSUB )
    return _HB_GSUB_Load_SubTable ( &st->st.gsub, stream, lookup_type );
  else
    return _HB_GPOS_Load_SubTable ( &st->st.gpos, stream, lookup_type );
}


static void  Free_SubTable( HB_SubTable*  st,
			    HB_Type       table_type,
			    HB_UShort      lookup_type )
{
  if ( table_type == HB_Type_GSUB )
    _HB_GSUB_Free_SubTable ( &st->st.gsub, lookup_type );
  else
    _HB_GPOS_Free_SubTable ( &st->st.gpos, lookup_type );
}


/* Lookup */

static HB_Error  Load_Lookup( HB_Lookup*   l,
			      HB_Stream     stream,
			      HB_Type      type )
{
  HB_Error   error;

  HB_UShort      n, m, count;
  HB_UInt       cur_offset, new_offset, base_offset;

  HB_SubTable*  st;

  HB_Bool        is_extension = FALSE;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 6L ) )
    return error;

  l->LookupType            = GET_UShort();
  l->LookupFlag            = GET_UShort();
  count = l->SubTableCount = GET_UShort();

  FORGET_Frame();

  l->SubTable = NULL;

  if ( ALLOC_ARRAY( l->SubTable, count, HB_SubTable ) )
    return error;

  st = l->SubTable;

  if ( ( type == HB_Type_GSUB && l->LookupType == HB_GSUB_LOOKUP_EXTENSION ) ||
       ( type == HB_Type_GPOS && l->LookupType == HB_GPOS_LOOKUP_EXTENSION ) )
    is_extension = TRUE;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();

    if ( is_extension )
    {
      if ( FILE_Seek( new_offset ) || ACCESS_Frame( 8L ) )
	goto Fail;

      if (GET_UShort() != 1) /* format should be 1 */
	goto Fail;

      l->LookupType = GET_UShort();
      new_offset += GET_ULong();

      FORGET_Frame();
    }

    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_SubTable( &st[n], stream,
				  type, l->LookupType ) ) != HB_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return HB_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_SubTable( &st[m], type, l->LookupType );

  FREE( l->SubTable );
  return error;
}


static void  Free_Lookup( HB_Lookup*   l,
			  HB_Type      type)
{
  HB_UShort      n, count;

  HB_SubTable*  st;


  if ( l->SubTable )
  {
    count = l->SubTableCount;
    st    = l->SubTable;

    for ( n = 0; n < count; n++ )
      Free_SubTable( &st[n], type, l->LookupType );

    FREE( st );
  }
}


/* LookupList */

HB_INTERNAL HB_Error
_HB_OPEN_Load_LookupList( HB_LookupList* ll,
			   HB_Stream        stream,
			   HB_Type         type )
{
  HB_Error   error;

  HB_UShort    n, m, count;
  HB_UInt     cur_offset, new_offset, base_offset;

  HB_Lookup*  l;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = ll->LookupCount = GET_UShort();

  FORGET_Frame();

  ll->Lookup = NULL;

  if ( ALLOC_ARRAY( ll->Lookup, count, HB_Lookup ) )
    return error;
  if ( ALLOC_ARRAY( ll->Properties, count, HB_UInt ) )
    goto Fail2;

  l = ll->Lookup;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_Lookup( &l[n], stream, type ) ) != HB_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return HB_Err_Ok;

Fail1:
  FREE( ll->Properties );

  for ( m = 0; m < n; m++ )
    Free_Lookup( &l[m], type );

Fail2:
  FREE( ll->Lookup );
  return error;
}


HB_INTERNAL void
_HB_OPEN_Free_LookupList( HB_LookupList* ll,
		       HB_Type         type )
{
  HB_UShort    n, count;

  HB_Lookup*  l;


  FREE( ll->Properties );

  if ( ll->Lookup )
  {
    count = ll->LookupCount;
    l     = ll->Lookup;

    for ( n = 0; n < count; n++ )
      Free_Lookup( &l[n], type );

    FREE( l );
  }
}



/*****************************
 * Coverage related functions
 *****************************/


/* CoverageFormat1 */

static HB_Error  Load_Coverage1( HB_CoverageFormat1*  cf1,
				 HB_Stream             stream )
{
  HB_Error   error;

  HB_UShort  n, count;

  HB_UShort* ga;


  if ( ACCESS_Frame( 2L ) )
    return error;

  count = cf1->GlyphCount = GET_UShort();

  FORGET_Frame();

  cf1->GlyphArray = NULL;

  if ( ALLOC_ARRAY( cf1->GlyphArray, count, HB_UShort ) )
    return error;

  ga = cf1->GlyphArray;

  if ( ACCESS_Frame( count * 2L ) )
  {
    FREE( cf1->GlyphArray );
    return error;
  }

  for ( n = 0; n < count; n++ )
    ga[n] = GET_UShort();

  FORGET_Frame();

  return HB_Err_Ok;
}


static void  Free_Coverage1( HB_CoverageFormat1*  cf1)
{
  FREE( cf1->GlyphArray );
}


/* CoverageFormat2 */

static HB_Error  Load_Coverage2( HB_CoverageFormat2*  cf2,
				 HB_Stream             stream )
{
  HB_Error   error;

  HB_UShort         n, count;

  HB_RangeRecord*  rr;


  if ( ACCESS_Frame( 2L ) )
    return error;

  count = cf2->RangeCount = GET_UShort();

  FORGET_Frame();

  cf2->RangeRecord = NULL;

  if ( ALLOC_ARRAY( cf2->RangeRecord, count, HB_RangeRecord ) )
    return error;

  rr = cf2->RangeRecord;

  if ( ACCESS_Frame( count * 6L ) )
    goto Fail;

  for ( n = 0; n < count; n++ )
  {
    rr[n].Start              = GET_UShort();
    rr[n].End                = GET_UShort();
    rr[n].StartCoverageIndex = GET_UShort();

    /* sanity check; we are limited to 16bit integers */
    if ( rr[n].Start > rr[n].End ||
	 ( rr[n].End - rr[n].Start + (long)rr[n].StartCoverageIndex ) >=
	   0x10000L )
    {
      error = ERR(HB_Err_Invalid_SubTable);
      goto Fail;
    }
  }

  FORGET_Frame();

  return HB_Err_Ok;

Fail:
  FREE( cf2->RangeRecord );
  return error;
}


static void  Free_Coverage2( HB_CoverageFormat2*  cf2 )
{
  FREE( cf2->RangeRecord );
}


HB_INTERNAL HB_Error
_HB_OPEN_Load_Coverage( HB_Coverage* c,
			 HB_Stream      stream )
{
  HB_Error   error;

  if ( ACCESS_Frame( 2L ) )
    return error;

  c->CoverageFormat = GET_UShort();

  FORGET_Frame();

  switch ( c->CoverageFormat )
  {
  case 1:  return Load_Coverage1( &c->cf.cf1, stream );
  case 2:  return Load_Coverage2( &c->cf.cf2, stream );
  default: return ERR(HB_Err_Invalid_SubTable_Format);
  }

  return HB_Err_Ok;               /* never reached */
}


HB_INTERNAL void
_HB_OPEN_Free_Coverage( HB_Coverage* c )
{
  switch ( c->CoverageFormat )
  {
  case 1:  Free_Coverage1( &c->cf.cf1 ); break;
  case 2:  Free_Coverage2( &c->cf.cf2 ); break;
  default:					 break;
  }
}


static HB_Error  Coverage_Index1( HB_CoverageFormat1*  cf1,
				  HB_UShort             glyphID,
				  HB_UShort*            index )
{
  HB_UShort min, max, new_min, new_max, middle;

  HB_UShort*  array = cf1->GlyphArray;


  /* binary search */

  if ( cf1->GlyphCount == 0 )
    return HB_Err_Not_Covered;

  new_min = 0;
  new_max = cf1->GlyphCount - 1;

  do
  {
    min = new_min;
    max = new_max;

    /* we use (min + max) / 2 = max - (max - min) / 2  to avoid
       overflow and rounding errors                             */

    middle = max - ( ( max - min ) >> 1 );

    if ( glyphID == array[middle] )
    {
      *index = middle;
      return HB_Err_Ok;
    }
    else if ( glyphID < array[middle] )
    {
      if ( middle == min )
	break;
      new_max = middle - 1;
    }
    else
    {
      if ( middle == max )
	break;
      new_min = middle + 1;
    }
  } while ( min < max );

  return HB_Err_Not_Covered;
}


static HB_Error  Coverage_Index2( HB_CoverageFormat2*  cf2,
				  HB_UShort             glyphID,
				  HB_UShort*            index )
{
  HB_UShort         min, max, new_min, new_max, middle;

  HB_RangeRecord*  rr = cf2->RangeRecord;


  /* binary search */

  if ( cf2->RangeCount == 0 )
    return HB_Err_Not_Covered;

  new_min = 0;
  new_max = cf2->RangeCount - 1;

  do
  {
    min = new_min;
    max = new_max;

    /* we use (min + max) / 2 = max - (max - min) / 2  to avoid
       overflow and rounding errors                             */

    middle = max - ( ( max - min ) >> 1 );

    if ( glyphID >= rr[middle].Start && glyphID <= rr[middle].End )
    {
      *index = rr[middle].StartCoverageIndex + glyphID - rr[middle].Start;
      return HB_Err_Ok;
    }
    else if ( glyphID < rr[middle].Start )
    {
      if ( middle == min )
	break;
      new_max = middle - 1;
    }
    else
    {
      if ( middle == max )
	break;
      new_min = middle + 1;
    }
  } while ( min < max );

  return HB_Err_Not_Covered;
}


HB_INTERNAL HB_Error
_HB_OPEN_Coverage_Index( HB_Coverage* c,
			  HB_UShort      glyphID,
			  HB_UShort*     index )
{
  switch ( c->CoverageFormat )
  {
  case 1:  return Coverage_Index1( &c->cf.cf1, glyphID, index );
  case 2:  return Coverage_Index2( &c->cf.cf2, glyphID, index );
  default: return ERR(HB_Err_Invalid_SubTable_Format);
  }

  return HB_Err_Ok;               /* never reached */
}



/*************************************
 * Class Definition related functions
 *************************************/


/* ClassDefFormat1 */

static HB_Error  Load_ClassDef1( HB_ClassDefinition*  cd,
				 HB_UShort             limit,
				 HB_Stream             stream )
{
  HB_Error   error;

  HB_UShort             n, count;

  HB_UShort*            cva;

  HB_ClassDefFormat1*  cdf1;


  cdf1 = &cd->cd.cd1;

  if ( ACCESS_Frame( 4L ) )
    return error;

  cdf1->StartGlyph         = GET_UShort();
  count = cdf1->GlyphCount = GET_UShort();

  FORGET_Frame();

  /* sanity check; we are limited to 16bit integers */

  if ( cdf1->StartGlyph + (long)count >= 0x10000L )
    return ERR(HB_Err_Invalid_SubTable);

  cdf1->ClassValueArray = NULL;

  if ( ALLOC_ARRAY( cdf1->ClassValueArray, count, HB_UShort ) )
    return error;

  cva = cdf1->ClassValueArray;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail;

  for ( n = 0; n < count; n++ )
  {
    cva[n] = GET_UShort();
    if ( cva[n] >= limit )
    {
      error = ERR(HB_Err_Invalid_SubTable);
      goto Fail;
    }
  }

  FORGET_Frame();

  return HB_Err_Ok;

Fail:
  FREE( cva );

  return error;
}


static void  Free_ClassDef1( HB_ClassDefFormat1*  cdf1 )
{
  FREE( cdf1->ClassValueArray );
}


/* ClassDefFormat2 */

static HB_Error  Load_ClassDef2( HB_ClassDefinition*  cd,
				 HB_UShort             limit,
				 HB_Stream             stream )
{
  HB_Error   error;

  HB_UShort              n, count;

  HB_ClassRangeRecord*  crr;

  HB_ClassDefFormat2*   cdf2;


  cdf2 = &cd->cd.cd2;

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = GET_UShort();
  cdf2->ClassRangeCount = 0; /* zero for now.  we fill with the number of good entries later */

  FORGET_Frame();

  cdf2->ClassRangeRecord = NULL;

  if ( ALLOC_ARRAY( cdf2->ClassRangeRecord, count, HB_ClassRangeRecord ) )
    return error;

  crr = cdf2->ClassRangeRecord;

  if ( ACCESS_Frame( count * 6L ) )
    goto Fail;

  for ( n = 0; n < count; n++ )
  {
    crr[n].Start = GET_UShort();
    crr[n].End   = GET_UShort();
    crr[n].Class = GET_UShort();

    /* sanity check */

    if ( crr[n].Start > crr[n].End ||
	 crr[n].Class >= limit )
    {
      /* XXX
       * Corrupt entry.  Skip it.
       * This is hit by Nafees Nastaliq font for example
       */
       n--;
       count--;
    }
  }

  FORGET_Frame();

  cdf2->ClassRangeCount = count;

  return HB_Err_Ok;

Fail:
  FREE( crr );

  return error;
}


static void  Free_ClassDef2( HB_ClassDefFormat2*  cdf2 )
{
  FREE( cdf2->ClassRangeRecord );
}


/* ClassDefinition */

HB_INTERNAL HB_Error
_HB_OPEN_Load_ClassDefinition( HB_ClassDefinition* cd,
				HB_UShort             limit,
				HB_Stream             stream )
{
  HB_Error   error;

  if ( ACCESS_Frame( 2L ) )
    return error;

  cd->ClassFormat = GET_UShort();

  FORGET_Frame();

  switch ( cd->ClassFormat )
  {
  case 1:  error = Load_ClassDef1( cd, limit, stream ); break;
  case 2:  error = Load_ClassDef2( cd, limit, stream ); break;
  default: error = ERR(HB_Err_Invalid_SubTable_Format);	break;
  }

  if ( error )
    return error;

  cd->loaded = TRUE;

  return HB_Err_Ok;
}


static HB_Error
_HB_OPEN_Load_EmptyClassDefinition( HB_ClassDefinition*  cd )
{
  HB_Error   error;

  cd->ClassFormat = 1; /* Meaningless */

  if ( ALLOC_ARRAY( cd->cd.cd1.ClassValueArray, 1, HB_UShort ) )
    return error;

  cd->loaded = TRUE;

  return HB_Err_Ok;
}

HB_INTERNAL HB_Error
_HB_OPEN_Load_EmptyOrClassDefinition( HB_ClassDefinition* cd,
					       HB_UShort             limit,
					       HB_UInt              class_offset,
					       HB_UInt              base_offset,
					       HB_Stream             stream )
{
  HB_Error error;
  HB_UInt               cur_offset;

  cur_offset = FILE_Pos();

  if ( class_offset )
    {
      if ( !FILE_Seek( class_offset + base_offset ) )
	error = _HB_OPEN_Load_ClassDefinition( cd, limit, stream );
    }
  else
     error = _HB_OPEN_Load_EmptyClassDefinition ( cd );

  if (error == HB_Err_Ok)
    (void)FILE_Seek( cur_offset ); /* Changes error as a side-effect */

  return error;
}

HB_INTERNAL void
_HB_OPEN_Free_ClassDefinition( HB_ClassDefinition*  cd )
{
  if ( !cd->loaded )
    return;

  switch ( cd->ClassFormat )
  {
  case 1:  Free_ClassDef1( &cd->cd.cd1 ); break;
  case 2:  Free_ClassDef2( &cd->cd.cd2 ); break;
  default:				  break;
  }
}


static HB_Error  Get_Class1( HB_ClassDefFormat1*  cdf1,
			     HB_UShort             glyphID,
			     HB_UShort*            klass,
			     HB_UShort*            index )
{
  HB_UShort*  cva = cdf1->ClassValueArray;


  if ( index )
    *index = 0;

  if ( glyphID >= cdf1->StartGlyph &&
       glyphID < cdf1->StartGlyph + cdf1->GlyphCount )
  {
    *klass = cva[glyphID - cdf1->StartGlyph];
    return HB_Err_Ok;
  }
  else
  {
    *klass = 0;
    return HB_Err_Not_Covered;
  }
}


/* we need the index value of the last searched class range record
   in case of failure for constructed GDEF tables                  */

static HB_Error  Get_Class2( HB_ClassDefFormat2*  cdf2,
			     HB_UShort             glyphID,
			     HB_UShort*            klass,
			     HB_UShort*            index )
{
  HB_Error               error = HB_Err_Ok;
  HB_UShort              min, max, new_min, new_max, middle;

  HB_ClassRangeRecord*  crr = cdf2->ClassRangeRecord;


  /* binary search */

  if ( cdf2->ClassRangeCount == 0 )
    {
      *klass = 0;
      if ( index )
	*index = 0;
      
      return HB_Err_Not_Covered;
    }

  new_min = 0;
  new_max = cdf2->ClassRangeCount - 1;

  do
  {
    min = new_min;
    max = new_max;

    /* we use (min + max) / 2 = max - (max - min) / 2  to avoid
       overflow and rounding errors                             */

    middle = max - ( ( max - min ) >> 1 );

    if ( glyphID >= crr[middle].Start && glyphID <= crr[middle].End )
    {
      *klass = crr[middle].Class;
      error  = HB_Err_Ok;
      break;
    }
    else if ( glyphID < crr[middle].Start )
    {
      if ( middle == min )
      {
	*klass = 0;
	error  = HB_Err_Not_Covered;
	break;
      }
      new_max = middle - 1;
    }
    else
    {
      if ( middle == max )
      {
	*klass = 0;
	error  = HB_Err_Not_Covered;
	break;
      }
      new_min = middle + 1;
    }
  } while ( min < max );

  if ( index )
    *index = middle;

  return error;
}


HB_INTERNAL HB_Error
_HB_OPEN_Get_Class( HB_ClassDefinition* cd,
		     HB_UShort             glyphID,
		    HB_UShort*          klass,
		     HB_UShort*            index )
{
  switch ( cd->ClassFormat )
  {
  case 1:  return Get_Class1( &cd->cd.cd1, glyphID, klass, index );
  case 2:  return Get_Class2( &cd->cd.cd2, glyphID, klass, index );
  default: return ERR(HB_Err_Invalid_SubTable_Format);
  }

  return HB_Err_Ok;               /* never reached */
}



/***************************
 * Device related functions
 ***************************/


HB_INTERNAL HB_Error
_HB_OPEN_Load_Device( HB_Device** device,
		       HB_Stream    stream )
{
  HB_Device*  d;
  HB_Error   error;

  HB_UShort   n, count;

  HB_UShort*  dv;


  if ( ACCESS_Frame( 6L ) )
    return error;

  if ( ALLOC( *device, sizeof(HB_Device)) )
  {
    *device = 0;
    return error;
  }

  d = *device;

  d->StartSize   = GET_UShort();
  d->EndSize     = GET_UShort();
  d->DeltaFormat = GET_UShort();

  FORGET_Frame();

  d->DeltaValue = NULL;

  if ( d->StartSize > d->EndSize ||
       d->DeltaFormat == 0 || d->DeltaFormat > 3 )
    {
      /* XXX
       * I've seen fontforge generate DeltaFormat == 0.
       * Just return Ok and let the NULL DeltaValue disable
       * this table.
       */
      return HB_Err_Ok;
    }

  count = ( ( d->EndSize - d->StartSize + 1 ) >>
	      ( 4 - d->DeltaFormat ) ) + 1;

  if ( ALLOC_ARRAY( d->DeltaValue, count, HB_UShort ) )
  {
    FREE( *device );
    *device = 0;
    return error;
  }

  if ( ACCESS_Frame( count * 2L ) )
  {
    FREE( d->DeltaValue );
    FREE( *device );
    *device = 0;
    return error;
  }

  dv = d->DeltaValue;

  for ( n = 0; n < count; n++ )
    dv[n] = GET_UShort();

  FORGET_Frame();

  return HB_Err_Ok;
}


HB_INTERNAL void
_HB_OPEN_Free_Device( HB_Device* d )
{
  if ( d )
  {
    FREE( d->DeltaValue );
    FREE( d );
  }
}


/* Since we have the delta values stored in compressed form, we must
   uncompress it now.  To simplify the interface, the function always
   returns a meaningful value in `value'; the error is just for
   information.
			       |                |
   format = 1: 0011223344556677|8899101112131415|...
			       |                |
		    byte 1           byte 2

     00: (byte >> 14) & mask
     11: (byte >> 12) & mask
     ...

     mask = 0x0003
			       |                |
   format = 2: 0000111122223333|4444555566667777|...
			       |                |
		    byte 1           byte 2

     0000: (byte >> 12) & mask
     1111: (byte >>  8) & mask
     ...

     mask = 0x000F
			       |                |
   format = 3: 0000000011111111|2222222233333333|...
			       |                |
		    byte 1           byte 2

     00000000: (byte >> 8) & mask
     11111111: (byte >> 0) & mask
     ....

     mask = 0x00FF                                    */

HB_INTERNAL HB_Error
_HB_OPEN_Get_Device( HB_Device* d,
		      HB_UShort    size,
		      HB_Short*    value )
{
  HB_UShort  byte, bits, mask, s;

  if ( d && d->DeltaValue && size >= d->StartSize && size <= d->EndSize )
  {
    HB_UShort f = d->DeltaFormat;
    s    = size - d->StartSize;
    byte = d->DeltaValue[s >> ( 4 - f )];
    bits = byte >> ( 16 - ( ( s % ( 1 << ( 4 - f ) ) + 1 ) << f ) );
    mask = 0xFFFF >> ( 16 - ( 1 << f ) );

    *value = (HB_Short)( bits & mask );

    /* conversion to a signed value */

    if ( *value >= ( ( mask + 1 ) >> 1 ) )
      *value -= mask + 1;

    return HB_Err_Ok;
  }
  else
  {
    *value = 0;
    return HB_Err_Not_Covered;
  }
}


/* END */
