/*
 **********************************************************************
 *   Copyright (C) 1997-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *
 * File USCRIPT.H
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   07/06/2001    Ram         Creation.
 ******************************************************************************
 */

#ifndef USCRIPT_H
#define USCRIPT_H
#include "unicode/utypes.h"

/**
 * \file
 * \brief C API: Unicode Script Information
 */
 
/**
 * Constants for ISO 15924 script codes.
 *
 * Many of these script codes - those from Unicode's ScriptNames.txt -
 * are character property values for Unicode's Script property.
 * See UAX #24 Script Names (http://www.unicode.org/reports/tr24/).
 *
 * Starting with ICU 3.6, constants for most ISO 15924 script codes
 * are included (currently excluding private-use codes Qaaa..Qabx).
 * For scripts for which there are codes in ISO 15924 but which are not
 * used in the Unicode Character Database (UCD), there are no Unicode characters
 * associated with those scripts.
 *
 * For example, there are no characters that have a UCD script code of
 * Hans or Hant. All Han ideographs have the Hani script code.
 * The Hans and Hant script codes are used with CLDR data.
 *
 * ISO 15924 script codes are included for use with CLDR and similar.
 *
 * @stable ICU 2.2
 */
typedef enum UScriptCode {
      USCRIPT_INVALID_CODE = -1,
      USCRIPT_COMMON       =  0,  /* Zyyy */
      USCRIPT_INHERITED    =  1,  /* Zinh */ /* "Code for inherited script", for non-spacing combining marks; also Qaai */
      USCRIPT_ARABIC       =  2,  /* Arab */
      USCRIPT_ARMENIAN     =  3,  /* Armn */
      USCRIPT_BENGALI      =  4,  /* Beng */
      USCRIPT_BOPOMOFO     =  5,  /* Bopo */
      USCRIPT_CHEROKEE     =  6,  /* Cher */
      USCRIPT_COPTIC       =  7,  /* Copt */
      USCRIPT_CYRILLIC     =  8,  /* Cyrl */
      USCRIPT_DESERET      =  9,  /* Dsrt */
      USCRIPT_DEVANAGARI   = 10,  /* Deva */
      USCRIPT_ETHIOPIC     = 11,  /* Ethi */
      USCRIPT_GEORGIAN     = 12,  /* Geor */
      USCRIPT_GOTHIC       = 13,  /* Goth */
      USCRIPT_GREEK        = 14,  /* Grek */
      USCRIPT_GUJARATI     = 15,  /* Gujr */
      USCRIPT_GURMUKHI     = 16,  /* Guru */
      USCRIPT_HAN          = 17,  /* Hani */
      USCRIPT_HANGUL       = 18,  /* Hang */
      USCRIPT_HEBREW       = 19,  /* Hebr */
      USCRIPT_HIRAGANA     = 20,  /* Hira */
      USCRIPT_KANNADA      = 21,  /* Knda */
      USCRIPT_KATAKANA     = 22,  /* Kana */
      USCRIPT_KHMER        = 23,  /* Khmr */
      USCRIPT_LAO          = 24,  /* Laoo */
      USCRIPT_LATIN        = 25,  /* Latn */
      USCRIPT_MALAYALAM    = 26,  /* Mlym */
      USCRIPT_MONGOLIAN    = 27,  /* Mong */
      USCRIPT_MYANMAR      = 28,  /* Mymr */
      USCRIPT_OGHAM        = 29,  /* Ogam */
      USCRIPT_OLD_ITALIC   = 30,  /* Ital */
      USCRIPT_ORIYA        = 31,  /* Orya */
      USCRIPT_RUNIC        = 32,  /* Runr */
      USCRIPT_SINHALA      = 33,  /* Sinh */
      USCRIPT_SYRIAC       = 34,  /* Syrc */
      USCRIPT_TAMIL        = 35,  /* Taml */
      USCRIPT_TELUGU       = 36,  /* Telu */
      USCRIPT_THAANA       = 37,  /* Thaa */
      USCRIPT_THAI         = 38,  /* Thai */
      USCRIPT_TIBETAN      = 39,  /* Tibt */
      /** Canadian_Aboriginal script. @stable ICU 2.6 */
      USCRIPT_CANADIAN_ABORIGINAL = 40,  /* Cans */
      /** Canadian_Aboriginal script (alias). @stable ICU 2.2 */
      USCRIPT_UCAS         = USCRIPT_CANADIAN_ABORIGINAL,
      USCRIPT_YI           = 41,  /* Yiii */
      USCRIPT_TAGALOG      = 42,  /* Tglg */
      USCRIPT_HANUNOO      = 43,  /* Hano */
      USCRIPT_BUHID        = 44,  /* Buhd */
      USCRIPT_TAGBANWA     = 45,  /* Tagb */

      /* New scripts in Unicode 4 @stable ICU 2.6 */
      USCRIPT_BRAILLE      = 46,  /* Brai */
      USCRIPT_CYPRIOT      = 47,  /* Cprt */
      USCRIPT_LIMBU        = 48,  /* Limb */
      USCRIPT_LINEAR_B     = 49,  /* Linb */
      USCRIPT_OSMANYA      = 50,  /* Osma */
      USCRIPT_SHAVIAN      = 51,  /* Shaw */
      USCRIPT_TAI_LE       = 52,  /* Tale */
      USCRIPT_UGARITIC     = 53,  /* Ugar */

      /** New script code in Unicode 4.0.1 @stable ICU 3.0 */
      USCRIPT_KATAKANA_OR_HIRAGANA = 54,/*Hrkt */

      /* New scripts in Unicode 4.1 @stable ICU 3.4 */
      USCRIPT_BUGINESE      = 55, /* Bugi */
      USCRIPT_GLAGOLITIC    = 56, /* Glag */
      USCRIPT_KHAROSHTHI    = 57, /* Khar */
      USCRIPT_SYLOTI_NAGRI  = 58, /* Sylo */
      USCRIPT_NEW_TAI_LUE   = 59, /* Talu */
      USCRIPT_TIFINAGH      = 60, /* Tfng */
      USCRIPT_OLD_PERSIAN   = 61, /* Xpeo */

      /* New script codes from ISO 15924 @stable ICU 3.6 */
      USCRIPT_BALINESE                      = 62, /* Bali */
      USCRIPT_BATAK                         = 63, /* Batk */
      USCRIPT_BLISSYMBOLS                   = 64, /* Blis */
      USCRIPT_BRAHMI                        = 65, /* Brah */
      USCRIPT_CHAM                          = 66, /* Cham */
      USCRIPT_CIRTH                         = 67, /* Cirt */
      USCRIPT_OLD_CHURCH_SLAVONIC_CYRILLIC  = 68, /* Cyrs */
      USCRIPT_DEMOTIC_EGYPTIAN              = 69, /* Egyd */
      USCRIPT_HIERATIC_EGYPTIAN             = 70, /* Egyh */
      USCRIPT_EGYPTIAN_HIEROGLYPHS          = 71, /* Egyp */
      USCRIPT_KHUTSURI                      = 72, /* Geok */
      USCRIPT_SIMPLIFIED_HAN                = 73, /* Hans */
      USCRIPT_TRADITIONAL_HAN               = 74, /* Hant */
      USCRIPT_PAHAWH_HMONG                  = 75, /* Hmng */
      USCRIPT_OLD_HUNGARIAN                 = 76, /* Hung */
      USCRIPT_HARAPPAN_INDUS                = 77, /* Inds */
      USCRIPT_JAVANESE                      = 78, /* Java */
      USCRIPT_KAYAH_LI                      = 79, /* Kali */
      USCRIPT_LATIN_FRAKTUR                 = 80, /* Latf */
      USCRIPT_LATIN_GAELIC                  = 81, /* Latg */
      USCRIPT_LEPCHA                        = 82, /* Lepc */
      USCRIPT_LINEAR_A                      = 83, /* Lina */
      /** @stable ICU 4.6 */
      USCRIPT_MANDAIC                       = 84, /* Mand */
      /** @stable ICU 3.6 */
      USCRIPT_MANDAEAN                      = USCRIPT_MANDAIC,
      USCRIPT_MAYAN_HIEROGLYPHS             = 85, /* Maya */
      /** @stable ICU 4.6 */
      USCRIPT_MEROITIC_HIEROGLYPHS          = 86, /* Mero */
      /** @stable ICU 3.6 */
      USCRIPT_MEROITIC                      = USCRIPT_MEROITIC_HIEROGLYPHS,
      USCRIPT_NKO                           = 87, /* Nkoo */
      USCRIPT_ORKHON                        = 88, /* Orkh */
      USCRIPT_OLD_PERMIC                    = 89, /* Perm */
      USCRIPT_PHAGS_PA                      = 90, /* Phag */
      USCRIPT_PHOENICIAN                    = 91, /* Phnx */
      USCRIPT_PHONETIC_POLLARD              = 92, /* Plrd */
      USCRIPT_RONGORONGO                    = 93, /* Roro */
      USCRIPT_SARATI                        = 94, /* Sara */
      USCRIPT_ESTRANGELO_SYRIAC             = 95, /* Syre */
      USCRIPT_WESTERN_SYRIAC                = 96, /* Syrj */
      USCRIPT_EASTERN_SYRIAC                = 97, /* Syrn */
      USCRIPT_TENGWAR                       = 98, /* Teng */
      USCRIPT_VAI                           = 99, /* Vaii */
      USCRIPT_VISIBLE_SPEECH                = 100,/* Visp */
      USCRIPT_CUNEIFORM                     = 101,/* Xsux */
      USCRIPT_UNWRITTEN_LANGUAGES           = 102,/* Zxxx */
      USCRIPT_UNKNOWN                       = 103,/* Zzzz */ /* Unknown="Code for uncoded script", for unassigned code points */

      /* New script codes from ISO 15924 @stable ICU 3.8 */
      USCRIPT_CARIAN                        = 104,/* Cari */
      USCRIPT_JAPANESE                      = 105,/* Jpan */
      USCRIPT_LANNA                         = 106,/* Lana */
      USCRIPT_LYCIAN                        = 107,/* Lyci */
      USCRIPT_LYDIAN                        = 108,/* Lydi */
      USCRIPT_OL_CHIKI                      = 109,/* Olck */
      USCRIPT_REJANG                        = 110,/* Rjng */
      USCRIPT_SAURASHTRA                    = 111,/* Saur */
      USCRIPT_SIGN_WRITING                  = 112,/* Sgnw */
      USCRIPT_SUNDANESE                     = 113,/* Sund */
      USCRIPT_MOON                          = 114,/* Moon */
      USCRIPT_MEITEI_MAYEK                  = 115,/* Mtei */

      /* New script codes from ISO 15924 @stable ICU 4.0 */
      USCRIPT_IMPERIAL_ARAMAIC              = 116,/* Armi */
      USCRIPT_AVESTAN                       = 117,/* Avst */
      USCRIPT_CHAKMA                        = 118,/* Cakm */
      USCRIPT_KOREAN                        = 119,/* Kore */
      USCRIPT_KAITHI                        = 120,/* Kthi */
      USCRIPT_MANICHAEAN                    = 121,/* Mani */
      USCRIPT_INSCRIPTIONAL_PAHLAVI         = 122,/* Phli */
      USCRIPT_PSALTER_PAHLAVI               = 123,/* Phlp */
      USCRIPT_BOOK_PAHLAVI                  = 124,/* Phlv */
      USCRIPT_INSCRIPTIONAL_PARTHIAN        = 125,/* Prti */
      USCRIPT_SAMARITAN                     = 126,/* Samr */
      USCRIPT_TAI_VIET                      = 127,/* Tavt */
      USCRIPT_MATHEMATICAL_NOTATION         = 128,/* Zmth */
      USCRIPT_SYMBOLS                       = 129,/* Zsym */

      /* New script codes from ISO 15924 @stable ICU 4.4 */
      USCRIPT_BAMUM                         = 130,/* Bamu */
      USCRIPT_LISU                          = 131,/* Lisu */
      USCRIPT_NAKHI_GEBA                    = 132,/* Nkgb */
      USCRIPT_OLD_SOUTH_ARABIAN             = 133,/* Sarb */

      /* New script codes from ISO 15924 @stable ICU 4.6 */
      USCRIPT_BASSA_VAH                     = 134,/* Bass */
      USCRIPT_DUPLOYAN_SHORTAND             = 135,/* Dupl */
      USCRIPT_ELBASAN                       = 136,/* Elba */
      USCRIPT_GRANTHA                       = 137,/* Gran */
      USCRIPT_KPELLE                        = 138,/* Kpel */
      USCRIPT_LOMA                          = 139,/* Loma */
      USCRIPT_MENDE                         = 140,/* Mend */
      USCRIPT_MEROITIC_CURSIVE              = 141,/* Merc */
      USCRIPT_OLD_NORTH_ARABIAN             = 142,/* Narb */
      USCRIPT_NABATAEAN                     = 143,/* Nbat */
      USCRIPT_PALMYRENE                     = 144,/* Palm */
      USCRIPT_SINDHI                        = 145,/* Sind */
      USCRIPT_WARANG_CITI                   = 146,/* Wara */

      /* Private use codes from Qaaa - Qabx are not supported */
      USCRIPT_CODE_LIMIT    = 147
} UScriptCode;

