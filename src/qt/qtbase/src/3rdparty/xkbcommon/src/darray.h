/*
 * Copyright (C) 2011 Joseph Adams <joeyadams3.14159@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CCAN_DARRAY_H
#define CCAN_DARRAY_H

/* Originally taken from: http://ccodearchive.net/info/darray.html
 * But modified for libxkbcommon. */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define darray(type) struct { type *item; unsigned size; unsigned alloc; }

#define darray_new() { 0, 0, 0 }

#define darray_init(arr) do { \
    (arr).item = 0; (arr).size = 0; (arr).alloc = 0; \
} while (0)

#define darray_free(arr) do { \
    free((arr).item); \
    darray_init(arr); \
} while (0)

/*
 * Typedefs for darrays of common types.  These are useful
 * when you want to pass a pointer to an darray(T) around.
 *
 * The following will produce an incompatible pointer warning:
 *
 *     void foo(darray(int) *arr);
 *     darray(int) arr = darray_new();
 *     foo(&arr);
 *
 * The workaround:
 *
 *     void foo(darray_int *arr);
 *     darray_int arr = darray_new();
 *     foo(&arr);
 */

typedef darray (char)           darray_char;
typedef darray (signed char)    darray_schar;
typedef darray (unsigned char)  darray_uchar;

typedef darray (short)          darray_short;
typedef darray (int)            darray_int;
typedef darray (long)           darray_long;

typedef darray (unsigned short) darray_ushort;
typedef darray (unsigned int)   darray_uint;
typedef darray (unsigned long)  darray_ulong;

/*** Access ***/

#define darray_item(arr, i)     ((arr).item[i])
#define darray_size(arr)        ((arr).size)
#define darray_empty(arr)       ((arr).size == 0)
#define darray_mem(arr, offset) ((arr).item + (offset))

/*** Insertion (single item) ***/

#define darray_append(arr, ...)  do { \
    darray_resize(arr, (arr).size + 1); \
    (arr).item[(arr).size - 1] = (__VA_ARGS__); \
} while (0)

/*** Insertion (multiple items) ***/

#define darray_append_items(arr, items, count) do { \
    unsigned __count = (count), __oldSize = (arr).size; \
    darray_resize(arr, __oldSize + __count); \
    memcpy((arr).item + __oldSize, items, __count * sizeof(*(arr).item)); \
} while (0)

#define darray_from_items(arr, items, count) do { \
    unsigned __count = (count); \
    darray_resize(arr, __count); \
    memcpy((arr).item, items, __count * sizeof(*(arr).item)); \
} while (0)

#define darray_copy(arr_to, arr_from) \
    darray_from_items((arr_to), (arr_from).item, (arr_from).size)

/*** String buffer ***/

#define darray_append_string(arr, str) do { \
    const char *__str = (str); \
    darray_append_items(arr, __str, strlen(__str) + 1); \
    (arr).size--; \
} while (0)

#define darray_append_lit(arr, stringLiteral) do { \
    darray_append_items(arr, stringLiteral, sizeof(stringLiteral)); \
    (arr).size--; \
} while (0)

#define darray_appends_nullterminate(arr, items, count) do { \
    unsigned __count = (count), __oldSize = (arr).size; \
    darray_resize(arr, __oldSize + __count + 1); \
    memcpy((arr).item + __oldSize, items, __count * sizeof(*(arr).item)); \
    (arr).item[--(arr).size] = 0; \
} while (0)

#define darray_prepends_nullterminate(arr, items, count) do { \
    unsigned __count = (count), __oldSize = (arr).size; \
    darray_resize(arr, __count + __oldSize + 1); \
    memmove((arr).item + __count, (arr).item, \
            __oldSize * sizeof(*(arr).item)); \
    memcpy((arr).item, items, __count * sizeof(*(arr).item)); \
    (arr).item[--(arr).size] = 0; \
} while (0)

/*** Size management ***/

#define darray_resize(arr, newSize) \
    darray_growalloc(arr, (arr).size = (newSize))

#define darray_resize0(arr, newSize) do { \
    unsigned __oldSize = (arr).size, __newSize = (newSize); \
    (arr).size = __newSize; \
    if (__newSize > __oldSize) { \
        darray_growalloc(arr, __newSize); \
        memset(&(arr).item[__oldSize], 0, \
               (__newSize - __oldSize) * sizeof(*(arr).item)); \
    } \
} while (0)

#define darray_realloc(arr, newAlloc) do { \
    (arr).item = realloc((arr).item, \
                         ((arr).alloc = (newAlloc)) * sizeof(*(arr).item)); \
} while (0)

#define darray_growalloc(arr, need)   do { \
    unsigned __need = (need); \
    if (__need > (arr).alloc) \
        darray_realloc(arr, darray_next_alloc((arr).alloc, __need, \
                                              sizeof(*(arr).item))); \
} while (0)

static inline unsigned
darray_next_alloc(unsigned alloc, unsigned need, unsigned itemSize)
{
    assert(need < UINT_MAX / itemSize / 2); /* Overflow. */
    if (alloc == 0)
        alloc = 4;
    while (alloc < need)
        alloc *= 2;
    return alloc;
}

/*** Traversal ***/

#define darray_foreach(i, arr) \
    for ((i) = &(arr).item[0]; (i) < &(arr).item[(arr).size]; (i)++)

#define darray_foreach_from(i, arr, from) \
    for ((i) = &(arr).item[from]; (i) < &(arr).item[(arr).size]; (i)++)

/* Iterate on index and value at the same time, like Python's enumerate. */
#define darray_enumerate(idx, val, arr) \
    for ((idx) = 0, (val) = &(arr).item[0]; \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#define darray_enumerate_from(idx, val, arr, from) \
    for ((idx) = (from), (val) = &(arr).item[0]; \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#define darray_foreach_reverse(i, arr) \
    for ((i) = &(arr).item[(arr).size]; (i)-- > &(arr).item[0]; )

#endif /* CCAN_DARRAY_H */
