/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#ifndef SVGExternalResourcesRequired_h
#define SVGExternalResourcesRequired_h

#if ENABLE(SVG)
#include "QualifiedName.h"
#include <wtf/HashSet.h>

namespace WebCore {

class Attribute;
class SVGElement;

// Notes on a SVG 1.1 spec discrepancy:
// The SVG DOM defines the attribute externalResourcesRequired as being of type SVGAnimatedBoolean, whereas the 
// SVG language definition says that externalResourcesRequired is not animated. Because the SVG language definition
// states that externalResourcesRequired cannot be animated, the animVal will always be the same as the baseVal.
// FIXME: When implementing animVal support, make sure that animVal==baseVal for externalResourcesRequired
class SVGExternalResourcesRequired {
public:
    virtual ~SVGExternalResourcesRequired() { }

    bool parseAttribute(const QualifiedName&, const AtomicString&);
    bool isKnownAttribute(const QualifiedName&);
    void addSupportedAttributes(HashSet<QualifiedName>&);
    bool handleAttributeChange(SVGElement*, const QualifiedName&);

protected:
    // These types look a bit awkward, but have to match the generic types of the SVGAnimatedProperty macros.
    virtual void setExternalResourcesRequiredBaseValue(const bool&, const bool validValue = true) = 0;
    virtual bool& externalResourcesRequiredBaseValue() const = 0;

    virtual void setHaveFiredLoadEvent(bool) { }
    virtual bool isParserInserted() const { return false; }
    virtual bool haveFiredLoadEvent() const { return false; }

    void dispatchLoadEvent(SVGElement*);
    void insertedIntoDocument(SVGElement*);
    void finishParsingChildren();
    bool haveLoadedRequiredResources() const;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
