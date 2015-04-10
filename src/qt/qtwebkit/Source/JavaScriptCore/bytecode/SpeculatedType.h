/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SpeculatedType_h
#define SpeculatedType_h

#include "JSCJSValue.h"
#include <wtf/PrintStream.h>

namespace JSC {

class Structure;

typedef uint32_t SpeculatedType;
static const SpeculatedType SpecNone              = 0x00000000; // We don't know anything yet.
static const SpeculatedType SpecFinalObject       = 0x00000001; // It's definitely a JSFinalObject.
static const SpeculatedType SpecArray             = 0x00000002; // It's definitely a JSArray.
static const SpeculatedType SpecFunction          = 0x00000008; // It's definitely a JSFunction or one of its subclasses.
static const SpeculatedType SpecInt8Array         = 0x00000010; // It's definitely an Int8Array or one of its subclasses.
static const SpeculatedType SpecInt16Array        = 0x00000020; // It's definitely an Int16Array or one of its subclasses.
static const SpeculatedType SpecInt32Array        = 0x00000040; // It's definitely an Int32Array or one of its subclasses.
static const SpeculatedType SpecUint8Array        = 0x00000080; // It's definitely an Uint8Array or one of its subclasses.
static const SpeculatedType SpecUint8ClampedArray = 0x00000100; // It's definitely an Uint8ClampedArray or one of its subclasses.
static const SpeculatedType SpecUint16Array       = 0x00000200; // It's definitely an Uint16Array or one of its subclasses.
static const SpeculatedType SpecUint32Array       = 0x00000400; // It's definitely an Uint32Array or one of its subclasses.
static const SpeculatedType SpecFloat32Array      = 0x00000800; // It's definitely an Uint16Array or one of its subclasses.
static const SpeculatedType SpecFloat64Array      = 0x00001000; // It's definitely an Uint16Array or one of its subclasses.
static const SpeculatedType SpecArguments         = 0x00002000; // It's definitely an Arguments object.
static const SpeculatedType SpecStringObject      = 0x00004000; // It's definitely a StringObject.
static const SpeculatedType SpecObjectOther       = 0x00008000; // It's definitely an object but not JSFinalObject, JSArray, or JSFunction.
static const SpeculatedType SpecObject            = 0x0000ffff; // Bitmask used for testing for any kind of object prediction.
static const SpeculatedType SpecString            = 0x00010000; // It's definitely a JSString.
static const SpeculatedType SpecCellOther         = 0x00020000; // It's definitely a JSCell but not a subclass of JSObject and definitely not a JSString.
static const SpeculatedType SpecCell              = 0x0003ffff; // It's definitely a JSCell.
static const SpeculatedType SpecInt32             = 0x00800000; // It's definitely an Int32.
static const SpeculatedType SpecDoubleReal        = 0x01000000; // It's definitely a non-NaN double.
static const SpeculatedType SpecDoubleNaN         = 0x02000000; // It's definitely a NaN.
static const SpeculatedType SpecDouble            = 0x03000000; // It's either a non-NaN or a NaN double.
static const SpeculatedType SpecRealNumber        = 0x01800000; // It's either an Int32 or a DoubleReal.
static const SpeculatedType SpecNumber            = 0x03800000; // It's either an Int32 or a Double.
static const SpeculatedType SpecBoolean           = 0x04000000; // It's definitely a Boolean.
static const SpeculatedType SpecOther             = 0x08000000; // It's definitely none of the above.
static const SpeculatedType SpecTop               = 0x0fffffff; // It can be any of the above.
static const SpeculatedType SpecEmpty             = 0x10000000; // It's definitely an empty value marker.
static const SpeculatedType SpecEmptyOrTop        = 0x1fffffff; // It can be any of the above.
static const SpeculatedType FixedIndexedStorageMask    = SpecInt8Array | SpecInt16Array | SpecInt32Array | SpecUint8Array | SpecUint8ClampedArray | SpecUint16Array | SpecUint32Array | SpecFloat32Array | SpecFloat64Array;

typedef bool (*SpeculatedTypeChecker)(SpeculatedType);

// Dummy prediction checker, only useful if someone insists on requiring a prediction checker.
inline bool isAnySpeculation(SpeculatedType)
{
    return true;
}

inline bool isCellSpeculation(SpeculatedType value)
{
    return !!(value & SpecCell) && !(value & ~SpecCell);
}

inline bool isObjectSpeculation(SpeculatedType value)
{
    return !!(value & SpecObject) && !(value & ~SpecObject);
}

inline bool isObjectOrOtherSpeculation(SpeculatedType value)
{
    return !!(value & (SpecObject | SpecOther)) && !(value & ~(SpecObject | SpecOther));
}

inline bool isFinalObjectSpeculation(SpeculatedType value)
{
    return value == SpecFinalObject;
}

inline bool isFinalObjectOrOtherSpeculation(SpeculatedType value)
{
    return !!(value & (SpecFinalObject | SpecOther)) && !(value & ~(SpecFinalObject | SpecOther));
}

inline bool isFixedIndexedStorageObjectSpeculation(SpeculatedType value)
{
    return !!value && (value & FixedIndexedStorageMask) == value;
}

inline bool isStringSpeculation(SpeculatedType value)
{
    return value == SpecString;
}

inline bool isArraySpeculation(SpeculatedType value)
{
    return value == SpecArray;
}

inline bool isFunctionSpeculation(SpeculatedType value)
{
    return value == SpecFunction;
}

inline bool isInt8ArraySpeculation(SpeculatedType value)
{
    return value == SpecInt8Array;
}

inline bool isInt16ArraySpeculation(SpeculatedType value)
{
    return value == SpecInt16Array;
}

inline bool isInt32ArraySpeculation(SpeculatedType value)
{
    return value == SpecInt32Array;
}

inline bool isUint8ArraySpeculation(SpeculatedType value)
{
    return value == SpecUint8Array;
}

inline bool isUint8ClampedArraySpeculation(SpeculatedType value)
{
    return value == SpecUint8ClampedArray;
}

inline bool isUint16ArraySpeculation(SpeculatedType value)
{
    return value == SpecUint16Array;
}

inline bool isUint32ArraySpeculation(SpeculatedType value)
{
    return value == SpecUint32Array;
}

inline bool isFloat32ArraySpeculation(SpeculatedType value)
{
    return value == SpecFloat32Array;
}

inline bool isFloat64ArraySpeculation(SpeculatedType value)
{
    return value == SpecFloat64Array;
}

inline bool isArgumentsSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecArguments) == value;
}

