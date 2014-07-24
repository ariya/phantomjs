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

#include "config.h"
#include "SpeculatedType.h"

#include "Arguments.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Operations.h"
#include "StringObject.h"
#include "ValueProfile.h"
#include <wtf/BoundsCheckedPointer.h>
#include <wtf/StringPrintStream.h>

namespace JSC {

void dumpSpeculation(PrintStream& out, SpeculatedType value)
{
    if (value == SpecNone) {
        out.print("None");
        return;
    }
    
    StringPrintStream myOut;
    
    bool isTop = true;
    
    if (value & SpecCellOther)
        myOut.print("Othercell");
    else
        isTop = false;
    
    if (value & SpecObjectOther)
        myOut.print("Otherobj");
    else
        isTop = false;
    
    if (value & SpecFinalObject)
        myOut.print("Final");
    else
        isTop = false;

    if (value & SpecArray)
        myOut.print("Array");
    else
        isTop = false;
    
    if (value & SpecInt8Array)
        myOut.print("Int8array");
    else
        isTop = false;
    
    if (value & SpecInt16Array)
        myOut.print("Int16array");
    else
        isTop = false;
    
    if (value & SpecInt32Array)
        myOut.print("Int32array");
    else
        isTop = false;
    
    if (value & SpecUint8Array)
        myOut.print("Uint8array");
    else
        isTop = false;

    if (value & SpecUint8ClampedArray)
        myOut.print("Uint8clampedarray");
    else
        isTop = false;
    
    if (value & SpecUint16Array)
        myOut.print("Uint16array");
    else
        isTop = false;
    
    if (value & SpecUint32Array)
        myOut.print("Uint32array");
    else
        isTop = false;
    
    if (value & SpecFloat32Array)
        myOut.print("Float32array");
    else
        isTop = false;
    
    if (value & SpecFloat64Array)
        myOut.print("Float64array");
    else
        isTop = false;
    
    if (value & SpecFunction)
        myOut.print("Function");
    else
        isTop = false;
    
    if (value & SpecArguments)
        myOut.print("Arguments");
    else
        isTop = false;
    
    if (value & SpecString)
        myOut.print("String");
    else
        isTop = false;
    
    if (value & SpecStringObject)
        myOut.print("Stringobject");
    else
        isTop = false;
    
    if (value & SpecInt32)
        myOut.print("Int");
    else
        isTop = false;
    
    if (value & SpecDoubleReal)
        myOut.print("Doublereal");
    else
        isTop = false;
    
    if (value & SpecDoubleNaN)
        myOut.print("Doublenan");
    else
        isTop = false;
    
    if (value & SpecBoolean)
        myOut.print("Bool");
    else
        isTop = false;
    
    if (value & SpecOther)
        myOut.print("Other");
    else
        isTop = false;
    
    if (isTop)
        out.print("Top");
    else
        out.print(myOut.toCString());
    
    if (value & SpecEmpty)
        out.print("Empty");
}

// We don't expose this because we don't want anyone relying on the fact that this method currently
// just returns string constants.
static const char* speculationToAbbreviatedString(SpeculatedType prediction)
{
    if (isFinalObjectSpeculation(prediction))
        return "<Final>";
    if (isArraySpeculation(prediction))
        return "<Array>";
    if (isStringSpeculation(prediction))
        return "<String>";
    if (isFunctionSpeculation(prediction))
        return "<Function>";
    if (isInt8ArraySpeculation(prediction))
        return "<Int8array>";
    if (isInt16ArraySpeculation(prediction))
        return "<Int16array>";
    if (isInt32ArraySpeculation(prediction))
        return "<Int32array>";
    if (isUint8ArraySpeculation(prediction))
        return "<Uint8array>";
    if (isUint16ArraySpeculation(prediction))
        return "<Uint16array>";
    if (isUint32ArraySpeculation(prediction))
        return "<Uint32array>";
    if (isFloat32ArraySpeculation(prediction))
        return "<Float32array>";
    if (isFloat64ArraySpeculation(prediction))
        return "<Float64array>";
    if (isArgumentsSpeculation(prediction))
        return "<Arguments>";
    if (isStringObjectSpeculation(prediction))
        return "<StringObject>";
    if (isStringOrStringObjectSpeculation(prediction))
        return "<StringOrStringObject>";
    if (isObjectSpeculation(prediction))
        return "<Object>";
    if (isCellSpeculation(prediction))
        return "<Cell>";
    if (isInt32Speculation(prediction))
        return "<Int32>";
    if (isDoubleSpeculation(prediction))
        return "<Double>";
    if (isNumberSpeculation(prediction))
        return "<Number>";
    if (isBooleanSpeculation(prediction))
        return "<Boolean>";
    if (isOtherSpeculation(prediction))
        return "<Other>";
    return "";
}

void dumpSpeculationAbbreviated(PrintStream& out, SpeculatedType value)
{
    out.print(speculationToAbbreviatedString(value));
}

SpeculatedType speculationFromClassInfo(const ClassInfo* classInfo)
{
    if (classInfo == &JSFinalObject::s_info)
        return SpecFinalObject;
    
    if (classInfo == &JSArray::s_info)
        return SpecArray;
    
    if (classInfo == &Arguments::s_info)
        return SpecArguments;
    
    if (classInfo == &StringObject::s_info)
        return SpecStringObject;
    
    if (classInfo->isSubClassOf(&JSFunction::s_info))
        return SpecFunction;
    
    if (classInfo->typedArrayStorageType != TypedArrayNone) {
        switch (classInfo->typedArrayStorageType) {
        case TypedArrayInt8:
            return SpecInt8Array;
        case TypedArrayInt16:
            return SpecInt16Array;
        case TypedArrayInt32:
            return SpecInt32Array;
        case TypedArrayUint8:
            return SpecUint8Array;
        case TypedArrayUint8Clamped:
            return SpecUint8ClampedArray;
        case TypedArrayUint16:
            return SpecUint16Array;
        case TypedArrayUint32:
            return SpecUint32Array;
        case TypedArrayFloat32:
            return SpecFloat32Array;
        case TypedArrayFloat64:
            return SpecFloat64Array;
        default:
            break;
        }
    }
    
    if (classInfo->isSubClassOf(&JSObject::s_info))
        return SpecObjectOther;
    
    return SpecCellOther;
}

SpeculatedType speculationFromStructure(Structure* structure)
{
    if (structure->typeInfo().type() == StringType)
        return SpecString;
    return speculationFromClassInfo(structure->classInfo());
}

SpeculatedType speculationFromCell(JSCell* cell)
{
    return speculationFromStructure(cell->structure());
}

SpeculatedType speculationFromValue(JSValue value)
{
    if (value.isEmpty())
        return SpecEmpty;
    if (value.isInt32())
        return SpecInt32;
    if (value.isDouble()) {
        double number = value.asNumber();
        if (number == number)
            return SpecDoubleReal;
        return SpecDoubleNaN;
    }
    if (value.isCell())
        return speculationFromCell(value.asCell());
    if (value.isBoolean())
        return SpecBoolean;
    ASSERT(value.isUndefinedOrNull());
    return SpecOther;
}

} // namespace JSC

