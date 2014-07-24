/*
 * Copyright © 2012,2013  Mozilla Foundation.
 * Copyright © 2012,2013  Google, Inc.
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
 * Mozilla Author(s): Jonathan Kew
 * Google Author(s): Behdad Esfahbod
 */

#define HB_SHAPER coretext
#include "hb-shaper-impl-private.hh"

#include "hb-coretext.h"

#include "hb-face-private.hh"


#ifndef HB_DEBUG_CORETEXT
#define HB_DEBUG_CORETEXT (HB_DEBUG+0)
#endif


HB_SHAPER_DATA_ENSURE_DECLARE(coretext, face)
HB_SHAPER_DATA_ENSURE_DECLARE(coretext, font)


typedef bool (*qt_get_font_table_func_t) (void *user_data, unsigned int tag, unsigned char *buffer, unsigned int *length);

struct FontEngineFaceData {
  void *user_data;
  qt_get_font_table_func_t get_font_table;
};

struct CoreTextFontEngineData {
  CTFontRef ctFont;
  CGFontRef cgFont;
};


/*
 * shaper face data
 */

struct hb_coretext_shaper_face_data_t {
  CGFontRef cg_font;
};

static void
release_data (void *info, const void *data, size_t size)
{
  assert (hb_blob_get_length ((hb_blob_t *) info) == size &&
          hb_blob_get_data ((hb_blob_t *) info, NULL) == data);

  hb_blob_destroy ((hb_blob_t *) info);
}

hb_coretext_shaper_face_data_t *
_hb_coretext_shaper_face_data_create (hb_face_t *face)
{
  hb_blob_t *mort_blob = face->reference_table (HB_CORETEXT_TAG_MORT);
  /* Umm, we just reference the table to check whether it exists.
   * Maybe add better API for this? */
  if (!hb_blob_get_length (mort_blob))
  {
    hb_blob_destroy (mort_blob);
    mort_blob = face->reference_table (HB_CORETEXT_TAG_MORX);
    if (!hb_blob_get_length (mort_blob))
    {
      hb_blob_destroy (mort_blob);
      return NULL;
    }
  }
  hb_blob_destroy (mort_blob);

  hb_coretext_shaper_face_data_t *data = (hb_coretext_shaper_face_data_t *) calloc (1, sizeof (hb_coretext_shaper_face_data_t));
  if (unlikely (!data))
    return NULL;

  FontEngineFaceData *fontEngineFaceData = (FontEngineFaceData *) face->user_data;
  CoreTextFontEngineData *coreTextFontEngineData = (CoreTextFontEngineData *) fontEngineFaceData->user_data;
  data->cg_font = coreTextFontEngineData->cgFont;
  if (likely (data->cg_font))
    CFRetain (data->cg_font);

  if (unlikely (!data->cg_font)) {
    DEBUG_MSG (CORETEXT, face, "Face CGFontCreateWithDataProvider() failed");
    free (data);
    return NULL;
  }

  return data;
}

void
_hb_coretext_shaper_face_data_destroy (hb_coretext_shaper_face_data_t *data)
{
  CFRelease (data->cg_font);
  free (data);
}

CGFontRef
hb_coretext_face_get_cg_font (hb_face_t *face)
{
  if (unlikely (!hb_coretext_shaper_face_data_ensure (face))) return NULL;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);
  return face_data->cg_font;
}


/*
 * shaper font data
 */

struct hb_coretext_shaper_font_data_t {
  CTFontRef ct_font;
};

hb_coretext_shaper_font_data_t *
_hb_coretext_shaper_font_data_create (hb_font_t *font)
{
  if (unlikely (!hb_coretext_shaper_face_data_ensure (font->face))) return NULL;

  hb_coretext_shaper_font_data_t *data = (hb_coretext_shaper_font_data_t *) calloc (1, sizeof (hb_coretext_shaper_font_data_t));
  if (unlikely (!data))
    return NULL;

  hb_face_t *face = font->face;

  FontEngineFaceData *fontEngineFaceData = (FontEngineFaceData *) face->user_data;
  CoreTextFontEngineData *coreTextFontEngineData = (CoreTextFontEngineData *) fontEngineFaceData->user_data;
  data->ct_font = coreTextFontEngineData->ctFont;
  if (likely (data->ct_font))
    CFRetain (data->ct_font);

  if (unlikely (!data->ct_font)) {
    DEBUG_MSG (CORETEXT, font, "Font CTFontCreateWithGraphicsFont() failed");
    free (data);
    return NULL;
  }

  return data;
}

