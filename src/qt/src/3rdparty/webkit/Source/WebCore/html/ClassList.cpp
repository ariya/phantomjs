/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ClassList.h"

#include "Element.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "SpaceSplitString.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

ClassList::ClassList(Element* element)
    : m_element(element)
{
    if (m_element->document()->inQuirksMode())
        m_classNamesForQuirksMode.set(m_element->fastGetAttribute(classAttr), false);
}

void ClassList::ref()
{
    m_element->ref();
}

void ClassList::deref()
{
    m_element->deref();
}

unsigned ClassList::length() const
{
    return m_element->hasClass() ? classNames().size() : 0;
}

const AtomicString ClassList::item(unsigned index) const
{
    if (index >= length())
        return AtomicString();
    return classNames()[index];
}

bool ClassList::contains(const AtomicString& token, ExceptionCode& ec) const
{
    if (!validateToken(token, ec))
        return false;
    return containsInternal(token);
}

bool ClassList::containsInternal(const AtomicString& token) const
{
    return m_element->hasClass() && classNames().contains(token);
}

void ClassList::add(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec))
        return;
    addInternal(token);
}

void ClassList::addInternal(const AtomicString& token)
{
    const AtomicString& oldClassName(m_element->fastGetAttribute(classAttr));
    if (oldClassName.isEmpty())
        m_element->setAttribute(classAttr, token);
    else if (!containsInternal(token)) {
        const AtomicString& newClassName(addToken(oldClassName, token));
        m_element->setAttribute(classAttr, newClassName);
    }
}

void ClassList::remove(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec))
        return;
    removeInternal(token);
}

void ClassList::removeInternal(const AtomicString& token)
{
    // Check using contains first since it uses AtomicString comparisons instead
    // of character by character testing.
    if (!containsInternal(token))
        return;
    const AtomicString& newClassName(removeToken(m_element->fastGetAttribute(classAttr), token));
    m_element->setAttribute(classAttr, newClassName);
}

bool ClassList::toggle(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec))
        return false;

    if (containsInternal(token)) {
        removeInternal(token);
        return false;
    }
    addInternal(token);
    return true;
}

String ClassList::toString() const
{
    return m_element->fastGetAttribute(classAttr);
}

void ClassList::reset(const String& newClassName)
{
    if (!m_classNamesForQuirksMode.isNull())
        m_classNamesForQuirksMode.set(newClassName, false);
}

const SpaceSplitString& ClassList::classNames() const
{
    ASSERT(m_element->hasClass());
    if (!m_classNamesForQuirksMode.isNull())
        return m_classNamesForQuirksMode;
    return m_element->attributeMap()->classNames();
}

} // namespace WebCore
