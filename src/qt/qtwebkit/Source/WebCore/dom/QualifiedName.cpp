/*
 * Copyright (C) 2005, 2006, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#ifdef SKIP_STATIC_CONSTRUCTORS_ON_GCC
#define WEBCORE_QUALIFIEDNAME_HIDE_GLOBALS 1
#else
#define QNAME_DEFAULT_CONSTRUCTOR
#endif

#include "QualifiedName.h"
#include "HTMLNames.h"
#include "XLinkNames.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>
#include <wtf/StaticConstructors.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(MATHML)
#include "MathMLNames.h"
#endif

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

namespace WebCore {

static const int staticQualifiedNamesCount = HTMLNames::HTMLTagsCount + HTMLNames::HTMLAttrsCount
#if ENABLE(MATHML)
    + MathMLNames::MathMLTagsCount + MathMLNames::MathMLAttrsCount
#endif
#if ENABLE(SVG)
    + SVGNames::SVGTagsCount + SVGNames::SVGAttrsCount
#endif
    + XLinkNames::XLinkAttrsCount
    + XMLNSNames::XMLNSAttrsCount
    + XMLNames::XMLAttrsCount;

struct QualifiedNameHashTraits : public HashTraits<QualifiedName::QualifiedNameImpl*> {
    static const int minimumTableSize = WTF::HashTableCapacityForSize<staticQualifiedNamesCount>::value;
};

typedef HashSet<QualifiedName::QualifiedNameImpl*, QualifiedNameHash, QualifiedNameHashTraits> QNameSet;

struct QNameComponentsTranslator {
    static unsigned hash(const QualifiedNameComponents& components)
    {
        return hashComponents(components); 
    }
    static bool equal(QualifiedName::QualifiedNameImpl* name, const QualifiedNameComponents& c)
    {
        return c.m_prefix == name->m_prefix.impl() && c.m_localName == name->m_localName.impl() && c.m_namespace == name->m_namespace.impl();
    }
    static void translate(QualifiedName::QualifiedNameImpl*& location, const QualifiedNameComponents& components, unsigned)
    {
        location = QualifiedName::QualifiedNameImpl::create(components.m_prefix, components.m_localName, components.m_namespace).leakRef();
    }
};

static inline QNameSet& qualifiedNameCache()
{
    DEFINE_STATIC_LOCAL(QNameSet, nameCache, ());
    return nameCache;
}

QualifiedName::QualifiedName(const AtomicString& p, const AtomicString& l, const AtomicString& n)
{
    QualifiedNameComponents components = { p.impl(), l.impl(), n.isEmpty() ? nullAtom.impl() : n.impl() };
    QNameSet::AddResult addResult = qualifiedNameCache().add<QNameComponentsTranslator>(components);
    m_impl = *addResult.iterator;
    if (!addResult.isNewEntry)
        m_impl->ref();
}

QualifiedName::~QualifiedName()
{
    deref();
}

void QualifiedName::deref()
{
#ifdef QNAME_DEFAULT_CONSTRUCTOR
    if (!m_impl)
        return;
#endif
    ASSERT(!isHashTableDeletedValue());
    m_impl->deref();
}

QualifiedName::QualifiedNameImpl::~QualifiedNameImpl()
{
    qualifiedNameCache().remove(this);
}

String QualifiedName::toString() const
{
    if (!hasPrefix())
        return localName();

    return prefix().string() + ':' + localName().string();
}

// Global init routines
DEFINE_GLOBAL(QualifiedName, anyName, nullAtom, starAtom, starAtom)

void QualifiedName::init()
{
    static bool initialized = false;
    if (initialized)
        return;

    // Use placement new to initialize the globals.
    AtomicString::init();
    new (NotNull, (void*)&anyName) QualifiedName(nullAtom, starAtom, starAtom);
    initialized = true;
}

const QualifiedName& nullQName()
{
    DEFINE_STATIC_LOCAL(QualifiedName, nullName, (nullAtom, nullAtom, nullAtom));
    return nullName;
}

const AtomicString& QualifiedName::localNameUpper() const
{
    if (!m_impl->m_localNameUpper)
        m_impl->m_localNameUpper = m_impl->m_localName.upper();
    return m_impl->m_localNameUpper;
}

unsigned QualifiedName::QualifiedNameImpl::computeHash() const
{
    QualifiedNameComponents components = { m_prefix.impl(), m_localName.impl(), m_namespace.impl() };
    return hashComponents(components);
}

void createQualifiedName(void* targetAddress, StringImpl* name, const AtomicString& nameNamespace)
{
    new (NotNull, reinterpret_cast<void*>(targetAddress)) QualifiedName(nullAtom, AtomicString(name), nameNamespace);
}

void createQualifiedName(void* targetAddress, StringImpl* name)
{
    new (NotNull, reinterpret_cast<void*>(targetAddress)) QualifiedName(nullAtom, AtomicString(name), nullAtom);
}

}
