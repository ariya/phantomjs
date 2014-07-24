/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSCSSStyleDeclarationCustom.h"

#include "CSSParser.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSValue.h"
#include "HashTools.h"
#include "JSCSSValue.h"
#include "JSNode.h"
#include "RuntimeEnabledFeatures.h"
#include "Settings.h"
#include "StylePropertySet.h"
#include <runtime/StringPrototype.h>
#include <wtf/ASCIICType.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/text/WTFString.h>

using namespace JSC;
using namespace WTF;
using namespace std;

namespace WebCore {

void JSCSSStyleDeclaration::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSCSSStyleDeclaration* thisObject = jsCast<JSCSSStyleDeclaration*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);
    visitor.addOpaqueRoot(root(thisObject->impl()));
}

class CSSPropertyInfo {
public:
    CSSPropertyID propertyID;
    bool hadPixelOrPosPrefix;
};

enum PropertyNamePrefix
{
    PropertyNamePrefixNone,
    PropertyNamePrefixCSS,
    PropertyNamePrefixPixel,
    PropertyNamePrefixPos,
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    PropertyNamePrefixApple,
#endif
    PropertyNamePrefixEpub,
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    PropertyNamePrefixKHTML,
#endif
    PropertyNamePrefixWebKit
};

template<size_t prefixCStringLength>
static inline bool matchesCSSPropertyNamePrefix(const StringImpl& propertyName, const char (&prefix)[prefixCStringLength])
{
    size_t prefixLength = prefixCStringLength - 1;

    ASSERT(toASCIILower(propertyName[0]) == prefix[0]);
    const size_t offset = 1;

#ifndef NDEBUG
    for (size_t i = 0; i < prefixLength; ++i)
        ASSERT(isASCIILower(prefix[i]));
    ASSERT(!prefix[prefixLength]);
    ASSERT(propertyName.length());
#endif

    // The prefix within the property name must be followed by a capital letter.
    // Other characters in the prefix within the property name must be lowercase.
    if (propertyName.length() < (prefixLength + 1))
        return false;

    for (size_t i = offset; i < prefixLength; ++i) {
        if (propertyName[i] != prefix[i])
            return false;
    }

    if (!isASCIIUpper(propertyName[prefixLength]))
        return false;
    return true;
}

static PropertyNamePrefix getCSSPropertyNamePrefix(const StringImpl& propertyName)
{
    ASSERT(propertyName.length());

    // First character of the prefix within the property name may be upper or lowercase.
    UChar firstChar = toASCIILower(propertyName[0]);
    switch (firstChar) {
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    case 'a':
        if (RuntimeEnabledFeatures::legacyCSSVendorPrefixesEnabled() && matchesCSSPropertyNamePrefix(propertyName, "apple"))
            return PropertyNamePrefixApple;
        break;
#endif
    case 'c':
        if (matchesCSSPropertyNamePrefix(propertyName, "css"))
            return PropertyNamePrefixCSS;
        break;
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    case 'k':
        if (RuntimeEnabledFeatures::legacyCSSVendorPrefixesEnabled() && matchesCSSPropertyNamePrefix(propertyName, "khtml"))
            return PropertyNamePrefixKHTML;
        break;
#endif
    case 'e':
        if (matchesCSSPropertyNamePrefix(propertyName, "epub"))
            return PropertyNamePrefixEpub;
        break;
    case 'p':
        if (matchesCSSPropertyNamePrefix(propertyName, "pos"))
            return PropertyNamePrefixPos;
        if (matchesCSSPropertyNamePrefix(propertyName, "pixel"))
            return PropertyNamePrefixPixel;
        break;
    case 'w':
        if (matchesCSSPropertyNamePrefix(propertyName, "webkit"))
            return PropertyNamePrefixWebKit;
        break;
    default:
        break;
    }
    return PropertyNamePrefixNone;
}

static inline void writeWebKitPrefix(char*& buffer)
{
    *buffer++ = '-';
    *buffer++ = 'w';
    *buffer++ = 'e';
    *buffer++ = 'b';
    *buffer++ = 'k';
    *buffer++ = 'i';
    *buffer++ = 't';
    *buffer++ = '-';
}

static inline void writeEpubPrefix(char*& buffer)
{
    *buffer++ = '-';
    *buffer++ = 'e';
    *buffer++ = 'p';
    *buffer++ = 'u';
    *buffer++ = 'b';
    *buffer++ = '-';
}

