/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef STDINT_WIN32_H
#define STDINT_WIN32_H

#include <wtf/Platform.h>

/* This file emulates enough of stdint.h on Windows to make JavaScriptCore and WebCore
   compile using MSVC which does not ship with the stdint.h header. */

#if !COMPILER(MSVC)
#error "This stdint.h file should only be compiled with MSVC"
#endif

#include <limits.h>

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)
#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX  _UI64_MAX
#else
#define SIZE_MAX  _UI32_MAX
#endif
#endif
#endif

#ifndef CASSERT
#define CASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1];
#endif

CASSERT(sizeof(int8_t) == 1, int8_t_is_one_byte)
CASSERT(sizeof(uint8_t) == 1, uint8_t_is_one_byte)
CASSERT(sizeof(int16_t) == 2, int16_t_is_two_bytes)
CASSERT(sizeof(uint16_t) == 2, uint16_t_is_two_bytes)
CASSERT(sizeof(int32_t) == 4, int32_t_is_four_bytes)
CASSERT(sizeof(uint32_t) == 4, uint32_t_is_four_bytes)
CASSERT(sizeof(int64_t) == 8, int64_t_is_eight_bytes)
CASSERT(sizeof(uint64_t) == 8, uint64_t_is_eight_bytes)

#endif