/**
 * Gets script codes associated with the given locale or ISO 15924 abbreviation or name. 
 * Fills in USCRIPT_MALAYALAM given "Malayam" OR "Mlym".
 * Fills in USCRIPT_LATIN given "en" OR "en_US" 
 * If required capacity is greater than capacity of the destination buffer then the error code
 * is set to U_BUFFER_OVERFLOW_ERROR and the required capacity is returned
 *
 * <p>Note: To search by short or long script alias only, use
 * u_getPropertyValueEnum(UCHAR_SCRIPT, alias) instead.  This does
 * a fast lookup with no access of the locale data.
 * @param nameOrAbbrOrLocale name of the script, as given in
 * PropertyValueAliases.txt, or ISO 15924 code or locale
 * @param fillIn the UScriptCode buffer to fill in the script code
 * @param capacity the capacity (size) fo UScriptCode buffer passed in.
 * @param err the error status code.
 * @return The number of script codes filled in the buffer passed in 
 * @stable ICU 2.4
 */
U_STABLE int32_t  U_EXPORT2 
uscript_getCode(const char* nameOrAbbrOrLocale,UScriptCode* fillIn,int32_t capacity,UErrorCode *err);

/**
 * Gets a script name associated with the given script code. 
 * Returns  "Malayam" given USCRIPT_MALAYALAM
 * @param scriptCode UScriptCode enum
 * @return script long name as given in
 * PropertyValueAliases.txt, or NULL if scriptCode is invalid
 * @stable ICU 2.4
 */
