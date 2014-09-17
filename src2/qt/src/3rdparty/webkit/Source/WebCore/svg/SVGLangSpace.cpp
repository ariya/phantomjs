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

#include "config.h"

#if ENABLE(SVG)
#include "SVGLangSpace.h"

#include "Attribute.h"
#include "SVGElement.h"
#include "XMLNames.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

void SVGLangSpace::setXmllang(const AtomicString& xmlLang)
{
    m_lang = xmlLang;
}

const AtomicString& SVGLangSpace::xmlspace() const
{
    if (!m_space) {
        DEFINE_STATIC_LOCAL(const AtomicString, defaultString, ("default"));
        return defaultString;
    }

    return m_space;
}

void SVGLangSpace::setXmlspace(const AtomicString& xmlSpace)
{
    m_space = xmlSpace;
}

bool SVGLangSpace::parseMappedAttribute(Attribute* attr)
{
    if (attr->name().matches(XMLNames::langAttr)) {
        setXmllang(attr->value());
        return true;
    }
    if (attr->name().matches(XMLNames::spaceAttr)) {
        setXmlspace(attr->value());
        return true;
    }

    return false;
}

bool SVGLangSpace::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName.matches(XMLNames::langAttr) || attrName.matches(XMLNames::spaceAttr);
}

}

#endif // ENABLE(SVG)
