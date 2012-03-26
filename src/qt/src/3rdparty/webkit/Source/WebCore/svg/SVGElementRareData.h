/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGElementRareData_h
#define SVGElementRareData_h

#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

class CSSCursorImageValue;
class SVGCursorElement;
class SVGElement;
class SVGElementInstance;

class SVGElementRareData {
    WTF_MAKE_NONCOPYABLE(SVGElementRareData); WTF_MAKE_FAST_ALLOCATED;
public:
    SVGElementRareData()
        : m_cursorElement(0)
        , m_cursorImageValue(0)
        , m_instancesUpdatesBlocked(false)
        , m_hasPendingResources(false)
    {
    }

    typedef HashMap<const SVGElement*, SVGElementRareData*> SVGElementRareDataMap;

    static SVGElementRareDataMap& rareDataMap()
    {
        DEFINE_STATIC_LOCAL(SVGElementRareDataMap, rareDataMap, ());
        return rareDataMap;
    }

    static SVGElementRareData* rareDataFromMap(const SVGElement* element)
    {
        return rareDataMap().get(element);
    }

    HashSet<SVGElementInstance*>& elementInstances() { return m_elementInstances; }
    const HashSet<SVGElementInstance*>& elementInstances() const { return m_elementInstances; }

    bool instanceUpdatesBlocked() const { return m_instancesUpdatesBlocked; }
    void setInstanceUpdatesBlocked(bool value) { m_instancesUpdatesBlocked = value; }

    bool hasPendingResources() const { return m_hasPendingResources; }
    void setHasPendingResources(bool value) { m_hasPendingResources = value; }

    SVGCursorElement* cursorElement() const { return m_cursorElement; }
    void setCursorElement(SVGCursorElement* cursorElement) { m_cursorElement = cursorElement; }

    CSSCursorImageValue* cursorImageValue() const { return m_cursorImageValue; }
    void setCursorImageValue(CSSCursorImageValue* cursorImageValue) { m_cursorImageValue = cursorImageValue; }

private:
    HashSet<SVGElementInstance*> m_elementInstances;
    SVGCursorElement* m_cursorElement;
    CSSCursorImageValue* m_cursorImageValue;
    bool m_instancesUpdatesBlocked : 1;
    bool m_hasPendingResources : 1;
};

}

#endif