U_STABLE const char*  U_EXPORT2 
uscript_getName(UScriptCode scriptCode);

/**
 * Gets a script name associated with the given script code. 
 * Returns  "Mlym" given USCRIPT_MALAYALAM
 * @param scriptCode UScriptCode enum
 * @return script abbreviated name as given in
 * PropertyValueAliases.txt, or NULL if scriptCode is invalid
 * @stable ICU 2.4
 */
U_STABLE const char*  U_EXPORT2 
uscript_getShortName(UScriptCode scriptCode);

/**
 * Gets the script code associated with the given codepoint.
 * Returns USCRIPT_MALAYALAM given 0x0D02 
 * @param codepoint UChar32 codepoint
 * @param err the error status code.
 * @return The UScriptCode, or 0 if codepoint is invalid 
 * @stable ICU 2.4
 */
U_STABLE UScriptCode  U_EXPORT2 
uscript_getScript(UChar32 codepoint, UErrorCode *err);

/**
 * Is code point c used in script sc?
 * That is, does code point c have the Script property value sc,
 * or do code point c's Script_Extensions include script code sc?
 *
 * Some characters are commonly used in multiple scripts.
 * For more information, see UAX #24: http://www.unicode.org/reports/tr24/.
 *
 * The Script_Extensions property is provisional. It may be modified or removed
 * in future versions of the Unicode Standard, and thus in ICU.
 * @param c code point
 * @param sc script code
 * @return TRUE if Script(c)==sc or sc is in Script_Extensions(c)
 * @draft ICU 4.6
 */
U_DRAFT UBool U_EXPORT2
uscript_hasScript(UChar32 c, UScriptCode sc);

/**
 * Writes code point c's Script_Extensions as a list of UScriptCode values
 * to the output scripts array.
 *
 * Some characters are commonly used in multiple scripts.
 * For more information, see UAX #24: http://www.unicode.org/reports/tr24/.
 *
 * If there are more than capacity script codes to be written, then
 * U_BUFFER_OVERFLOW_ERROR is set and the number of Script_Extensions is returned.
 * (Usual ICU buffer handling behavior.)
 *
 * The Script_Extensions property is provisional. It may be modified or removed
 * in future versions of the Unicode Standard, and thus in ICU.
 * @param c code point
 * @param scripts output script code array
 * @param capacity capacity of the scripts array
 * @param errorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return number of script codes in c's Script_Extensions,
 *         written to scripts unless U_BUFFER_OVERFLOW_ERROR indicates insufficient capacity
 * @draft ICU 4.6
 */
U_DRAFT int32_t U_EXPORT2
uscript_getScriptExtensions(UChar32 c,
                            UScriptCode *scripts, int32_t capacity,
                            UErrorCode *pErrorCode);

#endif
