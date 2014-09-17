// This is a fuzzing harness for Harfbuzz. Since Harfbuzz's input is generally
// expected to be controlled by a remote party it's a possible vector for
// security issues.
//
// Fuzzing is a black-box testing scheme where the black-box (Harfbuzz's shaping
// engine in this case) is fed random input to see if it will misbehave.
// Misbehaviours can often be turned into security or crash issues.
//
// It's expected that one will generally run this under valgrind in order to get
// better detection of problems.

#include <stdint.h>
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../../src/harfbuzz-shaper.h"
#include "../../src/harfbuzz-global.h"
#include "../../src/harfbuzz-gpos.h"

extern "C" {
#include "../../contrib/harfbuzz-unicode.h"
#include "../../contrib/harfbuzz-freetype.h"
}

static FT_Library freetype;

static FT_Face loadFace(const char *path)
{
  FT_Face face;

  if (FT_New_Face(freetype, path, /* index */ 0, &face))
      return 0;
  return face;
}

static const int kWidth = 100;
static const int kHeight = 100;

static int
usage(const char *argv0) {
  fprintf(stderr, "Usage: %s <TTF file>\n", argv0);
  return 1;
}

int
main(int argc, char **argv) {
  FT_Init_FreeType(&freetype);

  if (argc != 2)
    return usage(argv[0]);

  FT_Face face;
  if (FT_New_Face(freetype, argv[1], 0 /* face index */, &face)) {
    fprintf(stderr, "Failed to load font file\n");
    return 1;
  }

  HB_Face hbFace = HB_NewFace(face, hb_freetype_table_sfnt_get);

  HB_FontRec hbFont;
  hbFont.klass = &hb_freetype_class;
  hbFont.userData = face;
  hbFont.x_ppem  = face->size->metrics.x_ppem;
  hbFont.y_ppem  = face->size->metrics.y_ppem;
  hbFont.x_scale = face->size->metrics.x_scale;
  hbFont.y_scale = face->size->metrics.y_scale;

  // This is the maximum number of bytes of input which we'll feed to Harfbuzz
  // in one shot. We also overload it and make it the size of the output arrays
  // as well. (Must be a power of two.)
  static const unsigned kMaxInputBytes = 1024;
  uint8_t str[kMaxInputBytes];

  HB_ShaperItem shaper_item;
  shaper_item.kerning_applied = false;
  shaper_item.string = (HB_UChar16 *) str;
  shaper_item.stringLength = 0;
  shaper_item.item.bidiLevel = 0;
  shaper_item.shaperFlags = 0;
  shaper_item.font = &hbFont;
  shaper_item.face = hbFace;
  shaper_item.glyphIndicesPresent = false;
  shaper_item.initialGlyphCount = 0;

  HB_Glyph out_glyphs[kMaxInputBytes];
  HB_GlyphAttributes out_attrs[kMaxInputBytes];
  HB_Fixed out_advs[kMaxInputBytes];
  HB_FixedPoint out_offsets[kMaxInputBytes];
  unsigned short out_logClusters[kMaxInputBytes];

  shaper_item.glyphs = out_glyphs;
  shaper_item.attributes = out_attrs;
  shaper_item.advances = out_advs;
  shaper_item.offsets = out_offsets;
  shaper_item.log_clusters = out_logClusters;
  shaper_item.num_glyphs = kMaxInputBytes;

  FILE *urandom = fopen("/dev/urandom", "rb");
  if (!urandom) {
    fprintf(stderr, "Cannot open /dev/urandom\n");
    return 1;
  }

  for (;;) {
    uint16_t len;
    fread(&len, sizeof(len), 1, urandom);
    len &= (kMaxInputBytes - 1);
    len &= ~1;
    fread(str, len, 1, urandom);

    ssize_t iterator = 0;

    for (;;) {
      if (!hb_utf16_script_run_next(NULL, &shaper_item.item, (uint16_t *) str, len >> 1, &iterator))
        break;

      HB_ShapeItem(&shaper_item);
    }
  }

  HB_FreeFace(hbFace);
}
