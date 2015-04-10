#ifndef __XCB_BITOPS_H__
#define __XCB_BITOPS_H__

/* Copyright (C) 2007 Bart Massey
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

#include <assert.h>
#include <inttypes.h>
#include <X11/Xfuncproto.h>

/**
 * @defgroup xcb__bitops XCB Bit Operations
 *
 * Inline functions for common bit ops used in XCB and elsewhere.
 *
 * @{
 */


/**
 * Create a low-order bitmask.
 * @param n Mask size.
 * @return Mask.
 *
 * Create a bitmask with the lower @p n bits set and the
 * rest of the word clear.
 * @ingroup xcb__bitops
 */
_X_INLINE static uint32_t
xcb_mask(uint32_t n)
{
    return n == 32 ? ~0 : (1 << n) - 1;
}


/**
 * Population count.
 * @param n Integer representing a bitset.
 * @return Number of 1 bits in the bitset.
 *
 * This is a reasonably fast algorithm for counting the bits
 * in a 32-bit word.  Currently a classic binary
 * divide-and-conquer popcount: popcount_2() from
 * http://en.wikipedia.org/wiki/Hamming_weight.
 * @ingroup xcb__bitops
 */


/* 15 ops, 3 long immediates, 14 stages, 9 alu ops, 9 alu stages */
_X_INLINE static uint32_t
xcb_popcount(uint32_t x)
{
    uint32_t m1 = 0x55555555;
    uint32_t m2 = 0x33333333;
    uint32_t m4 = 0x0f0f0f0f;
    x -= (x >> 1) & m1;
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    x += x >> 8;
    return (x + (x >> 16)) & 0x3f;
}


/**
 * Round up to the next power-of-two unit size.
 * @param base Number to be rounded up.
 * @param pad Multiple to be rounded to; must be a power of two.
 * @return Rounded-up number.
 *
 * Rounds @p base up to a multiple of @p pad, where @p pad
 * is a power of two.  The more general case is handled by
 * xcb_roundup().
 * @ingroup xcb__bitops
 */
_X_INLINE static uint32_t
xcb_roundup_2 (uint32_t base, uint32_t pad)
{
    return (base + pad - 1) & -pad;
}

/**
 * Round down to the next power-of-two unit size.
 * @param base Number to be rounded down.
 * @param pad Multiple to be rounded to; must be a power of two.
 * @return Rounded-down number.
 *
 * Rounds @p base down to a multiple of @p pad, where @p pad
 * is a power of two.  The more general case is handled by
 * xcb_rounddown().
 * @ingroup xcb__bitops
 */
_X_INLINE static uint32_t
xcb_rounddown_2 (uint32_t base, uint32_t pad)
{
    return base & -pad;
}

/**
 * Round up to the next unit size.
 * @param base Number to be rounded up.
 * @param pad Multiple to be rounded to.
 * @return Rounded-up number.
 *
 * This is a general routine for rounding @p base up
 * to a multiple of @p pad.  If you know that @p pad
 * is a power of two, you should probably call xcb_roundup_2()
 * instead.
 * @ingroup xcb__bitops
 */
_X_INLINE static uint32_t
xcb_roundup (uint32_t base, uint32_t pad)
{
    uint32_t b = base + pad - 1;
    /* faster if pad is a power of two */
    if (((pad - 1) & pad) == 0)
	return b & -pad;
    return b - b % pad;
}


/**
 * Round down to the next unit size.
 * @param base Number to be rounded down.
 * @param pad Multiple to be rounded to.
 * @return Rounded-down number.
 *
 * This is a general routine for rounding @p base down
 * to a multiple of @p pad.  If you know that @p pad
 * is a power of two, you should probably call xcb_rounddown_2()
 * instead.
 * @ingroup xcb__bitops
 */
_X_INLINE static uint32_t
xcb_rounddown (uint32_t base, uint32_t pad)
{
    /* faster if pad is a power of two */
    if (((pad - 1) & pad) == 0)
	return base & -pad;
    return base - base % pad;
}


/**
 * Reverse bits of word.
 * @param x Target word.
 * @param n Number of low-order bits to reverse.
 * @return Word with low @p n bits reversed, all others 0.
 *
 * Reverses the bottom @p n bits of @p x.
 * @ingroup xcb__bitops
 */
_X_INLINE static uint32_t
xcb_bit_reverse(uint32_t x, uint8_t n) {
    uint32_t m1 = 0x00ff00ff;
    uint32_t m2 = 0x0f0f0f0f;
    uint32_t m3 = 0x33333333;
    uint32_t m4 = 0x55555555;
    x = ((x << 16) | (x >> 16));
    x = ((x & m1) << 8) | ((x >> 8) & m1);
    x = ((x & m2) << 4) | ((x >> 4) & m2);
    x = ((x & m3) << 2) | ((x >> 2) & m3);
    x = ((x & m4) << 1) | ((x >> 1) & m4);
    x >>= 32 - n;
    return x;
}


/**
 * Host byte order.
 * @return The byte order of the host.
 *
 * Tests the host's byte order and returns either
 * XCB_IMAGE_ORDER_MSB_FIRST or XCB_IMAGE_ORDER_LSB_FIRST
 * as appropriate.
 * @ingroup xcb__bitops
 */
_X_INLINE static xcb_image_order_t
xcb_host_byte_order(void) {
  uint32_t           endian_test = 0x01020304;

  switch (*(char *)&endian_test) {
  case 0x01:
      return XCB_IMAGE_ORDER_MSB_FIRST;
  case 0x04:
      return XCB_IMAGE_ORDER_LSB_FIRST;
  }
  assert(0);
}

#endif /* __XCB_BITOPS_H__ */
