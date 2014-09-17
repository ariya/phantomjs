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
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>
#include <wtf/StaticConstructors.h>

namespace WebCore {

typedef HashSet<QualifiedName::QualifiedNameImpl*, QualifiedNameHash> QNameSet;

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
        location = QualifiedName::QualifiedNameImpl::create(components.m_prefix, components.m_localName, components.m_namespace).releaseRef();
    }
};

static QNameSet* gNameCache;

void QualifiedName::init(const AtomicString& p, const AtomicString& l, const AtomicString& n)
{
    if (!gNameCache)
        gNameCache = new QNameSet;
    QualifiedNameComponents components = { p.impl(), l.impl(), n.isEmpty() ? nullAtom.impl() : n.impl() };
    pair<QNameSet::iterator, bool> addResult = gNameCache->add<QualifiedNameComponents, QNameComponentsTranslator>(components);
    m_impl = *addResult.first;    
    if (!addResult.second)
        m_impl->ref();
}

QualifiedName::QualifiedName(const AtomicString& p, const AtomicString& l, const AtomicString& n)
{
    init(p, l, n);
}

QualifiedName::QualifiedName(const AtomicString& p, const char* l, const AtomicString& n)
{
    init(p, AtomicString(l), n);
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

    if (m_impl->hasOneRef())
        gNameCache->remove(m_impl);
    m_impl->deref();
}

String QualifiedName::toString() const
{
    String local = localName();
    if (hasPrefix()) {
        String result = prefix().string();
        result.append(":");
        result.append(local);
        return result;
    }
    return local;
}

// Global init routines
DEFINE_GLOBAL(QualifiedName, anyName, nullAtom, starAtom, starAtom)

void QualifiedName::init()
{
    static bool initialized;
    if (!initialized) {
        // Use placement new to initialize the globals.
        
        AtomicString::init();
        new ((void*)&anyName) QualifiedName(nullAtom, starAtom, starAtom);
        initialized = true;
    }
}

const AtomicString& QualifiedName::localNameUpper() const
{
    if (!m_impl->m_localNameUpper)
        m_impl->m_localNameUpper = m_impl->m_localName.upper();
    return m_impl->m_localNameUpper;
}

}