inline bool isActionableIntMutableArraySpeculation(SpeculatedType value)
{
    return isInt8ArraySpeculation(value)
        || isInt16ArraySpeculation(value)
        || isInt32ArraySpeculation(value)
        || isUint8ArraySpeculation(value)
        || isUint8ClampedArraySpeculation(value)
        || isUint16ArraySpeculation(value)
        || isUint32ArraySpeculation(value);
}

inline bool isActionableFloatMutableArraySpeculation(SpeculatedType value)
{
    return isFloat32ArraySpeculation(value)
        || isFloat64ArraySpeculation(value);
}

inline bool isActionableTypedMutableArraySpeculation(SpeculatedType value)
{
    return isActionableIntMutableArraySpeculation(value)
        || isActionableFloatMutableArraySpeculation(value);
}

inline bool isActionableMutableArraySpeculation(SpeculatedType value)
{
    return isArraySpeculation(value)
        || isArgumentsSpeculation(value)
        || isActionableTypedMutableArraySpeculation(value);
}

inline bool isActionableArraySpeculation(SpeculatedType value)
{
    return isStringSpeculation(value)
        || isActionableMutableArraySpeculation(value);
}

inline bool isArrayOrOtherSpeculation(SpeculatedType value)
{
    return !!(value & (SpecArray | SpecOther)) && !(value & ~(SpecArray | SpecOther));
}

inline bool isStringObjectSpeculation(SpeculatedType value)
{
    return value == SpecStringObject;
}

inline bool isStringOrStringObjectSpeculation(SpeculatedType value)
{
    return !!value && !(value & ~(SpecString | SpecStringObject));
}

inline bool isInt32Speculation(SpeculatedType value)
{
    return value == SpecInt32;
}

inline bool isInt32SpeculationForArithmetic(SpeculatedType value)
{
    return !(value & SpecDouble);
}

inline bool isInt32SpeculationExpectingDefined(SpeculatedType value)
{
    return isInt32Speculation(value & ~SpecOther);
}

inline bool isDoubleRealSpeculation(SpeculatedType value)
{
    return value == SpecDoubleReal;
}

inline bool isDoubleSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecDouble) == value;
}

inline bool isDoubleSpeculationForArithmetic(SpeculatedType value)
{
    return !!(value & SpecDouble);
}

inline bool isRealNumberSpeculation(SpeculatedType value)
{
    return !!(value & SpecRealNumber) && !(value & ~SpecRealNumber);
}

inline bool isNumberSpeculation(SpeculatedType value)
{
    return !!(value & SpecNumber) && !(value & ~SpecNumber);
}

inline bool isNumberSpeculationExpectingDefined(SpeculatedType value)
{
    return isNumberSpeculation(value & ~SpecOther);
}

inline bool isBooleanSpeculation(SpeculatedType value)
{
    return value == SpecBoolean;
}

inline bool isOtherSpeculation(SpeculatedType value)
{
    return value == SpecOther;
}

inline bool isOtherOrEmptySpeculation(SpeculatedType value)
{
    return !value || value == SpecOther;
}

inline bool isEmptySpeculation(SpeculatedType value)
{
    return value == SpecEmpty;
}

void dumpSpeculation(PrintStream&, SpeculatedType);
void dumpSpeculationAbbreviated(PrintStream&, SpeculatedType);

MAKE_PRINT_ADAPTOR(SpeculationDump, SpeculatedType, dumpSpeculation);
MAKE_PRINT_ADAPTOR(AbbreviatedSpeculationDump, SpeculatedType, dumpSpeculationAbbreviated);

// Merge two predictions. Note that currently this just does left | right. It may
// seem tempting to do so directly, but you would be doing so at your own peril,
// since the merging protocol SpeculatedType may change at any time (and has already
// changed several times in its history).
inline SpeculatedType mergeSpeculations(SpeculatedType left, SpeculatedType right)
{
    return left | right;
}

template<typename T>
inline bool mergeSpeculation(T& left, SpeculatedType right)
{
    SpeculatedType newSpeculation = static_cast<T>(mergeSpeculations(static_cast<SpeculatedType>(left), right));
    bool result = newSpeculation != static_cast<SpeculatedType>(left);
    left = newSpeculation;
    return result;
}

inline bool speculationChecked(SpeculatedType actual, SpeculatedType desired)
{
    return (actual | desired) == desired;
}

SpeculatedType speculationFromClassInfo(const ClassInfo*);
SpeculatedType speculationFromStructure(Structure*);
SpeculatedType speculationFromCell(JSCell*);
SpeculatedType speculationFromValue(JSValue);

} // namespace JSC

#endif // SpeculatedType_h
