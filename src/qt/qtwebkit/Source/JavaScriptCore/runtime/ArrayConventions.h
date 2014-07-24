/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef ArrayConventions_h
#define ArrayConventions_h

#include "IndexingHeader.h"
#include "WriteBarrier.h"

namespace JSC {

// Overview of JSArray
//
// Properties of JSArray objects may be stored in one of three locations:
//   * The regular JSObject property map.
//   * A storage vector.
//   * A sparse map of array entries.
//
// Properties with non-numeric identifiers, with identifiers that are not representable
// as an unsigned integer, or where the value is greater than  MAX_ARRAY_INDEX
// (specifically, this is only one property - the value 0xFFFFFFFFU as an unsigned 32-bit
// integer) are not considered array indices and will be stored in the JSObject property map.
//
// All properties with a numeric identifer, representable as an unsigned integer i,
// where (i <= MAX_ARRAY_INDEX), are an array index and will be stored in either the
// storage vector or the sparse map. An array index i will be handled in the following
// fashion:
//
//   * Where (i < MIN_SPARSE_ARRAY_INDEX) the value will be stored in the storage vector,
//     unless the array is in SparseMode in which case all properties go into the map.
//   * Where (MIN_SPARSE_ARRAY_INDEX <= i <= MAX_STORAGE_VECTOR_INDEX) the value will either
//     be stored in the storage vector or in the sparse array, depending on the density of
//     data that would be stored in the vector (a vector being used where at least
//     (1 / minDensityMultiplier) of the entries would be populated).
//   * Where (MAX_STORAGE_VECTOR_INDEX < i <= MAX_ARRAY_INDEX) the value will always be stored
//     in the sparse array.

// Define the maximum storage vector length to be 2^32 / sizeof(JSValue) / 2 to ensure that
// there is no risk of overflow.
#define MAX_STORAGE_VECTOR_LENGTH (static_cast<unsigned>(IndexingHeader::maximumLength))

// These values have to be macros to be used in max() and min() without introducing
// a PIC branch in Mach-O binaries, see <rdar://problem/5971391>.
#define MIN_SPARSE_ARRAY_INDEX 100000U
#define MAX_STORAGE_VECTOR_INDEX (MAX_STORAGE_VECTOR_LENGTH - 1)
// 0xFFFFFFFF is a bit weird -- is not an array index even though it's an integer.
#define MAX_ARRAY_INDEX 0xFFFFFFFEU

// The value BASE_VECTOR_LEN is the maximum number of vector elements we'll allocate
// for an array that was created with a sepcified length (e.g. a = new Array(123))
#define BASE_VECTOR_LEN 4U
    
// The upper bound to the size we'll grow a zero length array when the first element
// is added.
#define FIRST_VECTOR_GROW 4U

#define MIN_BEYOND_LENGTH_SPARSE_INDEX 1000

// Our policy for when to use a vector and when to use a sparse map.
// For all array indices under MIN_SPARSE_ARRAY_INDEX, we always use a vector.
// When indices greater than MIN_SPARSE_ARRAY_INDEX are involved, we use a vector
// as long as it is 1/8 full. If more sparse than that, we use a map.
static const unsigned minDensityMultiplier = 8;

inline bool isDenseEnoughForVector(unsigned length, unsigned numValues)
{
    return length / minDensityMultiplier <= numValues;
}

inline bool indexIsSufficientlyBeyondLengthForSparseMap(unsigned i, unsigned length)
{
    return i >= MIN_BEYOND_LENGTH_SPARSE_INDEX && i > length;
}

inline IndexingHeader indexingHeaderForArray(unsigned length, unsigned vectorLength)
{
    IndexingHeader result;
    result.setPublicLength(length);
    result.setVectorLength(vectorLength);
    return result;
}

inline IndexingHeader baseIndexingHeaderForArray(unsigned length)
{
    return indexingHeaderForArray(length, BASE_VECTOR_LEN);
}

} // namespace JSC

#endif // ArrayConventions_h

