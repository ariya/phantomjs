/*
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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

#ifndef WTF_dtoa_h
#define WTF_dtoa_h

#include <wtf/unicode/Unicode.h>

namespace WTF {
class Mutex;

extern WTF::Mutex* s_dtoaP5Mutex;

// s00: input string. Must not be 0 and must be terminated by 0.
// se: *se will have the last consumed character position + 1.
double strtod(const char* s00, char** se);

typedef char DtoaBuffer[80];

void dtoa(DtoaBuffer result, double dd, bool& sign, int& exponent, unsigned& precision);
void dtoaRoundSF(DtoaBuffer result, double dd, int ndigits, bool& sign, int& exponent, unsigned& precision);
void dtoaRoundDP(DtoaBuffer result, double dd, int ndigits, bool& sign, int& exponent, unsigned& precision);

// Size = 80 for sizeof(DtoaBuffer) + some sign bits, decimal point, 'e', exponent digits.
const unsigned NumberToStringBufferLength = 96;
typedef UChar NumberToStringBuffer[NumberToStringBufferLength];
unsigned numberToString(double, NumberToStringBuffer);

} // namespace WTF

using WTF::NumberToStringBuffer;
using WTF::numberToString;

#endif // WTF_dtoa_h