void
_hb_coretext_shaper_font_data_destroy (hb_coretext_shaper_font_data_t *data)
{
  CFRelease (data->ct_font);
  free (data);
}


/*
 * shaper shape_plan data
 */

struct hb_coretext_shaper_shape_plan_data_t {};

hb_coretext_shaper_shape_plan_data_t *
_hb_coretext_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					     const hb_feature_t *user_features HB_UNUSED,
					     unsigned int        num_user_features HB_UNUSED)
{
  return (hb_coretext_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_coretext_shaper_shape_plan_data_destroy (hb_coretext_shaper_shape_plan_data_t *data HB_UNUSED)
{
}

CTFontRef
hb_coretext_font_get_ct_font (hb_font_t *font)
{
  if (unlikely (!hb_coretext_shaper_font_data_ensure (font))) return NULL;
  hb_coretext_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);
  return font_data->ct_font;
}


/*
 * shaper
 */

struct feature_record_t {
  unsigned int feature;
  unsigned int setting;
};

struct active_feature_t {
  feature_record_t rec;
  unsigned int order;

  static int cmp (const active_feature_t *a, const active_feature_t *b) {
    return a->rec.feature < b->rec.feature ? -1 : a->rec.feature > b->rec.feature ? 1 :
	   a->order < b->order ? -1 : a->order > b->order ? 1 :
	   a->rec.setting < b->rec.setting ? -1 : a->rec.setting > b->rec.setting ? 1 :
	   0;
  }
  bool operator== (const active_feature_t *f) {
    return cmp (this, f) == 0;
  }
};

struct feature_event_t {
  unsigned int index;
  bool start;
  active_feature_t feature;

  static int cmp (const feature_event_t *a, const feature_event_t *b) {
    return a->index < b->index ? -1 : a->index > b->index ? 1 :
	   a->start < b->start ? -1 : a->start > b->start ? 1 :
	   active_feature_t::cmp (&a->feature, &b->feature);
  }
};

struct range_record_t {
  CTFontRef font;
  unsigned int index_first; /* == start */
  unsigned int index_last;  /* == end - 1 */
};


/* The following enum members are added in OS X 10.8. */
#define kAltHalfWidthTextSelector		6
#define kAltProportionalTextSelector		5
#define kAlternateHorizKanaOffSelector		1
#define kAlternateHorizKanaOnSelector		0
#define kAlternateKanaType			34
#define kAlternateVertKanaOffSelector		3
#define kAlternateVertKanaOnSelector		2
#define kCaseSensitiveLayoutOffSelector		1
#define kCaseSensitiveLayoutOnSelector		0
#define kCaseSensitiveLayoutType		33
#define kCaseSensitiveSpacingOffSelector	3
#define kCaseSensitiveSpacingOnSelector		2
#define kContextualAlternatesOffSelector	1
#define kContextualAlternatesOnSelector		0
#define kContextualAlternatesType		36
#define kContextualLigaturesOffSelector		19
#define kContextualLigaturesOnSelector		18
#define kContextualSwashAlternatesOffSelector	5
#define kContextualSwashAlternatesOnSelector	4
#define kDefaultLowerCaseSelector		0
#define kDefaultUpperCaseSelector		0
#define kHistoricalLigaturesOffSelector		21
#define kHistoricalLigaturesOnSelector		20
#define kHojoCharactersSelector			12
#define kJIS2004CharactersSelector		11
#define kLowerCasePetiteCapsSelector		2
#define kLowerCaseSmallCapsSelector		1
#define kLowerCaseType				37
#define kMathematicalGreekOffSelector		11
#define kMathematicalGreekOnSelector		10
#define kNLCCharactersSelector			13
#define kQuarterWidthTextSelector		4
#define kScientificInferiorsSelector		4
#define kStylisticAltEightOffSelector		17
#define kStylisticAltEightOnSelector		16
#define kStylisticAltEighteenOffSelector	37
#define kStylisticAltEighteenOnSelector		36
#define kStylisticAltElevenOffSelector		23
#define kStylisticAltElevenOnSelector		22
#define kStylisticAltFifteenOffSelector		31
#define kStylisticAltFifteenOnSelector		30
#define kStylisticAltFiveOffSelector		11
#define kStylisticAltFiveOnSelector		10
#define kStylisticAltFourOffSelector		9
#define kStylisticAltFourOnSelector		8
#define kStylisticAltFourteenOffSelector	29
#define kStylisticAltFourteenOnSelector		28
#define kStylisticAltNineOffSelector		19
#define kStylisticAltNineOnSelector		18
#define kStylisticAltNineteenOffSelector	39
#define kStylisticAltNineteenOnSelector		38
#define kStylisticAltOneOffSelector		3
#define kStylisticAltOneOnSelector		2
#define kStylisticAltSevenOffSelector		15
#define kStylisticAltSevenOnSelector		14
#define kStylisticAltSeventeenOffSelector	35
#define kStylisticAltSeventeenOnSelector	34
#define kStylisticAltSixOffSelector		13
#define kStylisticAltSixOnSelector		12
#define kStylisticAltSixteenOffSelector		33
#define kStylisticAltSixteenOnSelector		32
#define kStylisticAltTenOffSelector		21
#define kStylisticAltTenOnSelector		20
#define kStylisticAltThirteenOffSelector	27
#define kStylisticAltThirteenOnSelector		26
#define kStylisticAltThreeOffSelector		7
#define kStylisticAltThreeOnSelector		6
#define kStylisticAltTwelveOffSelector		25
#define kStylisticAltTwelveOnSelector		24
#define kStylisticAltTwentyOffSelector		41
#define kStylisticAltTwentyOnSelector		40
#define kStylisticAltTwoOffSelector		5
#define kStylisticAltTwoOnSelector		4
#define kStylisticAlternativesType		35
#define kSwashAlternatesOffSelector		3
#define kSwashAlternatesOnSelector		2
#define kThirdWidthTextSelector			3
#define kTraditionalNamesCharactersSelector	14
#define kUpperCasePetiteCapsSelector		2
#define kUpperCaseSmallCapsSelector		1
#define kUpperCaseType				38

/* Table data courtesy of Apple. */
struct feature_mapping_t {
    FourCharCode otFeatureTag;
    uint16_t aatFeatureType;
    uint16_t selectorToEnable;
    uint16_t selectorToDisable;
} feature_mappings[] = {
    { 'c2pc',   kUpperCaseType,             kUpperCasePetiteCapsSelector,           kDefaultUpperCaseSelector },
    { 'c2sc',   kUpperCaseType,             kUpperCaseSmallCapsSelector,            kDefaultUpperCaseSelector },
    { 'calt',   kContextualAlternatesType,  kContextualAlternatesOnSelector,        kContextualAlternatesOffSelector },
    { 'case',   kCaseSensitiveLayoutType,   kCaseSensitiveLayoutOnSelector,         kCaseSensitiveLayoutOffSelector },
    { 'clig',   kLigaturesType,             kContextualLigaturesOnSelector,         kContextualLigaturesOffSelector },
    { 'cpsp',   kCaseSensitiveLayoutType,   kCaseSensitiveSpacingOnSelector,        kCaseSensitiveSpacingOffSelector },
    { 'cswh',   kContextualAlternatesType,  kContextualSwashAlternatesOnSelector,   kContextualSwashAlternatesOffSelector },
    { 'dlig',   kLigaturesType,             kRareLigaturesOnSelector,               kRareLigaturesOffSelector },
    { 'expt',   kCharacterShapeType,        kExpertCharactersSelector,              16 },
    { 'frac',   kFractionsType,             kDiagonalFractionsSelector,             kNoFractionsSelector },
    { 'fwid',   kTextSpacingType,           kMonospacedTextSelector,                7 },
    { 'halt',   kTextSpacingType,           kAltHalfWidthTextSelector,              7 },
    { 'hist',   kLigaturesType,             kHistoricalLigaturesOnSelector,         kHistoricalLigaturesOffSelector },
    { 'hkna',   kAlternateKanaType,         kAlternateHorizKanaOnSelector,          kAlternateHorizKanaOffSelector, },
    { 'hlig',   kLigaturesType,             kHistoricalLigaturesOnSelector,         kHistoricalLigaturesOffSelector },
    { 'hngl',   kTransliterationType,       kHanjaToHangulSelector,                 kNoTransliterationSelector },
    { 'hojo',   kCharacterShapeType,        kHojoCharactersSelector,                16 },
    { 'hwid',   kTextSpacingType,           kHalfWidthTextSelector,                 7 },
    { 'ital',   kItalicCJKRomanType,        kCJKItalicRomanOnSelector,              kCJKItalicRomanOffSelector },
    { 'jp04',   kCharacterShapeType,        kJIS2004CharactersSelector,             16 },
    { 'jp78',   kCharacterShapeType,        kJIS1978CharactersSelector,             16 },
    { 'jp83',   kCharacterShapeType,        kJIS1983CharactersSelector,             16 },
    { 'jp90',   kCharacterShapeType,        kJIS1990CharactersSelector,             16 },
    { 'liga',   kLigaturesType,             kCommonLigaturesOnSelector,             kCommonLigaturesOffSelector },
    { 'lnum',   kNumberCaseType,            kUpperCaseNumbersSelector,              2 },
    { 'mgrk',   kMathematicalExtrasType,    kMathematicalGreekOnSelector,           kMathematicalGreekOffSelector },
    { 'nlck',   kCharacterShapeType,        kNLCCharactersSelector,                 16 },
    { 'onum',   kNumberCaseType,            kLowerCaseNumbersSelector,              2 },
    { 'ordn',   kVerticalPositionType,      kOrdinalsSelector,                      kNormalPositionSelector },
    { 'palt',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'pcap',   kLowerCaseType,             kLowerCasePetiteCapsSelector,           kDefaultLowerCaseSelector },
    { 'pkna',   kTextSpacingType,           kProportionalTextSelector,              7 },
    { 'pnum',   kNumberSpacingType,         kProportionalNumbersSelector,           4 },
    { 'pwid',   kTextSpacingType,           kProportionalTextSelector,              7 },
    { 'qwid',   kTextSpacingType,           kQuarterWidthTextSelector,              7 },
    { 'ruby',   kRubyKanaType,              kRubyKanaOnSelector,                    kRubyKanaOffSelector },
    { 'sinf',   kVerticalPositionType,      kScientificInferiorsSelector,           kNormalPositionSelector },
    { 'smcp',   kLowerCaseType,             kLowerCaseSmallCapsSelector,            kDefaultLowerCaseSelector },
    { 'smpl',   kCharacterShapeType,        kSimplifiedCharactersSelector,          16 },
    { 'ss01',   kStylisticAlternativesType, kStylisticAltOneOnSelector,             kStylisticAltOneOffSelector },
    { 'ss02',   kStylisticAlternativesType, kStylisticAltTwoOnSelector,             kStylisticAltTwoOffSelector },
    { 'ss03',   kStylisticAlternativesType, kStylisticAltThreeOnSelector,           kStylisticAltThreeOffSelector },
    { 'ss04',   kStylisticAlternativesType, kStylisticAltFourOnSelector,            kStylisticAltFourOffSelector },
    { 'ss05',   kStylisticAlternativesType, kStylisticAltFiveOnSelector,            kStylisticAltFiveOffSelector },
    { 'ss06',   kStylisticAlternativesType, kStylisticAltSixOnSelector,             kStylisticAltSixOffSelector },
    { 'ss07',   kStylisticAlternativesType, kStylisticAltSevenOnSelector,           kStylisticAltSevenOffSelector },
    { 'ss08',   kStylisticAlternativesType, kStylisticAltEightOnSelector,           kStylisticAltEightOffSelector },
    { 'ss09',   kStylisticAlternativesType, kStylisticAltNineOnSelector,            kStylisticAltNineOffSelector },
    { 'ss10',   kStylisticAlternativesType, kStylisticAltTenOnSelector,             kStylisticAltTenOffSelector },
    { 'ss11',   kStylisticAlternativesType, kStylisticAltElevenOnSelector,          kStylisticAltElevenOffSelector },
    { 'ss12',   kStylisticAlternativesType, kStylisticAltTwelveOnSelector,          kStylisticAltTwelveOffSelector },
    { 'ss13',   kStylisticAlternativesType, kStylisticAltThirteenOnSelector,        kStylisticAltThirteenOffSelector },
    { 'ss14',   kStylisticAlternativesType, kStylisticAltFourteenOnSelector,        kStylisticAltFourteenOffSelector },
    { 'ss15',   kStylisticAlternativesType, kStylisticAltFifteenOnSelector,         kStylisticAltFifteenOffSelector },
    { 'ss16',   kStylisticAlternativesType, kStylisticAltSixteenOnSelector,         kStylisticAltSixteenOffSelector },
    { 'ss17',   kStylisticAlternativesType, kStylisticAltSeventeenOnSelector,       kStylisticAltSeventeenOffSelector },
    { 'ss18',   kStylisticAlternativesType, kStylisticAltEighteenOnSelector,        kStylisticAltEighteenOffSelector },
    { 'ss19',   kStylisticAlternativesType, kStylisticAltNineteenOnSelector,        kStylisticAltNineteenOffSelector },
    { 'ss20',   kStylisticAlternativesType, kStylisticAltTwentyOnSelector,          kStylisticAltTwentyOffSelector },
    { 'subs',   kVerticalPositionType,      kInferiorsSelector,                     kNormalPositionSelector },
    { 'sups',   kVerticalPositionType,      kSuperiorsSelector,                     kNormalPositionSelector },
    { 'swsh',   kContextualAlternatesType,  kSwashAlternatesOnSelector,             kSwashAlternatesOffSelector },
    { 'titl',   kStyleOptionsType,          kTitlingCapsSelector,                   kNoStyleOptionsSelector },
    { 'tnam',   kCharacterShapeType,        kTraditionalNamesCharactersSelector,    16 },
    { 'tnum',   kNumberSpacingType,         kMonospacedNumbersSelector,             4 },
    { 'trad',   kCharacterShapeType,        kTraditionalCharactersSelector,         16 },
    { 'twid',   kTextSpacingType,           kThirdWidthTextSelector,                7 },
    { 'unic',   kLetterCaseType,            14,                                     15 },
    { 'valt',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'vert',   kVerticalSubstitutionType,  kSubstituteVerticalFormsOnSelector,     kSubstituteVerticalFormsOffSelector },
    { 'vhal',   kTextSpacingType,           kAltHalfWidthTextSelector,              7 },
    { 'vkna',   kAlternateKanaType,         kAlternateVertKanaOnSelector,           kAlternateVertKanaOffSelector },
    { 'vpal',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'vrt2',   kVerticalSubstitutionType,  kSubstituteVerticalFormsOnSelector,     kSubstituteVerticalFormsOffSelector },
    { 'zero',   kTypographicExtrasType,     kSlashedZeroOnSelector,                 kSlashedZeroOffSelector },
};

static int
_hb_feature_mapping_cmp (const void *key_, const void *entry_)
{
  unsigned int key = * (unsigned int *) key_;
  const feature_mapping_t * entry = (const feature_mapping_t *) entry_;
  return key < entry->otFeatureTag ? -1 :
	 key > entry->otFeatureTag ? 1 :
	 0;
}

hb_bool_t
_hb_coretext_shape (hb_shape_plan_t    *shape_plan,
		    hb_font_t          *font,
                    hb_buffer_t        *buffer,
                    const hb_feature_t *features,
                    unsigned int        num_features)
{
  hb_face_t *face = font->face;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);
  hb_coretext_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);

  /*
   * Set up features.
   * (copied + modified from code from hb-uniscribe.cc)
   */
  hb_auto_array_t<feature_record_t> feature_records;
  hb_auto_array_t<range_record_t> range_records;
  if (num_features)
  {
    /* Sort features by start/end events. */
    hb_auto_array_t<feature_event_t> feature_events;
    for (unsigned int i = 0; i < num_features; i++)
    {
      const feature_mapping_t * mapping = (const feature_mapping_t *) bsearch (&features[i].tag,
									       feature_mappings,
									       ARRAY_LENGTH (feature_mappings),
									       sizeof (feature_mappings[0]),
									       _hb_feature_mapping_cmp);
      if (!mapping)
        continue;

      active_feature_t feature;
      feature.rec.feature = mapping->aatFeatureType;
      feature.rec.setting = features[i].value ? mapping->selectorToEnable : mapping->selectorToDisable;
      feature.order = i;

      feature_event_t *event;

      event = feature_events.push ();
      if (unlikely (!event))
	goto fail_features;
      event->index = features[i].start;
      event->start = true;
      event->feature = feature;

      event = feature_events.push ();
      if (unlikely (!event))
	goto fail_features;
      event->index = features[i].end;
      event->start = false;
      event->feature = feature;
    }
    feature_events.sort ();
    /* Add a strategic final event. */
    {
      active_feature_t feature;
      feature.rec.feature = HB_TAG_NONE;
      feature.rec.setting = 0;
      feature.order = num_features + 1;

      feature_event_t *event = feature_events.push ();
      if (unlikely (!event))
	goto fail_features;
      event->index = 0; /* This value does magic. */
      event->start = false;
      event->feature = feature;
    }

    /* Scan events and save features for each range. */
    hb_auto_array_t<active_feature_t> active_features;
    unsigned int last_index = 0;
    for (unsigned int i = 0; i < feature_events.len; i++)
    {
      feature_event_t *event = &feature_events[i];

      if (event->index != last_index)
      {
        /* Save a snapshot of active features and the range. */
	range_record_t *range = range_records.push ();
	if (unlikely (!range))
	  goto fail_features;

	unsigned int offset = feature_records.len;

	if (active_features.len)
	{
	  CFMutableArrayRef features_array = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

	  /* TODO sort and resolve conflicting features? */
	  /* active_features.sort (); */
	  for (unsigned int j = 0; j < active_features.len; j++)
	  {
	    CFStringRef keys[2] = {
	      kCTFontFeatureTypeIdentifierKey,
	      kCTFontFeatureSelectorIdentifierKey
	    };
	    CFNumberRef values[2] = {
	      CFNumberCreate (kCFAllocatorDefault, kCFNumberIntType, &active_features[j].rec.feature),
	      CFNumberCreate (kCFAllocatorDefault, kCFNumberIntType, &active_features[j].rec.setting)
	    };
	    CFDictionaryRef dict = CFDictionaryCreate (kCFAllocatorDefault,
						       (const void **) keys,
						       (const void **) values,
						       2,
						       &kCFTypeDictionaryKeyCallBacks,
						       &kCFTypeDictionaryValueCallBacks);
	    CFRelease (values[0]);
	    CFRelease (values[1]);

	    CFArrayAppendValue (features_array, dict);
	    CFRelease (dict);

	  }

	  CFDictionaryRef attributes = CFDictionaryCreate (kCFAllocatorDefault,
							   (const void **) &kCTFontFeatureSettingsAttribute,
							   (const void **) &features_array,
							   1,
							   &kCFTypeDictionaryKeyCallBacks,
							   &kCFTypeDictionaryValueCallBacks);
	  CFRelease (features_array);

	  CTFontDescriptorRef font_desc = CTFontDescriptorCreateWithAttributes (attributes);
	  CFRelease (attributes);

	  range->font = CTFontCreateCopyWithAttributes (font_data->ct_font, 0.0, NULL, font_desc);

	  CFRelease (font_desc);
	}
	else
	{
	  range->font = NULL;
	}

	range->index_first = last_index;
	range->index_last  = event->index - 1;

	last_index = event->index;
      }

      if (event->start) {
        active_feature_t *feature = active_features.push ();
	if (unlikely (!feature))
	  goto fail_features;
	*feature = event->feature;
      } else {
        active_feature_t *feature = active_features.find (&event->feature);
	if (feature)
	  active_features.remove (feature - active_features.array);
      }
    }

    if (!range_records.len) /* No active feature found. */
      goto fail_features;
  }
  else
  {
  fail_features:
    num_features = 0;
  }

