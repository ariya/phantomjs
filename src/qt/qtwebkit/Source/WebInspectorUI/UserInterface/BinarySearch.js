/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2007, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @param {*} object
 * @param {Array.<*>} array
 * @param {function(*, *)} comparator
 */
function binarySearch(object, array, comparator)
{
    var first = 0;
    var last = array.length - 1;

    while (first <= last) {
        var mid = (first + last) >> 1;
        var c = comparator(object, array[mid]);
        if (c > 0)
            first = mid + 1;
        else if (c < 0)
            last = mid - 1;
        else
            return mid;
    }

    // Return the nearest lesser index, "-1" means "0, "-2" means "1", etc.
    return -(first + 1);
}

Object.defineProperty(Array.prototype, "binaryIndexOf", { value: function(value, comparator)
{
    var result = binarySearch(value, this, comparator);
    return result >= 0 ? result : -1;
}});

/**
 * @param {*} anObject
 * @param {Array.<*>} aList
 * @param {function(*, *)} aFunction
 */
function insertionIndexForObjectInListSortedByFunction(anObject, aList, aFunction)
{
    var index = binarySearch(anObject, aList, aFunction);
    if (index < 0)
        // See binarySearch implementation.
        return -index - 1;
    else {
        // Return the first occurance of an item in the list.
        while (index > 0 && aFunction(anObject, aList[index - 1]) === 0)
            index--;
        return index;
    }
}
