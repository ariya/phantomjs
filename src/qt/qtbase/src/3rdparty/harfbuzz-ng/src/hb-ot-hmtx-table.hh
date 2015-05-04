/*
 * Copyright © 2011,2012  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_HMTX_TABLE_HH
#define HB_OT_HMTX_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {


/*
 * hmtx -- The Horizontal Metrics Table
 */

#define HB_OT_TAG_hmtx HB_TAG('h','m','t','x')


struct LongHorMetric
{
  USHORT	advanceWidth;
  SHORT		lsb;
  public:
  DEFINE_SIZE_STATIC (4);
};

struct hmtx
{
  static const hb_tag_t tableTag	= HB_OT_TAG_hmtx;

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    /* We don't check for anything specific here.  The users of the
     * struct do all the hard work... */
    return TRACE_RETURN (true);
  }

  public:
  LongHorMetric	longHorMetric[VAR];	/* Paired advance width and left side
					 * bearing values for each glyph. The
					 * value numOfHMetrics comes from
					 * the 'hhea' table. If the font is
					 * monospaced, only one entry need
					 * be in the array, but that entry is
					 * required. The last entry applies to
					 * all subsequent glyphs. */
  SHORT		leftSideBearingX[VAR];	/* Here the advanceWidth is assumed
					 * to be the same as the advanceWidth
					 * for the last entry above. The
					 * number of entries in this array is
					 * derived from numGlyphs (from 'maxp'
					 * table) minus numberOfHMetrics. This
					 * generally is used with a run of
					 * monospaced glyphs (e.g., Kanji
					 * fonts or Courier fonts). Only one
					 * run is allowed and it must be at
					 * the end. This allows a monospaced
					 * font to vary the left side bearing
					 * values for each glyph. */
  public:
  DEFINE_SIZE_ARRAY2 (0, longHorMetric, leftSideBearingX);
};


} /* namespace OT */


#endif /* HB_OT_HMTX_TABLE_HH */
