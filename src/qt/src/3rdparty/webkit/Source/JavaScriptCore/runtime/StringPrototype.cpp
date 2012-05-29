/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Torch Mobile, Inc.
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

#include "config.h"
#include "StringPrototype.h"

#include "CachedCall.h"
#include "Error.h"
#include "Executable.h"
#include "JSGlobalObjectFunctions.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSStringBuilder.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "PropertyNameArray.h"
#include "RegExpCache.h"
#include "RegExpConstructor.h"
#include "RegExpObject.h"
#include <wtf/ASCIICType.h>
#include <wtf/MathExtras.h>
#include <wtf/unicode/Collator.h>

using namespace WTF;

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(StringPrototype);

static EncodedJSValue JSC_HOST_CALL stringProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncCharAt(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncCharCodeAt(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncConcat(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncIndexOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncLastIndexOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncMatch(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncReplace(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSearch(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSlice(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSplit(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSubstr(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSubstring(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncToLowerCase(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncToUpperCase(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncLocaleCompare(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncBig(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSmall(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncBlink(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncBold(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncFixed(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncItalics(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncStrike(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSub(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncSup(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncFontcolor(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncFontsize(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncAnchor(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncLink(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncTrim(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncTrimLeft(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringProtoFuncTrimRight(ExecState*);

}

#include "StringPrototype.lut.h"

namespace JSC {

const ClassInfo StringPrototype::s_info = { "String", &StringObject::s_info, 0, ExecState::stringTable };

/* Source for StringPrototype.lut.h
@begin stringTable 26
    toString              stringProtoFuncToString          DontEnum|Function       0
    valueOf               stringProtoFuncToString          DontEnum|Function       0
    charAt                stringProtoFuncCharAt            DontEnum|Function       1
    charCodeAt            stringProtoFuncCharCodeAt        DontEnum|Function       1
    concat                stringProtoFuncConcat            DontEnum|Function       1
    indexOf               stringProtoFuncIndexOf           DontEnum|Function       1
    lastIndexOf           stringProtoFuncLastIndexOf       DontEnum|Function       1
    match                 stringProtoFuncMatch             DontEnum|Function       1
    replace               stringProtoFuncReplace           DontEnum|Function       2
    search                stringProtoFuncSearch            DontEnum|Function       1
    slice                 stringProtoFuncSlice             DontEnum|Function       2
    split                 stringProtoFuncSplit             DontEnum|Function       2
    substr                stringProtoFuncSubstr            DontEnum|Function       2
    substring             stringProtoFuncSubstring         DontEnum|Function       2
    toLowerCase           stringProtoFuncToLowerCase       DontEnum|Function       0
    toUpperCase           stringProtoFuncToUpperCase       DontEnum|Function       0
    localeCompare         stringProtoFuncLocaleCompare     DontEnum|Function       1

    # toLocaleLowerCase and toLocaleUpperCase are currently identical to toLowerCase and toUpperCase
    toLocaleLowerCase     stringProtoFuncToLowerCase       DontEnum|Function       0
    toLocaleUpperCase     stringProtoFuncToUpperCase       DontEnum|Function       0

    big                   stringProtoFuncBig               DontEnum|Function       0
    small                 stringProtoFuncSmall             DontEnum|Function       0
    blink                 stringProtoFuncBlink             DontEnum|Function       0
    bold                  stringProtoFuncBold              DontEnum|Function       0
    fixed                 stringProtoFuncFixed             DontEnum|Function       0
    italics               stringProtoFuncItalics           DontEnum|Function       0
    strike                stringProtoFuncStrike            DontEnum|Function       0
    sub                   stringProtoFuncSub               DontEnum|Function       0
    sup                   stringProtoFuncSup               DontEnum|Function       0
    fontcolor             stringProtoFuncFontcolor         DontEnum|Function       1
    fontsize              stringProtoFuncFontsize          DontEnum|Function       1
    anchor                stringProtoFuncAnchor            DontEnum|Function       1
    link                  stringProtoFuncLink              DontEnum|Function       1
    trim                  stringProtoFuncTrim              DontEnum|Function       0
    trimLeft              stringProtoFuncTrimLeft          DontEnum|Function       0
    trimRight             stringProtoFuncTrimRight         DontEnum|Function       0
@end
*/

// ECMA 15.5.4
StringPrototype::StringPrototype(ExecState* exec, JSGlobalObject* globalObject, Structure* structure)
    : StringObject(exec, structure)
{
    ASSERT(inherits(&s_info));

    putAnonymousValue(exec->globalData(), 0, globalObject);
    // The constructor will be added later, after StringConstructor has been built
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(0), DontDelete | ReadOnly | DontEnum);
}

bool StringPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<StringObject>(exec, ExecState::stringTable(exec), this, propertyName, slot);
}

bool StringPrototype::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<StringObject>(exec, ExecState::stringTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions --------------------------

static NEVER_INLINE UString substituteBackreferencesSlow(const UString& replacement, const UString& source, const int* ovector, RegExp* reg, size_t i)
{
    Vector<UChar> substitutedReplacement;
    int offset = 0;
    do {
        if (i + 1 == replacement.length())
            break;

        UChar ref = replacement[i + 1];
        if (ref == '$') {
            // "$$" -> "$"
            ++i;
            substitutedReplacement.append(replacement.characters() + offset, i - offset);
            offset = i + 1;
            continue;
        }

        int backrefStart;
        int backrefLength;
        int advance = 0;
        if (ref == '&') {
            backrefStart = ovector[0];
            backrefLength = ovector[1] - backrefStart;
        } else if (ref == '`') {
            backrefStart = 0;
            backrefLength = ovector[0];
        } else if (ref == '\'') {
            backrefStart = ovector[1];
            backrefLength = source.length() - backrefStart;
        } else if (reg && ref >= '0' && ref <= '9') {
            // 1- and 2-digit back references are allowed
            unsigned backrefIndex = ref - '0';
            if (backrefIndex > reg->numSubpatterns())
                continue;
            if (replacement.length() > i + 2) {
                ref = replacement[i + 2];
                if (ref >= '0' && ref <= '9') {
                    backrefIndex = 10 * backrefIndex + ref - '0';
                    if (backrefIndex > reg->numSubpatterns())
                        backrefIndex = backrefIndex / 10;   // Fall back to the 1-digit reference
                    else
                        advance = 1;
                }
            }
            if (!backrefIndex)
                continue;
            backrefStart = ovector[2 * backrefIndex];
            backrefLength = ovector[2 * backrefIndex + 1] - backrefStart;
        } else
            continue;

        if (i - offset)
            substitutedReplacement.append(replacement.characters() + offset, i - offset);
        i += 1 + advance;
        offset = i + 1;
        if (backrefStart >= 0)
            substitutedReplacement.append(source.characters() + backrefStart, backrefLength);
    } while ((i = replacement.find('$', i + 1)) != notFound);

    if (replacement.length() - offset)
        substitutedReplacement.append(replacement.characters() + offset, replacement.length() - offset);

    substitutedReplacement.shrinkToFit();
    return UString::adopt(substitutedReplacement);
}

static inline UString substituteBackreferences(const UString& replacement, const UString& source, const int* ovector, RegExp* reg)
{
    size_t i = replacement.find('$', 0);
    if (UNLIKELY(i != notFound))
        return substituteBackreferencesSlow(replacement, source, ovector, reg, i);
    return replacement;
}

static inline int localeCompare(const UString& a, const UString& b)
{
    return Collator::userDefault()->collate(reinterpret_cast<const ::UChar*>(a.characters()), a.length(), reinterpret_cast<const ::UChar*>(b.characters()), b.length());
}

struct StringRange {
public:
    StringRange(int pos, int len)
        : position(pos)
        , length(len)
    {
    }

    StringRange()
    {
    }

    int position;
    int length;
};

static ALWAYS_INLINE JSValue jsSpliceSubstringsWithSeparators(ExecState* exec, JSString* sourceVal, const UString& source, const StringRange* substringRanges, int rangeCount, const UString* separators, int separatorCount)
{
    if (rangeCount == 1 && separatorCount == 0) {
        int sourceSize = source.length();
        int position = substringRanges[0].position;
        int length = substringRanges[0].length;
        if (position <= 0 && length >= sourceSize)
            return sourceVal;
        // We could call UString::substr, but this would result in redundant checks
        return jsString(exec, StringImpl::create(source.impl(), max(0, position), min(sourceSize, length)));
    }

    int totalLength = 0;
    for (int i = 0; i < rangeCount; i++)
        totalLength += substringRanges[i].length;
    for (int i = 0; i < separatorCount; i++)
        totalLength += separators[i].length();

    if (totalLength == 0)
        return jsString(exec, "");

    UChar* buffer;
    PassRefPtr<StringImpl> impl = StringImpl::tryCreateUninitialized(totalLength, buffer);
    if (!impl)
        return throwOutOfMemoryError(exec);

    int maxCount = max(rangeCount, separatorCount);
    int bufferPos = 0;
    for (int i = 0; i < maxCount; i++) {
        if (i < rangeCount) {
            if (int srcLen = substringRanges[i].length) {
                StringImpl::copyChars(buffer + bufferPos, source.characters() + substringRanges[i].position, srcLen);
                bufferPos += srcLen;
            }
        }
        if (i < separatorCount) {
            if (int sepLen = separators[i].length()) {
                StringImpl::copyChars(buffer + bufferPos, separators[i].characters(), sepLen);
                bufferPos += sepLen;
            }
        }
    }

    return jsString(exec, impl);
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncReplace(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSString* sourceVal = thisValue.toThisJSString(exec);
    JSValue pattern = exec->argument(0);
    JSValue replacement = exec->argument(1);

    UString replacementString;
    CallData callData;
    CallType callType = getCallData(replacement, callData);
    if (callType == CallTypeNone)
        replacementString = replacement.toString(exec);

    if (pattern.inherits(&RegExpObject::s_info)) {
        const UString& source = sourceVal->value(exec);
        unsigned sourceLen = source.length();
        if (exec->hadException())
            return JSValue::encode(JSValue());
        RegExp* reg = asRegExpObject(pattern)->regExp();
        bool global = reg->global();

        RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();

        int lastIndex = 0;
        unsigned startPosition = 0;

        Vector<StringRange, 16> sourceRanges;
        Vector<UString, 16> replacements;

        // This is either a loop (if global is set) or a one-way (if not).
        if (global && callType == CallTypeJS) {
            // reg->numSubpatterns() + 1 for pattern args, + 2 for match start and sourceValue
            int argCount = reg->numSubpatterns() + 1 + 2;
            JSFunction* func = asFunction(replacement);
            CachedCall cachedCall(exec, func, argCount);
            if (exec->hadException())
                return JSValue::encode(jsNull());
            while (true) {
                int matchIndex;
                int matchLen = 0;
                int* ovector;
                regExpConstructor->performMatch(reg, source, startPosition, matchIndex, matchLen, &ovector);
                if (matchIndex < 0)
                    break;

                sourceRanges.append(StringRange(lastIndex, matchIndex - lastIndex));

                int completeMatchStart = ovector[0];
                unsigned i = 0;
                for (; i < reg->numSubpatterns() + 1; ++i) {
                    int matchStart = ovector[i * 2];
                    int matchLen = ovector[i * 2 + 1] - matchStart;

                    if (matchStart < 0)
                        cachedCall.setArgument(i, jsUndefined());
                    else
                        cachedCall.setArgument(i, jsSubstring(exec, source, matchStart, matchLen));
                }

                cachedCall.setArgument(i++, jsNumber(completeMatchStart));
                cachedCall.setArgument(i++, sourceVal);

                cachedCall.setThis(exec->globalThisValue());
                JSValue result = cachedCall.call();
                if (LIKELY(result.isString()))
                    replacements.append(asString(result)->value(exec));
                else
                    replacements.append(result.toString(cachedCall.newCallFrame(exec)));
                if (exec->hadException())
                    break;

                lastIndex = matchIndex + matchLen;
                startPosition = lastIndex;

                // special case of empty match
                if (matchLen == 0) {
                    startPosition++;
                    if (startPosition > sourceLen)
                        break;
                }
            }
        } else {
            do {
                int matchIndex;
                int matchLen = 0;
                int* ovector;
                regExpConstructor->performMatch(reg, source, startPosition, matchIndex, matchLen, &ovector);
                if (matchIndex < 0)
                    break;

                if (callType != CallTypeNone) {
                    sourceRanges.append(StringRange(lastIndex, matchIndex - lastIndex));

                    int completeMatchStart = ovector[0];
                    MarkedArgumentBuffer args;

                    for (unsigned i = 0; i < reg->numSubpatterns() + 1; ++i) {
                        int matchStart = ovector[i * 2];
                        int matchLen = ovector[i * 2 + 1] - matchStart;
 
                        if (matchStart < 0)
                            args.append(jsUndefined());
                        else
                            args.append(jsSubstring(exec, source, matchStart, matchLen));
                    }

                    args.append(jsNumber(completeMatchStart));
                    args.append(sourceVal);

                    replacements.append(call(exec, replacement, callType, callData, exec->globalThisValue(), args).toString(exec));
                    if (exec->hadException())
                        break;
                } else {
                    int replLen = replacementString.length();
                    if (lastIndex < matchIndex || replLen) {
                        sourceRanges.append(StringRange(lastIndex, matchIndex - lastIndex));
 
                        if (replLen)
                            replacements.append(substituteBackreferences(replacementString, source, ovector, reg));
                        else
                            replacements.append(UString());
                    }
                }

                lastIndex = matchIndex + matchLen;
                startPosition = lastIndex;

                // special case of empty match
                if (matchLen == 0) {
                    startPosition++;
                    if (startPosition > sourceLen)
                        break;
                }
            } while (global);
        }

        if (!lastIndex && replacements.isEmpty())
            return JSValue::encode(sourceVal);

        if (static_cast<unsigned>(lastIndex) < sourceLen)
            sourceRanges.append(StringRange(lastIndex, sourceLen - lastIndex));

        return JSValue::encode(jsSpliceSubstringsWithSeparators(exec, sourceVal, source, sourceRanges.data(), sourceRanges.size(), replacements.data(), replacements.size()));
    }

    // Not a regular expression, so treat the pattern as a string.

    UString patternString = pattern.toString(exec);
    // Special case for single character patterns without back reference replacement
    if (patternString.length() == 1 && callType == CallTypeNone && replacementString.find('$', 0) == notFound)
        return JSValue::encode(sourceVal->replaceCharacter(exec, patternString[0], replacementString));

    const UString& source = sourceVal->value(exec);
    size_t matchPos = source.find(patternString);

    if (matchPos == notFound)
        return JSValue::encode(sourceVal);

    int matchLen = patternString.length();
    if (callType != CallTypeNone) {
        MarkedArgumentBuffer args;
        args.append(jsSubstring(exec, source, matchPos, matchLen));
        args.append(jsNumber(matchPos));
        args.append(sourceVal);

        replacementString = call(exec, replacement, callType, callData, exec->globalThisValue(), args).toString(exec);
    }
    
    size_t matchEnd = matchPos + matchLen;
    int ovector[2] = { matchPos, matchEnd };
    return JSValue::encode(jsString(exec, source.substringSharingImpl(0, matchPos), substituteBackreferences(replacementString, source, ovector, 0), source.substringSharingImpl(matchEnd)));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    // Also used for valueOf.

    if (thisValue.isString())
        return JSValue::encode(thisValue);

    if (thisValue.inherits(&StringObject::s_info))
        return JSValue::encode(asStringObject(thisValue)->internalValue());

    return throwVMTypeError(exec);
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncCharAt(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    unsigned len = s.length();
    JSValue a0 = exec->argument(0);
    if (a0.isUInt32()) {
        uint32_t i = a0.asUInt32();
        if (i < len)
            return JSValue::encode(jsSingleCharacterSubstring(exec, s, i));
        return JSValue::encode(jsEmptyString(exec));
    }
    double dpos = a0.toInteger(exec);
    if (dpos >= 0 && dpos < len)
        return JSValue::encode(jsSingleCharacterSubstring(exec, s, static_cast<unsigned>(dpos)));
    return JSValue::encode(jsEmptyString(exec));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncCharCodeAt(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    unsigned len = s.length();
    JSValue a0 = exec->argument(0);
    if (a0.isUInt32()) {
        uint32_t i = a0.asUInt32();
        if (i < len)
            return JSValue::encode(jsNumber(s.characters()[i]));
        return JSValue::encode(jsNaN());
    }
    double dpos = a0.toInteger(exec);
    if (dpos >= 0 && dpos < len)
        return JSValue::encode(jsNumber(s[static_cast<int>(dpos)]));
    return JSValue::encode(jsNaN());
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncConcat(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isString() && (exec->argumentCount() == 1)) {
        JSValue v = exec->argument(0);
        return JSValue::encode(v.isString()
            ? jsString(exec, asString(thisValue), asString(v))
            : jsString(exec, asString(thisValue), v.toString(exec)));
    }
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    return JSValue::encode(jsString(exec, thisValue));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncIndexOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    int len = s.length();

    JSValue a0 = exec->argument(0);
    JSValue a1 = exec->argument(1);
    UString u2 = a0.toString(exec);
    int pos;
    if (a1.isUndefined())
        pos = 0;
    else if (a1.isUInt32())
        pos = min<uint32_t>(a1.asUInt32(), len);
    else {
        double dpos = a1.toInteger(exec);
        if (dpos < 0)
            dpos = 0;
        else if (dpos > len)
            dpos = len;
        pos = static_cast<int>(dpos);
    }

    size_t result = s.find(u2, pos);
    if (result == notFound)
        return JSValue::encode(jsNumber(-1));
    return JSValue::encode(jsNumber(result));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncLastIndexOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    int len = s.length();

    JSValue a0 = exec->argument(0);
    JSValue a1 = exec->argument(1);

    UString u2 = a0.toString(exec);
    double dpos = a1.toIntegerPreserveNaN(exec);
    if (dpos < 0)
        dpos = 0;
    else if (!(dpos <= len)) // true for NaN
        dpos = len;
#if OS(SYMBIAN)
    // Work around for broken NaN compare operator
    else if (isnan(dpos))
        dpos = len;
#endif

    size_t result = s.reverseFind(u2, static_cast<unsigned>(dpos));
    if (result == notFound)
        return JSValue::encode(jsNumber(-1));
    return JSValue::encode(jsNumber(result));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncMatch(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);

    JSValue a0 = exec->argument(0);

    RefPtr<RegExp> reg;
    if (a0.inherits(&RegExpObject::s_info))
        reg = asRegExpObject(a0)->regExp();
    else {
        /*
         *  ECMA 15.5.4.12 String.prototype.search (regexp)
         *  If regexp is not an object whose [[Class]] property is "RegExp", it is
         *  replaced with the result of the expression new RegExp(regexp).
         */
        reg = exec->globalData().regExpCache()->lookupOrCreate(a0.toString(exec), NoFlags);
    }
    RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();
    int pos;
    int matchLength = 0;
    regExpConstructor->performMatch(reg.get(), s, 0, pos, matchLength);
    if (!(reg->global())) {
        // case without 'g' flag is handled like RegExp.prototype.exec
        if (pos < 0)
            return JSValue::encode(jsNull());
        return JSValue::encode(regExpConstructor->arrayOfMatches(exec));
    }

    // return array of matches
    MarkedArgumentBuffer list;
    while (pos >= 0) {
        list.append(jsSubstring(exec, s, pos, matchLength));
        pos += matchLength == 0 ? 1 : matchLength;
        regExpConstructor->performMatch(reg.get(), s, pos, pos, matchLength);
    }
    if (list.isEmpty()) {
        // if there are no matches at all, it's important to return
        // Null instead of an empty array, because this matches
        // other browsers and because Null is a false value.
        return JSValue::encode(jsNull());
    }

    return JSValue::encode(constructArray(exec, list));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSearch(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);

    JSValue a0 = exec->argument(0);

    RefPtr<RegExp> reg;
    if (a0.inherits(&RegExpObject::s_info))
        reg = asRegExpObject(a0)->regExp();
    else { 
        /*
         *  ECMA 15.5.4.12 String.prototype.search (regexp)
         *  If regexp is not an object whose [[Class]] property is "RegExp", it is
         *  replaced with the result of the expression new RegExp(regexp).
         */
        reg = exec->globalData().regExpCache()->lookupOrCreate(a0.toString(exec), NoFlags);
    }
    RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();
    int pos;
    int matchLength = 0;
    regExpConstructor->performMatch(reg.get(), s, 0, pos, matchLength);
    return JSValue::encode(jsNumber(pos));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSlice(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    int len = s.length();

    JSValue a0 = exec->argument(0);
    JSValue a1 = exec->argument(1);

    // The arg processing is very much like ArrayProtoFunc::Slice
    double start = a0.toInteger(exec);
    double end = a1.isUndefined() ? len : a1.toInteger(exec);
    double from = start < 0 ? len + start : start;
    double to = end < 0 ? len + end : end;
    if (to > from && to > 0 && from < len) {
        if (from < 0)
            from = 0;
        if (to > len)
            to = len;
        return JSValue::encode(jsSubstring(exec, s, static_cast<unsigned>(from), static_cast<unsigned>(to) - static_cast<unsigned>(from)));
    }

    return JSValue::encode(jsEmptyString(exec));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSplit(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);

    JSValue a0 = exec->argument(0);
    JSValue a1 = exec->argument(1);

    JSArray* result = constructEmptyArray(exec);
    unsigned i = 0;
    unsigned p0 = 0;
    unsigned limit = a1.isUndefined() ? 0xFFFFFFFFU : a1.toUInt32(exec);
    if (a0.inherits(&RegExpObject::s_info)) {
        RegExp* reg = asRegExpObject(a0)->regExp();
        if (s.isEmpty() && reg->match(s, 0) >= 0) {
            // empty string matched by regexp -> empty array
            return JSValue::encode(result);
        }
        unsigned pos = 0;
        while (i != limit && pos < s.length()) {
            Vector<int, 32> ovector;
            int mpos = reg->match(s, pos, &ovector);
            if (mpos < 0)
                break;
            int mlen = ovector[1] - ovector[0];
            pos = mpos + (mlen == 0 ? 1 : mlen);
            if (static_cast<unsigned>(mpos) != p0 || mlen) {
                result->put(exec, i++, jsSubstring(exec, s, p0, mpos - p0));
                p0 = mpos + mlen;
            }
            for (unsigned si = 1; si <= reg->numSubpatterns(); ++si) {
                int spos = ovector[si * 2];
                if (spos < 0)
                    result->put(exec, i++, jsUndefined());
                else
                    result->put(exec, i++, jsSubstring(exec, s, spos, ovector[si * 2 + 1] - spos));
            }
        }
    } else {
        UString u2 = a0.toString(exec);
        if (u2.isEmpty()) {
            if (s.isEmpty()) {
                // empty separator matches empty string -> empty array
                return JSValue::encode(result);
            }
            while (i != limit && p0 < s.length() - 1)
                result->put(exec, i++, jsSingleCharacterSubstring(exec, s, p0++));
        } else {
            size_t pos;
            while (i != limit && (pos = s.find(u2, p0)) != notFound) {
                result->put(exec, i++, jsSubstring(exec, s, p0, pos - p0));
                p0 = pos + u2.length();
            }
        }
    }

    // add remaining string
    if (i != limit)
        result->put(exec, i++, jsSubstring(exec, s, p0, s.length() - p0));

    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSubstr(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    unsigned len;
    JSString* jsString = 0;
    UString uString;
    if (thisValue.isString()) {
        jsString = static_cast<JSString*>(thisValue.asCell());
        len = jsString->length();
    } else {
        uString = thisValue.toThisObject(exec)->toString(exec);
        len = uString.length();
    }

    JSValue a0 = exec->argument(0);
    JSValue a1 = exec->argument(1);

    double start = a0.toInteger(exec);
    double length = a1.isUndefined() ? len : a1.toInteger(exec);
    if (start >= len || length <= 0)
        return JSValue::encode(jsEmptyString(exec));
    if (start < 0) {
        start += len;
        if (start < 0)
            start = 0;
    }
    if (start + length > len)
        length = len - start;
    unsigned substringStart = static_cast<unsigned>(start);
    unsigned substringLength = static_cast<unsigned>(length);
    if (jsString)
        return JSValue::encode(jsSubstring(exec, jsString, substringStart, substringLength));
    return JSValue::encode(jsSubstring(exec, uString, substringStart, substringLength));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSubstring(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    int len;
    JSString* jsString = 0;
    UString uString;
    if (thisValue.isString()) {
        jsString = static_cast<JSString*>(thisValue.asCell());
        len = jsString->length();
    } else {
        uString = thisValue.toThisObject(exec)->toString(exec);
        len = uString.length();
    }

    JSValue a0 = exec->argument(0);
    JSValue a1 = exec->argument(1);

    double start = a0.toNumber(exec);
    double end;
    if (!(start >= 0)) // check for negative values or NaN
        start = 0;
    else if (start > len)
        start = len;
    if (a1.isUndefined())
        end = len;
    else { 
        end = a1.toNumber(exec);
        if (!(end >= 0)) // check for negative values or NaN
            end = 0;
        else if (end > len)
            end = len;
    }
    if (start > end) {
        double temp = end;
        end = start;
        start = temp;
    }
    unsigned substringStart = static_cast<unsigned>(start);
    unsigned substringLength = static_cast<unsigned>(end) - substringStart;
    if (jsString)
        return JSValue::encode(jsSubstring(exec, jsString, substringStart, substringLength));
    return JSValue::encode(jsSubstring(exec, uString, substringStart, substringLength));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncToLowerCase(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    JSString* sVal = thisValue.toThisJSString(exec);
    const UString& s = sVal->value(exec);

    int sSize = s.length();
    if (!sSize)
        return JSValue::encode(sVal);

    const UChar* sData = s.characters();
    Vector<UChar> buffer(sSize);

    UChar ored = 0;
    for (int i = 0; i < sSize; i++) {
        UChar c = sData[i];
        ored |= c;
        buffer[i] = toASCIILower(c);
    }
    if (!(ored & ~0x7f))
        return JSValue::encode(jsString(exec, UString::adopt(buffer)));

    bool error;
    int length = Unicode::toLower(buffer.data(), sSize, sData, sSize, &error);
    if (error) {
        buffer.resize(length);
        length = Unicode::toLower(buffer.data(), length, sData, sSize, &error);
        if (error)
            return JSValue::encode(sVal);
    }
    if (length == sSize) {
        if (memcmp(buffer.data(), sData, length * sizeof(UChar)) == 0)
            return JSValue::encode(sVal);
    } else
        buffer.resize(length);
    return JSValue::encode(jsString(exec, UString::adopt(buffer)));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncToUpperCase(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    JSString* sVal = thisValue.toThisJSString(exec);
    const UString& s = sVal->value(exec);

    int sSize = s.length();
    if (!sSize)
        return JSValue::encode(sVal);

    const UChar* sData = s.characters();
    Vector<UChar> buffer(sSize);

    UChar ored = 0;
    for (int i = 0; i < sSize; i++) {
        UChar c = sData[i];
        ored |= c;
        buffer[i] = toASCIIUpper(c);
    }
    if (!(ored & ~0x7f))
        return JSValue::encode(jsString(exec, UString::adopt(buffer)));

    bool error;
    int length = Unicode::toUpper(buffer.data(), sSize, sData, sSize, &error);
    if (error) {
        buffer.resize(length);
        length = Unicode::toUpper(buffer.data(), length, sData, sSize, &error);
        if (error)
            return JSValue::encode(sVal);
    }
    if (length == sSize) {
        if (memcmp(buffer.data(), sData, length * sizeof(UChar)) == 0)
            return JSValue::encode(sVal);
    } else
        buffer.resize(length);
    return JSValue::encode(jsString(exec, UString::adopt(buffer)));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncLocaleCompare(ExecState* exec)
{
    if (exec->argumentCount() < 1)
      return JSValue::encode(jsNumber(0));

    JSValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);

    UString s = thisValue.toThisString(exec);
    JSValue a0 = exec->argument(0);
    return JSValue::encode(jsNumber(localeCompare(s, a0.toString(exec))));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncBig(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<big>", s, "</big>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSmall(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<small>", s, "</small>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncBlink(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<blink>", s, "</blink>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncBold(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<b>", s, "</b>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncFixed(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<tt>", s, "</tt>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncItalics(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<i>", s, "</i>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncStrike(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<strike>", s, "</strike>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSub(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<sub>", s, "</sub>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncSup(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return JSValue::encode(jsMakeNontrivialString(exec, "<sup>", s, "</sup>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncFontcolor(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    JSValue a0 = exec->argument(0);
    return JSValue::encode(jsMakeNontrivialString(exec, "<font color=\"", a0.toString(exec), "\">", s, "</font>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncFontsize(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    JSValue a0 = exec->argument(0);

    uint32_t smallInteger;
    if (a0.getUInt32(smallInteger) && smallInteger <= 9) {
        unsigned stringSize = s.length();
        unsigned bufferSize = 22 + stringSize;
        UChar* buffer;
        PassRefPtr<StringImpl> impl = StringImpl::tryCreateUninitialized(bufferSize, buffer);
        if (!impl)
            return JSValue::encode(jsUndefined());
        buffer[0] = '<';
        buffer[1] = 'f';
        buffer[2] = 'o';
        buffer[3] = 'n';
        buffer[4] = 't';
        buffer[5] = ' ';
        buffer[6] = 's';
        buffer[7] = 'i';
        buffer[8] = 'z';
        buffer[9] = 'e';
        buffer[10] = '=';
        buffer[11] = '"';
        buffer[12] = '0' + smallInteger;
        buffer[13] = '"';
        buffer[14] = '>';
        memcpy(&buffer[15], s.characters(), stringSize * sizeof(UChar));
        buffer[15 + stringSize] = '<';
        buffer[16 + stringSize] = '/';
        buffer[17 + stringSize] = 'f';
        buffer[18 + stringSize] = 'o';
        buffer[19 + stringSize] = 'n';
        buffer[20 + stringSize] = 't';
        buffer[21 + stringSize] = '>';
        return JSValue::encode(jsNontrivialString(exec, impl));
    }

    return JSValue::encode(jsMakeNontrivialString(exec, "<font size=\"", a0.toString(exec), "\">", s, "</font>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncAnchor(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    JSValue a0 = exec->argument(0);
    return JSValue::encode(jsMakeNontrivialString(exec, "<a name=\"", a0.toString(exec), "\">", s, "</a>"));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncLink(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    JSValue a0 = exec->argument(0);
    UString linkText = a0.toString(exec);

    unsigned linkTextSize = linkText.length();
    unsigned stringSize = s.length();
    unsigned bufferSize = 15 + linkTextSize + stringSize;
    UChar* buffer;
    PassRefPtr<StringImpl> impl = StringImpl::tryCreateUninitialized(bufferSize, buffer);
    if (!impl)
        return JSValue::encode(jsUndefined());
    buffer[0] = '<';
    buffer[1] = 'a';
    buffer[2] = ' ';
    buffer[3] = 'h';
    buffer[4] = 'r';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '=';
    buffer[8] = '"';
    memcpy(&buffer[9], linkText.characters(), linkTextSize * sizeof(UChar));
    buffer[9 + linkTextSize] = '"';
    buffer[10 + linkTextSize] = '>';
    memcpy(&buffer[11 + linkTextSize], s.characters(), stringSize * sizeof(UChar));
    buffer[11 + linkTextSize + stringSize] = '<';
    buffer[12 + linkTextSize + stringSize] = '/';
    buffer[13 + linkTextSize + stringSize] = 'a';
    buffer[14 + linkTextSize + stringSize] = '>';
    return JSValue::encode(jsNontrivialString(exec, impl));
}

enum {
    TrimLeft = 1,
    TrimRight = 2
};

static inline bool isTrimWhitespace(UChar c)
{
    return isStrWhiteSpace(c) || c == 0x200b;
}

static inline JSValue trimString(ExecState* exec, JSValue thisValue, int trimKind)
{
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwTypeError(exec);
    UString str = thisValue.toThisString(exec);
    unsigned left = 0;
    if (trimKind & TrimLeft) {
        while (left < str.length() && isTrimWhitespace(str[left]))
            left++;
    }
    unsigned right = str.length();
    if (trimKind & TrimRight) {
        while (right > left && isTrimWhitespace(str[right - 1]))
            right--;
    }

    // Don't gc allocate a new string if we don't have to.
    if (left == 0 && right == str.length() && thisValue.isString())
        return thisValue;

    return jsString(exec, str.substringSharingImpl(left, right - left));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncTrim(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(trimString(exec, thisValue, TrimLeft | TrimRight));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncTrimLeft(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(trimString(exec, thisValue, TrimLeft));
}

EncodedJSValue JSC_HOST_CALL stringProtoFuncTrimRight(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    return JSValue::encode(trimString(exec, thisValue, TrimRight));
}
    
    
} // namespace JSC
