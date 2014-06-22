/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf  */
/* Computed positions: -k'1-2,5' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif


#include "xkbcomp-priv.h"
#include "parser-priv.h"

static unsigned int
keyword_gperf_hash(const char *str, unsigned int len);

static const struct keyword_tok *
keyword_gperf_lookup(const char *str, unsigned int len);
struct keyword_tok { int name; int tok; };
#include <string.h>
/* maximum key range = 70, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRCMP
#define GPERF_CASE_STRCMP 1
static int
gperf_case_strcmp (register const char *s1, register const char *s2)
{
  for (;;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        continue;
      return (int)c1 - (int)c2;
    }
}
#endif

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
keyword_gperf_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73,  0, 73,  5, 36,  0,
      10,  1, 15, 15, 73,  0, 10, 20, 35, 20,
      50, 73, 10, 10,  5,  0, 15, 73,  0, 15,
      73, 73, 73, 73, 73, 73, 73,  0, 73,  5,
      36,  0, 10,  1, 15, 15, 73,  0, 10, 20,
      35, 20, 50, 73, 10, 10,  5,  0, 15, 73,
       0, 15, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73, 73, 73, 73, 73,
      73, 73, 73, 73, 73, 73
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct stringpool_t
  {
    char stringpool_str3[sizeof("key")];
    char stringpool_str4[sizeof("keys")];
    char stringpool_str7[sizeof("augment")];
    char stringpool_str9[sizeof("text")];
    char stringpool_str10[sizeof("xkb_keymap")];
    char stringpool_str11[sizeof("keypad_keys")];
    char stringpool_str12[sizeof("xkb_keycodes")];
    char stringpool_str13[sizeof("xkb_geometry")];
    char stringpool_str14[sizeof("xkb_types")];
    char stringpool_str15[sizeof("xkb_compat")];
    char stringpool_str17[sizeof("replace")];
    char stringpool_str19[sizeof("xkb_compat_map")];
    char stringpool_str20[sizeof("xkb_layout")];
    char stringpool_str21[sizeof("xkb_symbols")];
    char stringpool_str22[sizeof("xkb_compatibility")];
    char stringpool_str23[sizeof("xkb_semantics")];
    char stringpool_str24[sizeof("type")];
    char stringpool_str25[sizeof("alias")];
    char stringpool_str26[sizeof("xkb_compatibility_map")];
    char stringpool_str27[sizeof("alphanumeric_keys")];
    char stringpool_str28[sizeof("function_keys")];
    char stringpool_str29[sizeof("alternate")];
    char stringpool_str30[sizeof("shape")];
    char stringpool_str31[sizeof("action")];
    char stringpool_str32[sizeof("section")];
    char stringpool_str33[sizeof("row")];
    char stringpool_str34[sizeof("logo")];
    char stringpool_str35[sizeof("alternate_group")];
    char stringpool_str36[sizeof("hidden")];
    char stringpool_str37[sizeof("virtual")];
    char stringpool_str42[sizeof("outline")];
    char stringpool_str43[sizeof("default")];
    char stringpool_str46[sizeof("modmap")];
    char stringpool_str47[sizeof("virtual_modifiers")];
    char stringpool_str52[sizeof("overlay")];
    char stringpool_str53[sizeof("override")];
    char stringpool_str57[sizeof("include")];
    char stringpool_str62[sizeof("modifier_map")];
    char stringpool_str63[sizeof("modifier_keys")];
    char stringpool_str64[sizeof("indicator")];
    char stringpool_str66[sizeof("group")];
    char stringpool_str67[sizeof("mod_map")];
    char stringpool_str69[sizeof("interpret")];
    char stringpool_str71[sizeof("solid")];
    char stringpool_str72[sizeof("partial")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "key",
    "keys",
    "augment",
    "text",
    "xkb_keymap",
    "keypad_keys",
    "xkb_keycodes",
    "xkb_geometry",
    "xkb_types",
    "xkb_compat",
    "replace",
    "xkb_compat_map",
    "xkb_layout",
    "xkb_symbols",
    "xkb_compatibility",
    "xkb_semantics",
    "type",
    "alias",
    "xkb_compatibility_map",
    "alphanumeric_keys",
    "function_keys",
    "alternate",
    "shape",
    "action",
    "section",
    "row",
    "logo",
    "alternate_group",
    "hidden",
    "virtual",
    "outline",
    "default",
    "modmap",
    "virtual_modifiers",
    "overlay",
    "override",
    "include",
    "modifier_map",
    "modifier_keys",
    "indicator",
    "group",
    "mod_map",
    "interpret",
    "solid",
    "partial"
  };
#define stringpool ((const char *) &stringpool_contents)
#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct keyword_tok *
keyword_gperf_lookup (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 45,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 21,
      MIN_HASH_VALUE = 3,
      MAX_HASH_VALUE = 72
    };

  static const struct keyword_tok wordlist[] =
    {
      {-1}, {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str3,                    KEY},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str4,                   KEYS},
      {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str7,                AUGMENT},
      {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str9,                   TEXT},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str10,             XKB_KEYMAP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str11,            KEYPAD_KEYS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str12,           XKB_KEYCODES},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str13,           XKB_GEOMETRY},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str14,              XKB_TYPES},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str15,             XKB_COMPATMAP},
      {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str17,                REPLACE},
      {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str19,         XKB_COMPATMAP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str20,             XKB_LAYOUT},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str21,            XKB_SYMBOLS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str22,      XKB_COMPATMAP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str23,          XKB_SEMANTICS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str24,                   TYPE},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str25,                  ALIAS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str26,  XKB_COMPATMAP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str27,      ALPHANUMERIC_KEYS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str28,          FUNCTION_KEYS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str29,              ALTERNATE},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str30,                  SHAPE},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str31,                 ACTION_TOK},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str32,                SECTION},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str33,                    ROW},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str34,                   LOGO},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str35,        ALTERNATE_GROUP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str36,                 HIDDEN},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str37,                VIRTUAL},
      {-1}, {-1}, {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str42,                OUTLINE},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str43,                DEFAULT},
      {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str46,                 MODIFIER_MAP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str47,      VIRTUAL_MODS},
      {-1}, {-1}, {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str52,                OVERLAY},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str53,               OVERRIDE},
      {-1}, {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str57,                INCLUDE},
      {-1}, {-1}, {-1}, {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str62,           MODIFIER_MAP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str63,          MODIFIER_KEYS},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str64,              INDICATOR},
      {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str66,                  GROUP},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str67,                MODIFIER_MAP},
      {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str69,              INTERPRET},
      {-1},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str71,                  SOLID},
      {(int)(long)&((struct stringpool_t *)0)->stringpool_str72,                PARTIAL}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = keyword_gperf_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int o = wordlist[key].name;
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strcmp (str, s))
                return &wordlist[key];
            }
        }
    }
  return 0;
}


int
keyword_to_token(const char *string, unsigned int len)
{
    const struct keyword_tok *kt = keyword_gperf_lookup(string, len);
    if (!kt)
        return -1;
    return kt->tok;
}
