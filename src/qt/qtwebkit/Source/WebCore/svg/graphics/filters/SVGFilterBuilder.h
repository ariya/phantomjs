/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#ifndef SVGFilterBuilder_h
#define SVGFilterBuilder_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FilterEffect.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class RenderObject;

class SVGFilterBuilder : public RefCounted<SVGFilterBuilder> {
public:
    typedef HashSet<FilterEffect*> FilterEffectSet;

    static PassRefPtr<SVGFilterBuilder> create(PassRefPtr<FilterEffect> sourceGraphic, PassRefPtr<FilterEffect> sourceAlpha) { return adoptRef(new SVGFilterBuilder(sourceGraphic, sourceAlpha)); }

    void add(const AtomicString& id, PassRefPtr<FilterEffect>);

    FilterEffect* getEffectById(const AtomicString& id) const;
    FilterEffect* lastEffect() const { return m_lastEffect.get(); }

    void appendEffectToEffectReferences(PassRefPtr<FilterEffect>, RenderObject*);

    inline FilterEffectSet& effectReferences(FilterEffect* effect)
    {
        // Only allowed for effects belongs to this builder.
        ASSERT(m_effectReferences.contains(effect));
        return m_effectReferences.find(effect)->value;
    }

    // Required to change the attributes of a filter during an svgAttributeChanged.
    inline FilterEffect* effectByRenderer(RenderObject* object) { return m_effectRenderer.get(object); }

    void clearEffects();
    void clearResultsRecursive(FilterEffect*);

private:
    SVGFilterBuilder(PassRefPtr<FilterEffect> sourceGraphic, PassRefPtr<FilterEffect> sourceAlpha);

    inline void addBuiltinEffects()
    {
        HashMap<AtomicString, RefPtr<FilterEffect> >::iterator end = m_builtinEffects.end();
        for (HashMap<AtomicString, RefPtr<FilterEffect> >::iterator iterator = m_builtinEffects.begin(); iterator != end; ++iterator)
             m_effectReferences.add(iterator->value, FilterEffectSet());
    }

    HashMap<AtomicString, RefPtr<FilterEffect> > m_builtinEffects;
    HashMap<AtomicString, RefPtr<FilterEffect> > m_namedEffects;
    // The value is a list, which contains those filter effects,
    // which depends on the key filter effect.
    HashMap<RefPtr<FilterEffect>, FilterEffectSet> m_effectReferences;
    HashMap<RenderObject*, FilterEffect*> m_effectRenderer;

    RefPtr<FilterEffect> m_lastEffect;
};
    
} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
#endif // SVGFilterBuilder_h
