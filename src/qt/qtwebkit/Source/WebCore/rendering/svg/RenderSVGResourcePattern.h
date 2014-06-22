/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef RenderSVGResourcePattern_h
#define RenderSVGResourcePattern_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "FloatRect.h"
#include "ImageBuffer.h"
#include "Pattern.h"
#include "PatternAttributes.h"
#include "RenderSVGResourceContainer.h"
#include "SVGPatternElement.h"
#include "SVGUnitTypes.h"

#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

struct PatternData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RefPtr<Pattern> pattern;
    AffineTransform transform;
};

class RenderSVGResourcePattern : public RenderSVGResourceContainer {
public:
    RenderSVGResourcePattern(SVGPatternElement*);

    virtual const char* renderName() const { return "RenderSVGResourcePattern"; }

    virtual void removeAllClientsFromCache(bool markForInvalidation = true);
    virtual void removeClientFromCache(RenderObject*, bool markForInvalidation = true);

    virtual bool applyResource(RenderObject*, RenderStyle*, GraphicsContext*&, unsigned short resourceMode);
    virtual void postApplyResource(RenderObject*, GraphicsContext*&, unsigned short resourceMode, const Path*, const RenderSVGShape*);
    virtual FloatRect resourceBoundingBox(RenderObject*) { return FloatRect(); }

    virtual RenderSVGResourceType resourceType() const { return s_resourceType; }
    static RenderSVGResourceType s_resourceType;

private:
    bool buildTileImageTransform(RenderObject*, const PatternAttributes&, const SVGPatternElement*, FloatRect& patternBoundaries, AffineTransform& tileImageTransform) const;

    PassOwnPtr<ImageBuffer> createTileImage(const PatternAttributes&, const FloatRect& tileBoundaries,
                                            const FloatRect& absoluteTileBoundaries, const AffineTransform& tileImageTransform,
                                            FloatRect& clampedAbsoluteTileBoundaries) const;

    PatternData* buildPattern(RenderObject*, unsigned short resourceMode);

    bool m_shouldCollectPatternAttributes : 1;
    PatternAttributes m_attributes;
    HashMap<RenderObject*, OwnPtr<PatternData> > m_patternMap;
};

}

#endif
#endif
