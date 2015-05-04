/*
 * Copyright © 2011,2014  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod, Roozbeh Pournader
 */

#include "hb-private.hh"

#include "hb-ot.h"

#include "hb-font-private.hh"

#include "hb-ot-cmap-table.hh"
#include "hb-ot-hhea-table.hh"
#include "hb-ot-hmtx-table.hh"



struct hb_ot_font_t
{
  unsigned int num_glyphs;
  unsigned int num_hmetrics;
  const OT::hmtx *hmtx;
  hb_blob_t *hmtx_blob;

  const OT::CmapSubtable *cmap;
  const OT::CmapSubtable *cmap_uvs;
  hb_blob_t *cmap_blob;
};


static hb_ot_font_t *
_hb_ot_font_create (hb_font_t *font)
{
  hb_ot_font_t *ot_font = (hb_ot_font_t *) calloc (1, sizeof (hb_ot_font_t));

  if (unlikely (!ot_font))
    return NULL;

  ot_font->num_glyphs = font->face->get_num_glyphs ();

  {
    hb_blob_t *hhea_blob = OT::Sanitizer<OT::hhea>::sanitize (font->face->reference_table (HB_OT_TAG_hhea));
    const OT::hhea *hhea = OT::Sanitizer<OT::hhea>::lock_instance (hhea_blob);
    ot_font->num_hmetrics = hhea->numberOfHMetrics;
    hb_blob_destroy (hhea_blob);
  }
  ot_font->hmtx_blob = OT::Sanitizer<OT::hmtx>::sanitize (font->face->reference_table (HB_OT_TAG_hmtx));
  if (unlikely (!ot_font->num_hmetrics ||
		2 * (ot_font->num_hmetrics + ot_font->num_glyphs) < hb_blob_get_length (ot_font->hmtx_blob)))
  {
    hb_blob_destroy (ot_font->hmtx_blob);
    free (ot_font);
    return NULL;
  }
  ot_font->hmtx = OT::Sanitizer<OT::hmtx>::lock_instance (ot_font->hmtx_blob);

  ot_font->cmap_blob = OT::Sanitizer<OT::cmap>::sanitize (font->face->reference_table (HB_OT_TAG_cmap));
  const OT::cmap *cmap = OT::Sanitizer<OT::cmap>::lock_instance (ot_font->cmap_blob);
  const OT::CmapSubtable *subtable = NULL;
  const OT::CmapSubtable *subtable_uvs = NULL;

  /* 32-bit subtables. */
  if (!subtable) subtable = cmap->find_subtable (0, 6);
  if (!subtable) subtable = cmap->find_subtable (0, 4);
  if (!subtable) subtable = cmap->find_subtable (3, 10);
  /* 16-bit subtables. */
  if (!subtable) subtable = cmap->find_subtable (0, 3);
  if (!subtable) subtable = cmap->find_subtable (3, 1);
  /* Meh. */
  if (!subtable) subtable = &OT::Null(OT::CmapSubtable);

  /* UVS subtable. */
  if (!subtable_uvs) subtable_uvs = cmap->find_subtable (0, 5);
  /* Meh. */
  if (!subtable_uvs) subtable_uvs = &OT::Null(OT::CmapSubtable);

  ot_font->cmap = subtable;
  ot_font->cmap_uvs = subtable_uvs;

  return ot_font;
}

static void
_hb_ot_font_destroy (hb_ot_font_t *ot_font)
{
  hb_blob_destroy (ot_font->cmap_blob);
  hb_blob_destroy (ot_font->hmtx_blob);

  free (ot_font);
}


static hb_bool_t
hb_ot_get_glyph (hb_font_t *font HB_UNUSED,
		 void *font_data,
		 hb_codepoint_t unicode,
		 hb_codepoint_t variation_selector,
		 hb_codepoint_t *glyph,
		 void *user_data HB_UNUSED)

{
  const hb_ot_font_t *ot_font = (const hb_ot_font_t *) font_data;

  if (unlikely (variation_selector))
  {
    switch (ot_font->cmap_uvs->get_glyph_variant (unicode,
						  variation_selector,
						  glyph))
    {
      case OT::GLYPH_VARIANT_NOT_FOUND:		return false;
      case OT::GLYPH_VARIANT_FOUND:		return true;
      case OT::GLYPH_VARIANT_USE_DEFAULT:	break;
    }
  }

  return ot_font->cmap->get_glyph (unicode, glyph);
}

