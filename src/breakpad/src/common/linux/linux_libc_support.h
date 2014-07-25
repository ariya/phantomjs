// Copyright (c) 2009, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This header provides replacements for libc functions that we need. We if
// call the libc functions directly we risk crashing in the dynamic linker as
// it tries to resolve uncached PLT entries.

#ifndef CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_
#define CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_

#include <stdint.h>
#include <limits.h>
#include <sys/types.h>

extern "C" {

static inline size_t
my_strlen(const char* s) {
  size_t len = 0;
  while (*s++) len++;
  return len;
}

static inline int
my_strcmp(const char* a, const char* b) {
  for (;;) {
    if (*a < *b)
      return -1;
    else if (*a > *b)
      return 1;
    else if (*a == 0)
      return 0;
    a++;
    b++;
  }
}

static inline int
my_strncmp(const char* a, const char* b, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (*a < *b)
      return -1;
    else if (*a > *b)
      return 1;
    else if (*a == 0)
      return 0;
    a++;
    b++;
  }

  return 0;
}

// Parse a non-negative integer.
//   result: (output) the resulting non-negative integer
//   s: a NUL terminated string
// Return true iff successful.
static inline bool
my_strtoui(int* result, const char* s) {
  if (*s == 0)
    return false;
  int r = 0;
  for (;; s++) {
    if (*s == 0)
      break;
    const int old_r = r;
    r *= 10;
    if (*s < '0' || *s > '9')
      return false;
    r += *s - '0';
    if (r < old_r)
      return false;
  }

  *result = r;
  return true;
}

// Return the length of the given, non-negative integer when expressed in base
// 10.
static inline unsigned
my_int_len(intmax_t i) {
  if (!i)
    return 1;

  int len = 0;
  while (i) {
    len++;
    i /= 10;
  }

  return len;
}

// Convert a non-negative integer to a string
//   output: (output) the resulting string is written here. This buffer must be
//     large enough to hold the resulting string. Call |my_int_len| to get the
//     required length.
//   i: the non-negative integer to serialise.
//   i_len: the length of the integer in base 10 (see |my_int_len|).
static inline void
my_itos(char* output, intmax_t i, unsigned i_len) {
  for (unsigned index = i_len; index; --index, i /= 10)
    output[index - 1] = '0' + (i % 10);
}

static inline const char*
my_strchr(const char* haystack, char needle) {
  while (*haystack && *haystack != needle)
    haystack++;
  if (*haystack == needle)
    return haystack;
  return (const char*) 0;
}

// Read a hex value
//   result: (output) the resulting value
//   s: a string
// Returns a pointer to the first invalid charactor.
static inline const char*
my_read_hex_ptr(uintptr_t* result, const char* s) {
  uintptr_t r = 0;

  for (;; ++s) {
    if (*s >= '0' && *s <= '9') {
      r <<= 4;
      r += *s - '0';
    } else if (*s >= 'a' && *s <= 'f') {
      r <<= 4;
      r += (*s - 'a') + 10;
    } else if (*s >= 'A' && *s <= 'F') {
      r <<= 4;
      r += (*s - 'A') + 10;
    } else {
      break;
    }
  }

  *result = r;
  return s;
}

static inline void
my_memset(void* ip, char c, size_t len) {
  char* p = (char *) ip;
  while (len--)
    *p++ = c;
}

}  // extern "C"

#endif  // CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_
