/*
 * Copyright © 2010,2012  Google, Inc.
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

#include "hb-ot-shape-complex-private.hh"
#include "hb-ot-shape-private.hh"


/* buffer var allocations */
#define arabic_shaping_action() complex_var_u8_0() /* arabic shaping action */


/*
 * Bits used in the joining tables
 */
enum {
  JOINING_TYPE_U		= 0,
  JOINING_TYPE_L		= 1,
  JOINING_TYPE_R		= 2,
  JOINING_TYPE_D		= 3,
  JOINING_TYPE_C		= JOINING_TYPE_D,
  JOINING_GROUP_ALAPH		= 4,
  JOINING_GROUP_DALATH_RISH	= 5,
  NUM_STATE_MACHINE_COLS	= 6,

  JOINING_TYPE_T = 7,
  JOINING_TYPE_X = 8  /* means: use general-category to choose between U or T. */
};

/*
 * Joining types:
 */

#include "hb-ot-shape-complex-arabic-table.hh"

static unsigned int get_joining_type (hb_codepoint_t u, hb_unicode_general_category_t gen_cat)
{
  if (likely (hb_in_range<hb_codepoint_t> (u, JOINING_TABLE_FIRST, JOINING_TABLE_LAST))) {
    unsigned int j_type = joining_table[u - JOINING_TABLE_FIRST];
    if (likely (j_type != JOINING_TYPE_X))
      return j_type;
  }

  /* Mongolian joining data is not in ArabicJoining.txt yet. */
  if (unlikely (hb_in_range<hb_codepoint_t> (u, 0x1800, 0x18AF)))
  {
    if (unlikely (hb_in_range<hb_codepoint_t> (u, 0x1880, 0x1886)))
      return JOINING_TYPE_U;

    /* All letters, SIBE SYLLABLE BOUNDARY MARKER, and NIRUGU are D */
    if ((FLAG(gen_cat) & (FLAG (HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER) |
			  FLAG (HB_UNICODE_GENERAL_CATEGORY_MODIFIER_LETTER)))
	|| u == 0x1807 || u == 0x180A)
      return JOINING_TYPE_D;
  }

  /* 'Phags-pa joining data is not in ArabicJoining.txt yet. */
  if (unlikely (hb_in_range<hb_codepoint_t> (u, 0xA840, 0xA872)))
  {
      if (unlikely (u == 0xA872))
	return JOINING_TYPE_L;

      return JOINING_TYPE_D;
  }

  if (unlikely (hb_in_range<hb_codepoint_t> (u, 0x200C, 0x200D)))
  {
    return u == 0x200C ? JOINING_TYPE_U : JOINING_TYPE_C;
  }

  return (FLAG(gen_cat) & (FLAG(HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) | FLAG(HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK) | FLAG(HB_UNICODE_GENERAL_CATEGORY_FORMAT))) ?
	 JOINING_TYPE_T : JOINING_TYPE_U;
}

static const hb_tag_t arabic_features[] =
{
  HB_TAG('i','n','i','t'),
  HB_TAG('m','e','d','i'),
  HB_TAG('f','i','n','a'),
  HB_TAG('i','s','o','l'),
  /* Syriac */
  HB_TAG('m','e','d','2'),
  HB_TAG('f','i','n','2'),
  HB_TAG('f','i','n','3'),
  HB_TAG_NONE
};


/* Same order as the feature array */
enum {
  INIT,
  MEDI,
  FINA,
  ISOL,

  /* Syriac */
  MED2,
  FIN2,
  FIN3,

  NONE,

  ARABIC_NUM_FEATURES = NONE
};

