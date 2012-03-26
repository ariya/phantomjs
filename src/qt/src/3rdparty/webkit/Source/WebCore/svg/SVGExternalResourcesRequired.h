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
namespace WebCore {

class Attribute;
class QualifiedName;

// Notes on a SVG 1.1 spec discrepancy:
// The SVG DOM defines the attribute externalResourcesRequired as being of type SVGAnimatedBoolean, whereas the 
// SVG language definition says that externalResourcesRequired is not animated. Because the SVG language definition
// states that externalResourcesRequired cannot be animated, the animVal will always be the same as the baseVal.
// FIXME: When implementing animVal support, make sure that animVal==baseVal for externalResourcesRequired
class SVGExternalResourcesRequired {
public:
    virtual ~SVGExternalResourcesRequired() { }

    bool parseMappedAttribute(Attribute*);
    bool isKnownAttribute(const QualifiedName&);

protected:
    virtual void setExternalResourcesRequiredBaseValue(const bool&) = 0;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
