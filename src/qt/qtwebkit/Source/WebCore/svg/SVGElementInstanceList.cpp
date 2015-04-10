/*
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "SVGElementInstanceList.h"

#include "SVGElementInstance.h"

namespace WebCore {

SVGElementInstanceList::SVGElementInstanceList(PassRefPtr<SVGElementInstance> rootInstance)
    : m_rootInstance(rootInstance)
{
}

SVGElementInstanceList::~SVGElementInstanceList()
{
}

unsigned SVGElementInstanceList::length() const
{
    // NOTE: We could use the same caching facilities, like the ones "ChildNodeList" uses.
    unsigned length = 0;
    for (SVGElementInstance* instance = m_rootInstance->firstChild(); instance; instance = instance->nextSibling())
        length++;
    return length;
}

SVGElementInstance* SVGElementInstanceList::item(unsigned index)
{
    unsigned pos = 0;
    SVGElementInstance* instance = m_rootInstance->firstChild();
    while (instance && pos < index) {
        instance = instance->nextSibling();
        pos++;
    }
    return instance;
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