static const struct arabic_state_table_entry {
	uint8_t prev_action;
	uint8_t curr_action;
	uint16_t next_state;
} arabic_state_table[][NUM_STATE_MACHINE_COLS] =
{
  /*   jt_U,          jt_L,          jt_R,          jt_D,          jg_ALAPH,      jg_DALATH_RISH */

  /* State 0: prev was U, not willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,6}, },

  /* State 1: prev was R or ISOL/ALAPH, not willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN2,5}, {NONE,ISOL,6}, },

  /* State 2: prev was D/L in ISOL form, willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {INIT,FINA,1}, {INIT,FINA,3}, {INIT,FINA,4}, {INIT,FINA,6}, },

  /* State 3: prev was D in FINA form, willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {MEDI,FINA,1}, {MEDI,FINA,3}, {MEDI,FINA,4}, {MEDI,FINA,6}, },

  /* State 4: prev was FINA ALAPH, not willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {MED2,ISOL,1}, {MED2,ISOL,2}, {MED2,FIN2,5}, {MED2,ISOL,6}, },

  /* State 5: prev was FIN2/FIN3 ALAPH, not willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {ISOL,ISOL,1}, {ISOL,ISOL,2}, {ISOL,FIN2,5}, {ISOL,ISOL,6}, },

  /* State 6: prev was DALATH/RISH, not willing to join. */
  { {NONE,NONE,0}, {NONE,ISOL,2}, {NONE,ISOL,1}, {NONE,ISOL,2}, {NONE,FIN3,5}, {NONE,ISOL,6}, }
};


static void
nuke_joiners (const hb_ot_shape_plan_t *plan,
	      hb_font_t *font,
	      hb_buffer_t *buffer);

static void
arabic_fallback_shape (const hb_ot_shape_plan_t *plan,
		       hb_font_t *font,
		       hb_buffer_t *buffer);

static void
collect_features_arabic (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  /* For Language forms (in ArabicOT speak), we do the iso/fina/medi/init together,
   * then rlig and calt each in their own stage.  This makes IranNastaliq's ALLAH
   * ligature work correctly. It's unfortunate though...
   *
   * This also makes Arial Bold in Windows7 work.  See:
   * https://bugzilla.mozilla.org/show_bug.cgi?id=644184
   *
   * TODO: Add test cases for these two.
   */

  map->add_gsub_pause (nuke_joiners);

  map->add_global_bool_feature (HB_TAG('c','c','m','p'));
  map->add_global_bool_feature (HB_TAG('l','o','c','l'));

  map->add_gsub_pause (NULL);

  for (unsigned int i = 0; i < ARABIC_NUM_FEATURES; i++)
    map->add_feature (arabic_features[i], 1, i < 4 ? F_HAS_FALLBACK : F_NONE); /* The first four features have fallback. */

  map->add_gsub_pause (NULL);

  map->add_feature (HB_TAG('r','l','i','g'), 1, F_GLOBAL|F_HAS_FALLBACK);
  map->add_gsub_pause (arabic_fallback_shape);

  map->add_global_bool_feature (HB_TAG('c','a','l','t'));
  map->add_gsub_pause (NULL);

  map->add_global_bool_feature (HB_TAG('m','s','e','t'));
}

#include "hb-ot-shape-complex-arabic-fallback.hh"

struct arabic_shape_plan_t
{
  ASSERT_POD ();

  /* The "+ 1" in the next array is to accommodate for the "NONE" command,
   * which is not an OpenType feature, but this simplifies the code by not
   * having to do a "if (... < NONE) ..." and just rely on the fact that
   * mask_array[NONE] == 0. */
  hb_mask_t mask_array[ARABIC_NUM_FEATURES + 1];

  bool do_fallback;
  arabic_fallback_plan_t *fallback_plan;
};

static void *
data_create_arabic (const hb_ot_shape_plan_t *plan)
{
  arabic_shape_plan_t *arabic_plan = (arabic_shape_plan_t *) calloc (1, sizeof (arabic_shape_plan_t));
  if (unlikely (!arabic_plan))
    return NULL;

  arabic_plan->do_fallback = plan->props.script == HB_SCRIPT_ARABIC;
  for (unsigned int i = 0; i < ARABIC_NUM_FEATURES; i++) {
    arabic_plan->mask_array[i] = plan->map.get_1_mask (arabic_features[i]);
    if (i < 4)
      arabic_plan->do_fallback = arabic_plan->do_fallback && plan->map.needs_fallback (arabic_features[i]);
  }

  return arabic_plan;
}

static void
data_destroy_arabic (void *data)
{
  arabic_shape_plan_t *arabic_plan = (arabic_shape_plan_t *) data;

  arabic_fallback_plan_destroy (arabic_plan->fallback_plan);

  free (data);
}

