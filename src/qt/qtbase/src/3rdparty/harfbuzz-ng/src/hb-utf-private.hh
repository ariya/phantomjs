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

#ifndef HB_UTF_PRIVATE_HH
#define HB_UTF_PRIVATE_HH

#include "hb-private.hh"


/* UTF-8 */

#define HB_UTF8_COMPUTE(Char, Mask, Len) \
  if (Char < 128) { Len = 1; Mask = 0x7f; } \
  else if ((Char & 0xe0) == 0xc0) { Len = 2; Mask = 0x1f; } \
  else if ((Char & 0xf0) == 0xe0) { Len = 3; Mask = 0x0f; } \
  else if ((Char & 0xf8) == 0xf0) { Len = 4; Mask = 0x07; } \
  else Len = 0;

static inline const uint8_t *
hb_utf_next (const uint8_t *text,
	     const uint8_t *end,
	     hb_codepoint_t *unicode)
{
  hb_codepoint_t c = *text, mask;
  unsigned int len;

  /* TODO check for overlong sequences? */

  HB_UTF8_COMPUTE (c, mask, len);
  if (unlikely (!len || (unsigned int) (end - text) < len)) {
    *unicode = -1;
    return text + 1;
  } else {
    hb_codepoint_t result;
    unsigned int i;
    result = c & mask;
    for (i = 1; i < len; i++)
      {
	if (unlikely ((text[i] & 0xc0) != 0x80))
	  {
	    *unicode = -1;
	    return text + 1;
	  }
	result <<= 6;
	result |= (text[i] & 0x3f);
      }
    *unicode = result;
    return text + len;
  }
}

static inline const uint8_t *
hb_utf_prev (const uint8_t *text,
	     const uint8_t *start,
	     hb_codepoint_t *unicode)
{
  const uint8_t *end = text--;
  while (start < text && (*text & 0xc0) == 0x80 && end - text < 4)
    text--;

  hb_codepoint_t c = *text, mask;
  unsigned int len;

  /* TODO check for overlong sequences? */

  HB_UTF8_COMPUTE (c, mask, len);
  if (unlikely (!len || (unsigned int) (end - text) != len)) {
    *unicode = -1;
    return end - 1;
  } else {
    hb_codepoint_t result;
    unsigned int i;
    result = c & mask;
    for (i = 1; i < len; i++)
      {
	result <<= 6;
	result |= (text[i] & 0x3f);
      }
    *unicode = result;
    return text;
  }
}


static inline unsigned int
hb_utf_strlen (const uint8_t *text)
{
  return strlen ((const char *) text);
}


/* UTF-16 */

static inline const uint16_t *
hb_utf_next (const uint16_t *text,
	     const uint16_t *end,
	     hb_codepoint_t *unicode)
{
  hb_codepoint_t c = *text++;

  if (unlikely (hb_in_range<hb_codepoint_t> (c, 0xd800, 0xdbff)))
  {
    /* high surrogate */
    hb_codepoint_t l;
    if (text < end && ((l = *text), likely (hb_in_range<hb_codepoint_t> (l, 0xdc00, 0xdfff))))
    {
      /* low surrogate */
      *unicode = (c << 10) + l - ((0xd800 << 10) - 0x10000 + 0xdc00);
       text++;
    } else
      *unicode = -1;
  } else
    *unicode = c;

  return text;
}

static inline const uint16_t *
hb_utf_prev (const uint16_t *text,
	     const uint16_t *start,
	     hb_codepoint_t *unicode)
{
  hb_codepoint_t c = *--text;

  if (unlikely (hb_in_range<hb_codepoint_t> (c, 0xdc00, 0xdfff)))
  {
    /* low surrogate */
    hb_codepoint_t h;
    if (start < text && ((h = *(text - 1)), likely (hb_in_range<hb_codepoint_t> (h, 0xd800, 0xdbff))))
    {
      /* high surrogate */
      *unicode = (h << 10) + c - ((0xd800 << 10) - 0x10000 + 0xdc00);
       text--;
    } else
      *unicode = -1;
  } else
    *unicode = c;

  return text;
}


static inline unsigned int
hb_utf_strlen (const uint16_t *text)
{
  unsigned int l = 0;
  while (*text++) l++;
  return l;
}


/* UTF-32 */

static inline const uint32_t *
hb_utf_next (const uint32_t *text,
	     const uint32_t *end HB_UNUSED,
	     hb_codepoint_t *unicode)
{
  *unicode = *text++;
  return text;
}

static inline const uint32_t *
hb_utf_prev (const uint32_t *text,
	     const uint32_t *start HB_UNUSED,
	     hb_codepoint_t *unicode)
{
  *unicode = *--text;
  return text;
}

static inline unsigned int
hb_utf_strlen (const uint32_t *text)
{
  unsigned int l = 0;
  while (*text++) l++;
  return l;
}


#endif /* HB_UTF_PRIVATE_HH */
