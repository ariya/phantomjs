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

#ifndef HB_OT_HHEA_TABLE_HH
#define HB_OT_HHEA_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {


/*
 * hhea -- The Horizontal Header Table
 */

#define HB_OT_TAG_hhea HB_TAG('h','h','e','a')


struct hhea
{
  static const hb_tag_t tableTag	= HB_OT_TAG_hhea;

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) && likely (version.major == 1));
  }

  protected:
  FixedVersion	version;		/* 0x00010000 for version 1.0. */
  FWORD		ascender;		/* Typographic ascent. <a
					 * href="http://developer.apple.com/fonts/TTRefMan/RM06/Chap6hhea.html">
					 * (Distance from baseline of highest
					 * ascender)</a> */
  FWORD		descender;		/* Typographic descent. <a
					 * href="http://developer.apple.com/fonts/TTRefMan/RM06/Chap6hhea.html">
					 * (Distance from baseline of lowest
					 * descender)</a> */
  FWORD		lineGap;		/* Typographic line gap. Negative
					 * LineGap values are treated as zero
					 * in Windows 3.1, System 6, and
					 * System 7. */
  UFWORD	advanceWidthMax;	/* Maximum advance width value in
					 * 'hmtx' table. */
  FWORD		minLeftSideBearing;	/* Minimum left sidebearing value in
					 * 'hmtx' table. */
  FWORD		minRightSideBearing;	/* Minimum right sidebearing value;
					 * calculated as Min(aw - lsb -
					 * (xMax - xMin)). */
  FWORD		xMaxExtent;		/* Max(lsb + (xMax - xMin)). */
  SHORT		caretSlopeRise;		/* Used to calculate the slope of the
					 * cursor (rise/run); 1 for vertical. */
  SHORT		caretSlopeRun;		/* 0 for vertical. */
  SHORT		caretOffset;		/* The amount by which a slanted
					 * highlight on a glyph needs
					 * to be shifted to produce the
					 * best appearance. Set to 0 for
					 * non--slanted fonts */
  SHORT		reserved1;		/* set to 0 */
  SHORT		reserved2;		/* set to 0 */
  SHORT		reserved3;		/* set to 0 */
  SHORT		reserved4;		/* set to 0 */
  SHORT		metricDataFormat;	/* 0 for current format. */
  USHORT	numberOfHMetrics;	/* Number of hMetric entries in 'hmtx'
					 * table */
  public:
  DEFINE_SIZE_STATIC (36);
};


} /* namespace OT */


#endif /* HB_OT_HHEA_TABLE_HH */
