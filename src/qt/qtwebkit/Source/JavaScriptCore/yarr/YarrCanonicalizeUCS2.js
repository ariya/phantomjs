/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

// See ES 5.1, 15.10.2.8
function canonicalize(ch)
{
    var u = String.fromCharCode(ch).toUpperCase();
    if (u.length > 1)
        return ch;
    var cu = u.charCodeAt(0);
    if (ch >= 128 && cu < 128)
        return ch;
    return cu;
}

var MAX_UCS2 = 0xFFFF;
var MAX_LATIN = 0xFF;

var groupedCanonically = [];
// Pass 1: populate groupedCanonically - this is mapping from canonicalized
// values back to the set of character code that canonicalize to them.
for (var i = 0; i <= MAX_UCS2; ++i) {
    var ch = canonicalize(i);
    if (!groupedCanonically[ch])
        groupedCanonically[ch] = [];
    groupedCanonically[ch].push(i);
}

var typeInfo = [];
var latinTypeInfo = [];
var characterSetInfo = [];
// Pass 2: populate typeInfo & characterSetInfo. For every character calculate
// a typeInfo value, described by the types above, and a value payload.
for (cu in groupedCanonically) {
    // The set of characters that canonicalize to cu
    var characters = groupedCanonically[cu];

    // If there is only one, it is unique.
    if (characters.length == 1) {
        typeInfo[characters[0]] = "CanonicalizeUnique:0";
        latinTypeInfo[characters[0]] = characters[0] <= MAX_LATIN ? "CanonicalizeLatinSelf:0" : "CanonicalizeLatinInvalid:0";
        continue;
    }

    // Sort the array.
    characters.sort(function(x,y){return x-y;});

    // If there are more than two characters, create an entry in characterSetInfo.
    if (characters.length > 2) {
        for (i in characters)
            typeInfo[characters[i]] = "CanonicalizeSet:" + characterSetInfo.length;
        characterSetInfo.push(characters);

        if (characters[1] <= MAX_LATIN)
            throw new Error("sets with more than one latin character not supported!");
        if (characters[0] <= MAX_LATIN) {
            for (i in characters)
                latinTypeInfo[characters[i]] = "CanonicalizeLatinOther:" + characters[0];
            latinTypeInfo[characters[0]] = "CanonicalizeLatinSelf:0";
        } else {
            for (i in characters)
                latinTypeInfo[characters[i]] = "CanonicalizeLatinInvalid:0";
        }

        continue;
    }

    // We have a pair, mark alternating ranges, otherwise track whether this is the low or high partner.
    var lo = characters[0];
    var hi = characters[1];
    var delta = hi - lo;
    if (delta == 1) {
        var type = lo & 1 ? "CanonicalizeAlternatingUnaligned:0" : "CanonicalizeAlternatingAligned:0";
        typeInfo[lo] = type;
        typeInfo[hi] = type;
    } else {
        typeInfo[lo] = "CanonicalizeRangeLo:" + delta;
        typeInfo[hi] = "CanonicalizeRangeHi:" + delta;
    }

    if (lo > MAX_LATIN) {
        latinTypeInfo[lo] = "CanonicalizeLatinInvalid:0"; 
        latinTypeInfo[hi] = "CanonicalizeLatinInvalid:0";
    } else if (hi > MAX_LATIN) {
        latinTypeInfo[lo] = "CanonicalizeLatinSelf:0"; 
        latinTypeInfo[hi] = "CanonicalizeLatinOther:" + lo;
    } else {
        if (delta != 0x20 || lo & 0x20)
            throw new Error("pairs of latin characters that don't mask with 0x20 not supported!");
        latinTypeInfo[lo] = "CanonicalizeLatinMask0x20:0";
        latinTypeInfo[hi] = "CanonicalizeLatinMask0x20:0";
    }
}

var rangeInfo = [];
// Pass 3: coallesce types into ranges.
for (var end = 0; end <= MAX_UCS2; ++end) {
    var begin = end;
    var type = typeInfo[end];
    while (end < MAX_UCS2 && typeInfo[end + 1] == type)
        ++end;
    rangeInfo.push({begin:begin, end:end, type:type});
}

