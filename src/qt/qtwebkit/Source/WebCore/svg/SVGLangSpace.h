/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGLangSpace_h
#define SVGLangSpace_h

#if ENABLE(SVG)
#include "QualifiedName.h"
#include <wtf/HashSet.h>

namespace WebCore {

class Attribute;

class SVGLangSpace {
public:
    const AtomicString& xmllang() const { return m_lang; }
    void setXmllang(const AtomicString& xmlLang);

    const AtomicString& xmlspace() const;
    void setXmlspace(const AtomicString& xmlSpace);

    bool parseAttribute(const QualifiedName&, const AtomicString&);
    bool isKnownAttribute(const QualifiedName&);
    void addSupportedAttributes(HashSet<QualifiedName>&);

private:
    AtomicString m_lang;
    AtomicString m_space;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGLangSpace_h
