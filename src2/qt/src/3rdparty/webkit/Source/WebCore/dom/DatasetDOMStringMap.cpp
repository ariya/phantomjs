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
#include "NamedNodeMap.h"
#include <wtf/ASCIICType.h>

namespace WebCore {

static bool isValidAttributeName(const String& name)
{
    if (!name.startsWith("data-"))
        return false;

    const UChar* characters = name.characters();
    unsigned length = name.length();
    for (unsigned i = 5; i < length; ++i) {
        if (isASCIIUpper(characters[i]))
            return false;
    }

    return true;
}

static String convertAttributeNameToPropertyName(const String& name)
{
    Vector<UChar> newStringBuffer;

    const UChar* characters = name.characters();
    unsigned length = name.length();
    for (unsigned i = 5; i < length; ++i) {
        if (characters[i] != '-')
            newStringBuffer.append(characters[i]);
        else {
            if ((i + 1 < length) && isASCIILower(characters[i + 1])) {
                newStringBuffer.append(toASCIIUpper(characters[i + 1]));
                ++i;
            } else
                newStringBuffer.append(characters[i]);
        }
    }

    return String::adopt(newStringBuffer);
}

static bool propertyNameMatchesAttributeName(const String& propertyName, const String& attributeName)
{
    if (!attributeName.startsWith("data-"))
        return false;

    const UChar* property = propertyName.characters();
    const UChar* attribute = attributeName.characters();
    unsigned propertyLength = propertyName.length();
    unsigned attributeLength = attributeName.length();
   
    unsigned a = 5;
    unsigned p = 0;
    bool wordBoundary = false;
    while (a < attributeLength && p < propertyLength) {
        if (attribute[a] == '-' && a + 1 < attributeLength && attribute[a + 1] != '-')
            wordBoundary = true;
        else {
            if ((wordBoundary ? toASCIIUpper(attribute[a]) : attribute[a]) != property[p])
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
    const UChar* characters = name.characters();
    unsigned length = name.length();
    for (unsigned i = 0; i < length; ++i) {
        if (characters[i] == '-' && (i + 1 < length) && isASCIILower(characters[i + 1]))
            return false;
    }
    return true;
}

static String convertPropertyNameToAttributeName(const String& name)
{
    Vector<UChar> newStringBuffer;

    newStringBuffer.append('d');
    newStringBuffer.append('a');
    newStringBuffer.append('t');
    newStringBuffer.append('a');
    newStringBuffer.append('-');

    const UChar* characters = name.characters();
    unsigned length = name.length();
    for (unsigned i = 0; i < length; ++i) {
        if (isASCIIUpper(characters[i])) {
            newStringBuffer.append('-');
            newStringBuffer.append(toASCIILower(characters[i]));
        } else
            newStringBuffer.append(characters[i]);
    }

    return String::adopt(newStringBuffer);
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
    NamedNodeMap* attributeMap = m_element->attributes(true);
    if (attributeMap) {
        unsigned length = attributeMap->length();
        for (unsigned i = 0; i < length; i++) {
            Attribute* attribute = attributeMap->attributeItem(i);
            if (isValidAttributeName(attribute->localName()))
                names.append(convertAttributeNameToPropertyName(attribute->localName()));
        }
    }
}

String DatasetDOMStringMap::item(const String& name)
{
    NamedNodeMap* attributeMap = m_element->attributes(true);
    if (attributeMap) {
        unsigned length = attributeMap->length();
        for (unsigned i = 0; i < length; i++) {
            Attribute* attribute = attributeMap->attributeItem(i);
            if (propertyNameMatchesAttributeName(name, attribute->localName()))
                return attribute->value();
        }
    }

    return String();
}

bool DatasetDOMStringMap::contains(const String& name)
{
    NamedNodeMap* attributeMap = m_element->attributes(true);
    if (attributeMap) {
        unsigned length = attributeMap->length();
        for (unsigned i = 0; i < length; i++) {
            Attribute* attribute = attributeMap->attributeItem(i);
            if (propertyNameMatchesAttributeName(name, attribute->localName()))
                return true;
        }
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

    ExceptionCode dummy;
    m_element->removeAttribute(convertPropertyNameToAttributeName(name), dummy);
}

} // namespace WebCore
