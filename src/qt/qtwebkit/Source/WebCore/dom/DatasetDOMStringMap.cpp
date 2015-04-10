/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DatasetDOMStringMap.h"

#include "Attribute.h"
#include "Element.h"
#include "ExceptionCode.h"
#include <wtf/ASCIICType.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static bool isValidAttributeName(const String& name)
{
    if (!name.startsWith("data-"))
        return false;

    unsigned length = name.length();
    for (unsigned i = 5; i < length; ++i) {
        if (isASCIIUpper(name[i]))
            return false;
    }

    return true;
}

static String convertAttributeNameToPropertyName(const String& name)
{
    StringBuilder stringBuilder;

    unsigned length = name.length();
    for (unsigned i = 5; i < length; ++i) {
        UChar character = name[i];
        if (character != '-')
            stringBuilder.append(character);
        else {
            if ((i + 1 < length) && isASCIILower(name[i + 1])) {
                stringBuilder.append(toASCIIUpper(name[i + 1]));
                ++i;
            } else
                stringBuilder.append(character);
        }
    }

    return stringBuilder.toString();
}

static bool propertyNameMatchesAttributeName(const String& propertyName, const String& attributeName)
{
    if (!attributeName.startsWith("data-"))
        return false;

    unsigned propertyLength = propertyName.length();
    unsigned attributeLength = attributeName.length();

    unsigned a = 5;
    unsigned p = 0;
    bool wordBoundary = false;
    while (a < attributeLength && p < propertyLength) {
        const UChar currentAttributeNameChar = attributeName[a];
        if (currentAttributeNameChar == '-' && a + 1 < attributeLength && attributeName[a + 1] != '-')
            wordBoundary = true;
        else {
            if ((wordBoundary ? toASCIIUpper(currentAttributeNameChar) : currentAttributeNameChar) != propertyName[p])
                return false;
            p++;
            wordBoundary = false;
        }
        a++;
    }

    return (a == attributeLength && p == propertyLength);
}

static bool isValidPropertyName(const String& name)
{
    unsigned length = name.length();
    for (unsigned i = 0; i < length; ++i) {
        if (name[i] == '-' && (i + 1 < length) && isASCIILower(name[i + 1]))
            return false;
    }
    return true;
}

static String convertPropertyNameToAttributeName(const String& name)
{
    StringBuilder builder;
    builder.append("data-");

    unsigned length = name.length();
    for (unsigned i = 0; i < length; ++i) {
        UChar character = name[i];
        if (isASCIIUpper(character)) {
            builder.append('-');
            builder.append(toASCIILower(character));
        } else
            builder.append(character);
    }

    return builder.toString();
}

void DatasetDOMStringMap::ref()
{
    m_element->ref();
}

void DatasetDOMStringMap::deref()
{
    m_element->deref();
}

void DatasetDOMStringMap::getNames(Vector<String>& names)
{
    if (!m_element->hasAttributes())
        return;

    unsigned length = m_element->attributeCount();
    for (unsigned i = 0; i < length; i++) {
        const Attribute* attribute = m_element->attributeItem(i);
        if (isValidAttributeName(attribute->localName()))
            names.append(convertAttributeNameToPropertyName(attribute->localName()));
    }
}

String DatasetDOMStringMap::item(const String& name)
{
    if (!m_element->hasAttributes())
        return String();

    unsigned length = m_element->attributeCount();
    for (unsigned i = 0; i < length; i++) {
        const Attribute* attribute = m_element->attributeItem(i);
        if (propertyNameMatchesAttributeName(name, attribute->localName()))
            return attribute->value();
    }

    return String();
}

bool DatasetDOMStringMap::contains(const String& name)
{
    if (!m_element->hasAttributes())
        return false;

    unsigned length = m_element->attributeCount();
    for (unsigned i = 0; i < length; i++) {
        const Attribute* attribute = m_element->attributeItem(i);
        if (propertyNameMatchesAttributeName(name, attribute->localName()))
            return true;
    }

    return false;
}

void DatasetDOMStringMap::setItem(const String& name, const String& value, ExceptionCode& ec)
{
    if (!isValidPropertyName(name)) {
        ec = SYNTAX_ERR;
        return;
    }

    m_element->setAttribute(convertPropertyNameToAttributeName(name), value, ec);
}

void DatasetDOMStringMap::deleteItem(const String& name, ExceptionCode& ec)
{
    if (!isValidPropertyName(name)) {
        ec = SYNTAX_ERR;
        return;
    }

    m_element->removeAttribute(convertPropertyNameToAttributeName(name));
}

} // namespace WebCore