static hb_position_t
hb_ot_get_glyph_h_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
  const hb_ot_font_t *ot_font = (const hb_ot_font_t *) font_data;

  if (unlikely (glyph >= ot_font->num_glyphs))
    return 0; /* Maybe better to return notdef's advance instead? */

  if (glyph >= ot_font->num_hmetrics)
    glyph = ot_font->num_hmetrics - 1;

  return font->em_scale_x (ot_font->hmtx->longHorMetric[glyph].advanceWidth);
}

static hb_position_t
hb_ot_get_glyph_v_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
  /* TODO */
  return 0;
}

static hb_bool_t
hb_ot_get_glyph_h_origin (hb_font_t *font HB_UNUSED,
			  void *font_data HB_UNUSED,
			  hb_codepoint_t glyph HB_UNUSED,
			  hb_position_t *x HB_UNUSED,
			  hb_position_t *y HB_UNUSED,
			  void *user_data HB_UNUSED)
{
  /* We always work in the horizontal coordinates. */
  return true;
}

static hb_bool_t
hb_ot_get_glyph_v_origin (hb_font_t *font HB_UNUSED,
			  void *font_data,
			  hb_codepoint_t glyph,
			  hb_position_t *x,
			  hb_position_t *y,
			  void *user_data HB_UNUSED)
{
  /* TODO */
  return false;
}

static hb_position_t
hb_ot_get_glyph_h_kerning (hb_font_t *font,
			   void *font_data,
			   hb_codepoint_t left_glyph,
			   hb_codepoint_t right_glyph,
			   void *user_data HB_UNUSED)
{
  /* TODO */
  return 0;
}

static hb_position_t
hb_ot_get_glyph_v_kerning (hb_font_t *font HB_UNUSED,
			   void *font_data HB_UNUSED,
			   hb_codepoint_t top_glyph HB_UNUSED,
			   hb_codepoint_t bottom_glyph HB_UNUSED,
			   void *user_data HB_UNUSED)
{
  return 0;
}

static hb_bool_t
hb_ot_get_glyph_extents (hb_font_t *font HB_UNUSED,
			 void *font_data,
			 hb_codepoint_t glyph,
			 hb_glyph_extents_t *extents,
			 void *user_data HB_UNUSED)
{
  /* TODO */
  return false;
}

static hb_bool_t
hb_ot_get_glyph_contour_point (hb_font_t *font HB_UNUSED,
			       void *font_data,
			       hb_codepoint_t glyph,
			       unsigned int point_index,
			       hb_position_t *x,
			       hb_position_t *y,
			       void *user_data HB_UNUSED)
{
  /* TODO */
  return false;
}

static hb_bool_t
hb_ot_get_glyph_name (hb_font_t *font HB_UNUSED,
		      void *font_data,
		      hb_codepoint_t glyph,
		      char *name, unsigned int size,
		      void *user_data HB_UNUSED)
{
  /* TODO */
  return false;
}

static hb_bool_t
hb_ot_get_glyph_from_name (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   const char *name, int len, /* -1 means nul-terminated */
			   hb_codepoint_t *glyph,
			   void *user_data HB_UNUSED)
{
  /* TODO */
  return false;
}


static hb_font_funcs_t *
_hb_ot_get_font_funcs (void)
{
  static const hb_font_funcs_t ot_ffuncs = {
    HB_OBJECT_HEADER_STATIC,

    true, /* immutable */

    {
#define HB_FONT_FUNC_IMPLEMENT(name) hb_ot_get_##name,
      HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
    }
  };

  return const_cast<hb_font_funcs_t *> (&ot_ffuncs);
}


void
hb_ot_font_set_funcs (hb_font_t *font)
{
  hb_ot_font_t *ot_font = _hb_ot_font_create (font);
  if (unlikely (!ot_font))
    return;

  hb_font_set_funcs (font,
		     _hb_ot_get_font_funcs (),
		     ot_font,
		     (hb_destroy_func_t) _hb_ot_font_destroy);
}