var latinRangeInfo = [];
// Pass 4: coallesce latin-1 types into ranges.
for (var end = 0; end <= MAX_UCS2; ++end) {
    var begin = end;
    var type = latinTypeInfo[end];
    while (end < MAX_UCS2 && latinTypeInfo[end + 1] == type)
        ++end;
    latinRangeInfo.push({begin:begin, end:end, type:type});
}


// Helper function to convert a number to a fixed width hex representation of a C uint16_t.
function hex(x)
{
    var s = Number(x).toString(16);
    while (s.length < 4)
        s = 0 + s;
    return "0x" + s + "u";
}

var copyright = (
    "/*"                                                                            + "\n" +
    " * Copyright (C) 2012 Apple Inc. All rights reserved."                         + "\n" +
    " *"                                                                            + "\n" +
    " * Redistribution and use in source and binary forms, with or without"         + "\n" +
    " * modification, are permitted provided that the following conditions"         + "\n" +
    " * are met:"                                                                   + "\n" +
    " * 1. Redistributions of source code must retain the above copyright"          + "\n" +
    " *    notice, this list of conditions and the following disclaimer."           + "\n" +
    " * 2. Redistributions in binary form must reproduce the above copyright"       + "\n" +
    " *    notice, this list of conditions and the following disclaimer in the"     + "\n" +
    " *    documentation and/or other materials provided with the distribution."    + "\n" +
    " *"                                                                            + "\n" +
    " * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY"                  + "\n" +
    " * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE"          + "\n" +
    " * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR"         + "\n" +
    " * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR"                   + "\n" +
    " * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,"      + "\n" +
    " * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,"        + "\n" +
    " * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR"         + "\n" +
    " * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY"        + "\n" +
    " * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT"               + "\n" +
    " * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE"      + "\n" +
    " * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. "      + "\n" +
    " */");

print(copyright);
print();
print("// DO NOT EDIT! - this file autogenerated by YarrCanonicalizeUCS2.js");
print();
print('#include "config.h"');
print('#include "YarrCanonicalizeUCS2.h"');
print();
print("namespace JSC { namespace Yarr {");
print();
print("#include <stdint.h>");
print();

for (i in characterSetInfo) {
    var characters = ""
    var set = characterSetInfo[i];
    for (var j in set)
        characters += hex(set[j]) + ", ";
    print("uint16_t ucs2CharacterSet" + i + "[] = { " + characters + "0 };");
}
print();
print("static const size_t UCS2_CANONICALIZATION_SETS = " + characterSetInfo.length + ";");
print("uint16_t* characterSetInfo[UCS2_CANONICALIZATION_SETS] = {");
for (i in characterSetInfo)
print("    ucs2CharacterSet" + i + ",");
print("};");
print();
print("const size_t UCS2_CANONICALIZATION_RANGES = " + rangeInfo.length + ";");
print("UCS2CanonicalizationRange rangeInfo[UCS2_CANONICALIZATION_RANGES] = {");
for (i in rangeInfo) {
    var info = rangeInfo[i];
    var typeAndValue = info.type.split(':');
    print("    { " + hex(info.begin) + ", " + hex(info.end) + ", " + hex(typeAndValue[1]) + ", " + typeAndValue[0] + " },");
}
print("};");
print();
print("const size_t LATIN_CANONICALIZATION_RANGES = " + latinRangeInfo.length + ";");
print("LatinCanonicalizationRange latinRangeInfo[LATIN_CANONICALIZATION_RANGES] = {");
for (i in latinRangeInfo) {
    var info = latinRangeInfo[i];
    var typeAndValue = info.type.split(':');
    print("    { " + hex(info.begin) + ", " + hex(info.end) + ", " + hex(typeAndValue[1]) + ", " + typeAndValue[0] + " },");
}
print("};");
print();
print("} } // JSC::Yarr");
print();

