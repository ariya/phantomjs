/*
 *  Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_Alignment_h
#define WTF_Alignment_h

#include <wtf/Platform.h>

#if COMPILER(GCC) || COMPILER(MINGW) || COMPILER(RVCT) || COMPILER(WINSCW) || COMPILER(GCCE)
    #define WTF_ALIGN_OF(type) __alignof__(type)
    #define WTF_ALIGNED(variable_type, variable, n) variable_type variable __attribute__((__aligned__(n)))
#elif COMPILER(MSVC)
    #define WTF_ALIGN_OF(type) __alignof(type)
    #define WTF_ALIGNED(variable_type, variable, n) __declspec(align(n)) variable_type variable
#else
    #error WTF_ALIGN macros need alignment control.
#endif

#endif // WTF_Alignment_h