#define FAIL(...) \
  HB_STMT_START { \
    DEBUG_MSG (CORETEXT, NULL, __VA_ARGS__); \
    return false; \
  } HB_STMT_END;

  unsigned int scratch_size;
  hb_buffer_t::scratch_buffer_t *scratch = buffer->get_scratch_buffer (&scratch_size);

#define ALLOCATE_ARRAY(Type, name, len) \
  Type *name = (Type *) scratch; \
  { \
    unsigned int _consumed = DIV_CEIL ((len) * sizeof (Type), sizeof (*scratch)); \
    assert (_consumed <= scratch_size); \
    scratch += _consumed; \
    scratch_size -= _consumed; \
  }

#define utf16_index() var1.u32

  ALLOCATE_ARRAY (UniChar, pchars, buffer->len * 2);

  unsigned int chars_len = 0;
  for (unsigned int i = 0; i < buffer->len; i++) {
    hb_codepoint_t c = buffer->info[i].codepoint;
    buffer->info[i].utf16_index() = chars_len;
    if (likely (c < 0x10000))
      pchars[chars_len++] = c;
    else if (unlikely (c >= 0x110000))
      pchars[chars_len++] = 0xFFFD;
    else {
      pchars[chars_len++] = 0xD800 + ((c - 0x10000) >> 10);
      pchars[chars_len++] = 0xDC00 + ((c - 0x10000) & ((1 << 10) - 1));
    }
  }