static CSSPropertyInfo cssPropertyIDForJSCSSPropertyName(PropertyName propertyName)
{
    CSSPropertyInfo propertyInfo = {CSSPropertyInvalid, false};
    bool hadPixelOrPosPrefix = false;

    StringImpl* propertyNameString = propertyName.publicName();
    if (!propertyNameString)
        return propertyInfo;
    unsigned length = propertyNameString->length();
    if (!length)
        return propertyInfo;

    String stringForCache = String(propertyNameString);
    typedef HashMap<String, CSSPropertyInfo> CSSPropertyInfoMap;
    DEFINE_STATIC_LOCAL(CSSPropertyInfoMap, propertyInfoCache, ());
    propertyInfo = propertyInfoCache.get(stringForCache);
    if (propertyInfo.propertyID)
        return propertyInfo;

    const size_t bufferSize = maxCSSPropertyNameLength + 1;
    char buffer[bufferSize];
    char* bufferPtr = buffer;
    const char* name = bufferPtr;

    unsigned i = 0;
    // Prefixes CSS, Pixel, Pos are ignored.
    // Prefixes Apple, KHTML and Webkit are transposed to "-webkit-".
    // The prefix "Epub" becomes "-epub-".
    switch (getCSSPropertyNamePrefix(*propertyNameString)) {
    case PropertyNamePrefixNone:
        if (isASCIIUpper((*propertyNameString)[0]))
            return propertyInfo;
        break;
    case PropertyNamePrefixCSS:
        i += 3;
        break;
    case PropertyNamePrefixPixel:
        i += 5;
        hadPixelOrPosPrefix = true;
        break;
    case PropertyNamePrefixPos:
        i += 3;
        hadPixelOrPosPrefix = true;
        break;
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    case PropertyNamePrefixApple:
    case PropertyNamePrefixKHTML:
        ASSERT(RuntimeEnabledFeatures::legacyCSSVendorPrefixesEnabled());
        writeWebKitPrefix(bufferPtr);
        i += 5;
        break;
#endif
    case PropertyNamePrefixEpub:
        writeEpubPrefix(bufferPtr);
        i += 4;
        break;
    case PropertyNamePrefixWebKit:
        writeWebKitPrefix(bufferPtr);
        i += 6;
        break;
    }

    *bufferPtr++ = toASCIILower((*propertyNameString)[i++]);

    char* bufferEnd = buffer + bufferSize;
    char* stringEnd = bufferEnd - 1;
    size_t bufferSizeLeft = stringEnd - bufferPtr;
    size_t propertySizeLeft = length - i;
    if (propertySizeLeft > bufferSizeLeft)
        return propertyInfo;

    for (; i < length; ++i) {
        UChar c = (*propertyNameString)[i];
        if (!c || c >= 0x7F)
            return propertyInfo; // illegal character
        if (isASCIIUpper(c)) {
            size_t bufferSizeLeft = stringEnd - bufferPtr;
            size_t propertySizeLeft = length - i + 1;
            if (propertySizeLeft > bufferSizeLeft)
                return propertyInfo;
            *bufferPtr++ = '-';
            *bufferPtr++ = toASCIILower(c);
        } else
            *bufferPtr++ = c;
        ASSERT(bufferPtr < bufferEnd);
    }
    ASSERT(bufferPtr < bufferEnd);
    *bufferPtr = '\0';

    unsigned outputLength = bufferPtr - buffer;
#if PLATFORM(IOS)
    cssPropertyNameIOSAliasing(buffer, name, outputLength);
#endif

    const Property* hashTableEntry = findProperty(name, outputLength);
    int propertyID = hashTableEntry ? hashTableEntry->id : 0;
    if (propertyID) {
        propertyInfo.hadPixelOrPosPrefix = hadPixelOrPosPrefix;
        propertyInfo.propertyID = static_cast<CSSPropertyID>(propertyID);
        propertyInfoCache.add(stringForCache, propertyInfo);
    }
    return propertyInfo;
}

static inline JSValue getPropertyValueFallback(ExecState* exec, JSCSSStyleDeclaration* thisObj, unsigned index)
{
    // If the property is a shorthand property (such as "padding"),
    // it can only be accessed using getPropertyValue.
    return jsStringWithCache(exec, thisObj->impl()->getPropertyValueInternal(static_cast<CSSPropertyID>(index)));
}

static inline JSValue cssPropertyGetterPixelOrPosPrefix(ExecState* exec, JSCSSStyleDeclaration* thisObj, unsigned propertyID)
{
    // Set up pixelOrPos boolean to handle the fact that
    // pixelTop returns "CSS Top" as number value in unit pixels
    // posTop returns "CSS top" as number value in unit pixels _if_ its a
    // positioned element. if it is not a positioned element, return 0
    // from MSIE documentation FIXME: IMPLEMENT THAT (Dirk)
    RefPtr<CSSValue> v = thisObj->impl()->getPropertyCSSValueInternal(static_cast<CSSPropertyID>(propertyID));
    if (v) {
        if (v->isPrimitiveValue())
            return jsNumber(static_pointer_cast<CSSPrimitiveValue>(v)->getFloatValue(CSSPrimitiveValue::CSS_PX));
        return jsStringOrNull(exec, v->cssText());
    }

    return getPropertyValueFallback(exec, thisObj, propertyID);
}