static void
arabic_joining (hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  unsigned int prev = (unsigned int) -1, state = 0;

  HB_BUFFER_ALLOCATE_VAR (buffer, arabic_shaping_action);

  /* Check pre-context */
  if (!(buffer->flags & HB_BUFFER_FLAG_BOT))
    for (unsigned int i = 0; i < buffer->context_len[0]; i++)
    {
      unsigned int this_type = get_joining_type (buffer->context[0][i], buffer->unicode->general_category (buffer->context[0][i]));

      if (unlikely (this_type == JOINING_TYPE_T))
	continue;

      const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];
      state = entry->next_state;
      break;
    }

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int this_type = get_joining_type (buffer->info[i].codepoint, _hb_glyph_info_get_general_category (&buffer->info[i]));

    if (unlikely (this_type == JOINING_TYPE_T)) {
      buffer->info[i].arabic_shaping_action() = NONE;
      continue;
    }

    const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];

    if (entry->prev_action != NONE && prev != (unsigned int) -1)
      for (; prev < i; prev++)
	buffer->info[prev].arabic_shaping_action() = entry->prev_action;

    buffer->info[i].arabic_shaping_action() = entry->curr_action;

    prev = i;
    state = entry->next_state;
  }

  if (!(buffer->flags & HB_BUFFER_FLAG_EOT))
    for (unsigned int i = 0; i < buffer->context_len[1]; i++)
    {
      unsigned int this_type = get_joining_type (buffer->context[1][i], buffer->unicode->general_category (buffer->context[1][i]));

      if (unlikely (this_type == JOINING_TYPE_T))
	continue;

      const arabic_state_table_entry *entry = &arabic_state_table[state][this_type];
      if (entry->prev_action != NONE && prev != (unsigned int) -1)
	buffer->info[prev].arabic_shaping_action() = entry->prev_action;
      break;
    }


  HB_BUFFER_DEALLOCATE_VAR (buffer, arabic_shaping_action);
}

static void
setup_masks_arabic (const hb_ot_shape_plan_t *plan,
		    hb_buffer_t              *buffer,
		    hb_font_t                *font HB_UNUSED)
{
  const arabic_shape_plan_t *arabic_plan = (const arabic_shape_plan_t *) plan->data;

  arabic_joining (buffer);
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    buffer->info[i].mask |= arabic_plan->mask_array[buffer->info[i].arabic_shaping_action()];
}


static void
nuke_joiners (const hb_ot_shape_plan_t *plan HB_UNUSED,
	      hb_font_t *font HB_UNUSED,
	      hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    if (_hb_glyph_info_is_zwj (&buffer->info[i]))
      _hb_glyph_info_flip_joiners (&buffer->info[i]);
}

static void
arabic_fallback_shape (const hb_ot_shape_plan_t *plan,
		       hb_font_t *font,
		       hb_buffer_t *buffer)
{
  const arabic_shape_plan_t *arabic_plan = (const arabic_shape_plan_t *) plan->data;

  if (!arabic_plan->do_fallback)
    return;

retry:
  arabic_fallback_plan_t *fallback_plan = (arabic_fallback_plan_t *) hb_atomic_ptr_get (&arabic_plan->fallback_plan);
  if (unlikely (!fallback_plan))
  {
    /* This sucks.  We need a font to build the fallback plan... */
    fallback_plan = arabic_fallback_plan_create (plan, font);
    if (unlikely (!hb_atomic_ptr_cmpexch (&(const_cast<arabic_shape_plan_t *> (arabic_plan))->fallback_plan, NULL, fallback_plan))) {
      arabic_fallback_plan_destroy (fallback_plan);
      goto retry;
    }
  }

  arabic_fallback_plan_shape (fallback_plan, font, buffer);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_arabic =
{
  "arabic",
  collect_features_arabic,
  NULL, /* override_features */
  data_create_arabic,
  data_destroy_arabic,
  NULL, /* preprocess_text_arabic */
  HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
  NULL, /* decompose */
  NULL, /* compose */
  setup_masks_arabic,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF_LATE,
  true, /* fallback_position */
};