#undef utf16_index

  CFStringRef string_ref = CFStringCreateWithCharactersNoCopy (NULL,
                                                               pchars, chars_len,
                                                               kCFAllocatorNull);

  CFMutableAttributedStringRef attr_string = CFAttributedStringCreateMutable (NULL, chars_len);
  CFAttributedStringReplaceString (attr_string, CFRangeMake (0, 0), string_ref);
  CFAttributedStringSetAttribute (attr_string, CFRangeMake (0, chars_len),
				  kCTFontAttributeName, font_data->ct_font);

  if (num_features)
  {
    ALLOCATE_ARRAY (unsigned int, log_clusters, chars_len);

    /* Need log_clusters to assign features. */
    chars_len = 0;
    for (unsigned int i = 0; i < buffer->len; i++)
    {
      hb_codepoint_t c = buffer->info[i].codepoint;
      unsigned int cluster = buffer->info[i].cluster;
      log_clusters[chars_len++] = cluster;
      if (c >= 0x10000 && c < 0x110000)
	log_clusters[chars_len++] = cluster; /* Surrogates. */
    }

    unsigned int start = 0;
    range_record_t *last_range = &range_records[0];
    for (unsigned int k = 0; k < chars_len; k++)
    {
      range_record_t *range = last_range;
      while (log_clusters[k] < range->index_first)
	range--;
      while (log_clusters[k] > range->index_last)
	range++;
      if (range != last_range)
      {
        if (last_range->font)
	  CFAttributedStringSetAttribute (attr_string, CFRangeMake (start, k - start),
					  kCTFontAttributeName, last_range->font);

	start = k;
      }

      last_range = range;
    }
    if (start != chars_len && last_range->font)
      CFAttributedStringSetAttribute (attr_string, CFRangeMake (start, chars_len - start - 1),
				      kCTFontAttributeName, last_range->font);

    for (unsigned int i = 0; i < range_records.len; i++)
      if (range_records[i].font)
	CFRelease (range_records[i].font);
  }

  CTLineRef line = CTLineCreateWithAttributedString (attr_string);
  CFRelease (attr_string);

  CFArrayRef glyph_runs = CTLineGetGlyphRuns (line);
  unsigned int num_runs = CFArrayGetCount (glyph_runs);

  buffer->len = 0;

  const CFRange range_all = CFRangeMake (0, 0);

  for (unsigned int i = 0; i < num_runs; i++)
  {
    CTRunRef run = (CTRunRef) CFArrayGetValueAtIndex (glyph_runs, i);

    /* CoreText does automatic font fallback (AKA "cascading") for  characters
     * not supported by the requested font, and provides no way to turn it off,
     * so we detect if the returned run uses a font other than the requested
     * one and fill in the buffer with .notdef glyphs instead of random glyph
     * indices from a different font.
     */
    CFDictionaryRef attributes = CTRunGetAttributes (run);
    CTFontRef run_ct_font = static_cast<CTFontRef>(CFDictionaryGetValue (attributes, kCTFontAttributeName));

    CFRange range = CTRunGetStringRange (run);
    if (!CFEqual (run_ct_font, font_data->ct_font))
    {
	buffer->ensure (buffer->len + range.length);
	if (buffer->in_error)
	  FAIL ("Buffer resize failed");
	hb_glyph_info_t *info = buffer->info + buffer->len;

	CGGlyph notdef = 0;
	double advance = CTFontGetAdvancesForGlyphs (font_data->ct_font, kCTFontHorizontalOrientation, &notdef, NULL, 1);

        for (CFIndex j = range.location; j < range.location + range.length; j++)
	{
	    UniChar ch = CFStringGetCharacterAtIndex (string_ref, j);
	    if (hb_in_range<UniChar> (ch, 0xDC00, 0xDFFF) && range.location < j)
	    {
	      ch = CFStringGetCharacterAtIndex (string_ref, j - 1);
	      if (hb_in_range<UniChar> (ch, 0xD800, 0xDBFF))
	        /* This is the second of a surrogate pair.  Don't need .notdef
		 * for this one. */
	        continue;
	    }

            info->codepoint = notdef;
	    /* TODO We have to fixup clusters later.  See vis_clusters in
	     * hb-uniscribe.cc for example. */
            info->cluster = j;

            info->mask = advance * 64;
            info->var1.u32 = 0;
            info->var2.u32 = 0;

	    info++;
	    buffer->len++;
        }
        continue;
    }

    /* CoreText throws away the PDF token, while the OpenType backend will add a zero-advance
     * glyph for this. We need to make sure the two produce the same output. */
    UniChar endGlyph = CFStringGetCharacterAtIndex(string_ref, range.location + range.length - 1);
    bool endWithPDF = endGlyph == 0x202c;

    unsigned int num_glyphs = CTRunGetGlyphCount (run);
    if (num_glyphs == 0)
      continue;

    buffer->ensure (buffer->len + num_glyphs + (endWithPDF ? 1 : 0));

    scratch = buffer->get_scratch_buffer (&scratch_size);

    /* Testing indicates that CTRunGetGlyphsPtr, etc (almost?) always
     * succeed, and so copying data to our own buffer will be rare. */

    const CGGlyph* glyphs = CTRunGetGlyphsPtr (run);
    if (!glyphs) {
      ALLOCATE_ARRAY (CGGlyph, glyph_buf, num_glyphs);
      CTRunGetGlyphs (run, range_all, glyph_buf);
      glyphs = glyph_buf;
    }

    const CGPoint* positions = CTRunGetPositionsPtr (run);
    if (!positions) {
      ALLOCATE_ARRAY (CGPoint, position_buf, num_glyphs);
      CTRunGetPositions (run, range_all, position_buf);
      positions = position_buf;
    }

    const CFIndex* string_indices = CTRunGetStringIndicesPtr (run);
    if (!string_indices) {
      ALLOCATE_ARRAY (CFIndex, index_buf, num_glyphs);
      CTRunGetStringIndices (run, range_all, index_buf);
      string_indices = index_buf;
    }

#undef ALLOCATE_ARRAY

    double run_width = CTRunGetTypographicBounds (run, range_all, NULL, NULL, NULL);

    for (unsigned int j = 0; j < num_glyphs; j++) {
      double advance = (j + 1 < num_glyphs ? positions[j + 1].x : positions[0].x + run_width) - positions[j].x;

      hb_glyph_info_t *info = &buffer->info[buffer->len];

      info->codepoint = glyphs[j];
      info->cluster = string_indices[j];

      /* Currently, we do all x-positioning by setting the advance, we never use x-offset. */
      info->mask = advance * 64;
      info->var1.u32 = 0;
      info->var2.u32 = positions[j].y * 64;

      buffer->len++;
    }

    if (endWithPDF) {
        hb_glyph_info_t *info = &buffer->info[buffer->len];

        info->codepoint = 0xffff;
        info->cluster = range.location + range.length - 1;

        /* Currently, we do all x-positioning by setting the advance, we never use x-offset. */
        info->mask = 0;
        info->var1.u32 = 0;
        info->var2.u32 = 0;

        buffer->len++;
    }
  }

  buffer->clear_positions ();

  bool bufferRtl = !HB_DIRECTION_IS_FORWARD (buffer->props.direction);
  bool runRtl = (CTRunGetStatus(static_cast<CTRunRef>(CFArrayGetValueAtIndex(glyph_runs, 0))) & kCTRunStatusRightToLeft);

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; ++i) {
    hb_glyph_info_t *info = &buffer->info[i];
    hb_glyph_position_t *pos = &buffer->pos[i];

    /* TODO vertical */
    pos->x_advance = info->mask;
    pos->x_offset = info->var1.u32;
    pos->y_offset = info->var2.u32;

    if (bufferRtl != runRtl && i < count / 2) {
        unsigned int temp = buffer->info[count - i - 1].cluster;
        buffer->info[count - i - 1].cluster = info->cluster;
        info->cluster = temp;
    }
  }

  /* Fix up clusters so that we never return out-of-order indices;
   * if core text has reordered glyphs, we'll merge them to the
   * beginning of the reordered cluster.
   *
   * This does *not* mean we'll form the same clusters as Uniscribe
   * or the native OT backend, only that the cluster indices will be
   * monotonic in the output buffer. */
  if (HB_DIRECTION_IS_FORWARD (buffer->props.direction)) {
    unsigned int prev_cluster = 0;
    for (unsigned int i = 0; i < count; i++) {
      unsigned int curr_cluster = buffer->info[i].cluster;
      if (curr_cluster < prev_cluster) {
        for (unsigned int j = i; j > 0; j--) {
          if (buffer->info[j - 1].cluster > curr_cluster)
            buffer->info[j - 1].cluster = curr_cluster;
          else
            break;
        }
      }
      prev_cluster = curr_cluster;
    }
  } else {
    unsigned int prev_cluster = (unsigned int)-1;
    for (unsigned int i = 0; i < count; i++) {
      unsigned int curr_cluster = buffer->info[i].cluster;
      if (curr_cluster > prev_cluster) {
        for (unsigned int j = i; j > 0; j--) {
          if (buffer->info[j - 1].cluster < curr_cluster)
            buffer->info[j - 1].cluster = curr_cluster;
          else
            break;
        }
      }
      prev_cluster = curr_cluster;
    }
  }

  CFRelease (string_ref);
  CFRelease (line);

  return true;
}