static JSValue cssPropertyGetterPixelOrPosPrefixCallback(ExecState* exec, JSValue slotBase, unsigned propertyID)
{
    return cssPropertyGetterPixelOrPosPrefix(exec, jsCast<JSCSSStyleDeclaration*>(asObject(slotBase)), propertyID);
}

static inline JSValue cssPropertyGetter(ExecState* exec, JSCSSStyleDeclaration* thisObj, unsigned propertyID)
{
    RefPtr<CSSValue> v = thisObj->impl()->getPropertyCSSValueInternal(static_cast<CSSPropertyID>(propertyID));
    if (v)
        return jsStringOrNull(exec, v->cssText());

    return getPropertyValueFallback(exec, thisObj, propertyID);
}

static JSValue cssPropertyGetterCallback(ExecState* exec, JSValue slotBase, unsigned propertyID)
{
    return cssPropertyGetter(exec, jsCast<JSCSSStyleDeclaration*>(asObject(slotBase)), propertyID);
}

bool JSCSSStyleDeclaration::getOwnPropertySlotDelegate(ExecState*, PropertyName propertyIdentifier, PropertySlot& slot)
{
    CSSPropertyInfo propertyInfo = cssPropertyIDForJSCSSPropertyName(propertyIdentifier);
    if (!propertyInfo.propertyID)
        return false;

    if (propertyInfo.hadPixelOrPosPrefix)
        slot.setCustomIndex(this, static_cast<unsigned>(propertyInfo.propertyID), cssPropertyGetterPixelOrPosPrefixCallback);
    else
        slot.setCustomIndex(this, static_cast<unsigned>(propertyInfo.propertyID), cssPropertyGetterCallback);
    return true;
}

bool JSCSSStyleDeclaration::getOwnPropertyDescriptorDelegate(JSC::ExecState* exec, JSC::PropertyName propertyIdentifier, JSC::PropertyDescriptor& descriptor)
{
    CSSPropertyInfo propertyInfo = cssPropertyIDForJSCSSPropertyName(propertyIdentifier);
    if (!propertyInfo.propertyID)
        return false;

    JSValue value;
    if (propertyInfo.hadPixelOrPosPrefix)
        value = cssPropertyGetterPixelOrPosPrefix(exec, this, propertyInfo.propertyID);
    else
        value = cssPropertyGetter(exec, this, propertyInfo.propertyID);
    descriptor.setDescriptor(value, ReadOnly | DontDelete | DontEnum);
    return true;
}

bool JSCSSStyleDeclaration::putDelegate(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot&)
{
    CSSPropertyInfo propertyInfo = cssPropertyIDForJSCSSPropertyName(propertyName);
    if (!propertyInfo.propertyID)
        return false;

    String propValue = valueToStringWithNullCheck(exec, value);
    if (propertyInfo.hadPixelOrPosPrefix)
        propValue.append("px");

    bool important = false;
    if (Settings::shouldRespectPriorityInCSSAttributeSetters()) {
        size_t importantIndex = propValue.find("!important", 0, false);
        if (importantIndex != notFound) {
            important = true;
            propValue = propValue.left(importantIndex - 1);
        }
    }

    ExceptionCode ec = 0;
    impl()->setPropertyInternal(static_cast<CSSPropertyID>(propertyInfo.propertyID), propValue, important, ec);
    setDOMException(exec, ec);
    return true;
}

JSValue JSCSSStyleDeclaration::getPropertyCSSValue(ExecState* exec)
{
    const String& propertyName = exec->argument(0).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();

    RefPtr<CSSValue> cssValue = impl()->getPropertyCSSValue(propertyName);
    if (!cssValue)
        return jsNull();

    currentWorld(exec)->m_cssValueRoots.add(cssValue.get(), root(impl())); // Balanced by JSCSSValueOwner::finalize().
    return toJS(exec, globalObject(), WTF::getPtr(cssValue));
}

void JSCSSStyleDeclaration::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSCSSStyleDeclaration* thisObject = jsCast<JSCSSStyleDeclaration*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);

    unsigned length = thisObject->impl()->length();
    for (unsigned i = 0; i < length; ++i)
        propertyNames.add(Identifier::from(exec, i));

    static Identifier* propertyIdentifiers = 0;
    if (!propertyIdentifiers) {
        Vector<String, numCSSProperties> jsPropertyNames;
        for (int id = firstCSSProperty; id < firstCSSProperty + numCSSProperties; ++id)
            jsPropertyNames.append(getJSPropertyName(static_cast<CSSPropertyID>(id)));
        sort(jsPropertyNames.begin(), jsPropertyNames.end(), WTF::codePointCompareLessThan);

        propertyIdentifiers = new Identifier[numCSSProperties];
        for (int i = 0; i < numCSSProperties; ++i)
            propertyIdentifiers[i] = Identifier(exec, jsPropertyNames[i].impl());
    }

    for (int i = 0; i < numCSSProperties; ++i)
        propertyNames.add(propertyIdentifiers[i]);

    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

} // namespace WebCore
